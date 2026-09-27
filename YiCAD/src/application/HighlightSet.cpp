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

/// @file HighlightSet.cpp
/// @brief 高亮集的实现

#include "HighlightSet.h"

#include <algorithm>

#include "DmDocument.h"
#include "DmEntity.h"
#include "EntityTable.h"

HighlightSet::HighlightSet(DmDocument& document)
    : m_document(document)
{
}

void HighlightSet::add(DmEntity* entity)
{
    if (!entity || entity->isErased())
    {
        return;
    }
    // 预览里的克隆、刚克隆出来的实体 id 无效（"0"），拾取到的子实体不在实体表里，都不加入
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

void HighlightSet::remove(DmEntity* entity)
{
    if (entity && m_ids.erase(entity->getId()) > 0)
    {
        emit changed();
    }
}

void HighlightSet::clear()
{
    if (!m_ids.empty())
    {
        m_ids.clear();
        emit changed();
    }
}

bool HighlightSet::contains(const DmEntity* entity) const
{
    if (!entity || m_ids.empty() || !entity->isVisible() || entity->isErased())
    {
        return false;
    }
    const DmId id = entity->getId();
    return m_ids.find(id) != m_ids.end() && m_document.getEntityTable()->find(id) == entity;
}

std::vector<DmEntity*> HighlightSet::entities() const
{
    // 画布每次重建缓存都取一次，开销只随集合大小（D3）：先按 id 在实体表里查
    std::vector<DmEntity*> found;
    EntityTable* table = m_document.getEntityTable();
    for (const DmId& id : m_ids)
    {
        DmEntity* e = table->find(id);
        if (e && !e->isErased() && e->isVisible())
        {
            found.push_back(e);
        }
    }
    if (found.size() < 2)
    {
        return found;
    }
    // 多于一个时按实体表的顺序排：遍历实体表只比较指针，不取 id、不查哈希，找齐即止
    std::vector<DmEntity*> ordered;
    ordered.reserve(found.size());
    for (auto e : *table)
    {
        if (std::find(found.begin(), found.end(), e) != found.end())
        {
            ordered.push_back(e);
            if (ordered.size() == found.size())
            {
                break;
            }
        }
    }
    return ordered;
}

std::vector<DmEntity*> HighlightSet::highlightedEntities() const
{
    return entities();
}
