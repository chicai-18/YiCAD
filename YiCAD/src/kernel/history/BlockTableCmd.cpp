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

/// @file BlockTableCmd.cpp
/// @brief 块表操作命令实现

#include "BlockTableCmd.h"
#include "DmBlockTable.h"
#include "DmBlock.h"
#include <sstream>

BlockTableAddCmd::BlockTableAddCmd(DmBlockTable *table, DmBlock *addedBlock)
: m_table(table)
, m_addedBlock(addedBlock)
, m_isOwnByCommand(false)
{

}

void BlockTableAddCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_addedBlock->getId());
    if (e == nullptr)
    {
        m_isOwnByCommand = true;
        m_table->add_direct(m_addedBlock);
    }
    else
    {
        m_isOwnByCommand = false;
        e->setErased(false);
    }
    ICmd::execute();
}

void BlockTableAddCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_addedBlock->getId());
    if (e == nullptr)
    {
        return;
    }
    else
    {
        e->setErased(true);
    }
    ICmd::undo();
}

void BlockTableAddCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_addedBlock->getId());
    if (e == nullptr)
    {
        m_table->add_direct(m_addedBlock);
    }
    else
    {
        e->setErased(false);
    }
    ICmd::redo();
}

void BlockTableAddCmd::clear()
{
    auto block = m_table->find(m_addedBlock->getId());
    if (m_isOwnByCommand)
    {
        m_table->remove_direct(block);
    }
}

BlockTableRemoveCmd::BlockTableRemoveCmd(DmBlockTable *table, DmBlock *removedBlock)
: m_table(table)
, m_removedBlock(removedBlock)
{

}

void BlockTableRemoveCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_removedBlock->getId());
    if (e == nullptr)
    {
        return;
    }
    else
    {
        e->setErased(true);
    }
    ICmd::execute();
}

void BlockTableRemoveCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_removedBlock->getId());
    if (e == nullptr)
    {
        return;
    }
    else
    {
        e->setErased(false);
    }
    ICmd::undo();
}

void BlockTableRemoveCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    execute();
    ICmd::redo();
}

void BlockTableRemoveCmd::clear()
{
}

BlockTableModifyCmd::BlockTableModifyCmd(DmBlockTable *table, DmBlock *modifiedBlock)
:m_table(table)
,m_modifiedBlock(modifiedBlock)
{
    std::ostringstream oss;
    OutputStream str(oss);
    m_modifiedBlock->saveStream(str);
    m_originData = oss.str();
}

void BlockTableModifyCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    std::ostringstream oss;
    OutputStream str(oss);
    m_modifiedBlock->saveStream(str);
    m_newData = oss.str();
    ICmd::execute();
}

void BlockTableModifyCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    std::istringstream iss(m_originData);
    InputStream str(iss);
    std::vector<PAIR> revs;
    m_modifiedBlock->restoreStream(str, revs);
    m_modifiedBlock->getEntityTable().updateContainer();
    ICmd::undo();
}

void BlockTableModifyCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    std::istringstream iss(m_newData);
    InputStream str(iss);
    std::vector<PAIR> revs;
    m_modifiedBlock->restoreStream(str, revs);
    m_modifiedBlock->getEntityTable().updateContainer();
    ICmd::redo();
}

void BlockTableModifyCmd::clear()
{

}
