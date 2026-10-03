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

/// @file DmChangeSet.cpp
/// @brief DmChangeSet 与 DmChangeTracker 实现

#include "DmChangeSet.h"

#include <algorithm>
#include <utility>

#include "DmBlock.h"
#include "DmEntity.h"

bool DmChangeSet::isEmpty() const
{
    return entities.empty() && destroyedEntities.empty() && blocks.empty() && destroyedBlocks.empty()
        && !layersChanged && !lineTypesChanged && !textStylesChanged && !dimensionStylesChanged
        && !variablesChanged && !fullRebuild;
}

void DmChangeTracker::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!enabled)
    {
        // 没有人取了：丢掉攒下的
        takeChanges();
    }
}

void DmChangeTracker::touchEntity(DmEntity* entity, const DmBlock* ownerBlock)
{
    if (!entity)
    {
        return;
    }
    entity->bumpRevision();
    if (!recording())
    {
        return;
    }
    if (m_touchedEntities.emplace(entity, m_pending.entities.size()).second)
    {
        m_pending.entities.push_back({entity, ownerBlock});
    }
    if (ownerBlock)
    {
        touchBlock(ownerBlock);
    }
}

void DmChangeTracker::destroyEntity(const DmEntity* entity, const DmBlock* ownerBlock)
{
    if (!entity || !recording())
    {
        return;
    }
    // 之前登记的改动作废：释放之后不能再解引用。先置空，取走时再滤掉（逐个删会让清空整表变成平方复杂度）
    auto it = m_touchedEntities.find(entity);
    if (it != m_touchedEntities.end())
    {
        m_pending.entities[it->second].entity = nullptr;
        m_touchedEntities.erase(it);
    }
    m_pending.destroyedEntities.push_back(entity);
    if (ownerBlock)
    {
        touchBlock(ownerBlock);
    }
}

void DmChangeTracker::touchBlock(const DmBlock* block)
{
    if (!block)
    {
        return;
    }
    const_cast<DmBlock*>(block)->bumpRevision();
    if (recording() && m_touchedBlocks.insert(block).second)
    {
        m_pending.blocks.push_back(block);
    }
}

void DmChangeTracker::destroyBlock(const DmBlock* block)
{
    if (!block || !recording())
    {
        return;
    }
    if (m_touchedBlocks.erase(block) > 0)
    {
        auto& list = m_pending.blocks;
        list.erase(std::remove(list.begin(), list.end(), block), list.end());
    }
    // 块里的图元随块一起释放（实体表析构时直接 delete），登记过的也要作废
    for (DmEntityChange& change : m_pending.entities)
    {
        if (change.entity && change.ownerBlock == block)
        {
            m_touchedEntities.erase(change.entity);
            change.entity = nullptr;
        }
    }
    m_pending.destroyedBlocks.push_back(block);
}

void DmChangeTracker::touchTable(DmSymbolTableKind kind)
{
    if (!recording())
    {
        return;
    }
    switch (kind)
    {
    case DmSymbolTableKind::Layer:
        m_pending.layersChanged = true;
        break;
    case DmSymbolTableKind::LineType:
        m_pending.lineTypesChanged = true;
        break;
    case DmSymbolTableKind::TextStyle:
        m_pending.textStylesChanged = true;
        break;
    case DmSymbolTableKind::DimensionStyle:
        m_pending.dimensionStylesChanged = true;
        break;
    }
}

void DmChangeTracker::touchVariables()
{
    if (recording())
    {
        m_pending.variablesChanged = true;
    }
}

void DmChangeTracker::beginBulk()
{
    ++m_bulkDepth;
}

void DmChangeTracker::endBulk()
{
    if (m_bulkDepth == 0)
    {
        return;
    }
    if (--m_bulkDepth == 0)
    {
        requestFullRebuild();
    }
}

void DmChangeTracker::requestFullRebuild()
{
    if (!m_enabled)
    {
        return;
    }
    takeChanges();
    m_pending.fullRebuild = true;
}

bool DmChangeTracker::hasPendingChanges() const
{
    return !m_pending.isEmpty();
}

bool DmChangeTracker::isPending(const DmEntity* entity) const
{
    return m_pending.fullRebuild || m_touchedEntities.count(entity) > 0;
}

DmChangeSet DmChangeTracker::takeChanges()
{
    DmChangeSet changes = std::move(m_pending);
    auto& list = changes.entities;
    list.erase(std::remove_if(list.begin(), list.end(), [](const DmEntityChange& c) { return !c.entity; }),
               list.end());
    m_pending = DmChangeSet();
    m_touchedEntities.clear();
    m_touchedBlocks.clear();
    return changes;
}
