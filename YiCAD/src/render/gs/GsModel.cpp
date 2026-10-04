/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file GsModel.cpp
/// @brief GsModel 实现

#include "GsModel.h"

#include <algorithm>
#include <optional>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>

#include "DmBlock.h"
#include "DmChangeSet.h"
#include "DmDocument.h"
#include "DmEntity.h"
#include "DmEntityContainer.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "DmLineType.h"
#include "DmLineTypeTable.h"
#include "EntityTable.h"
#include "GsDevice.h"
#include "ISelectionSource.h"
#include "ScopedTimer.h"
#include "YiCadLog.h"

#define GS_WARNING YICAD_LOG(yicad::log::render(), yicad::LogLevel::Warning) << "GS："

namespace
{

constexpr std::size_t kCellBudget = 64u * 1024u;  ///< 一个分块里节点的 GI 流总字节数的上限，超过就分裂（约 2 千条记录；改一个实体要重编所在分块，按基准图纸定）
constexpr int kMaxDepth = 40;
constexpr int kMaxExpandDepth = 32;                ///< 嵌套块展开的最大深度（防止块自己引用自己）
constexpr double kHugeExtent = 1.0e15;             ///< 包围框比它还大（射线、构造线）的实体放进总是可见的分块
constexpr std::uint32_t kVerifyBatch = 64;
constexpr std::size_t kSparseStateUploads = 256;     ///< 对象状态的改动不超过这么多个时逐段上传，否则按范围

std::uint32_t floatBits(float f)
{
    return std::bit_cast<std::uint32_t>(f);
}

float bitsFloat(std::uint32_t b)
{
    return std::bit_cast<float>(b);
}

template <typename T>
std::vector<std::byte> asBytes(const std::vector<T>& v)
{
    std::vector<std::byte> bytes(v.size() * sizeof(T));
    if (!v.empty())
    {
        std::memcpy(bytes.data(), v.data(), bytes.size());
    }
    return bytes;
}

bool validBounds(const DmVector& minCorner, const DmVector& maxCorner)
{
    return minCorner.valid && maxCorner.valid && std::isfinite(minCorner.x) && std::isfinite(minCorner.y)
        && std::isfinite(maxCorner.x) && std::isfinite(maxCorner.y) && maxCorner.x >= minCorner.x
        && maxCorner.y >= minCorner.y && maxCorner.x - minCorner.x < kHugeExtent
        && maxCorner.y - minCorner.y < kHugeExtent;
}

/// @brief 记录里存图元记录序号的纹素（每条记录的最后一个纹素的 w）
std::size_t primTexelOffset(GsClass c)
{
    return gsTexelsPerRecord(c) - 1;
}

/// @brief 把一个管线类的记录里的图元记录序号加上 base（线段的点保留断开位）
void rebasePrims(GsClass c, std::vector<GsTexel>& records, std::size_t first, std::uint32_t base)
{
    if (base == 0)
    {
        return;
    }
    const std::size_t stride = gsTexelsPerRecord(c);
    for (std::size_t i = first + primTexelOffset(c); i < records.size(); i += stride)
    {
        const std::uint32_t bits = floatBits(records[i].w);
        const std::uint32_t flag = bits & kGsPointBreak;
        records[i].w = bitsFloat(((bits & ~kGsPointBreak) + base) | flag);
    }
}

/// @brief 线型是否画成连续线：没有图案（Continuous 与随层、随块的保留记录都没有图案）
bool isContinuous(const DmLineType* lineType)
{
    return !lineType || const_cast<DmLineType*>(lineType)->getNum() == 0;
}

/// @brief 随层、随块的保留记录：图层不该以它们为线型（读入的图纸里可能有），图层表里当作连续线
bool isByLayerOrByBlock(const DmLineType* lineType)
{
    return DmLineTypeTable::isByLayer(lineType) || DmLineTypeTable::isByBlock(lineType);
}

/// @brief 线型表的一项：图案（最多 12 个元素）、周期与第一段划线的起止
GsLineTypeRecord lineTypeRecord(const std::vector<double>& data)
{
    GsLineTypeRecord record;
    const std::size_t count = std::min<std::size_t>(data.size(), record.elements.size());
    double period = 0.0;
    bool haveDash = false;
    for (std::size_t i = 0; i < count; ++i)
    {
        const double v = data[i];
        record.elements[i] = static_cast<float>(v);
        if (v > 1.0e-8 && !haveDash)
        {
            record.firstDashStart = static_cast<float>(period);
            record.firstDashEnd = static_cast<float>(period + v);
            haveDash = true;
        }
        period += std::abs(v);
    }
    record.count = static_cast<float>(count);
    record.period = static_cast<float>(period);
    return record;
}

/// @brief 样条离散结果的缓存（与原先旧渲染器的 GLCacheWorldDraw::NurbsSamples 相同：按曲线内容查找，
///        整图重建时标记—清除：这一轮用到的留下，没用到的丢掉）
/// @details 离散一条样条要递归求 B 样条基函数，比其余实体的编译贵得多（中图纸 2 千条样条约 0.6 秒，阶段 2 的记录），
///          整图重建（REGEN、进出块编辑）不应每次重来
class NurbsCache
{
public:
    const std::vector<DmVector>& sample(const GiNurbs& curve)
    {
        std::vector<Entry>& bucket = m_entries[hashOf(curve)];
        for (Entry& entry : bucket)
        {
            if (same(entry.curve, curve))
            {
                entry.generation = m_generation;
                return entry.points;
            }
        }
        Entry entry;
        entry.curve = curve;
        entry.generation = m_generation;
        curve.sample(entry.points);
        bucket.push_back(std::move(entry));
        return bucket.back().points;
    }

    /// @brief 开始一轮整图重建：之后用到的条目记上这一轮
    void beginSweep() { ++m_generation; }

    /// @brief 结束一轮整图重建：丢掉这一轮没用到的条目
    void endSweep()
    {
        for (auto it = m_entries.begin(); it != m_entries.end();)
        {
            std::vector<Entry>& bucket = it->second;
            bucket.erase(std::remove_if(bucket.begin(), bucket.end(),
                                        [this](const Entry& e) { return e.generation != m_generation; }),
                         bucket.end());
            it = bucket.empty() ? m_entries.erase(it) : std::next(it);
        }
    }

private:
    struct Entry
    {
        GiNurbs curve;
        std::vector<DmVector> points;
        std::uint64_t generation = 0;  ///< 最近一次用到它的整图重建
    };

    static void hashBytes(std::size_t& h, const void* data, std::size_t size)
    {
        const auto* bytes = static_cast<const unsigned char*>(data);
        for (std::size_t i = 0; i < size; ++i)
        {
            h ^= bytes[i];
            h *= 1099511628211ull;
        }
    }

    static std::size_t hashOf(const GiNurbs& curve)
    {
        std::size_t h = 14695981039346656037ull;
        hashBytes(h, &curve.degree, sizeof(curve.degree));
        hashBytes(h, &curve.closed, sizeof(curve.closed));
        for (double k : curve.knots)
        {
            hashBytes(h, &k, sizeof(k));
        }
        for (const DmVector& p : curve.controlPoints)
        {
            hashBytes(h, &p.x, sizeof(p.x));
            hashBytes(h, &p.y, sizeof(p.y));
        }
        for (double w : curve.weights)
        {
            hashBytes(h, &w, sizeof(w));
        }
        return h;
    }

    static bool same(const GiNurbs& a, const GiNurbs& b)
    {
        if (a.degree != b.degree || a.closed != b.closed || a.knots != b.knots || a.weights != b.weights
            || a.controlPoints.size() != b.controlPoints.size())
        {
            return false;
        }
        for (std::size_t i = 0; i < a.controlPoints.size(); ++i)
        {
            if (a.controlPoints[i].x != b.controlPoints[i].x || a.controlPoints[i].y != b.controlPoints[i].y)
            {
                return false;
            }
        }
        return true;
    }

    std::unordered_map<std::size_t, std::vector<Entry>> m_entries;
    std::uint64_t m_generation = 0;
};

}  // namespace

// ---------------------------------------------------------------------------
// 内部结构
// ---------------------------------------------------------------------------

/// @brief 共享几何的一组实例在某个分块（或节点）里的位置
struct GsInstanceGroup
{
    void* shared = nullptr;
    std::uint32_t firstInstance = 0;
    std::uint32_t count = 0;
};

struct GsModel::Node
{
    DmEntity* entity = nullptr;
    GiStream stream;
    std::uint64_t revision = 0;
    std::uint32_t slot = kGsNoSlot;
    Cell* cell = nullptr;
    DmVector minCorner;
    DmVector maxCorner;
    bool global = false;
    std::size_t weight = 0;
    // 编译分块时填写
    std::array<GsRange, kGsClassCount> ranges{};   ///< 在各数据区里的绝对位置
    std::vector<GsInstanceGroup> groups;           ///< 块参照、字形的实例
    std::vector<std::uint32_t> images;             ///< 分块图片列表里属于本节点的下标
    std::vector<Shared*> uses;                     ///< 直接用到的共享几何（依赖索引）
    bool nonUniform = false;
};

struct GsModel::QuadNode
{
    DmVector center;
    double half = 0.0;
    int depth = 0;
    QuadNode* parent = nullptr;
    std::array<std::unique_ptr<QuadNode>, 4> children;
    bool split = false;
    Cell* cell = nullptr;
};

struct GsModel::Cell
{
    std::uint32_t index = 0;
    QuadNode* quad = nullptr;          ///< 为空：总是可见的分块（序号 0）
    DmVector origin;
    std::vector<Node*> nodes;
    std::size_t weight = 0;
    DmVector minCorner;
    DmVector maxCorner;
    bool hasBounds = false;
    bool dirty = false;
    // 编译结果
    std::array<GsRange, kGsClassCount> ranges{};
    GsRange prims;
    GsRange instances;                 ///< 第一条是顶层几何的恒等实例
    std::vector<GsInstanceGroup> groups;
    std::vector<GsRunSummary> runs;    ///< 按线型画的线的长度摘要（LTSCALE、线型改了据此判断要不要重新分段）
    bool hasPieces = false;            ///< 有按线型分了段的线
    struct Image
    {
        std::uint32_t record = 0;          ///< Image 数据区里的记录序号
        std::uint32_t firstInstance = 0;
        std::uint32_t instanceCount = 1;
        bool local = false;                ///< 顶层几何的图片：记录序号与实例序号相对分块的区段
        RhiTexturePtr texture;
        RhiBindGroupPtr group;
        GsImageSource source;
    };
    std::vector<Image> images;
    std::array<std::vector<RhiDrawIndirectArgs>, kGsClassCount> commands;  ///< 编译时备好的绘制命令（不含图片）
    std::vector<GsInfiniteLine> infinite;  ///< 世界坐标，图元记录为绝对序号
    /// @brief 实例记录的来源，整体变换改了据此重写实例记录
    struct InstanceSource
    {
        Shared* shared = nullptr;          ///< 为空：恒等实例
        GiTransform transform;
        GsAttributes byBlock;
        std::uint32_t slot = kGsNoSlot;
        double lineTypeScale = 1.0;        ///< 块参照的线型比例
    };
    std::vector<InstanceSource> instanceSources;
};

struct GsModel::Shared
{
    const IGiDrawable* drawable = nullptr;
    DmVector origin;
    std::array<GsRange, kGsClassCount> ranges{};
    GsRange prims;
    std::vector<GsPrimRecord> primRecords;     ///< CPU 副本（展开无限线时复制）
    std::vector<GsSharedUse> children;
    std::vector<GsInfiniteLine> infinite;      ///< 定义坐标
    std::vector<GsImageSource> images;
    bool explicitDashed = false;               ///< 有指定了非连续线型的图元
    bool byBlockLineType = false;              ///< 有线型随块的图元
    std::vector<std::uint16_t> byLayerLineTypeLayers;  ///< 线型随层的图元用到的图层（可含实例图层）
    bool compiled = false;
    bool dirty = true;
    std::unordered_set<Node*> users;
    std::unordered_set<Shared*> parents;
};

/// @brief 编译器向模型要的东西
class GsModel::CompileContext final : public GsCompileContext
{
public:
    explicit CompileContext(GsModel& model) : m_model(model) {}

    std::uint16_t layerIndex(const DmLayer* layer) override { return m_model.layerIndexOf(layer); }
    std::uint16_t lineTypeIndex(const DmLineType* lineType) override { return m_model.lineTypeIndexOf(lineType); }
    const std::vector<DmVector>& sampleNurbs(const GiNurbs& curve) override { return m_nurbs.sample(curve); }
    bool needsFlatten(const IGiDrawable& drawable, const GiTransform& transform, const GsAttributes& byBlock,
                      bool* nonUniform) override
    {
        Shared& shared = m_model.sharedFor(drawable);
        return m_model.flattenCheck(shared, transform, byBlock, nonUniform, 0);
    }
    std::uint16_t patternIndex(const std::vector<double>& dashes) override { return m_model.patternIndexOf(dashes); }
    bool splitLongRuns() const override { return m_model.m_document != nullptr; }
    double globalLineTypeScale() const override { return m_model.m_lineTypeScale; }
    bool lineTypeMetrics(const GsLineTypeRef& lineType, double& period, double& firstDashCenter) override
    {
        return m_model.lineTypeMetrics(lineType, period, firstDashCenter);
    }

    void beginSweep() { m_nurbs.beginSweep(); }
    void endSweep() { m_nurbs.endSweep(); }

private:
    GsModel& m_model;
    NurbsCache m_nurbs;
};

void GsDrawList::clear()
{
    for (auto& c : commands)
    {
        c.clear();
    }
    images.clear();
}

bool GsDrawList::empty() const
{
    for (const auto& c : commands)
    {
        if (!c.empty())
        {
            return false;
        }
    }
    return images.empty();
}

// ---------------------------------------------------------------------------
// 构造与根
// ---------------------------------------------------------------------------

GsModel::GsModel(DmDocument& document)
    : m_document(&document)
    , m_primArena("gs prims", sizeof(GsPrimRecord), RhiBufferUsage::Texel, RhiFormat::RGBA32Uint)
    , m_instanceArena("gs instances", sizeof(GsInstanceRecord), RhiBufferUsage::Vertex, RhiFormat::Undefined)
{
    initialize();
    m_document->addListener(this);
}

GsModel::GsModel(const DmEntityContainer* container)
    : m_container(container)
    , m_primArena("gs prims", sizeof(GsPrimRecord), RhiBufferUsage::Texel, RhiFormat::RGBA32Uint)
    , m_instanceArena("gs instances", sizeof(GsInstanceRecord), RhiBufferUsage::Vertex, RhiFormat::Undefined)
{
    initialize();
}

GsModel::~GsModel()
{
    if (m_document)
    {
        m_document->removeListener(this);
    }
    // GPU 资源先于设备释放（设备由最后一个持有者销毁）
    m_staged.clear();
    for (auto& cell : m_cells)
    {
        if (cell)
        {
            cell->images.clear();
        }
    }
    m_modelGroup.reset();
    for (auto& g : m_geometryGroups)
    {
        g.reset();
    }
    m_shared.clear();
    m_cells.clear();
    m_classArenas = {};
    m_primArena = GsArena("gs prims", sizeof(GsPrimRecord), RhiBufferUsage::Texel, RhiFormat::RGBA32Uint);
    m_instanceArena = GsArena("gs instances", sizeof(GsInstanceRecord), RhiBufferUsage::Vertex, RhiFormat::Undefined);
    m_stateBuffer.reset();
    m_layerBuffer.reset();
    m_lineTypeBuffer.reset();
    m_infiniteBuffer.reset();
    m_device.reset();
}

void GsModel::initialize()
{
    m_compileContext = std::make_unique<CompileContext>(*this);
    m_compiler = std::make_unique<GsCompiler>(*m_compileContext);
    for (std::size_t i = 0; i < kGsClassCount; ++i)
    {
        const GsClass c = static_cast<GsClass>(i);
        if (c == GsClass::InfiniteLine)
        {
            continue;
        }
        m_classArenas[i] = std::make_unique<GsArena>("gs geometry", gsTexelsPerRecord(c) * sizeof(GsTexel),
                                                     RhiBufferUsage::Texel, RhiFormat::RGBA32Float);
    }
    m_verify = qEnvironmentVariableIntValue("YICAD_GS_VERIFY") != 0;
    m_fullRebuild = true;
}

void GsModel::paintContainerChanged(DmEntityContainer*)
{
    m_fullRebuild = true;
}

void GsModel::entitiesChanged(const DmChangeSet& changes)
{
    if (changes.fullRebuild)
    {
        m_fullRebuild = true;
        m_pending = {};
        return;
    }
    if (!m_fullRebuild)
    {
        m_pending.merge(changes);
    }
}

void GsModel::PendingChanges::merge(const DmChangeSet& changes)
{
    // 先删除：之前对同一地址的修改作废（那个实体已经不在了）
    for (const void* destroyed : changes.destroyedEntities)
    {
        auto it = entityIndex.find(destroyed);
        if (it != entityIndex.end())
        {
            entities[it->second].entity = nullptr;
            entityIndex.erase(it);
        }
        destroyedEntities.push_back(destroyed);
    }
    for (const void* destroyed : changes.destroyedBlocks)
    {
        auto it = blockIndex.find(destroyed);
        if (it != blockIndex.end())
        {
            blocks[it->second] = nullptr;
            blockIndex.erase(it);
        }
        // 块里的图元随块一起删除
        for (DmEntityChange& change : entities)
        {
            if (change.entity && change.ownerBlock == destroyed)
            {
                entityIndex.erase(change.entity);
                change.entity = nullptr;
            }
        }
        destroyedBlocks.push_back(destroyed);
    }
    for (const DmEntityChange& change : changes.entities)
    {
        auto [it, inserted] = entityIndex.try_emplace(change.entity, entities.size());
        if (inserted)
        {
            entities.push_back(change);
        }
        else
        {
            entities[it->second].ownerBlock = change.ownerBlock;
        }
    }
    for (const DmBlock* block : changes.blocks)
    {
        if (blockIndex.try_emplace(block, blocks.size()).second)
        {
            blocks.push_back(block);
        }
    }
    layersChanged = layersChanged || changes.layersChanged;
    lineTypesChanged = lineTypesChanged || changes.lineTypesChanged;
    variablesChanged = variablesChanged || changes.variablesChanged;
}

bool GsModel::PendingChanges::isEmpty() const
{
    return entities.empty() && destroyedEntities.empty() && blocks.empty() && destroyedBlocks.empty() &&
           !layersChanged && !lineTypesChanged && !variablesChanged;
}

void GsModel::setContainer(const DmEntityContainer* container)
{
    m_container = container;
    m_fullRebuild = true;
}

void GsModel::invalidate()
{
    m_fullRebuild = true;
}

void GsModel::setRootTransform(const GiTransform& transform)
{
    if (transform == m_rootTransform)
    {
        return;
    }
    m_rootTransform = transform;
    m_rootTransformDirty = true;
}

void GsModel::setSelectionSource(const ISelectionSource* source)
{
    m_selection = source;
    m_selectionDirty = true;
}

void GsModel::selectionChanged()
{
    m_selectionDirty = true;
}

template <typename F>
void GsModel::forEachRootEntity(F&& f) const
{
    if (m_document)
    {
        if (m_rootTable)
        {
            for (DmEntity* e : *const_cast<EntityTable*>(m_rootTable))
            {
                f(e);
            }
        }
        return;
    }
    if (m_container)
    {
        for (DmEntity* e : *m_container)
        {
            if (e && !e->isErased())
            {
                f(e);
            }
        }
    }
}

void GsModel::releaseAll()
{
    for (auto& cell : m_cells)
    {
        if (cell)
        {
            releaseCell(*cell);
        }
    }
    for (auto& [drawable, shared] : m_shared)
    {
        releaseShared(*shared);
    }
    m_staged.clear();
    m_nodes.clear();
    m_cells.clear();
    m_freeCells.clear();
    m_cellOrigins.clear();
    m_dirtyCells.clear();
    m_shared.clear();
    m_dirtyShared.clear();
    m_root.reset();
    m_slotNodes.clear();
    m_freeSlots.clear();
    m_states.clear();
    m_selectedSlots.clear();
    m_statesResized = true;
    for (auto& arena : m_classArenas)
    {
        if (arena)
        {
            arena->reset();
        }
    }
    m_primArena.reset();
    m_instanceArena.reset();
}

void GsModel::rebuildAll()
{
    releaseAll();
    m_pending = {};
    m_fullRebuild = false;

    if (m_document)
    {
        m_rootTable = m_document->getEntityTable();
        m_rootOwner = m_rootTable ? m_rootTable->ownerBlock() : nullptr;
    }
    // 绘图次序：按实体表（容器）的顺序重新编号；已删除的实体保留原来的次序以便撤销时回到原位
    m_orders.clear();
    m_nextOrder = 0;

    // 总是可见的分块（序号 0）
    auto global = std::make_unique<Cell>();
    global->index = 0;
    global->origin = DmVector(0.0, 0.0);
    m_cells.push_back(std::move(global));
    m_cellOrigins.push_back(DmVector(0.0, 0.0));

    m_layerIndex.clear();
    m_layers.clear();
    m_lineTypeIndex.clear();
    m_patternIndex.clear();
    m_lineTypes.clear();
    m_lineTypeDashed.clear();
    m_lineTypeMetrics.clear();
    readLayers();
    readLineTypeScale();

    // 先建全部节点（记录 GI 流），再按内容的范围定四叉树的根，最后逐个放进分块
    std::vector<Node*> nodes;
    DmVector minCorner(std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    DmVector maxCorner(-std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
    bool any = false;
    forEachRootEntity([&](DmEntity* e) {
        if (m_document || e->isVisible())
        {
            Node* node = createNode(e);
            nodes.push_back(node);
            if (!node->global)
            {
                minCorner.x = std::min(minCorner.x, node->minCorner.x);
                minCorner.y = std::min(minCorner.y, node->minCorner.y);
                maxCorner.x = std::max(maxCorner.x, node->maxCorner.x);
                maxCorner.y = std::max(maxCorner.y, node->maxCorner.y);
                any = true;
            }
        }
    });
    m_root = std::make_unique<QuadNode>();
    if (any)
    {
        m_root->center = (minCorner + maxCorner) * 0.5;
        const double extent = std::max({maxCorner.x - minCorner.x, maxCorner.y - minCorner.y, 1.0e-6});
        m_root->half = std::exp2(std::ceil(std::log2(extent * 0.5 * 1.0001)));
    }
    else
    {
        m_root->center = DmVector(0.0, 0.0);
        m_root->half = 1024.0;
    }
    for (Node* node : nodes)
    {
        placeNode(*node);
    }
    m_tablesDirty = true;
    m_selectionDirty = true;
    m_infiniteDirty = true;
    ++m_version;
}

// ---------------------------------------------------------------------------
// 节点
// ---------------------------------------------------------------------------

std::uint32_t GsModel::allocateSlot()
{
    if (!m_freeSlots.empty())
    {
        const std::uint32_t slot = m_freeSlots.back();
        m_freeSlots.pop_back();
        return slot;
    }
    const std::uint32_t slot = static_cast<std::uint32_t>(m_slotNodes.size());
    m_slotNodes.push_back(nullptr);
    if (m_states.size() <= slot)
    {
        m_states.resize(std::max<std::size_t>(1024, m_states.size() * 2));
        m_statesResized = true;
    }
    return slot;
}

std::uint32_t GsModel::orderOf(const DmEntity* entity)
{
    auto it = m_orders.find(entity);
    if (it != m_orders.end())
    {
        return it->second;
    }
    if (m_nextOrder >= kGsMaxOrder - 1)
    {
        // 次序用完：按现在的顺序重新编号（很少见），全部节点的状态都要重写
        m_orders.clear();
        m_nextOrder = 0;
        forEachRootEntity([this](DmEntity* e) { m_orders[e] = m_nextOrder++; });
        for (auto& [key, node] : m_nodes)
        {
            m_states[node->slot].order = m_orders[node->entity];
        }
        m_statesResized = true;  // 全部重传
        auto again = m_orders.find(entity);
        if (again != m_orders.end())
        {
            return again->second;
        }
    }
    const std::uint32_t order = m_nextOrder++;
    m_orders[entity] = order;
    return order;
}

GsModel::Node* GsModel::createNode(DmEntity* entity)
{
    auto node = std::make_unique<Node>();
    node->entity = entity;
    node->slot = allocateSlot();
    m_slotNodes[node->slot] = node.get();
    Node* raw = node.get();
    m_nodes[entity] = std::move(node);
    refreshNode(*raw);
    GsObjectState& state = m_states[raw->slot];
    state.order = orderOf(entity);
    markStateDirty(raw->slot);
    return raw;
}

void GsModel::refreshNode(Node& node)
{
    DmEntity* e = node.entity;
    node.stream = GiStreamRecorder::record(*e, GiRegenType::Display);
    node.revision = e->revision();
    node.weight = node.stream.byteSize();
    node.minCorner = e->getMin();
    node.maxCorner = e->getMax();
    node.global = !validBounds(node.minCorner, node.maxCorner);
    GsObjectState& state = m_states[node.slot];
    state.layer = layerIndexOf(e->getLayer());
    const bool hidden = !e->getFlag(DM::FlagVisible);
    state.flags = hidden ? (state.flags | kGsStateHidden) : (state.flags & ~kGsStateHidden);
    markStateDirty(node.slot);
}

void GsModel::removeNode(Node* node)
{
    unplaceNode(*node);
    for (Shared* s : node->uses)
    {
        s->users.erase(node);
    }
    m_states[node->slot] = GsObjectState{};
    markStateDirty(node->slot);
    m_slotNodes[node->slot] = nullptr;
    m_freeSlots.push_back(node->slot);
    m_nodes.erase(node->entity);
}

// ---------------------------------------------------------------------------
// 分块
// ---------------------------------------------------------------------------

GsModel::Cell& GsModel::createCell(QuadNode* quad, const DmVector& origin)
{
    std::uint32_t index = 0;
    if (!m_freeCells.empty())
    {
        index = m_freeCells.back();
        m_freeCells.pop_back();
    }
    else
    {
        index = static_cast<std::uint32_t>(m_cells.size());
        m_cells.emplace_back();
        m_cellOrigins.emplace_back();
    }
    auto cell = std::make_unique<Cell>();
    cell->index = index;
    cell->quad = quad;
    cell->origin = origin;
    m_cellOrigins[index] = origin;
    Cell* raw = cell.get();
    m_cells[index] = std::move(cell);
    if (quad)
    {
        quad->cell = raw;
    }
    return *raw;
}

void GsModel::growRoot(const DmVector& minCorner, const DmVector& maxCorner)
{
    // 根的范围（不含松散外扩）装不下时，往包围框的方向加一层父节点，旧根成为它的一个子节点
    for (int guard = 0; guard < 64; ++guard)
    {
        const QuadNode& r = *m_root;
        const bool inside = minCorner.x >= r.center.x - r.half && maxCorner.x <= r.center.x + r.half
                         && minCorner.y >= r.center.y - r.half && maxCorner.y <= r.center.y + r.half;
        if (inside)
        {
            return;
        }
        const DmVector mid = (minCorner + maxCorner) * 0.5;
        const double sx = mid.x >= r.center.x ? 1.0 : -1.0;
        const double sy = mid.y >= r.center.y ? 1.0 : -1.0;
        auto parent = std::make_unique<QuadNode>();
        parent->half = r.half * 2.0;
        parent->center = DmVector(r.center.x + sx * r.half, r.center.y + sy * r.half);
        parent->split = true;
        // 旧根在新根里的象限：与新根中心的相对位置
        const int qx = r.center.x >= parent->center.x ? 1 : 0;
        const int qy = r.center.y >= parent->center.y ? 1 : 0;
        std::unique_ptr<QuadNode> old = std::move(m_root);
        old->parent = parent.get();
        // 深度从根算起：全部子树加一
        std::vector<QuadNode*> stack{old.get()};
        while (!stack.empty())
        {
            QuadNode* q = stack.back();
            stack.pop_back();
            ++q->depth;
            for (auto& c : q->children)
            {
                if (c)
                {
                    stack.push_back(c.get());
                }
            }
        }
        parent->children[qy * 2 + qx] = std::move(old);
        m_root = std::move(parent);
    }
}

GsModel::Cell& GsModel::cellFor(Node& node)
{
    if (node.global)
    {
        return *m_cells[0];
    }
    growRoot(node.minCorner, node.maxCorner);
    const DmVector center = (node.minCorner + node.maxCorner) * 0.5;
    const double extent = std::max(node.maxCorner.x - node.minCorner.x, node.maxCorner.y - node.minCorner.y);
    QuadNode* q = m_root.get();
    // 松散四叉树：子节点的松散范围是它自身的两倍，尺寸不超过子节点边长的对象按中心放进子节点
    while (q->split && q->depth < kMaxDepth && extent <= q->half)
    {
        const int qx = center.x >= q->center.x ? 1 : 0;
        const int qy = center.y >= q->center.y ? 1 : 0;
        std::unique_ptr<QuadNode>& child = q->children[qy * 2 + qx];
        if (!child)
        {
            child = std::make_unique<QuadNode>();
            child->parent = q;
            child->depth = q->depth + 1;
            child->half = q->half * 0.5;
            child->center = DmVector(q->center.x + (qx ? 1.0 : -1.0) * child->half,
                                     q->center.y + (qy ? 1.0 : -1.0) * child->half);
        }
        q = child.get();
    }
    if (!q->cell)
    {
        createCell(q, q->center);
    }
    else if ((q->split || q->depth >= kMaxDepth) && q->cell->weight >= kCellBudget)
    {
        // 分裂过（或到了最深）的节点：留在这一层的是比子节点大的实体，再多也分不下去。当前的分块满了就另开一个，
        // 旧的照常画，节点只认新的；改一个实体只重编它所在的那一个（RENDER_PLAN.md 第 4.3.5 节，修改一个实体的开销以分块为上限）
        createCell(q, q->center);
    }
    return *q->cell;
}

void GsModel::placeNode(Node& node)
{
    Cell& cell = cellFor(node);
    node.cell = &cell;
    cell.nodes.push_back(&node);
    cell.weight += node.weight;
    markCellDirty(cell);
    splitIfNeeded(cell);
}

void GsModel::unplaceNode(Node& node)
{
    Cell* cell = node.cell;
    if (!cell)
    {
        return;
    }
    auto it = std::find(cell->nodes.begin(), cell->nodes.end(), &node);
    if (it != cell->nodes.end())
    {
        *it = cell->nodes.back();
        cell->nodes.pop_back();
    }
    cell->weight -= std::min(cell->weight, node.weight);
    node.cell = nullptr;
    markCellDirty(*cell);
}

void GsModel::splitIfNeeded(Cell& cell)
{
    QuadNode* q = cell.quad;
    if (!q || q->split || cell.weight <= kCellBudget || q->depth >= kMaxDepth || cell.nodes.size() < 2)
    {
        return;
    }
    q->split = true;
    std::vector<Node*> nodes = std::move(cell.nodes);
    cell.nodes.clear();
    cell.weight = 0;
    for (Node* n : nodes)
    {
        n->cell = nullptr;
    }
    for (Node* n : nodes)
    {
        placeNode(*n);
    }
    markCellDirty(cell);
}

void GsModel::markCellDirty(Cell& cell)
{
    if (!cell.dirty)
    {
        cell.dirty = true;
        m_dirtyCells.insert(&cell);
    }
}

void GsModel::releaseCell(Cell& cell)
{
    for (std::size_t i = 0; i < kGsClassCount; ++i)
    {
        if (m_classArenas[i] && !cell.ranges[i].empty())
        {
            m_classArenas[i]->free(cell.ranges[i]);
        }
        cell.ranges[i] = {};
    }
    if (!cell.prims.empty())
    {
        m_primArena.free(cell.prims);
    }
    if (!cell.instances.empty())
    {
        m_instanceArena.free(cell.instances);
    }
    cell.groups.clear();
    for (auto& commands : cell.commands)
    {
        commands.clear();
    }
    // 图片纹理留到这一轮更新结束：分块重新编译后多半还用同一张图，设备的纹理缓存里它还活着就不用重新解码、上传
    for (Cell::Image& image : cell.images)
    {
        if (image.texture)
        {
            m_retainedTextures.push_back(std::move(image.texture));
        }
    }
    cell.images.clear();
    cell.instanceSources.clear();
    if (!cell.infinite.empty())
    {
        cell.infinite.clear();
        m_infiniteDirty = true;
    }
}

// ---------------------------------------------------------------------------
// 共享几何
// ---------------------------------------------------------------------------

GsModel::Shared& GsModel::sharedFor(const IGiDrawable& drawable)
{
    auto it = m_shared.find(&drawable);
    if (it != m_shared.end())
    {
        if (!it->second->compiled || it->second->dirty)
        {
            compileShared(*it->second);
        }
        return *it->second;
    }
    auto shared = std::make_unique<Shared>();
    shared->drawable = &drawable;
    // 块定义取它的基点作原点（块里的实体一般在基点附近）；字形取 (0, 0)
    if (const auto* block = dynamic_cast<const DmBlock*>(&drawable))
    {
        shared->origin = const_cast<DmBlock*>(block)->getBasePoint();
    }
    else
    {
        shared->origin = DmVector(0.0, 0.0);
    }
    Shared& ref = *shared;
    m_shared[&drawable] = std::move(shared);
    compileShared(ref);
    return ref;
}

void GsModel::compileShared(Shared& shared)
{
    releaseShared(shared);
    shared.compiled = true;
    shared.dirty = false;

    GsCompiled out;
    // 先标记已编译（上面），块自己引用自己时不会无限递归；用自己的编译器：可能正在编译某个节点时被要求编译
    GsCompiler compiler(*m_compileContext);
    compiler.compileShared(*shared.drawable, shared.origin, out);

    shared.primRecords = out.prims;
    shared.children = out.shared;
    shared.infinite = out.infinite;
    shared.images = out.images;
    for (const GsPrimRecord& p : out.prims)
    {
        const GsKind lineTypeKind = static_cast<GsKind>((p.kinds >> 2) & 3u);
        if (lineTypeKind == GsKind::Value && (p.lineTypeAndWeight & 0xFFFFu) != 0)
        {
            shared.explicitDashed = true;
        }
        else if (lineTypeKind == GsKind::ByBlock)
        {
            shared.byBlockLineType = true;
        }
        else if (lineTypeKind == GsKind::ByLayer)
        {
            const std::uint16_t layer = static_cast<std::uint16_t>(p.layers1 & 0xFFFFu);
            if (std::find(shared.byLayerLineTypeLayers.begin(), shared.byLayerLineTypeLayers.end(), layer)
                == shared.byLayerLineTypeLayers.end())
            {
                shared.byLayerLineTypeLayers.push_back(layer);
            }
        }
    }
    // 依赖：子共享几何改了，本共享几何的用户要重新展开
    for (const GsSharedUse& use : shared.children)
    {
        Shared& child = sharedFor(*use.drawable);
        child.parents.insert(&shared);
    }

    // 记录与图元记录放进数据区
    shared.prims = m_primArena.allocate(static_cast<std::uint32_t>(out.prims.size()));
    for (std::size_t i = 0; i < kGsClassCount; ++i)
    {
        const GsClass c = static_cast<GsClass>(i);
        auto& records = out.records[i];
        if (records.empty() || !m_classArenas[i])
        {
            continue;
        }
        rebasePrims(c, records, 0, shared.prims.offset);
        shared.ranges[i] = m_classArenas[i]->allocate(
            static_cast<std::uint32_t>(records.size() / gsTexelsPerRecord(c)));
        stage(*m_classArenas[i], shared.ranges[i], asBytes(records));
    }
    stage(m_primArena, shared.prims, asBytes(out.prims));
    ++m_version;
}

void GsModel::releaseShared(Shared& shared)
{
    for (std::size_t i = 0; i < kGsClassCount; ++i)
    {
        if (m_classArenas[i] && !shared.ranges[i].empty())
        {
            m_classArenas[i]->free(shared.ranges[i]);
        }
        shared.ranges[i] = {};
    }
    if (!shared.prims.empty())
    {
        m_primArena.free(shared.prims);
    }
    shared.primRecords.clear();
    shared.children.clear();
    shared.infinite.clear();
    shared.images.clear();
    shared.explicitDashed = false;
    shared.byBlockLineType = false;
    shared.byLayerLineTypeLayers.clear();
    shared.compiled = false;
}

void GsModel::markSharedDirty(Shared& shared)
{
    if (shared.dirty && m_dirtyShared.count(&shared))
    {
        return;
    }
    shared.dirty = true;
    m_dirtyShared.insert(&shared);
    for (Node* user : shared.users)
    {
        if (user->cell)
        {
            markCellDirty(*user->cell);
        }
    }
    for (Shared* parent : shared.parents)
    {
        markSharedDirty(*parent);
    }
}

void GsModel::destroyShared(const IGiDrawable* drawable)
{
    auto it = m_shared.find(drawable);
    if (it == m_shared.end())
    {
        return;
    }
    Shared* shared = it->second.get();
    // 用到它的节点与共享几何都要重新编译；之后不会再引用已释放的块（块参照找不到块定义就不画）
    markSharedDirty(*shared);
    for (Node* user : shared->users)
    {
        user->uses.erase(std::remove(user->uses.begin(), user->uses.end(), shared), user->uses.end());
    }
    for (auto& [key, other] : m_shared)
    {
        other->parents.erase(shared);
    }
    m_dirtyShared.erase(shared);
    releaseShared(*shared);
    m_shared.erase(it);
}

GsAttributes GsModel::resolveAgainst(const GsAttributes& inner, const GsAttributes& outer)
{
    // inner 是共享几何里一处嵌套使用的属性（随块指外层实例、实例图层指外层的实例图层），outer 是外层已解析的属性
    GsAttributes r = inner;
    const std::uint16_t outerLayer = outer.layer;
    auto layerOf = [outerLayer](std::uint16_t layer) { return layer == kGsLayerInstance ? outerLayer : layer; };
    r.layer = layerOf(inner.layer);

    if (inner.color.kind == GsKind::ByBlock)
    {
        r.color = outer.color;
    }
    else if (inner.color.kind == GsKind::ByLayer)
    {
        r.color.layer = layerOf(inner.color.layer);
    }
    if (r.color.kind == GsKind::ByLayer && r.color.layer == kGsLayerNone)
    {
        r.color = GsColorRef();
    }

    if (inner.lineType.kind == GsKind::ByBlock)
    {
        r.lineType = outer.lineType;
    }
    else if (inner.lineType.kind == GsKind::ByLayer)
    {
        r.lineType.layer = layerOf(inner.lineType.layer);
    }
    if (r.lineType.kind == GsKind::ByLayer && r.lineType.layer == kGsLayerNone)
    {
        r.lineType = GsLineTypeRef();
    }

    if (inner.lineWeight.kind == GsKind::ByBlock)
    {
        r.lineWeight = outer.lineWeight;
    }
    else if (inner.lineWeight.kind == GsKind::ByLayer)
    {
        r.lineWeight.layer = layerOf(inner.lineWeight.layer);
    }
    if (r.lineWeight.kind == GsKind::ByLayer && r.lineWeight.layer == kGsLayerNone)
    {
        r.lineWeight = GsLineWeightRef{GsKind::Value, static_cast<std::int16_t>(DM::WidthByLayer), kGsLayerNone};
    }
    return r;
}

bool GsModel::layerDashed(std::uint16_t layer) const
{
    if (layer >= m_layers.size())
    {
        return false;
    }
    const std::uint32_t lineType = m_layers[layer].lineTypeAndWeight & 0xFFFFu;
    return lineType != 0 && lineType < m_lineTypeDashed.size() && m_lineTypeDashed[lineType];
}

bool GsModel::isDashed(const Shared& shared, const GsAttributes& byBlock)
{
    if (shared.explicitDashed)
    {
        return true;
    }
    if (shared.byBlockLineType)
    {
        const GsLineTypeRef& lt = byBlock.lineType;
        if ((lt.kind == GsKind::Value && lt.index != 0) || (lt.kind == GsKind::ByLayer && layerDashed(lt.layer)))
        {
            return true;
        }
    }
    for (std::uint16_t layer : shared.byLayerLineTypeLayers)
    {
        if (layerDashed(layer == kGsLayerInstance ? byBlock.layer : layer))
        {
            return true;
        }
    }
    return false;
}

bool GsModel::flattenCheck(Shared& shared, const GiTransform& transform, const GsAttributes& byBlock, bool* nonUniform,
                           int depth)
{
    if (depth > kMaxExpandDepth)
    {
        return false;
    }
    const bool similar = transform.isSimilarity();
    if (!similar)
    {
        if (nonUniform)
        {
            *nonUniform = true;
        }
        if (isDashed(shared, byBlock))
        {
            return true;
        }
    }
    for (const GsSharedUse& use : shared.children)
    {
        Shared& child = sharedFor(*use.drawable);
        if (flattenCheck(child, transform * use.transform, resolveAgainst(use.byBlock, byBlock), nonUniform, depth + 1))
        {
            return true;
        }
    }
    return false;
}

void GsModel::expand(Shared& shared, const GiTransform& transform, const GsAttributes& byBlock, double lineTypeScale,
                     std::vector<Leaf>& leaves, int depth)
{
    if (depth > kMaxExpandDepth)
    {
        return;
    }
    if (shared.dirty || !shared.compiled)
    {
        compileShared(shared);
    }
    leaves.push_back({&shared, transform, byBlock, lineTypeScale});
    for (const GsSharedUse& use : shared.children)
    {
        Shared& child = sharedFor(*use.drawable);
        expand(child, transform * use.transform, resolveAgainst(use.byBlock, byBlock), lineTypeScale * use.lineTypeScale,
               leaves, depth + 1);
    }
}

GsInstanceRecord GsModel::instanceRecord(const Leaf& leaf, const DmVector& origin, std::uint32_t slot,
                                         std::uint32_t cell) const
{
    // 世界 = 变换(共享原点 + 局部)；相对分块原点 = 线性部分·局部 + (变换(共享原点) - 分块原点)
    const GiTransform t = m_rootTransform * leaf.transform;
    const DmVector anchor = t.apply(leaf.shared->origin);
    GsInstanceRecord r;
    r.linear = {static_cast<float>(t.a()), static_cast<float>(t.b()), static_cast<float>(t.c()),
                static_cast<float>(t.d())};
    double scale = 1.0;
    if (!t.isSimilarity(&scale))
    {
        scale = 1.0;
    }
    r.translate = {static_cast<float>(anchor.x - origin.x), static_cast<float>(anchor.y - origin.y),
                   static_cast<float>(scale), static_cast<float>(leaf.lineTypeScale)};
    r.slot = slot;
    const GsAttributes& b = leaf.byBlock;
    r.byBlockColor = b.color.rgba;
    const std::uint32_t colorKind = b.color.kind == GsKind::ByLayer ? 1u : 0u;
    const std::uint32_t lineTypeKind = b.lineType.kind == GsKind::ByLayer ? 1u : 0u;
    const std::uint32_t lineWeightKind = b.lineWeight.kind == GsKind::ByLayer ? 1u : 0u;
    r.byBlockKinds = colorKind | (lineTypeKind << 2) | (lineWeightKind << 4);
    // 随块属性随层时用的图层：颜色、线型、线宽来自同一个调用方，取第一个随层的
    std::uint16_t byBlockLayer = kGsLayerNone;
    if (colorKind)
    {
        byBlockLayer = b.color.layer;
    }
    else if (lineTypeKind)
    {
        byBlockLayer = b.lineType.layer;
    }
    else if (lineWeightKind)
    {
        byBlockLayer = b.lineWeight.layer;
    }
    r.layers = static_cast<std::uint32_t>(b.layer) | (static_cast<std::uint32_t>(byBlockLayer) << 16);
    r.byBlockLineTypeAndWeight = gsPackLineTypeAndWeight(b.lineType.kind == GsKind::Value ? b.lineType.index : 0,
                                                         b.lineWeight.kind == GsKind::Value ? b.lineWeight.code : 0);
    r.cell = cell;
    return r;
}

// ---------------------------------------------------------------------------
// 分块的编译
// ---------------------------------------------------------------------------

void GsModel::compileCell(Cell& cell)
{
    releaseCell(cell);
    cell.dirty = false;
    cell.hasBounds = false;
    cell.runs.clear();
    cell.hasPieces = false;

    std::array<std::vector<GsTexel>, kGsClassCount> records;
    std::vector<GsPrimRecord> prims;
    std::vector<Cell::InstanceSource> sources;
    sources.push_back({});  // 顶层几何的恒等实例
    // 每个共享几何的实例：按节点顺序排，同一节点的实例连续
    std::map<Shared*, std::vector<std::pair<Node*, Cell::InstanceSource>>> leavesByShared;

    GsCompiled out;
    for (Node* node : cell.nodes)
    {
        out.clear();
        GiStreamDrawable drawable(node->stream);
        m_compiler->compileNode(drawable, node->slot, cell.origin, out);

        const std::uint32_t primBase = static_cast<std::uint32_t>(prims.size());
        for (std::size_t i = 0; i < kGsClassCount; ++i)
        {
            const GsClass c = static_cast<GsClass>(i);
            auto& src = out.records[i];
            if (src.empty())
            {
                node->ranges[i] = {};
                continue;
            }
            const std::size_t first = records[i].size();
            records[i].insert(records[i].end(), src.begin(), src.end());
            rebasePrims(c, records[i], first, primBase);
            node->ranges[i] = {static_cast<std::uint32_t>(first / gsTexelsPerRecord(c)),
                               static_cast<std::uint32_t>(src.size() / gsTexelsPerRecord(c))};
        }
        prims.insert(prims.end(), out.prims.begin(), out.prims.end());

        // 按线型画的线的长度摘要：每种线型留最长的
        cell.hasPieces = cell.hasPieces || out.hasPieces;
        for (const GsRunSummary& run : out.runs)
        {
            auto it = std::find_if(cell.runs.begin(), cell.runs.end(),
                                   [&run](const GsRunSummary& r) { return r.lineType == run.lineType; });
            if (it == cell.runs.end())
            {
                cell.runs.push_back(run);
            }
            else
            {
                it->length = std::max(it->length, run.length);
            }
        }

        // 图片：每张一条
        node->images.clear();
        for (std::size_t k = 0; k < out.images.size(); ++k)
        {
            Cell::Image image;
            image.record = node->ranges[static_cast<std::size_t>(GsClass::Image)].offset + static_cast<std::uint32_t>(k);
            image.local = true;
            image.source = out.images[k];
            node->images.push_back(static_cast<std::uint32_t>(cell.images.size()));
            cell.images.push_back(std::move(image));
        }

        // 无限线（整体变换在 rebuildInfiniteLines 里代入）
        for (GsInfiniteLine line : out.infinite)
        {
            line.prim += primBase;
            cell.infinite.push_back(line);
        }

        // 块参照、字形：展开到叶子。依赖索引（共享几何的引用者）只增删变化的部分：
        // 常用字形的引用者有几万个，每次重编都先删后插会在大集合上做几千次哈希操作
        std::vector<Shared*> oldUses = std::move(node->uses);
        node->uses.clear();
        node->groups.clear();
        node->nonUniform = out.hasNonUniformUse;
        std::vector<Leaf> leaves;
        for (const GsSharedUse& use : out.shared)
        {
            Shared& shared = sharedFor(*use.drawable);
            if (std::find(node->uses.begin(), node->uses.end(), &shared) == node->uses.end())
            {
                node->uses.push_back(&shared);
                if (std::find(oldUses.begin(), oldUses.end(), &shared) == oldUses.end())
                {
                    shared.users.insert(node);
                }
            }
            leaves.clear();
            expand(shared, use.transform, use.byBlock, use.lineTypeScale, leaves, 0);
            for (const Leaf& leaf : leaves)
            {
                leavesByShared[leaf.shared].push_back(
                    {node, {leaf.shared, leaf.transform, leaf.byBlock, node->slot, leaf.lineTypeScale}});
                // 共享几何里的射线、构造线：按这个插入变换到世界坐标，图元记录复制一份并代入实例的属性
                for (const GsInfiniteLine& local : leaf.shared->infinite)
                {
                    GsPrimRecord p = leaf.shared->primRecords[local.prim];
                    p.slot = node->slot;
                    p.lineTypeScale *= static_cast<float>(leaf.lineTypeScale);  // 无限线不经实例记录
                    GsInfiniteLine line;
                    line.base = leaf.transform.apply(local.base);
                    line.direction = leaf.transform.applyVector(local.direction);
                    line.ray = local.ray;
                    // 随块与实例图层在这里代入（无限线不经实例记录）
                    const GsAttributes& b = leaf.byBlock;
                    const GsKind colorKind = static_cast<GsKind>(p.kinds & 3u);
                    if (colorKind == GsKind::ByBlock)
                    {
                        p.color = b.color.rgba;
                        p.kinds = (p.kinds & ~3u) | static_cast<std::uint32_t>(b.color.kind);
                        p.layers0 = (p.layers0 & 0xFFFFu) | (static_cast<std::uint32_t>(b.color.layer) << 16);
                    }
                    if ((p.layers0 & 0xFFFFu) == kGsLayerInstance)
                    {
                        p.layers0 = (p.layers0 & 0xFFFF0000u) | b.layer;
                    }
                    line.prim = static_cast<std::uint32_t>(prims.size());
                    prims.push_back(p);
                    cell.infinite.push_back(line);
                }
            }
        }
        for (Shared* old : oldUses)
        {
            if (std::find(node->uses.begin(), node->uses.end(), old) == node->uses.end())
            {
                old->users.erase(node);
            }
        }

        // 包围框
        if (!node->global)
        {
            if (!cell.hasBounds)
            {
                cell.minCorner = node->minCorner;
                cell.maxCorner = node->maxCorner;
                cell.hasBounds = true;
            }
            else
            {
                cell.minCorner.x = std::min(cell.minCorner.x, node->minCorner.x);
                cell.minCorner.y = std::min(cell.minCorner.y, node->minCorner.y);
                cell.maxCorner.x = std::max(cell.maxCorner.x, node->maxCorner.x);
                cell.maxCorner.y = std::max(cell.maxCorner.y, node->maxCorner.y);
            }
        }
    }

    // 实例：恒等实例之后按共享几何分组
    for (auto& [shared, list] : leavesByShared)
    {
        GsInstanceGroup group;
        group.shared = shared;
        group.firstInstance = static_cast<std::uint32_t>(sources.size());
        Node* current = nullptr;
        for (auto& [node, source] : list)
        {
            if (node != current)
            {
                node->groups.push_back({shared, static_cast<std::uint32_t>(sources.size()), 0});
                current = node;
            }
            ++node->groups.back().count;
            sources.push_back(source);
        }
        group.count = static_cast<std::uint32_t>(sources.size()) - group.firstInstance;
        cell.groups.push_back(group);
        // 块里的图片：每张按这组实例画
        for (std::size_t k = 0; k < shared->images.size(); ++k)
        {
            Cell::Image image;
            image.record = shared->ranges[static_cast<std::size_t>(GsClass::Image)].offset + static_cast<std::uint32_t>(k);
            image.firstInstance = group.firstInstance;
            image.instanceCount = group.count;
            image.source = shared->images[k];
            cell.images.push_back(std::move(image));
        }
    }

    // 分配并写入
    cell.prims = m_primArena.allocate(static_cast<std::uint32_t>(prims.size()));
    for (std::size_t i = 0; i < kGsClassCount; ++i)
    {
        const GsClass c = static_cast<GsClass>(i);
        if (records[i].empty() || !m_classArenas[i])
        {
            continue;
        }
        rebasePrims(c, records[i], 0, cell.prims.offset);
        cell.ranges[i] = m_classArenas[i]->allocate(static_cast<std::uint32_t>(records[i].size() / gsTexelsPerRecord(c)));
        stage(*m_classArenas[i], cell.ranges[i], asBytes(records[i]));
    }
    stage(m_primArena, cell.prims, asBytes(prims));
    for (GsInfiniteLine& line : cell.infinite)
    {
        line.prim += cell.prims.offset;
    }
    if (!cell.infinite.empty())
    {
        m_infiniteDirty = true;
    }

    cell.instances = m_instanceArena.allocate(static_cast<std::uint32_t>(sources.size()));
    cell.instanceSources = std::move(sources);
    // 绝对位置：组与节点的实例序号加上区段起点；节点的几何范围加上数据区里的起点
    for (GsInstanceGroup& g : cell.groups)
    {
        g.firstInstance += cell.instances.offset;
    }
    for (Cell::Image& image : cell.images)
    {
        image.firstInstance += cell.instances.offset;
        if (image.local)
        {
            image.record += cell.ranges[static_cast<std::size_t>(GsClass::Image)].offset;
        }
    }
    for (Node* node : cell.nodes)
    {
        for (std::size_t i = 0; i < kGsClassCount; ++i)
        {
            if (!node->ranges[i].empty())
            {
                node->ranges[i].offset += cell.ranges[i].offset;
            }
        }
        for (GsInstanceGroup& g : node->groups)
        {
            g.firstInstance += cell.instances.offset;
        }
    }
    stage(m_instanceArena, cell.instances, asBytes(instanceRecords(cell)));

    // 分块的绘制命令备好：每帧收集可见分块时整段复制，不再逐个去取共享几何的区段（大图纸上那要几毫秒）
    auto addRange = [&cell](GsClass c, const GsRange& range, std::uint32_t instanceCount, std::uint32_t firstInstance) {
        if (range.empty() || c == GsClass::Image)
        {
            return;
        }
        const std::uint32_t v = gsVerticesPerRecord(c);
        cell.commands[static_cast<std::size_t>(c)].push_back({v * range.count, instanceCount, v * range.offset, firstInstance});
    };
    for (std::size_t i = 0; i < kGsClassCount; ++i)
    {
        addRange(static_cast<GsClass>(i), cell.ranges[i], 1, cell.instances.offset);
    }
    for (const GsInstanceGroup& g : cell.groups)
    {
        const Shared* shared = static_cast<const Shared*>(g.shared);
        for (std::size_t i = 0; i < kGsClassCount; ++i)
        {
            addRange(static_cast<GsClass>(i), shared->ranges[i], g.count, g.firstInstance);
        }
    }
    ++m_version;
}

std::vector<GsInstanceRecord> GsModel::instanceRecords(const Cell& cell) const
{
    std::vector<GsInstanceRecord> instances;
    instances.reserve(cell.instanceSources.size());
    for (const Cell::InstanceSource& s : cell.instanceSources)
    {
        if (!s.shared)
        {
            // 顶层几何：恒等变换（容器模型为整体变换）
            GsInstanceRecord identity;
            const DmVector moved = m_rootTransform.apply(cell.origin);
            identity.linear = {static_cast<float>(m_rootTransform.a()), static_cast<float>(m_rootTransform.b()),
                               static_cast<float>(m_rootTransform.c()), static_cast<float>(m_rootTransform.d())};
            double scale = 1.0;
            if (!m_rootTransform.isSimilarity(&scale))
            {
                scale = 1.0;
            }
            identity.translate = {static_cast<float>(moved.x - cell.origin.x), static_cast<float>(moved.y - cell.origin.y),
                                  static_cast<float>(scale), 1.0f};
            identity.cell = cell.index;
            instances.push_back(identity);
        }
        else
        {
            instances.push_back(
                instanceRecord({s.shared, s.transform, s.byBlock, s.lineTypeScale}, cell.origin, s.slot, cell.index));
        }
    }
    return instances;
}

// ---------------------------------------------------------------------------
// 表
// ---------------------------------------------------------------------------

std::uint16_t GsModel::layerIndexOf(const DmLayer* layer)
{
    if (!layer)
    {
        return kGsLayerNone;
    }
    auto it = m_layerIndex.find(layer);
    if (it != m_layerIndex.end())
    {
        return it->second;
    }
    const std::uint16_t index = static_cast<std::uint16_t>(m_layers.size());
    m_layerIndex[layer] = index;
    GsLayerRecord record;
    const DmPen pen = layer->getPen();
    const DmColor& color = pen.getColor();
    record.color = gsPackColor(color.red(), color.green(), color.blue(), color.alpha());
    const DmLineType* lt = pen.getLineType();
    const std::uint16_t lineType = isByLayerOrByBlock(lt) ? 0 : lineTypeIndexOf(lt);
    record.lineTypeAndWeight = gsPackLineTypeAndWeight(lineType, pen.getWidth());
    record.flags = layer->isFrozen() ? kGsLayerFrozen : 0;
    m_layers.push_back(record);
    m_tablesDirty = true;
    return index;
}

std::uint16_t GsModel::lineTypeIndexOf(const DmLineType* lineType)
{
    if (m_lineTypes.empty())
    {
        m_lineTypes.push_back(GsLineTypeRecord{});  // 0：连续线
        m_lineTypeDashed.push_back(false);
        m_lineTypeMetrics.emplace_back(0.0, 0.0);
    }
    if (isContinuous(lineType))
    {
        return 0;
    }
    auto it = m_lineTypeIndex.find(lineType);
    if (it != m_lineTypeIndex.end())
    {
        return it->second;
    }
    const std::uint16_t index = static_cast<std::uint16_t>(m_lineTypes.size());
    m_lineTypeIndex[lineType] = index;
    setLineTypeEntry(index, const_cast<DmLineType*>(lineType)->getLineTypeData());
    return index;
}

std::uint16_t GsModel::patternIndexOf(const std::vector<double>& dashes)
{
    if (m_lineTypes.empty())
    {
        lineTypeIndexOf(nullptr);
    }
    auto it = m_patternIndex.find(dashes);
    if (it != m_patternIndex.end())
    {
        return it->second;
    }
    const std::uint16_t index = static_cast<std::uint16_t>(m_lineTypes.size());
    m_patternIndex[dashes] = index;
    setLineTypeEntry(index, dashes);
    return index;
}

void GsModel::setLineTypeEntry(std::uint16_t index, const std::vector<double>& dashes)
{
    if (index >= m_lineTypes.size())
    {
        m_lineTypes.resize(index + 1);
        m_lineTypeDashed.resize(index + 1, false);
        m_lineTypeMetrics.resize(index + 1);
    }
    m_lineTypes[index] = lineTypeRecord(dashes);
    m_lineTypeDashed[index] = true;
    double period = 0.0;
    double center = 0.0;
    gsPatternMetrics(dashes, period, center);
    m_lineTypeMetrics[index] = {period, center};
    m_tablesDirty = true;
}

bool GsModel::lineTypeMetrics(const GsLineTypeRef& lineType, double& period, double& firstDashCenter) const
{
    std::uint32_t index = 0;
    if (lineType.kind == GsKind::Value)
    {
        index = lineType.index;
    }
    else if (lineType.kind == GsKind::ByLayer && lineType.layer < m_layers.size())
    {
        index = m_layers[lineType.layer].lineTypeAndWeight & 0xFFFFu;
    }
    if (index == 0 || index >= m_lineTypeMetrics.size() || !(m_lineTypeMetrics[index].first > 0.0))
    {
        return false;
    }
    period = m_lineTypeMetrics[index].first;
    firstDashCenter = m_lineTypeMetrics[index].second;
    return true;
}

bool GsModel::readLineTypeScale()
{
    double scale = 1.0;
    if (m_document)
    {
        scale = m_document->getVariableDouble(QStringLiteral("$LTSCALE"), 1.0);
        if (!std::isfinite(scale) || scale <= 0.0)
        {
            scale = 1.0;
        }
    }
    const bool changed = scale != m_lineTypeScale;
    m_lineTypeScale = scale;
    return changed;
}

void GsModel::resplitLongRuns()
{
    for (auto& cell : m_cells)
    {
        if (!cell || cell->dirty)
        {
            continue;
        }
        bool resplit = cell->hasPieces;
        for (const GsRunSummary& run : cell->runs)
        {
            double period = 0.0;
            double center = 0.0;
            if (!resplit && lineTypeMetrics(run.lineType, period, center)
                && run.length / (period * m_lineTypeScale) > kGsPieceLimitPeriods)
            {
                resplit = true;
            }
        }
        if (resplit)
        {
            markCellDirty(*cell);
        }
    }
}

bool GsModel::readLayers()
{
    bool lineTypeChanged = false;
    // 已知的图层原地刷新（序号不变，编译过的几何仍然有效），文档里新的图层追加
    if (m_document && m_document->getLayerTable())
    {
        for (DmLayer* layer : *m_document->getLayerTable())
        {
            layerIndexOf(layer);
        }
    }
    for (auto& [layer, index] : m_layerIndex)
    {
        if (m_document)
        {
            // 已删除的图层不再读（指针可能已释放）
            bool alive = false;
            for (DmLayer* l : *m_document->getLayerTable())
            {
                if (l == layer)
                {
                    alive = true;
                    break;
                }
            }
            if (!alive)
            {
                continue;
            }
        }
        const DmPen pen = layer->getPen();
        const DmColor& color = pen.getColor();
        GsLayerRecord& record = m_layers[index];
        record.color = gsPackColor(color.red(), color.green(), color.blue(), color.alpha());
        const DmLineType* lt = pen.getLineType();
        const std::uint16_t lineType = isByLayerOrByBlock(lt) ? 0 : lineTypeIndexOf(lt);
        lineTypeChanged = lineTypeChanged || (record.lineTypeAndWeight & 0xFFFFu) != lineType;
        record.lineTypeAndWeight = gsPackLineTypeAndWeight(lineType, pen.getWidth());
        record.flags = layer->isFrozen() ? kGsLayerFrozen : 0;
    }
    m_tablesDirty = true;
    return lineTypeChanged;
}

void GsModel::readLineTypes()
{
    // 线型的图案改了：已知的线型原地重读（序号不变，编译过的几何里的序号仍然对）
    std::unordered_set<const DmLineType*> alive;
    if (m_document && m_document->getLineTypeTable())
    {
        for (DmLineType* lt : *m_document->getLineTypeTable())
        {
            alive.insert(lt);
        }
    }
    for (auto& [lt, index] : m_lineTypeIndex)
    {
        if (m_document && !alive.count(lt))
        {
            continue;
        }
        setLineTypeEntry(index, const_cast<DmLineType*>(lt)->getLineTypeData());
    }
    m_tablesDirty = true;
}

void GsModel::setStateFlag(std::uint32_t slot, std::uint32_t flag, bool on)
{
    GsObjectState& s = m_states[slot];
    const std::uint32_t flags = on ? (s.flags | flag) : (s.flags & ~flag);
    if (flags != s.flags)
    {
        s.flags = flags;
        markStateDirty(slot);
    }
}

void GsModel::markStateDirty(std::uint32_t slot)
{
    m_statesDirtyBegin = std::min(m_statesDirtyBegin, slot);
    m_statesDirtyEnd = std::max(m_statesDirtyEnd, slot + 1);
    // 改动少时逐个上传：点选两个相隔很远的实体时，按范围上传会把中间几十万个槽位都传一遍
    if (m_dirtySlots.size() <= kSparseStateUploads)
    {
        m_dirtySlots.push_back(slot);
    }
}

bool GsModel::isSelectedSlot(std::uint32_t slot) const
{
    return slot < m_states.size() && (m_states[slot].flags & kGsStateSelected) != 0;
}

void GsModel::updateSelection()
{
    YICAD_SCOPED_TIMER(yicad::counters::regenSelection());
    // 选中标记就在对象状态里；上一次选中的槽位列表用来清掉不再选中的（第 4.9 节：只改变化的槽位）
    std::vector<std::uint32_t> selected;
    if (m_selection && m_selection->hasMoreThan(m_slotNodes.size() / 8))
    {
        // 选中的多（全选）：遍历全部对象问选择集，比逐个按实体找槽位快（少一次按 ID 找实体、一次按实体找节点）
        selected.reserve(m_slotNodes.size());
        for (std::uint32_t slot = 0; slot < m_slotNodes.size(); ++slot)
        {
            const Node* node = m_slotNodes[slot];
            if (!node)
            {
                continue;
            }
            const bool on = m_selection->isSelected(*node->entity);
            setStateFlag(slot, kGsStateSelected, on);
            if (on)
            {
                selected.push_back(slot);
            }
        }
    }
    else
    {
        if (m_selection)
        {
            const std::vector<DmEntity*> entities = m_selection->selectedEntities();
            selected.reserve(entities.size());
            for (DmEntity* e : entities)
            {
                const std::uint32_t slot = slotOf(e);
                if (slot != kGsNoSlot)
                {
                    selected.push_back(slot);
                }
            }
        }
        // 先清掉上一次的，再标这一次的：两次都选中的槽位标记不变，不算改动
        std::vector<std::uint8_t> keep;
        if (!m_selectedSlots.empty())
        {
            keep.assign(m_states.size(), 0);
            for (std::uint32_t slot : selected)
            {
                keep[slot] = 1;
            }
            for (std::uint32_t slot : m_selectedSlots)
            {
                if (slot < keep.size() && !keep[slot])
                {
                    setStateFlag(slot, kGsStateSelected, false);
                }
            }
        }
        for (std::uint32_t slot : selected)
        {
            setStateFlag(slot, kGsStateSelected, true);
        }
    }
    m_selectedSlots = std::move(selected);
}

std::uint32_t GsModel::slotOf(const DmEntity* entity) const
{
    auto it = m_nodes.find(entity);
    return it == m_nodes.end() ? kGsNoSlot : it->second->slot;
}

// ---------------------------------------------------------------------------
// 更新
// ---------------------------------------------------------------------------

void GsModel::update(GsDevice& device)
{
    if (!m_device)
    {
        m_device = device.shared_from_this();
    }

    // 文档模型的整图重建计整个过程：建节点、编译分块与共享几何、上传（与原先旧渲染器的 render.regen 同一口径）；
    // 容器模型（预览等）内容一变就整体重建，不计入
    std::optional<yicad::ScopedTimer> regenTimer;
    const bool fullRebuild = m_fullRebuild;
    if (fullRebuild)
    {
        if (m_document && yicad::Profiler::isEnabled())
        {
            regenTimer.emplace(yicad::counters::regen());
        }
        m_compileContext->beginSweep();
        rebuildAll();
    }
    else if (!m_pending.isEmpty())
    {
        YICAD_SCOPED_TIMER(yicad::counters::gsChanges());
        const PendingChanges pending = std::move(m_pending);
        m_pending = {};
        const bool layers = pending.layersChanged;
        const bool lineTypes = pending.lineTypesChanged;
        const bool variables = pending.variablesChanged;
        for (const void* destroyed : pending.destroyedEntities)
        {
            auto it = m_nodes.find(destroyed);
            if (it != m_nodes.end())
            {
                removeNode(it->second.get());
            }
            m_orders.erase(destroyed);
        }
        for (const void* destroyed : pending.destroyedBlocks)
        {
            destroyShared(static_cast<const IGiDrawable*>(static_cast<const DmBlock*>(destroyed)));
        }
        for (const DmBlock* block : pending.blocks)
        {
            if (!block)
            {
                continue;
            }
            auto it = m_shared.find(static_cast<const IGiDrawable*>(block));
            if (it != m_shared.end())
            {
                markSharedDirty(*it->second);
            }
        }
        for (const DmEntityChange& change : pending.entities)
        {
            if (!change.entity || change.ownerBlock != m_rootOwner)
            {
                continue;  // 已删除的；块定义里的图元已按块定义处理
            }
            auto it = m_nodes.find(change.entity);
            if (change.entity->isErased())
            {
                if (it != m_nodes.end())
                {
                    removeNode(it->second.get());
                }
                continue;
            }
            if (it == m_nodes.end())
            {
                Node* node = createNode(change.entity);
                placeNode(*node);
            }
            else
            {
                Node& node = *it->second;
                unplaceNode(node);
                refreshNode(node);
                placeNode(node);
            }
        }
        bool resplit = false;
        if (lineTypes)
        {
            readLineTypes();
            resplit = true;
        }
        if (layers || lineTypes)
        {
            resplit = readLayers() || resplit;
            // 图层、线型改了：非等比插入要不要展开可能变了
            for (auto& [key, node] : m_nodes)
            {
                if (node->nonUniform && node->cell)
                {
                    markCellDirty(*node->cell);
                }
            }
            ++m_version;
        }
        // LTSCALE 只改每帧常量；分了段的超长虚线（段的相位按它算）与该分段的才重新编译
        if (variables && readLineTypeScale())
        {
            resplit = true;
            ++m_version;
        }
        if (resplit)
        {
            resplitLongRuns();
        }
    }

    // 共享几何先于分块编译：分块展开实例时要用
    if (!m_dirtyShared.empty())
    {
        std::vector<Shared*> dirty(m_dirtyShared.begin(), m_dirtyShared.end());
        m_dirtyShared.clear();
        for (Shared* s : dirty)
        {
            if (s->dirty)
            {
                compileShared(*s);
            }
        }
    }
    if (!m_dirtyCells.empty())
    {
        YICAD_SCOPED_TIMER(yicad::counters::gsCompile());
        std::vector<Cell*> dirty(m_dirtyCells.begin(), m_dirtyCells.end());
        m_dirtyCells.clear();
        for (Cell* cell : dirty)
        {
            if (cell->dirty)
            {
                compileCell(*cell);
            }
        }
    }
    if (m_rootTransformDirty)
    {
        // 整体变换（预览的拖动）：几何不动，只重写实例记录与无限线（RENDER_PLAN.md 第 4.3.9 节）
        m_rootTransformDirty = false;
        for (auto& cell : m_cells)
        {
            if (cell && !cell->instances.empty())
            {
                stage(m_instanceArena, cell->instances, asBytes(instanceRecords(*cell)));
            }
        }
        m_infiniteDirty = true;
        ++m_version;
    }
    if (m_selectionDirty)
    {
        m_selectionDirty = false;
        updateSelection();
        ++m_version;
    }
    if (m_infiniteDirty)
    {
        rebuildInfiniteLines();
    }

    if (fullRebuild)
    {
        m_compileContext->endSweep();
    }
    uploadAll(device);
    m_retainedTextures.clear();
    if (m_verify)
    {
        verifyRevisions();
    }
}

void GsModel::rebuildInfiniteLines()
{
    m_infiniteDirty = false;
    std::vector<GsTexel> texels;
    for (auto& cell : m_cells)
    {
        if (!cell)
        {
            continue;
        }
        for (const GsInfiniteLine& line : cell->infinite)
        {
            auto split = [](double v, float& hi, float& lo) {
                hi = static_cast<float>(v);
                lo = static_cast<float>(v - static_cast<double>(hi));
            };
            const DmVector base = m_rootTransform.apply(line.base);
            const DmVector direction = m_rootTransform.applyVector(line.direction);
            GsTexel a;
            split(base.x, a.x, a.z);
            split(base.y, a.y, a.w);
            const double len = std::hypot(direction.x, direction.y);
            GsTexel b;
            b.x = static_cast<float>(len > 0.0 ? direction.x / len : 1.0);
            b.y = static_cast<float>(len > 0.0 ? direction.y / len : 0.0);
            b.z = line.ray ? 1.0f : 0.0f;
            b.w = bitsFloat(line.prim);
            texels.push_back(a);
            texels.push_back(b);
        }
    }
    m_infiniteCount = static_cast<std::uint32_t>(texels.size() / 2);
    if (!m_device)
    {
        return;
    }
    RhiDevice& rhi = m_device->rhi();
    const std::size_t bytes = std::max<std::size_t>(texels.size(), 2) * sizeof(GsTexel);
    if (!m_infiniteBuffer || m_infiniteBuffer->size() < bytes)
    {
        RhiBufferDesc desc;
        desc.size = std::max<std::size_t>(bytes * 2, 1024);
        desc.usage = RhiBufferUsage::Texel;
        desc.debugName = "gs infinite lines";
        m_infiniteBuffer = rhi.createBuffer(desc);
        m_boundGeneration = ~0ull;
    }
    if (!texels.empty() && m_infiniteBuffer)
    {
        const std::vector<std::byte> data = asBytes(texels);
        rhi.upload(*m_infiniteBuffer, 0, data);
    }
    ++m_version;
}

void GsModel::stage(GsArena& arena, const GsRange& range, std::vector<std::byte> bytes)
{
    if (range.empty() || bytes.empty())
    {
        return;
    }
    m_staged.push_back({&arena, range, std::move(bytes)});
}

void GsModel::uploadAll(GsDevice& device)
{
    RhiDevice& rhi = device.rhi();
    // 数据区的容量跟上分配：要复制旧内容时开一个只做复制的帧
    bool needCopy = m_primArena.needsCopy() || m_instanceArena.needsCopy();
    for (auto& arena : m_classArenas)
    {
        needCopy = needCopy || (arena && arena->needsCopy());
    }
    RhiCommandList* copies = needCopy ? &rhi.beginOffscreenFrame() : nullptr;
    bool changed = m_primArena.sync(rhi, copies);
    changed = m_instanceArena.sync(rhi, copies) || changed;
    for (auto& arena : m_classArenas)
    {
        if (arena)
        {
            changed = arena->sync(rhi, copies) || changed;
        }
    }
    if (copies)
    {
        rhi.endFrame();
    }
    for (StagedWrite& w : m_staged)
    {
        w.arena->write(rhi, w.range, w.bytes);
    }
    m_staged.clear();

    // 图片纹理
    for (auto& cell : m_cells)
    {
        if (!cell)
        {
            continue;
        }
        for (Cell::Image& image : cell->images)
        {
            if (!image.texture)
            {
                image.texture = device.imageTexture(image.source.key, image.source.load);
                if (image.texture)
                {
                    RhiBindGroupDesc desc;
                    desc.layout = device.layout(GsLayout::Image);
                    RhiBindGroupEntry entry;
                    entry.binding = 0;
                    entry.texture = image.texture;
                    entry.sampler = device.imageSampler();
                    desc.entries = {entry};
                    desc.debugName = "gs image";
                    image.group = rhi.createBindGroup(desc);
                }
            }
        }
    }

    uploadStates(device);
    if (m_infiniteDirty)
    {
        rebuildInfiniteLines();
    }
    if (!m_infiniteBuffer)
    {
        rebuildInfiniteLines();
    }
    if (changed || !m_modelGroup)
    {
        m_boundGeneration = ~0ull;
    }
    rebuildBindGroups(device);
}

void GsModel::uploadStates(GsDevice& device)
{
    RhiDevice& rhi = device.rhi();
    // 对象状态
    const std::size_t stateBytes = std::max<std::size_t>(m_states.size(), 1) * sizeof(GsObjectState);
    if (!m_stateBuffer || m_stateBuffer->size() < stateBytes)
    {
        RhiBufferDesc desc;
        desc.size = stateBytes;
        desc.usage = RhiBufferUsage::Texel;
        desc.debugName = "gs object states";
        m_stateBuffer = rhi.createBuffer(desc);
        m_statesResized = true;
        m_boundGeneration = ~0ull;
    }
    if (m_stateBuffer && !m_states.empty())
    {
        bool wholeRange = m_dirtySlots.size() > kSparseStateUploads;
        if (m_statesResized)
        {
            m_statesDirtyBegin = 0;
            m_statesDirtyEnd = static_cast<std::uint32_t>(m_states.size());
            m_statesResized = false;
            wholeRange = true;
        }
        auto uploadRange = [&](std::size_t first, std::size_t end) {
            end = std::min(end, m_states.size());
            if (end <= first)
            {
                return;
            }
            rhi.upload(*m_stateBuffer, first * sizeof(GsObjectState),
                       std::span<const std::byte>(reinterpret_cast<const std::byte*>(m_states.data() + first),
                                                  (end - first) * sizeof(GsObjectState)));
        };
        if (m_statesDirtyEnd > m_statesDirtyBegin)
        {
            if (!wholeRange)
            {
                // 少：逐段上传连续的槽位
                std::sort(m_dirtySlots.begin(), m_dirtySlots.end());
                m_dirtySlots.erase(std::unique(m_dirtySlots.begin(), m_dirtySlots.end()), m_dirtySlots.end());
                std::size_t i = 0;
                while (i < m_dirtySlots.size())
                {
                    std::size_t j = i + 1;
                    while (j < m_dirtySlots.size() && m_dirtySlots[j] == m_dirtySlots[j - 1] + 1)
                    {
                        ++j;
                    }
                    uploadRange(m_dirtySlots[i], m_dirtySlots[j - 1] + 1);
                    i = j;
                }
            }
            else
            {
                uploadRange(m_statesDirtyBegin, m_statesDirtyEnd);
            }
        }
        m_statesDirtyBegin = std::numeric_limits<std::uint32_t>::max();
        m_statesDirtyEnd = 0;
        m_dirtySlots.clear();
    }

    // 图层表、线型表：小，整个重传
    if (m_tablesDirty)
    {
        m_tablesDirty = false;
        if (m_layers.empty())
        {
            m_layers.push_back(GsLayerRecord{});
        }
        if (m_lineTypes.empty())
        {
            lineTypeIndexOf(nullptr);
        }
        const std::size_t layerBytes = m_layers.size() * sizeof(GsLayerRecord);
        if (!m_layerBuffer || m_layerBuffer->size() < layerBytes)
        {
            RhiBufferDesc desc;
            desc.size = std::max<std::size_t>(layerBytes * 2, 1024);
            desc.usage = RhiBufferUsage::Texel;
            desc.debugName = "gs layers";
            m_layerBuffer = rhi.createBuffer(desc);
            m_boundGeneration = ~0ull;
        }
        const std::size_t lineTypeBytes = m_lineTypes.size() * sizeof(GsLineTypeRecord);
        if (!m_lineTypeBuffer || m_lineTypeBuffer->size() < lineTypeBytes)
        {
            RhiBufferDesc desc;
            desc.size = std::max<std::size_t>(lineTypeBytes * 2, 1024);
            desc.usage = RhiBufferUsage::Texel;
            desc.debugName = "gs line types";
            m_lineTypeBuffer = rhi.createBuffer(desc);
            m_boundGeneration = ~0ull;
        }
        if (m_layerBuffer)
        {
            rhi.upload(*m_layerBuffer, 0, asBytes(m_layers));
        }
        if (m_lineTypeBuffer)
        {
            rhi.upload(*m_lineTypeBuffer, 0, asBytes(m_lineTypes));
        }
        ++m_version;
    }
}

void GsModel::rebuildBindGroups(GsDevice& device)
{
    if (m_boundGeneration != ~0ull && m_modelGroup)
    {
        return;
    }
    if (!m_stateBuffer || !m_layerBuffer || !m_lineTypeBuffer || !m_primArena.buffer() || !m_infiniteBuffer)
    {
        return;
    }
    RhiDevice& rhi = device.rhi();
    auto texel = [](std::uint32_t binding, const RhiBufferPtr& buffer, RhiFormat format) {
        RhiBindGroupEntry e;
        e.binding = binding;
        e.buffer = buffer;
        e.texelFormat = format;
        return e;
    };
    RhiBindGroupDesc model;
    model.layout = device.layout(GsLayout::Model);
    model.entries = {texel(0, m_stateBuffer, RhiFormat::RGBA32Uint), texel(1, m_layerBuffer, RhiFormat::RGBA32Uint),
                     texel(2, m_lineTypeBuffer, RhiFormat::RGBA32Float),
                     texel(3, m_primArena.buffer(), RhiFormat::RGBA32Uint)};
    model.debugName = "gs model";
    m_modelGroup = rhi.createBindGroup(model);
    for (std::size_t i = 0; i < kGsClassCount; ++i)
    {
        const GsClass c = static_cast<GsClass>(i);
        const RhiBufferPtr& buffer = c == GsClass::InfiniteLine ? m_infiniteBuffer : m_classArenas[i]->buffer();
        if (!buffer)
        {
            m_geometryGroups[i].reset();
            continue;
        }
        RhiBindGroupDesc geometry;
        geometry.layout = device.layout(GsLayout::Geometry);
        geometry.entries = {texel(0, buffer, RhiFormat::RGBA32Float)};
        geometry.debugName = "gs geometry";
        m_geometryGroups[i] = rhi.createBindGroup(geometry);
    }
    m_boundGeneration = 0;
}

void GsModel::verifyRevisions()
{
    // 抽查：修订号对不上又不在待处理的变更里，说明有人改了实体却没登记（第 4.3.6 节）
    if (m_slotNodes.empty() || !m_document)
    {
        return;
    }
    for (std::uint32_t k = 0; k < kVerifyBatch; ++k)
    {
        const std::size_t slot = (m_verifyCursor++) % m_slotNodes.size();
        Node* node = m_slotNodes[slot];
        if (!node)
        {
            continue;
        }
        if (node->entity->revision() != node->revision && !m_document->changeTracker().isPending(node->entity))
        {
            GS_WARNING << "实体改了却没有登记变更（修订号 " << node->revision << " -> " << node->entity->revision()
                       << "，类型 " << static_cast<int>(node->entity->getEntityType()) << "）";
            node->revision = node->entity->revision();
            ++m_verifyMismatches;
        }
    }
}

// ---------------------------------------------------------------------------
// 收集绘制命令
// ---------------------------------------------------------------------------

void GsModel::collectVisible(const DmVector& minCorner, const DmVector& maxCorner, GsDrawList& out) const
{
    for (const auto& cell : m_cells)
    {
        if (!cell || cell->instances.empty())
        {
            continue;
        }
        if (cell->quad && cell->hasBounds
            && (cell->maxCorner.x < minCorner.x || cell->minCorner.x > maxCorner.x
                || cell->maxCorner.y < minCorner.y || cell->minCorner.y > maxCorner.y))
        {
            continue;
        }
        for (std::size_t i = 0; i < kGsClassCount; ++i)
        {
            const auto& commands = cell->commands[i];
            out.commands[i].insert(out.commands[i].end(), commands.begin(), commands.end());
        }
        for (const Cell::Image& image : cell->images)
        {
            if (image.group)
            {
                out.images.push_back({{6, image.instanceCount, 6 * image.record, image.firstInstance}, image.group.get()});
            }
        }
    }
    if (m_infiniteCount > 0 && !m_cells.empty() && m_cells[0] && !m_cells[0]->instances.empty())
    {
        out.commands[static_cast<std::size_t>(GsClass::InfiniteLine)].push_back(
            {6 * m_infiniteCount, 1, 0, m_cells[0]->instances.offset});
    }
}

void GsModel::collectEntities(const std::vector<DmEntity*>& entities, GsDrawList& out, bool skipSelected) const
{
    for (DmEntity* e : entities)
    {
        auto it = m_nodes.find(e);
        if (it == m_nodes.end() || !it->second->cell)
        {
            continue;
        }
        const Node& node = *it->second;
        if (skipSelected && isSelectedSlot(node.slot))
        {
            continue;
        }
        appendNode(node, out);
    }
}

bool GsModel::collectSelected(GsDrawList& out, std::size_t limit) const
{
    if (m_selectedSlots.size() > limit)
    {
        return false;
    }
    for (std::uint32_t slot : m_selectedSlots)
    {
        const Node* node = slot < m_slotNodes.size() ? m_slotNodes[slot] : nullptr;
        if (node && node->cell && isSelectedSlot(slot))
        {
            appendNode(*node, out);
        }
    }
    return true;
}

void GsModel::appendNode(const Node& node, GsDrawList& out) const
{
    const Cell& cell = *node.cell;
    if (cell.instances.empty())
    {
        return;
    }
    for (std::size_t i = 0; i < kGsClassCount; ++i)
    {
        const GsClass c = static_cast<GsClass>(i);
        if (node.ranges[i].empty() || c == GsClass::Image)
        {
            continue;
        }
        const std::uint32_t v = gsVerticesPerRecord(c);
        out.commands[i].push_back({v * node.ranges[i].count, 1, v * node.ranges[i].offset, cell.instances.offset});
    }
    for (const GsInstanceGroup& g : node.groups)
    {
        const Shared* shared = static_cast<const Shared*>(g.shared);
        for (std::size_t i = 0; i < kGsClassCount; ++i)
        {
            const GsClass c = static_cast<GsClass>(i);
            if (shared->ranges[i].empty() || c == GsClass::Image)
            {
                continue;
            }
            const std::uint32_t v = gsVerticesPerRecord(c);
            out.commands[i].push_back({v * shared->ranges[i].count, g.count, v * shared->ranges[i].offset, g.firstInstance});
        }
    }
    for (std::uint32_t k : node.images)
    {
        const Cell::Image& image = cell.images[k];
        if (image.group)
        {
            out.images.push_back({{6, image.instanceCount, 6 * image.record, image.firstInstance}, image.group.get()});
        }
    }
}

void GsModel::collectBlock(const DmBlock* block, const GiTransform& transform, GsDrawList& out,
                           std::vector<GsInstanceRecord>& instances)
{
    if (!block)
    {
        return;
    }
    Shared& root = sharedFor(*block);
    std::vector<Leaf> leaves;
    GsAttributes byBlock;
    expand(root, transform, byBlock, 1.0, leaves, 0);
    std::map<Shared*, std::vector<GsInstanceRecord>> grouped;
    for (const Leaf& leaf : leaves)
    {
        grouped[leaf.shared].push_back(instanceRecord(leaf, DmVector(0.0, 0.0), kGsNoSlot, 0));
    }
    for (auto& [shared, list] : grouped)
    {
        const std::uint32_t first = static_cast<std::uint32_t>(instances.size());
        instances.insert(instances.end(), list.begin(), list.end());
        const std::uint32_t count = static_cast<std::uint32_t>(list.size());
        for (std::size_t i = 0; i < kGsClassCount; ++i)
        {
            const GsClass c = static_cast<GsClass>(i);
            if (shared->ranges[i].empty() || c == GsClass::Image)
            {
                continue;
            }
            const std::uint32_t v = gsVerticesPerRecord(c);
            out.commands[i].push_back({v * shared->ranges[i].count, count, v * shared->ranges[i].offset, first});
        }
    }
}

bool GsModel::bounds(DmVector& minCorner, DmVector& maxCorner) const
{
    bool any = false;
    for (const auto& [key, node] : m_nodes)
    {
        if (node->global)
        {
            continue;
        }
        if (!any)
        {
            minCorner = node->minCorner;
            maxCorner = node->maxCorner;
            any = true;
        }
        else
        {
            minCorner.x = std::min(minCorner.x, node->minCorner.x);
            minCorner.y = std::min(minCorner.y, node->minCorner.y);
            maxCorner.x = std::max(maxCorner.x, node->maxCorner.x);
            maxCorner.y = std::max(maxCorner.y, node->maxCorner.y);
        }
    }
    return any;
}
