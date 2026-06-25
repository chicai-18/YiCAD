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

/// @file BlockEditCmd.cpp
/// @brief 块编辑括号命令实现

#include "BlockEditCmd.h"

#include <QSet>

#include "DmDocument.h"
#include "DmBlock.h"
#include "DmBlockTable.h"
#include "DmBlockReference.h"
#include "EntityTable.h"
#include "GuiDocumentView.h"

// ============================================================================
// 辅助函数：查找编辑指定块后会受影响的所有块名称
// ============================================================================

static QSet<QString> getAllAffectedBlockNames(DmDocument* doc, const QString& editedBlockName)
{
    QSet<QString> affected;
    affected.insert(editedBlockName);

    DmBlockTable* blockTable = doc->getBlockTable();
    if (!blockTable)
        return affected;

    bool changed = true;
    while (changed)
    {
        changed = false;
        QSet<QString> snapshot = affected;
        for (auto blk : *blockTable)
        {
            if (!blk || blk->isErased())
                continue;
            if (affected.contains(blk->getName()))
                continue;
            for (const QString& name : snapshot)
            {
                QStringList chain = blk->findNestedInsert(name);
                if (!chain.empty())
                {
                    affected.insert(blk->getName());
                    changed = true;
                    break;
                }
            }
        }
    }
    return affected;
}

// ============================================================================
// BlockEditEnterCmd：进入块编辑命令
// ============================================================================

BlockEditEnterCmd::BlockEditEnterCmd(DmDocument* doc, const QString& blockName,
                                     GuiDocumentView* docView)
    : m_pDocument(doc)
    , m_blockName(blockName)
    , m_pDocView(docView)
{
}

void BlockEditEnterCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }

    DmBlockTable* blockTable = m_pDocument->getBlockTable();
    if (!blockTable)
    {
        return;
    }

    DmBlock* block = blockTable->find(m_blockName);
    if (!block)
    {
        return;
    }

    m_pDocument->setEditBlock(block);
    m_pDocument->regenerate();

    m_isExecuted = true;
}

void BlockEditEnterCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }

    m_pDocument->setEditBlock(nullptr);
    updateBlockRefs();
    m_pDocument->regenerate();

    m_isExecuted = false;
}

void BlockEditEnterCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }

    DmBlockTable* blockTable = m_pDocument->getBlockTable();
    if (!blockTable)
    {
        return;
    }

    DmBlock* block = blockTable->find(m_blockName);
    if (!block)
    {
        return;
    }

    m_pDocument->setEditBlock(block);
    m_pDocument->regenerate();

    m_isExecuted = true;
}

void BlockEditEnterCmd::clear()
{
}

void BlockEditEnterCmd::updateBlockRefs()
{
    QSet<QString> affected = getAllAffectedBlockNames(m_pDocument, m_blockName);
    auto entTable = m_pDocument->getDocumentEntityTable();
    for (auto e : *entTable)
    {
        if (e && !e->isErased()
            && e->getEntityType() == DM::EntityBlockReference)
        {
            DmBlockReference* ref = static_cast<DmBlockReference*>(e);
            if (affected.contains(ref->getName()))
            {
                ref->update();
                entTable->notifyEntityModified(ref);
            }
        }
    }
}

// ============================================================================
// BlockEditExitCmd：退出块编辑命令
// ============================================================================

BlockEditExitCmd::BlockEditExitCmd(DmDocument* doc, const QString& blockName,
                                   GuiDocumentView* docView, bool save)
    : m_pDocument(doc)
    , m_blockName(blockName)
    , m_pDocView(docView)
    , m_save(save)
{
}

void BlockEditExitCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }

    m_pDocument->setEditBlock(nullptr);

    if (m_save)
    {
        // 此时已经调用 setEditBlock(nullptr)，因此 getEntityTable()
        // 现在返回的是文档实体表
        updateBlockRefs();
    }

    m_pDocument->regenerate();

    m_isExecuted = true;
}

void BlockEditExitCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }

    DmBlockTable* blockTable = m_pDocument->getBlockTable();
    if (!blockTable)
    {
        return;
    }

    DmBlock* block = blockTable->find(m_blockName);
    if (!block)
    {
        return;
    }

    m_pDocument->setEditBlock(block);
    m_pDocument->regenerate();

    m_isExecuted = false;
}

void BlockEditExitCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }

    m_pDocument->setEditBlock(nullptr);

    if (m_save)
    {
        updateBlockRefs();
    }

    m_pDocument->regenerate();

    m_isExecuted = true;
}

void BlockEditExitCmd::clear()
{
}

void BlockEditExitCmd::updateBlockRefs()
{
    QSet<QString> affected = getAllAffectedBlockNames(m_pDocument, m_blockName);
    auto entTable = m_pDocument->getDocumentEntityTable();
    for (auto e : *entTable)
    {
        if (e && !e->isErased()
            && e->getEntityType() == DM::EntityBlockReference)
        {
            DmBlockReference* ref = static_cast<DmBlockReference*>(e);
            if (affected.contains(ref->getName()))
            {
                ref->update();
                entTable->notifyEntityModified(ref);
            }
        }
    }
}
