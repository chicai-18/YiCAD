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
#include "DmDocument.h"
#include "DmIdManager.h"
#include "EntityTableCmd.h"
#include "Cmd.h"
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
        // 查找doc下的实体，从已找到的子实体中向上寻找父实体
        std::unordered_map<DmId, DmEntity*> docEntMap;
        std::vector<DmEntity*> docEnts;
        std::vector<DmEntity*> subEnts;
        m_searchTree.search(min, max, subEnts);
        for (auto subEnt : subEnts)
        {
            DmEntity* parent = subEnt->getParent();
            DmEntity* child = subEnt;
            while (parent)
            {
                child = parent;
                parent = parent->getParent();
            }
            if (docEntMap.find(child->getId()) == docEntMap.end())
            {
                docEntMap[child->getId()] = child;
                docEnts.emplace_back(child);
            }
        }
        ents = docEnts;
    }

    // 仅可见实体
    if (onlyVisible)
    {
        std::vector<DmEntity*> theEnts;
        theEnts.reserve(ents.size());
        for (auto e : ents)
        {
            if (e->isVisible() && !e->isErased())
            {
               theEnts.emplace_back(e);
            }
        }
        ents = theEnts;
    }
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

/// @brief 检查是否有选中的实体
bool EntityTable::hasSelect() const
{
    for (auto it = m_ents.begin(); it != m_ents.end(); ++it)
    {
        if (!(*it)->isErased() && (*it)->isSelected())
            return true;
    }
    return false;
}

/// @brief 获取选中实体数量
int EntityTable::countSelect() const
{
    int c = 0;
    for (auto it = m_ents.begin(); it != m_ents.end(); ++it)
    {
        if (!(*it)->isErased() && (*it)->isSelected())
            c++;
    }
    return c;
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

/// @brief 获得选中实体中最近的拖拽点
DmVector EntityTable::getNearestSelectedRef(const DmVector& coord, double* dist /*= nullptr*/) const
{
    double minDist = DM_MAXDOUBLE;  // minimum measured distance
    double curDist;                 // currently measured distance
    DmVector closestPoint(false);   // closest found endpoint
    DmVector point;                 // endpoint found

    for (auto en : m_ents)
    {
        if (!en->isErased() && en->isVisible() && en->isSelected() && !en->isParentSelected())
        {
            point = en->getNearestSelectedRef(coord, &curDist);
            if (point.valid && curDist < minDist)
            {
                closestPoint = point;
                minDist = curDist;
                if (dist)
                {
                    *dist = minDist;
                }
            }
        }
    }
    return closestPoint;
}

/// @brief 查找从指定坐标沿指定角度方向延伸的虚拟构造线与最近实体的交点
/// @param [in] coord 起始坐标
/// @param [in] angle 射线角度（弧度）
/// @param [out] dist 返回最近交点的距离（可为 nullptr）
/// @return 最近的虚拟交点；若无交点或最近实体不存在，则返回 coord 本身
DmVector EntityTable::getNearestVirtualIntersection(const DmVector& coord, const double& angle, double* dist)
{
    DmVector point;

    // 查找离起始坐标最近的实体（排除文本和图片实体）
    DmEntity* closestEntity = m_entContainer.getNearestEntity(coord, nullptr, DM::ResolveAllButTextImage);

    if (closestEntity)
    {
        // 创建一条通过起始坐标、方向为指定角度的临时构造线（无限直线）
        DmVector direction;
        direction.set(angle);
        DmConstructionLineData data(coord, coord + direction);
        auto line = new DmConstructionLine(&m_entContainer, data);

        // 计算构造线与最近实体的几何交点
        DmVectorSolutions sol = Information::getIntersection(closestEntity, line, true);
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
