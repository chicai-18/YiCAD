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

/// @file GsModel.h
/// @brief 图形系统的模型：一份文档（或一个实体容器）的图形缓存（RENDER_PLAN.md 第 4.3 节）
///
/// 每个顶层实体一个节点，常驻它的 GI 流（第 4.3.2 节）；节点按包围框放进松散四叉树的分块（第 4.3.5 节），
/// 一个分块里全部节点的几何编译成连续的区段放进各管线类的数据区（第 4.3.4 节），坐标相对分块原点。
/// 块定义与字形各编译一份共享几何，块参照、字符是它的实例（第 4.3.3 节），嵌套块展开到叶子。
/// 选中、隐藏、绘图次序在对象状态里，图层的颜色、线型、线宽、冻结在图层表里，改它们只改表（第 4.3.7 节）。
///
/// 两种用法：
/// - 文档模型：AppDocument 持有，注册为文档的监听者，按变更集（DmChangeSet）增量更新，同一文档的视图共用；
///   块编辑时根换成被编辑的块；
/// - 容器模型：画一个实体容器（视图的预览、多行文字编辑器），不跟踪变更，内容改了整体重建（invalidate）。
///
/// 只在渲染线程（UI 线程）上使用。

#ifndef GSMODEL_H
#define GSMODEL_H

#include <array>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "DmChangeSet.h"
#include "DmDocumentListener.h"
#include "DmVector.h"
#include "GiStream.h"
#include "GiTransform.h"
#include "GsArena.h"
#include "GsCompiler.h"
#include "GsTypes.h"

class DmBlock;
class DmDocument;
class DmEntity;
class DmEntityContainer;
class DmLayer;
class DmLineType;
class EntityTable;
class GsDevice;
class ISelectionSource;

/// @brief 视图要画的一批命令：每个管线类一组间接绘制参数，图片各自绑纹理
struct GsDrawList
{
    std::array<std::vector<RhiDrawIndirectArgs>, kGsClassCount> commands;

    /// @brief 一张图片（或块里的一张图片的全部实例）
    struct ImageDraw
    {
        RhiDrawIndirectArgs args;
        const RhiBindGroup* textureGroup = nullptr;
    };
    std::vector<ImageDraw> images;

    void clear();
    bool empty() const;
};

/// @brief 图形系统的模型，见文件说明
class GsModel : public DmDocumentListener
{
public:
    /// @brief 文档模型：注册为文档的监听者，根为文档的实体表（块编辑时为被编辑块的实体表）
    explicit GsModel(DmDocument& document);

    /// @brief 容器模型：画 container 里可见的实体；容器可以为空
    explicit GsModel(const DmEntityContainer* container);

    ~GsModel() override;
    GsModel(const GsModel&) = delete;
    GsModel& operator=(const GsModel&) = delete;

    // ---- DmDocumentListener ----
    void documentModified() override {}
    void redrawRequested() override {}
    /// @brief 进入、退出块编辑：根换成文档当前的实体表，全部重建
    void paintContainerChanged(DmEntityContainer* container) override;
    void entitiesChanged(const DmChangeSet& changes) override;

    // ---- 容器模型 ----
    /// @brief 换容器并全部重建
    void setContainer(const DmEntityContainer* container);
    /// @brief 内容改了：下一次 update() 全部重建
    void invalidate();
    /// @brief 整体的变换（预览的拖动）：只改实例记录，不重新编译
    void setRootTransform(const GiTransform& transform);

    // ---- 选中 ----
    /// @brief 判断选中的来源（文档的选择集）；为空时没有选中
    void setSelectionSource(const ISelectionSource* source);
    /// @brief 选择集变了：下一次 update() 与上一次的集合求差，只改变化的槽位
    void selectionChanged();

    /// @brief 处理累积的变更：重建节点、编译分块、上传；在开始一帧之前调用
    void update(GsDevice& device);

    /// @brief 内容的版本：可见的东西（几何、选中、图层表）变了就递增，视图据此判断场景底图是否作废
    std::uint64_t version() const { return m_version; }

    /// @brief 可见分块的绘制命令
    /// @param minCorner、maxCorner 视口（世界坐标，外扩过）
    void collectVisible(const DmVector& minCorner, const DmVector& maxCorner, GsDrawList& out) const;

    /// @brief 一组顶层实体的绘制命令（高亮叠加）；不在模型里的跳过
    /// @param skipSelected 跳过选中的（选中优先：已选中的按选中色画在场景里，不再高亮）
    void collectEntities(const std::vector<DmEntity*>& entities, GsDrawList& out, bool skipSelected) const;

    /// @brief 选中的实体的绘制命令（场景里细线画不出加宽，选中的线另按四边形画）
    /// @return 选中的多于 limit 个时什么也不收集，返回 false
    bool collectSelected(GsDrawList& out, std::size_t limit) const;

    /// @brief 实体的槽位；不在模型里时为 kGsNoSlot
    std::uint32_t slotOf(const DmEntity* entity) const;

    /// @brief 槽位的容量（每视图的状态位图按它分配）
    std::uint32_t slotCapacity() const { return static_cast<std::uint32_t>(m_states.size()); }

    /// @brief 各分块的原点（按分块序号；视图每帧减去视点放进分块偏移）
    const std::vector<DmVector>& cellOrigins() const { return m_cellOrigins; }

    /// @brief 模型有没有可画的 GPU 数据（update 之后）
    bool isReady() const { return m_modelGroup != nullptr; }

    /// @brief 第 1 组（对象状态、图层表、线型表、图元记录）
    const RhiBindGroup* modelGroup() const { return m_modelGroup.get(); }
    /// @brief 第 2 组：某个管线类的几何
    const RhiBindGroup* geometryGroup(GsClass c) const { return m_geometryGroups[static_cast<std::size_t>(c)].get(); }
    /// @brief 实例记录（几何管线的顶点缓冲）
    const RhiBuffer* instanceBuffer() const { return m_instanceArena.buffer().get(); }

    /// @brief 块缩略图：块定义 block 的共享几何（含嵌套块）按 transform 画的命令与实例记录
    /// @details 实例记录由调用方放进自己的实例缓冲（firstInstance 从 0 起），分块序号为 0。块不在模型里时先编译它
    void collectBlock(const DmBlock* block, const GiTransform& transform, GsDrawList& out,
                      std::vector<GsInstanceRecord>& instances);

    /// @brief 实例的槽位、绘图次序等不变时，模型里的东西的包围框（容器模型取景用）
    bool bounds(DmVector& minCorner, DmVector& maxCorner) const;

    /// @brief YICAD_GS_VERIFY=1 时抽查到的"实体改了却没有登记变更"的次数（测试用）
    std::uint64_t verifyMismatches() const { return m_verifyMismatches; }

private:
    struct Node;
    struct Cell;
    struct Shared;

    // ---- 根与全部重建 ----
    void initialize();
    void rebuildAll();
    void releaseAll();
    template <typename F>
    void forEachRootEntity(F&& f) const;

    // ---- 节点 ----
    Node* createNode(DmEntity* entity);
    void refreshNode(Node& node);
    void removeNode(Node* node);
    void placeNode(Node& node);
    void unplaceNode(Node& node);
    std::uint32_t allocateSlot();
    std::uint32_t orderOf(const DmEntity* entity);

    // ---- 分块（松散四叉树） ----
    struct QuadNode;
    Cell& cellFor(Node& node);
    Cell& createCell(QuadNode* quad, const DmVector& origin);
    void growRoot(const DmVector& minCorner, const DmVector& maxCorner);
    void splitIfNeeded(Cell& cell);
    void markCellDirty(Cell& cell);
    void compileCell(Cell& cell);
    void releaseCell(Cell& cell);

    // ---- 共享几何 ----
    Shared& sharedFor(const IGiDrawable& drawable);
    void compileShared(Shared& shared);
    void releaseShared(Shared& shared);
    void markSharedDirty(Shared& shared);
    void destroyShared(const IGiDrawable* drawable);
    bool isDashed(const Shared& shared, const GsAttributes& byBlock);
    bool flattenCheck(Shared& shared, const GiTransform& transform, const GsAttributes& byBlock, bool* nonUniform, int depth);
    /// @brief 一处共享对象的使用展开到叶子实例
    struct Leaf
    {
        Shared* shared = nullptr;
        GiTransform transform;   ///< 定义坐标（不含共享几何的原点）-> 世界
        GsAttributes byBlock;    ///< 已解析：种类只有值与随层
    };
    void expand(Shared& shared, const GiTransform& transform, const GsAttributes& byBlock, std::vector<Leaf>& leaves,
                int depth);
    static GsAttributes resolveAgainst(const GsAttributes& inner, const GsAttributes& outer);
    GsInstanceRecord instanceRecord(const Leaf& leaf, const DmVector& origin, std::uint32_t slot, std::uint32_t cell) const;
    /// @brief 一个节点的绘制命令（顶层几何、共享几何的实例、图片）
    void appendNode(const Node& node, GsDrawList& out) const;
    /// @brief 分块的全部实例记录（按 instanceSources 的顺序），整体变换改了时只重写它们
    std::vector<GsInstanceRecord> instanceRecords(const Cell& cell) const;

    // ---- 表 ----
    void readLayers();
    void readLineTypes();
    std::uint16_t layerIndexOf(const DmLayer* layer);
    std::uint16_t lineTypeIndexOf(const DmLineType* lineType);
    bool layerDashed(std::uint16_t layer) const;
    void updateSelection();
    void setStateFlag(std::uint32_t slot, std::uint32_t flag, bool on);
    /// @brief 对象状态的这个槽位要重新上传
    void markStateDirty(std::uint32_t slot);
    bool isSelectedSlot(std::uint32_t slot) const;

    // ---- GPU ----
    void uploadAll(GsDevice& device);
    void rebuildInfiniteLines();
    void stage(GsArena& arena, const GsRange& range, std::vector<std::byte> bytes);
    void uploadStates(GsDevice& device);
    void rebuildBindGroups(GsDevice& device);
    void verifyRevisions();

    class CompileContext;
    friend class CompileContext;

    DmDocument* m_document = nullptr;
    const DmEntityContainer* m_container = nullptr;
    const EntityTable* m_rootTable = nullptr;
    const DmBlock* m_rootOwner = nullptr;   ///< 根实体表所属的块；为空为模型空间

    std::unique_ptr<CompileContext> m_compileContext;
    std::unique_ptr<GsCompiler> m_compiler;

    std::unordered_map<const void*, std::unique_ptr<Node>> m_nodes;   ///< 实体地址 -> 节点
    std::unordered_map<const void*, std::uint32_t> m_orders;          ///< 实体地址 -> 绘图次序（删除后撤销时恢复原位）
    std::uint32_t m_nextOrder = 0;
    std::vector<std::uint32_t> m_freeSlots;
    std::vector<Node*> m_slotNodes;                                    ///< 槽位 -> 节点

    std::unique_ptr<QuadNode> m_root;
    std::vector<std::unique_ptr<Cell>> m_cells;                        ///< 按分块序号；序号 0 是总是可见的分块
    std::vector<std::uint32_t> m_freeCells;
    std::vector<DmVector> m_cellOrigins;
    std::unordered_set<Cell*> m_dirtyCells;

    std::unordered_map<const IGiDrawable*, std::unique_ptr<Shared>> m_shared;
    std::unordered_set<Shared*> m_dirtyShared;

    std::unordered_map<const DmLayer*, std::uint16_t> m_layerIndex;
    std::vector<GsLayerRecord> m_layers;
    std::unordered_map<const DmLineType*, std::uint16_t> m_lineTypeIndex;
    std::vector<GsLineTypeRecord> m_lineTypes;
    std::vector<bool> m_lineTypeDashed;
    bool m_tablesDirty = true;

    std::vector<GsObjectState> m_states;   ///< 按槽位；CPU 上的副本
    std::uint32_t m_statesDirtyBegin = 0;
    std::uint32_t m_statesDirtyEnd = 0;
    bool m_statesResized = true;

    const ISelectionSource* m_selection = nullptr;
    std::vector<std::uint32_t> m_selectedSlots;      ///< 上一次选中的槽位（选中标记在对象状态里）
    std::vector<std::uint32_t> m_dirtySlots;         ///< 改过的槽位，多了就不再记、按范围上传
    bool m_selectionDirty = false;

    GiTransform m_rootTransform;          ///< 容器模型的整体变换
    bool m_rootTransformDirty = false;

    bool m_fullRebuild = true;

    /// @brief 累积的变更：收到变更集时就合并，删除的地址抵消之前对它的修改
    /// @details 不能存下各个变更集再按顺序处理：前一个变更集里的实体可能在后一个里已经删除，
    ///          处理前一个时就会访问到悬空指针。合并后先处理删除、再处理修改；同一地址删除后又分配给新实体时，
    ///          旧实体的节点先删掉，新实体再建节点
    struct PendingChanges
    {
        std::vector<DmEntityChange> entities;                    ///< 按第一次修改的顺序；删除的置空
        std::unordered_map<const void*, std::size_t> entityIndex;
        std::vector<const void*> destroyedEntities;
        std::vector<const DmBlock*> blocks;                      ///< 删除的置空
        std::unordered_map<const void*, std::size_t> blockIndex;
        std::vector<const void*> destroyedBlocks;
        bool layersChanged = false;
        bool lineTypesChanged = false;

        void merge(const DmChangeSet& changes);
        bool isEmpty() const;
    };
    PendingChanges m_pending;

    // GPU
    std::shared_ptr<GsDevice> m_device;
    std::array<std::unique_ptr<GsArena>, kGsClassCount> m_classArenas;
    GsArena m_primArena;
    GsArena m_instanceArena;
    struct StagedWrite
    {
        GsArena* arena = nullptr;
        GsRange range;
        std::vector<std::byte> bytes;
    };
    std::vector<StagedWrite> m_staged;
    RhiBufferPtr m_stateBuffer;
    RhiBufferPtr m_layerBuffer;
    RhiBufferPtr m_lineTypeBuffer;
    RhiBufferPtr m_infiniteBuffer;          ///< 无限线：全部节点的射线、构造线
    std::uint32_t m_infiniteCount = 0;
    bool m_infiniteDirty = false;
    RhiBindGroupPtr m_modelGroup;
    std::array<RhiBindGroupPtr, kGsClassCount> m_geometryGroups;
    std::uint64_t m_boundGeneration = ~0ull;
    std::uint64_t m_version = 1;
    std::uint64_t m_verifyCursor = 0;
    std::uint64_t m_verifyMismatches = 0;
    bool m_verify = false;
    std::vector<RhiTexturePtr> m_retainedTextures;   ///< 重新编译的分块原先用的图片纹理，留到这一轮更新结束
};

#endif // GSMODEL_H
