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

/// @file HiddenSet.cpp
/// @brief 临时隐藏集的实现

#include "HiddenSet.h"

#include "DmDocument.h"
#include "DmEntity.h"
#include "EntityTable.h"

HiddenSet::HiddenSet(DmDocument& document)
    : m_document(document)
{
}

void HiddenSet::add(DmEntity* entity)
{
    if (!entity || entity->isErased())
    {
        return;
    }
    const DmId id = entity->getId();
    if (!id.isValid() || m_document.getEntityTable()->find(id) != entity)
    {
        return;
    }
    if (m_ids.insert(id).second)
    {
        emit changed();
    }
}

void HiddenSet::remove(DmEntity* entity)
{
    if (entity && m_ids.erase(entity->getId()) > 0)
    {
        emit changed();
    }
}

void HiddenSet::clear()
{
    if (!m_ids.empty())
    {
        m_ids.clear();
        emit changed();
    }
}

std::vector<DmEntity*> HiddenSet::hiddenEntities() const
{
    std::vector<DmEntity*> found;
    EntityTable* table = m_document.getEntityTable();
    for (const DmId& id : m_ids)
    {
        DmEntity* e = table->find(id);
        if (e && !e->isErased())
        {
            found.push_back(e);
        }
    }
    return found;
}

bool HiddenSet::isHidden(const DmEntity& entity) const
{
    if (m_ids.empty() || entity.isErased())
    {
        return false;
    }
    const DmId id = entity.getId();
    return m_ids.find(id) != m_ids.end() && m_document.getEntityTable()->find(id) == &entity;
}
