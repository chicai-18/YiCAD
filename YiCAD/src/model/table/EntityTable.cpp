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

/// @file EntityTable.cpp
/// @brief 实体表实现

#include "EntityTable.h"
#include "ScopedTimer.h"
#include "DmDocument.h"
#include "DmIdManager.h"
#include "EntityTableCmd.h"
#include "Cmd.h"
#include "DmSolid.h"
#include "DmTriangle.h"
#include <algorithm>
#include <limits>
#include <unordered_set>

namespace
{
void collectSearchEntitiesRecursive(DmEntity* entity, std::vector<DmEntity*>& ents,
    std::unordered_set<DmEntity*>& visited)
{
    if (!entity || visited.find(entity) != visited.end())
    {
        return;
    }

    visited.insert(entity);

    std::list<DmEntity*> subEntities = entity->getSubEntities();
    if (!subEntities.empty())
    {
        for (auto sub : subEntities)
        {
            collectSearchEntitiesRecursive(sub, ents, visited);
        }
        return;
    }

    ents.emplace_back(entity);
}

/// @brief 交叉选（从右往左）时判断实体是否与窗口相交
/// @details 基本实体直接与窗口四条边求交；文字等有子实体的复杂实体逐个判断子实体。
///          放在 entitiesInRect 之外：逐实体的快速判断保持短小，只有需要时才进入这里。
bool crossesWindow(DmEntity* e, const DmVector& v1, const DmVector& v2)
{
    bool included = false;
    DmEntityContainer l;
    l.addRectangle(v1, v2);
    DmVectorSolutions sol;

    auto subEntities = e->getSubEntities();
    // 直线，圆弧，Solid，样条线等基本实体
    if (subEntities.size() == 0)
    {
        if (e->getEntityType() == DM::EntityTriangle)
        {
            included = static_cast<DmTriangle*>(e)->isInCrossWindow(v1, v2);
        }
        else if (e->getEntityType() == DM::EntitySolid)
        {
            included = static_cast<DmSolid*>(e)->isInCrossWindow(v1, v2);
        }
        else
        {
            for (auto line : l)
            {
                sol = Information::getIntersection(e, line, true);
                if (sol.hasValid())
                {
                    included = true;
                    break;
                }
            }
        }
    }
    // 文字等复杂实体，判断子实体是否相交
    else
    {
        for (auto subEnt : subEntities)
        {
            if (subEnt->isInWindow(v1, v2))
            {
                included = true;
            }
            else if (subEnt->getEntityType() == DM::EntityTriangle)
            {
                included = static_cast<DmTriangle*>(subEnt)->isInCrossWindow(v1, v2);
            }
            else if (subEnt->getEntityType() == DM::EntitySolid)
            {
                included = static_cast<DmSolid*>(subEnt)->isInCrossWindow(v1, v2);
            }
            else
            {
                for (auto line : l)
                {
                    sol = Information::getIntersection(subEnt, line, true);
                    if (sol.hasValid())
                    {
                        included = true;
                        break;
                    }
                }
            }

            if (included)
            {
                break;
            }
        }
    }
    return included;
}
}

EntityTable::EntityTable()
{
    m_entContainer.setOwner(false);//m_entContainer没有控制权
}

EntityTable::~EntityTable()
{
    for (auto e : m_ents)
    {
        delete e;
    }
    m_ents.clear();
    m_entMap.clear();
}

/// @brief 直接添加实体
bool EntityTable::add_direct(DmEntity *e)
{
    if (!m_pDoc)
        return false;
    DmId id = e->getId();
    if (!id.isValid())
    {
        id = m_pDoc->getIdManager()->assignID(e);
    }
    auto it = m_entMap.find(id);
    if (it != m_entMap.end())
        return false;
    m_entMap[id] = e;
    m_ents.emplace_back(e);
    m_searchTree.insert(e);
    touch(e);
    return true;
}

/// @brief 搜索包围框与指定区域有重叠的实体
void EntityTable::searchEntities(const DmVector &min, const DmVector &max, std::vector<DmEntity *> &ents, bool onlyVisible, bool searchSubEnts)
{
    if (searchSubEnts)
    {
        std::vector<DmEntity*> found;
        m_searchTree.search(min, max, found);

        std::unordered_set<DmEntity*> visited;
        std::vector<DmEntity*> expanded;
        expanded.reserve(found.size());
        for (auto e : found)
        {
            collectSearchEntitiesRecursive(e, expanded, visited);
        }
        ents = std::move(expanded);
    }
    else
    {
        std::vector<DmEntity*> found;
        m_searchTree.search(min, max, found);

        // 树中每个实体只有一份（SpacialSearchTree::insert 对已在树中的实体只做更新），
        // 命中的若都没有父实体，结果本身不会重复，不必逐个去重——覆盖全图的
        // 框选会命中全部实体，去重的开销远大于树查询本身。
        bool hasParent = std::any_of(found.begin(), found.end(),
            [](DmEntity* e) { return e->getParent() != nullptr; });
        if (!hasParent)
        {
            ents = std::move(found);
        }
        else
        {
            // 从已找到的子实体中向上寻找顶层实体，按指针去重
            std::unordered_set<DmEntity*> seen;
            seen.reserve(found.size());
            std::vector<DmEntity*> docEnts;
            docEnts.reserve(found.size());
            for (auto subEnt : found)
            {
                DmEntity* parent = subEnt->getParent();
                DmEntity* child = subEnt;
                while (parent)
                {
                    child = parent;
                    parent = parent->getParent();
                }
                if (seen.insert(child).second)
                {
                    docEnts.emplace_back(child);
                }
            }
            ents = std::move(docEnts);
        }
    }

    // 仅可见实体
    if (onlyVisible)
    {
        ents.erase(std::remove_if(ents.begin(), ents.end(),
            [](DmEntity* e) { return !e->isVisible() || e->isErased(); }), ents.end());
    }
}

/// @brief 完全落在矩形内的顶层实体（窗选）
std::vector<DmEntity*> EntityTable::entitiesInsideRect(const DmVector& corner1, const DmVector& corner2,
    const std::list<DM::EntityType>& types)
{
    return entitiesInRect(corner1, corner2, false, types);
}

/// @brief 落在矩形内或与矩形边界相交的顶层实体（交叉选）
std::vector<DmEntity*> EntityTable::entitiesCrossingRect(const DmVector& corner1, const DmVector& corner2,
    const std::list<DM::EntityType>& types)
{
    return entitiesInRect(corner1, corner2, true, types);
}

/// @brief 两个矩形查询的共同实现
std::vector<DmEntity*> EntityTable::entitiesInRect(const DmVector& corner1, const DmVector& corner2,
    bool crossing, const std::list<DM::EntityType>& types)
{
    DmVector min(std::min(corner1.x, corner2.x), std::min(corner1.y, corner2.y));
    DmVector max(std::max(corner1.x, corner2.x), std::max(corner1.y, corner2.y));

    std::vector<DmEntity*> hits;

    // 判断单个顶层实体是否命中
    auto collectIfHit = [&](DmEntity* e)
    {
        if (!types.empty() && std::find(types.begin(), types.end(), e->getEntityType()) == types.end())
        {
            return;
        }

        if (!e->isVisible() || e->isErased())
        {
            return;
        }

        // 先用顶层实体包围盒做一次粗过滤，避免全量跑几何相交判断。
        if (e->getMax().x < min.x || e->getMin().x > max.x
            || e->getMax().y < min.y || e->getMin().y > max.y)
        {
            return;
        }

        // 完全包含；交叉选时与矩形相交也算
        if (e->isInWindow(corner1, corner2) || (crossing && crossesWindow(e, corner1, corner2)))
        {
            hits.push_back(e);
        }
    };

    // 候选实体的取法（P10）：矩形盖住全部实体的包围框时每个实体都会命中，
    // 顺序遍历实体表最快；否则用空间搜索树只取包围盒与矩形重叠的顶层实体，
    // 取法与点选（Snapper::catchEntity 的 ResolveNone 分支）相同。两条路径对每个
    // 实体做同样的判断，结果一致，只是快慢不同。
    DmVector allMin, allMax;
    bool coversAll = getSearchBounds(allMin, allMax)
        && min.x <= allMin.x && min.y <= allMin.y && max.x >= allMax.x && max.y >= allMax.y;
    if (coversAll)
    {
        for (auto e : *this)
        {
            collectIfHit(e);
        }
    }
    else
    {
        // 可见性留给 collectIfHit 判断，不让 searchEntities 再过滤一遍
        std::vector<DmEntity*> candidates;
        searchEntities(min, max, candidates, false, false);
        for (auto e : candidates)
        {
            collectIfHit(e);
        }
    }
    return hits;
}

/// @brief 获得第一个未被删除的索引
int EntityTable::getFirstValidIndex() const
{
    int firstValidIdx = -1;
    int size = (int)m_ents.size();
    for (int i = 0; i < size; i++)
    {
        if (!m_ents.at(i)->isErased())
        {
            firstValidIdx = i;
            break;
        }
    }
    return firstValidIdx;
}

/// @brief 更新实体容器
void EntityTable::updateContainer()
{
    m_entContainer.clear();
    for (auto obj : m_ents)
    {
        if (!obj->isErased())
        {
            m_entContainer.addEntity(obj);
        }
    }
}

/// @brief 直接删除实体
bool EntityTable::remove_direct(DmEntity *obj)
{
    m_pDoc->changeTracker().destroyEntity(obj, m_ownerBlock);
    auto it2 = std::find(m_ents.begin(), m_ents.end(), obj);
    m_ents.erase(it2);
    m_entMap.erase(obj->getId());
    m_searchTree.remove(obj);
    m_pDoc->getIdManager()->removeID(obj->getId());
    delete obj;
    return true;
}

/// @brief 直接清空所有实体
void EntityTable::clear_direct()
{
    for (auto obj : m_ents)
    {
        m_pDoc->changeTracker().destroyEntity(obj, m_ownerBlock);
        m_entMap.erase(obj->getId());
        m_searchTree.remove(obj);
        m_pDoc->getIdManager()->removeID(obj->getId());
        delete obj;
    }
    m_ents.clear();
    m_entContainer.clear();
}

/// @brief 添加实体
void EntityTable::add(DmEntity *e)
{
    if (!m_pDoc)
        return;
    DmId id = e->getId();
    if (!id.isValid())
    {
        id = m_pDoc->getIdManager()->assignID(e);
    }
    EntityTableAddCmd* cmd = new EntityTableAddCmd(this, e);
    m_pDoc->getCmdManager()->addAndExecuteCmd(cmd);
}

/// @brief 通过 id 移除实体
void EntityTable::remove(DmId id)
{
    if (!m_pDoc)
        return;
    auto it = m_entMap.find(id);
    if (it == m_entMap.end())
        return;
    EntityTableRemoveCmd* cmd = new EntityTableRemoveCmd(this, it->second);
    m_pDoc->getCmdManager()->addAndExecuteCmd(cmd);
}

/// @brief 移除实体
void EntityTable::remove(DmEntity *e)
{
    DmId id = e->getId();
    remove(id);
}

/// @brief 通过 id 查找实体
DmEntity *EntityTable::find(DmId id)
{
    auto it = m_entMap.find(id);
    if (it == m_entMap.end())
        return nullptr;
    return it->second;
}

/// @brief 开始修改实体
void EntityTable::startModify(DmObject *e)
{
    DmEntity* ent = static_cast<DmEntity*>(e);
    EntityTableModifyCmd* cmd = new EntityTableModifyCmd(this, ent);
    m_pDoc->getCmdManager()->addToCurrentCmd(cmd);
    touch(ent);
}

void EntityTable::notifyEntityModified(DmEntity* e)
{
    m_searchTree.update(e);
    touch(e);
}

void EntityTable::touch(DmEntity* e)
{
    if (m_pDoc)
    {
        m_pDoc->changeTracker().touchEntity(e, m_ownerBlock);
    }
}

EntityTable::iterator EntityTable::begin()
{
    return EntityTable::iterator(m_ents.begin(), m_ents.end());
}

EntityTable::iterator EntityTable::end()
{
    return EntityTable::iterator(m_ents.end());
}

EntityTable::const_iterator EntityTable::begin() const
{
    return EntityTable::const_iterator(m_ents.begin(), m_ents.end());
}

EntityTable::const_iterator EntityTable::end() const
{
    return EntityTable::const_iterator(m_ents.end());
}

/// @brief 获得实体数（不含已删除）
int EntityTable::count() const
{
    int c = 0;
    for (auto it = m_ents.begin(); it != m_ents.end(); ++it)
    {
        if (!(*it)->isErased())
            c++;
    }
    return c;
}

/// @brief 查找从指定坐标沿指定角度方向延伸的虚拟构造线与最近实体的交点
/// @param [in] coord 起始坐标
/// @param [in] angle 射线角度（弧度）
/// @param [out] dist 返回最近交点的距离（可为 nullptr）
/// @return 最近的虚拟交点；若无交点或最近实体不存在，则返回 coord 本身
DmVector EntityTable::getNearestVirtualIntersection(const DmVector& coord, const double& angle, double* dist)
{
    // 虚拟交点捕捉耗时埋点，默认关闭，见 ScopedTimer.h。
    YICAD_SCOPED_TIMER(yicad::counters::nearestVirtualIntersection());

    DmVector point;

    // 查找离起始坐标最近的实体，不限距离，跳过不可见实体与顶层图片实体，
    // 取解析到的子实体——与原先对渲染容器调用
    // getNearestEntity(coord, nullptr, DM::ResolveAllButTextImage) 的语义一致，
    // 改为在空间搜索树上做最近邻查询，不再遍历全部实体（P10）。
    const DmEntity* nearest = m_searchTree.nearest(coord, [&coord](DmEntity* e)
    {
        if (e->isErased() || !e->isVisible() || e->getEntityType() == DM::EntityImage)
        {
            return std::numeric_limits<double>::infinity();
        }
        DmEntity* subEntity = nullptr;
        return e->getDistanceToPoint(coord, &subEntity, DM::ResolveAllButTextImage);
    });
    DmEntity* closestEntity = nullptr;
    if (nearest)
    {
        nearest->getDistanceToPoint(coord, &closestEntity, DM::ResolveAllButTextImage);
    }

    if (closestEntity)
    {
        // 创建一条通过起始坐标、方向为指定角度的临时构造线（无限直线）
        DmVector direction;
        direction.set(angle);
        DmConstructionLineData data(coord, coord + direction);
        DmConstructionLine line(nullptr, data);

        // 计算构造线与最近实体的几何交点
        DmVectorSolutions sol = Information::getIntersection(closestEntity, &line, true);
        if (sol.getVector().empty())
        {
            // 无交点，返回原始坐标
            return coord;
        }
        else
        {
            // 返回离起始坐标最近的交点
            point = sol.getClosest(coord, dist, nullptr);
            return point;
        }
    }
    else
    {
        // 附近无实体，返回原始坐标
        return coord;
    }
}
