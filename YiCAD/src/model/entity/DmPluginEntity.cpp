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

/// @file DmPluginEntity.cpp
/// @brief 插件实体的实现

#include "DmPluginEntity.h"

#include "IGiDrawable.h"
#include "Stream.h"

TYPESYSTEM_SOURCE(DmPluginEntity, DmCustomEntity, 0)

namespace
{
/// @brief 拖动夹点时按位置认夹点的容差（夹点位置由 getRefPoints 原样交给编辑工具）
constexpr double kGripTolerance = 1.0e-6;

/// @brief 把插件的 worldDraw 当作可绘制对象，记 GI 流用
class PluginDraw final : public IGiDrawable
{
public:
    PluginDraw(const DmPluginEntityClass& entityClass, const std::string& data, void* cache, const DmEntity& entity)
        : m_class(entityClass)
        , m_data(data)
        , m_cache(cache)
        , m_entity(entity)
    {
    }

    void worldDraw(IGiWorldDraw& wd) const override
    {
        m_class.worldDraw(m_data, m_cache, wd, &m_entity);
    }

private:
    const DmPluginEntityClass& m_class;
    const std::string& m_data;
    void* m_cache;
    const DmEntity& m_entity;
};
}  // namespace

DmPluginEntity::DmPluginEntity(std::shared_ptr<DmPluginEntityClass> entityClass, std::string data)
    : m_class(std::move(entityClass))
    , m_data(std::move(data))
{
    if (!m_data.empty())
    {
        update();
    }
}

DmPluginEntity::DmPluginEntity(const DmPluginEntity& other)
    : DmCustomEntity(other)
    , m_class(other.m_class)
    , m_data(other.m_data)
    , m_graphics(other.m_graphics)
{
    // 实例缓存属原对象，副本自己建一份
    if (callable())
    {
        m_cache = m_class->createCache(m_data);
    }
}

DmPluginEntity::~DmPluginEntity()
{
    // 插件卸载时已收回全部缓存
    if (m_cache && m_class && m_class->alive())
    {
        m_class->destroyCache(m_cache);
    }
}

DmEntity* DmPluginEntity::clone() const
{
    auto* copy = new DmPluginEntity(*this);
    copy->m_ulID = DmId();
    return copy;
}

QString DmPluginEntity::className() const
{
    return m_class ? m_class->name() : QString();
}

std::uint32_t DmPluginEntity::classVersion() const
{
    return m_class ? m_class->version() : 0;
}

DmProxyFlags DmPluginEntity::proxyFlags() const
{
    return m_class ? m_class->proxyFlags() : DmProxyFlags::None;
}

void DmPluginEntity::setData(std::string data)
{
    m_data = std::move(data);
    update();
}

void DmPluginEntity::resetCache()
{
    if (!m_class || !m_class->alive())
    {
        return;
    }
    if (m_cache)
    {
        m_class->destroyCache(m_cache);
        m_cache = nullptr;
    }
    if (callable())
    {
        m_cache = m_class->createCache(m_data);
    }
}

void DmPluginEntity::update()
{
    // 插件已卸载（程序退出时）：保留原来记下的图形。还没有数据（读盘时按类建出、尚未读入）时不调插件
    resetCache();
    if (callable())
    {
        if (m_class->threadSafeDraw())
        {
            m_graphics.clear();
        }
        else
        {
            m_graphics = GiStreamRecorder::record(PluginDraw(*m_class, m_data, m_cache, *this), GiRegenType::Display);
        }
    }
    DmCustomEntity::update();
}

void DmPluginEntity::worldDraw(IGiWorldDraw& wd) const
{
    if (callable() && m_class->threadSafeDraw())
    {
        m_class->worldDraw(m_data, m_cache, wd, this);
        return;
    }
    // 只重放图元段：属性按实体自己的（setAttributes），与代理实体相同
    GiStreamDrawable(m_graphics).worldDraw(wd);
}

GiStream DmPluginEntity::proxyGraphics() const
{
    if (callable() && m_class->threadSafeDraw())
    {
        return DmCustomEntity::proxyGraphics();
    }
    return m_graphics;
}

void DmPluginEntity::calculateBorders()
{
    DmVector minCorner;
    DmVector maxCorner;
    if (callable() && m_class->extents(m_data, m_cache, minCorner, maxCorner))
    {
        resetBorders();
        minV = minCorner;
        maxV = maxCorner;
        return;
    }
    DmCustomEntity::calculateBorders();
}

void DmPluginEntity::applyTransform(const GiTransform& transform)
{
    std::string out;
    if (callable() && m_class->transform(m_data, m_cache, transform, out))
    {
        m_data = std::move(out);
    }
    update();
}

void DmPluginEntity::move(const DmVector& offset)
{
    applyTransform(GiTransform::translation(offset));
}

void DmPluginEntity::rotate(const DmVector& center, const DmVector& angleVector)
{
    applyTransform(GiTransform::rotation(angleVector.angle(), center));
}

void DmPluginEntity::scale(const DmVector& center, const DmVector& factor)
{
    applyTransform(GiTransform::scaling(factor, center));
}

void DmPluginEntity::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    applyTransform(GiTransform::mirroring(axisPoint1, axisPoint2));
}

DmVectorSolutions DmPluginEntity::getRefPoints() const
{
    DmVectorSolutions points;
    std::vector<DmVector> grips;
    if (callable() && m_class->hasGrips() && m_class->grips(m_data, m_cache, grips))
    {
        for (const DmVector& p : grips)
        {
            points.push_back(p);
        }
    }
    return points;
}

void DmPluginEntity::moveRef(const DmVector& ref, const DmVector& offset)
{
    std::vector<DmVector> grips;
    if (!callable() || !m_class->hasGrips() || !m_class->grips(m_data, m_cache, grips))
    {
        return;
    }
    // 重合的夹点一起拖（同 AutoCAD 拖动重合夹点）
    std::vector<std::uint32_t> indices;
    for (std::size_t i = 0; i < grips.size(); ++i)
    {
        if (grips[i].distanceTo(ref) < kGripTolerance)
        {
            indices.push_back(static_cast<std::uint32_t>(i));
        }
    }
    if (indices.empty())
    {
        return;
    }
    std::string out;
    if (m_class->moveGrips(m_data, m_cache, indices, offset, out))
    {
        m_data = std::move(out);
    }
    update();
}

DmVector DmPluginEntity::nearestSnap(DmPluginEntityClass::SnapMode mode, const DmVector& coord, double* dist) const
{
    std::vector<DmVector> candidates;
    double best = DM_MAXDOUBLE;
    DmVector result(false);
    if (m_class->snapPoints(m_data, m_cache, mode, coord, candidates))
    {
        for (const DmVector& p : candidates)
        {
            const double d = p.distanceTo(coord);
            if (d < best)
            {
                best = d;
                result = p;
            }
        }
    }
    if (dist)
    {
        *dist = best;
    }
    return result;
}

DmVector DmPluginEntity::getNearestEndpoint(const DmVector& coord, double* dist) const
{
    if (callable() && m_class->hasSnapPoints())
    {
        return nearestSnap(DmPluginEntityClass::SnapMode::Endpoint, coord, dist);
    }
    return DmCustomEntity::getNearestEndpoint(coord, dist);
}

DmVector DmPluginEntity::getNearestPointOnEntity(const DmVector& coord, bool onEntity, double* dist,
                                                 DmEntity** entity) const
{
    if (callable() && m_class->hasSnapPoints())
    {
        if (entity)
        {
            *entity = const_cast<DmPluginEntity*>(this);
        }
        return nearestSnap(DmPluginEntityClass::SnapMode::Nearest, coord, dist);
    }
    return DmCustomEntity::getNearestPointOnEntity(coord, onEntity, dist, entity);
}

DmVector DmPluginEntity::getNearestCenter(const DmVector& coord, double* dist) const
{
    if (callable() && m_class->hasSnapPoints())
    {
        return nearestSnap(DmPluginEntityClass::SnapMode::Center, coord, dist);
    }
    return DmCustomEntity::getNearestCenter(coord, dist);
}

DmVector DmPluginEntity::getNearestMiddle(const DmVector& coord, double* dist, int middlePoints) const
{
    if (callable() && m_class->hasSnapPoints())
    {
        return nearestSnap(DmPluginEntityClass::SnapMode::Midpoint, coord, dist);
    }
    return DmCustomEntity::getNearestMiddle(coord, dist, middlePoints);
}

std::vector<DmEntity*> DmPluginEntity::explode() const
{
    if (callable() && m_class->hasExplode())
    {
        std::vector<DmEntity*> out;
        if (m_class->explode(m_data, m_cache, *this, out))
        {
            return out;
        }
        // 插件炸开失败：丢掉做了一半的结果，按 GI 流炸开（炸开命令会删掉原实体，不能交出空结果）
        for (DmEntity* e : out)
        {
            delete e;
        }
    }
    return DmCustomEntity::explode();
}

void DmPluginEntity::saveData(OutputStream& out) const
{
    for (char c : m_data)
    {
        out << static_cast<std::uint8_t>(c);
    }
}

bool DmPluginEntity::restoreData(InputStream& in, std::uint32_t version)
{
    std::string bytes;
    while (!in.end())
    {
        std::uint8_t value = 0;
        in >> value;
        bytes.push_back(static_cast<char>(value));
    }
    return restoreDataBytes(bytes, version);
}

void DmPluginEntity::saveDataBytes(OutputStream& out) const
{
    // 与 DmCustomEntity::saveDataBytes 的格式相同：数据版本、带长度的字节
    out << classVersion();
    out << static_cast<std::uint64_t>(m_data.size());
    for (char c : m_data)
    {
        out << static_cast<std::uint8_t>(c);
    }
}

bool DmPluginEntity::restoreDataBytes(const std::string& bytes, std::uint32_t version)
{
    if (!m_class)
    {
        return false;
    }
    const std::uint32_t current = m_class->version();
    if (version > current)
    {
        // 比插件新的数据读不了：宿主改建代理，数据原样保留
        return false;
    }
    if (version < current)
    {
        // 插件没提供升级时读不了旧编码，宁可不碰：同样改建代理
        std::string upgraded;
        if (!callable() || !m_class->hasUpgrade() || !m_class->upgrade(version, bytes, upgraded))
        {
            return false;
        }
        m_data = std::move(upgraded);
        return true;
    }
    m_data = bytes;
    return true;
}

void DmPluginEntity::applyStoredTransform(const GiTransform& transform)
{
    // 代理被变换过：一次交给插件；之后 restoreStream 会 update()。实例缓存还是换数据之前的，不给插件
    std::string out;
    if (!transform.isIdentity() && callable() && m_class->transform(m_data, nullptr, transform, out))
    {
        m_data = std::move(out);
    }
}
