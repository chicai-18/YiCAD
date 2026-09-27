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
}

void SelectionSet::add(DmEntity* entity)
{
    if (entity)
    {
        // 锁定图层上的实体由 setSelected 拒绝
        entity->setSelected(true);
    }
    notifyChanged();
}

void SelectionSet::remove(DmEntity* entity)
{
    if (entity)
    {
        entity->setSelected(false);
    }
    notifyChanged();
}

void SelectionSet::toggle(DmEntity* entity)
{
    if (entity && !(entity->getLayer() && entity->getLayer()->isLocked()))
    {
        entity->toggleSelected();
        notifyChanged();
    }
}

void SelectionSet::clear()
{
    for (auto e : *m_document.getEntityTable())
    {
        if (e->isVisible())
        {
            e->setSelected(false);
        }
    }
    notifyChanged();
}

void SelectionSet::selectWindow(const DmVector& corner1, const DmVector& corner2, bool select, bool cross,
                                const std::list<DM::EntityType>& types)
{
    // 框选耗时埋点，默认关闭，见 ScopedTimer.h。
    YICAD_SCOPED_TIMER(yicad::counters::selectWindow());

    // 几何判断由实体表的矩形查询完成，这里只置位
    EntityTable* table = m_document.getEntityTable();
    const std::vector<DmEntity*> hits =
        cross ? table->entitiesCrossingRect(corner1, corner2, types) : table->entitiesInsideRect(corner1, corner2, types);
    for (auto e : hits)
    {
        e->setSelected(select);
    }
    notifyChanged();
}

void SelectionSet::selectLayer(const QString& layerName, bool select)
{
    for (auto e : *m_document.getEntityTable())
    {
        if (e && e->isVisible() && e->isSelected() != select && !(e->getLayer() && e->getLayer()->isLocked()))
        {
            DmLayer* layer = e->getLayer(true);
            if (layer && layer->getName() == layerName)
            {
                e->setSelected(select);
            }
        }
    }
    notifyChanged();
}

void SelectionSet::selectAll()
{
    for (auto e : *m_document.getEntityTable())
    {
        if (e->isVisible())
        {
            e->setSelected(true);
        }
    }
    notifyChanged();
}

bool SelectionSet::contains(const DmEntity* entity) const
{
    return entity && entity->isSelected();
}

int SelectionSet::count() const
{
    return m_document.getEntityTable()->countSelect();
}

bool SelectionSet::isEmpty() const
{
    return !m_document.getEntityTable()->hasSelect();
}

std::vector<DmEntity*> SelectionSet::entities() const
{
    std::vector<DmEntity*> selected;
    for (auto e : *m_document.getEntityTable())
    {
        if (e->isSelected())
        {
            selected.push_back(e);
        }
    }
    return selected;
}

DmVector SelectionSet::nearestRef(const DmVector& coord, double* dist) const
{
    return m_document.getEntityTable()->getNearestSelectedRef(coord, dist);
}

bool SelectionSet::isSelected(const DmEntity& entity) const
{
    return contains(&entity);
}

void SelectionSet::notifyChanged()
{
    m_document.notifyDocumentModified();
    m_document.requestRedraw();
}
