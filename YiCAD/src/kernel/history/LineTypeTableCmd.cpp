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

/// @file LineTypeTableCmd.cpp
/// @brief 线型表命令实现

#include "LineTypeTableCmd.h"
#include "DmLineTypeTable.h"
#include "DmLineType.h"
#include <sstream>

LineTypeTableAddCmd::LineTypeTableAddCmd(DmLineTypeTable *table, DmLineType *addedLineType)
        : m_table(table)
        , m_addedLineType(addedLineType)
        , m_isOwnByCommand(false)
{

}

void LineTypeTableAddCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_addedLineType->getId());
    if (e == nullptr)
    {
        m_isOwnByCommand = true;
        m_table->add_direct(m_addedLineType);
    }
    else
    {
        m_isOwnByCommand = false;
        e->setErased(false);
    }
    ICmd::execute();
}

void LineTypeTableAddCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_addedLineType->getId());
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

void LineTypeTableAddCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_addedLineType->getId());
    e->setErased(false);
    ICmd::redo();
}

void LineTypeTableAddCmd::clear()
{
    auto layer = m_table->find(m_addedLineType->getId());
    if (m_isOwnByCommand)
    {
        m_table->remove_direct(layer);
    }
}

LineTypeTableActivateCmd::LineTypeTableActivateCmd(DmLineTypeTable *table, DmLineType *activatedLineType)
        :m_table(table)
        ,m_activatedLineType(activatedLineType)
{
    m_originActiveLineType = table->getActive();
}

void LineTypeTableActivateCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_activatedLineType->getId());
    if (e == nullptr)
    {
        return;
    }
    m_table->activate_direct(m_activatedLineType);
    ICmd::execute();
}

void LineTypeTableActivateCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    m_table->activate_direct(m_originActiveLineType);
    ICmd::undo();
}

void LineTypeTableActivateCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    m_table->activate_direct(m_activatedLineType);
    ICmd::redo();
}

void LineTypeTableActivateCmd::clear()
{

}

LineTypeTableRemoveCmd::LineTypeTableRemoveCmd(DmLineTypeTable *table, DmLineType *removedLineType)
        : m_table(table)
        , m_removedLineType(removedLineType)
{

}

void LineTypeTableRemoveCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_removedLineType->getId());
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

void LineTypeTableRemoveCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_removedLineType->getId());
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

void LineTypeTableRemoveCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    execute();
    ICmd::redo();
}

void LineTypeTableRemoveCmd::clear()
{
}

LineTypeTableModifyCmd::LineTypeTableModifyCmd(DmLineTypeTable *table, DmLineType *modifiedLineType)
        :m_table(table)
        ,m_modifiedLineType(modifiedLineType)
{
    std::ostringstream oss;
    OutputStream str(oss);
    m_modifiedLineType->saveStream(str);
    m_originData = oss.str();
}

void LineTypeTableModifyCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    std::ostringstream oss;
    OutputStream str(oss);
    m_modifiedLineType->saveStream(str);
    m_newData = oss.str();
    ICmd::execute();
}

void LineTypeTableModifyCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    std::istringstream iss(m_originData);
    InputStream str(iss);
    m_modifiedLineType->restoreStream(str);
    ICmd::undo();
}

void LineTypeTableModifyCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    std::istringstream iss(m_newData);
    InputStream str(iss);
    m_modifiedLineType->restoreStream(str);
    ICmd::redo();
}

void LineTypeTableModifyCmd::clear()
{

}
