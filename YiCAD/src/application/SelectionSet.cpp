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

/// @file SelectionSet.cpp
/// @brief 选择集的实现：并入了原 model/edit/Selection 的选择函数

#include "SelectionSet.h"

#include "DmDocument.h"
#include "DmEntity.h"
#include "DmLayer.h"
#include "EntityTable.h"
#include "ScopedTimer.h"

SelectionSet::SelectionSet(DmDocument& document)
    : m_document(document)
{
    m_document.addListener(this);
}

SelectionSet::~SelectionSet()
{
    m_document.removeListener(this);
}

void SelectionSet::add(DmEntity* entity)
{
    if (canAdd(entity))
    {
        m_ids.insert(entity->getId());
    }
    emit changed();
}

void SelectionSet::remove(DmEntity* entity)
{
    if (entity)
    {
        m_ids.erase(entity->getId());
    }
    emit changed();
}

void SelectionSet::toggle(DmEntity* entity)
{
    if (!entity || entity->isLocked())
    {
        return;
    }
    if (contains(entity))
    {
        m_ids.erase(entity->getId());
    }
    else if (canAdd(entity))
    {
        m_ids.insert(entity->getId());
    }
    emit changed();
}

void SelectionSet::clear()
{
    m_ids.clear();
    emit changed();
}

void SelectionSet::selectWindow(const DmVector& corner1, const DmVector& corner2, bool select, bool cross,
                                const std::list<DM::EntityType>& types)
{
    // 框选耗时埋点，默认关闭，见 ScopedTimer.h。
    YICAD_SCOPED_TIMER(yicad::counters::selectWindow());

    // 几何判断由实体表的矩形查询完成，命中的都是当前实体表里可见、未删除的实体
    EntityTable* table = m_document.getEntityTable();
    const std::vector<DmEntity*> hits =
        cross ? table->entitiesCrossingRect(corner1, corner2, types) : table->entitiesInsideRect(corner1, corner2, types);
    for (auto e : hits)
    {
        if (!select)
        {
            m_ids.erase(e->getId());
        }
        else if (!e->isLocked())
        {
            m_ids.insert(e->getId());
        }
    }
    emit changed();
}

void SelectionSet::selectLayer(const QString& layerName, bool select)
{
    for (auto e : *m_document.getEntityTable())
    {
        if (!e->isVisible() || e->isLocked())
        {
            continue;
        }
        DmLayer* layer = e->getLayer(true);
        if (layer && layer->getName() == layerName)
        {
            if (select)
            {
                m_ids.insert(e->getId());
            }
            else
            {
                m_ids.erase(e->getId());
            }
        }
    }
    emit changed();
}

void SelectionSet::selectAll()
{
    for (auto e : *m_document.getEntityTable())
    {
        if (e->isVisible() && !e->isLocked())
        {
            m_ids.insert(e->getId());
        }
    }
    emit changed();
}

bool SelectionSet::contains(const DmEntity* entity) const
{
    // 集合为空是常态，先判断，画布重建缓存时对每个实体都要问一次
    return entity && !m_ids.empty() && entity->isVisible() && !entity->isErased()
           && m_ids.find(entity->getIdRef()) != m_ids.end();
}

int SelectionSet::count() const
{
    int n = 0;
    for (const DmId& id : m_ids)
    {
        if (findVisible(id))
        {
            ++n;
        }
    }
    return n;
}

bool SelectionSet::isEmpty() const
{
    for (const DmId& id : m_ids)
    {
        if (findVisible(id))
        {
            return false;
        }
    }
    return true;
}

std::vector<DmEntity*> SelectionSet::entities() const
{
    std::vector<DmEntity*> selected;
    if (m_ids.empty())
    {
        return selected;
    }
    // 实体表的迭代跳过已删除的实体
    for (auto e : *m_document.getEntityTable())
    {
        if (e->isVisible() && m_ids.find(e->getId()) != m_ids.end())
        {
            selected.push_back(e);
        }
    }
    return selected;
}

DmVector SelectionSet::nearestRef(const DmVector& coord, double* dist) const
{
    double minDist = DM_MAXDOUBLE;
    DmVector closest(false);
    // 按实体表的顺序，距离相等时取在前的实体的夹点，与原 EntityTable::getNearestSelectedRef 相同
    for (auto e : entities())
    {
        double curDist = DM_MAXDOUBLE;
        const DmVector point = e->getNearestRef(coord, &curDist);
        if (point.valid && curDist < minDist)
        {
            closest = point;
            minDist = curDist;
            if (dist)
            {
                *dist = minDist;
            }
        }
    }
    return closest;
}

bool SelectionSet::isSelected(const DmEntity& entity) const
{
    return contains(&entity);
}

std::vector<DmEntity*> SelectionSet::selectedEntities() const
{
    std::vector<DmEntity*> selected;
    selected.reserve(m_ids.size());
    for (const DmId& id : m_ids)
    {
        if (DmEntity* e = findVisible(id))
        {
            selected.push_back(e);
        }
    }
    return selected;
}

bool SelectionSet::hasMoreThan(std::size_t count) const
{
    if (m_ids.size() <= count)
    {
        return false;
    }
    std::size_t n = 0;
    for (const DmId& id : m_ids)
    {
        if (findVisible(id) && ++n > count)
        {
            return true;
        }
    }
    return false;
}

void SelectionSet::documentModified()
{
    EntityTable* table = m_document.getEntityTable();
    const size_t before = m_ids.size();
    for (auto it = m_ids.begin(); it != m_ids.end();)
    {
        const DmEntity* e = table->find(*it);
        if (!e || e->isErased())
        {
            it = m_ids.erase(it);
        }
        else
        {
            ++it;
        }
    }
    if (m_ids.size() != before)
    {
        emit changed();
    }
}

void SelectionSet::paintContainerChanged(DmEntityContainer* /*container*/)
{
    m_ids.clear();
    emit changed();
}

bool SelectionSet::canAdd(const DmEntity* entity) const
{
    if (!entity || entity->isErased() || entity->isLocked())
    {
        return false;
    }
    // 预览里的实体、克隆出来还没加入实体表的实体 id 都无效（"0"），不能记录，否则它们会彼此算作选中
    const DmId id = entity->getId();
    return id.isValid() && m_document.getEntityTable()->find(id) == entity;
}

DmEntity* SelectionSet::findVisible(const DmId& id) const
{
    DmEntity* e = m_document.getEntityTable()->find(id);
    return (e && !e->isErased() && e->isVisible()) ? e : nullptr;
}
