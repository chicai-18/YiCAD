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

/// @file DimensionStyleTableCmd.cpp
/// @brief 标注样式表命令实现

#include "DimensionStyleTableCmd.h"
#include "DmDimensionStyleTable.h"
#include "DmDimensionStyle.h"
#include <sstream>

DimensionStyleTableAddCmd::DimensionStyleTableAddCmd(DmDimensionStyleTable *table, DmDimensionStyle *addedDimStyle)
        : m_table(table)
        , m_addedDimStyle(addedDimStyle)
        , m_isOwnByCommand(false)
{

}

void DimensionStyleTableAddCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_addedDimStyle->getId());
    if (e == nullptr)
    {
        m_isOwnByCommand = true;
        m_table->add_direct(m_addedDimStyle);
    }
    else
    {
        m_isOwnByCommand = false;
        e->setErased(false);
    }
    ICmd::execute();
}

void DimensionStyleTableAddCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_addedDimStyle->getId());
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

void DimensionStyleTableAddCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_addedDimStyle->getId());
    e->setErased(false);
    ICmd::redo();
}

void DimensionStyleTableAddCmd::clear()
{
    auto layer = m_table->find(m_addedDimStyle->getId());
    if (m_isOwnByCommand)
    {
        m_table->remove_direct(layer);
    }
}

DimensionStyleTableModifyCmd::DimensionStyleTableModifyCmd(DmDimensionStyleTable *table,
                                                           DmDimensionStyle *modifiedDimStyle)
        :m_table(table)
        ,m_modifiedDimStyle(modifiedDimStyle)
{
    std::ostringstream oss;
    OutputStream str(oss);
    m_modifiedDimStyle->saveStream(str);
    m_originData = oss.str();
}

void DimensionStyleTableModifyCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    std::ostringstream oss;
    OutputStream str(oss);
    m_modifiedDimStyle->saveStream(str);
    m_newData = oss.str();
    m_modifiedDimStyle->update();
    ICmd::execute();
}

void DimensionStyleTableModifyCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    std::istringstream iss(m_originData);
    InputStream str(iss);
    m_modifiedDimStyle->restoreStream(str);
    ICmd::undo();
}

void DimensionStyleTableModifyCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    std::istringstream iss(m_newData);
    InputStream str(iss);
    m_modifiedDimStyle->restoreStream(str);
    ICmd::redo();
}

void DimensionStyleTableModifyCmd::clear()
{

}

DimensionStyleTableRemoveCmd::DimensionStyleTableRemoveCmd(DmDimensionStyleTable *table,
                                                           DmDimensionStyle *removedDimStyle)
        : m_table(table)
        , m_removedDimStyle(removedDimStyle)
{

}

void DimensionStyleTableRemoveCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_removedDimStyle->getId());
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

void DimensionStyleTableRemoveCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_removedDimStyle->getId());
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

void DimensionStyleTableRemoveCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    execute();
    ICmd::redo();
}

void DimensionStyleTableRemoveCmd::clear()
{

}

DimensionStyleTableActivateCmd::DimensionStyleTableActivateCmd(DmDimensionStyleTable *table,
                                                               DmDimensionStyle *activatedDimStyle)
        :m_table(table)
        ,m_activatedDimStyle(activatedDimStyle)
{
    m_originActiveDimStyle = table->getActive();
}

void DimensionStyleTableActivateCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_activatedDimStyle->getId());
    if (e == nullptr)
    {
        return;
    }
    m_table->activate_direct(m_activatedDimStyle);
    ICmd::execute();
}

void DimensionStyleTableActivateCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    m_table->activate_direct(m_originActiveDimStyle);
    ICmd::undo();
}

void DimensionStyleTableActivateCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    m_table->activate_direct(m_activatedDimStyle);
    ICmd::redo();
}

void DimensionStyleTableActivateCmd::clear()
{

}
