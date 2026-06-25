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

/// @file LayerTableCmd.cpp
/// @brief 图层表命令实现

#include "LayerTableCmd.h"
#include "DmLayerTable.h"
#include "DmLayer.h"
#include <sstream>

LayerTableAddCmd::LayerTableAddCmd(DmLayerTable *table, DmLayer *addedLayer)
: m_table(table)
, m_addedLayer(addedLayer)
, m_isOwnByCommand(false)
{

}

void LayerTableAddCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_addedLayer->getId());
    if (e == nullptr)
    {
        m_isOwnByCommand = true;
        m_table->add_direct(m_addedLayer);
    }
    else
    {
        m_isOwnByCommand = false;
        e->setErased(false);
    }
    ICmd::execute();
}

void LayerTableAddCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_addedLayer->getId());
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

void LayerTableAddCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_addedLayer->getId());
    e->setErased(false);
    ICmd::redo();
}

void LayerTableAddCmd::clear()
{
    auto layer = m_table->find(m_addedLayer->getId());
    if (m_isOwnByCommand)
    {
        m_table->remove_direct(layer);
    }
}

LayerTableActivateCmd::LayerTableActivateCmd(DmLayerTable *table, DmLayer *activatedLayer)
:m_table(table)
,m_activatedLayer(activatedLayer)
{
    m_originActiveLayer = table->getActive();
}

void LayerTableActivateCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_activatedLayer->getId());
    if (e == nullptr)
    {
        return;
    }
    m_table->activate_direct(m_activatedLayer);
    ICmd::execute();
}

void LayerTableActivateCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    m_table->activate_direct(m_originActiveLayer);
    ICmd::undo();
}

void LayerTableActivateCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    m_table->activate_direct(m_activatedLayer);
    ICmd::redo();
}

void LayerTableActivateCmd::clear()
{

}

LayerTableRemoveCmd::LayerTableRemoveCmd(DmLayerTable *table, DmLayer *removedLayer)
: m_table(table)
, m_removedLayer(removedLayer)
{

}

void LayerTableRemoveCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_removedLayer->getId());
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

void LayerTableRemoveCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_removedLayer->getId());
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

void LayerTableRemoveCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    execute();
    ICmd::redo();
}

void LayerTableRemoveCmd::clear()
{
}

LayerTableModifyCmd::LayerTableModifyCmd(DmLayerTable *table, DmLayer *modifiedLayer)
:m_table(table)
,m_modifiedLayer(modifiedLayer)
{
    std::ostringstream oss;
    OutputStream str(oss);
    m_modifiedLayer->saveStream(str);
    m_originData = oss.str();
}

void LayerTableModifyCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    std::ostringstream oss;
    OutputStream str(oss);
    m_modifiedLayer->saveStream(str);
    m_newData = oss.str();
    ICmd::execute();
}

void LayerTableModifyCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    std::istringstream iss(m_originData);
    InputStream str(iss);
    m_modifiedLayer->restoreStream(str);
    ICmd::undo();
}

void LayerTableModifyCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    std::istringstream iss(m_newData);
    InputStream str(iss);
    m_modifiedLayer->restoreStream(str);
    ICmd::redo();
}

void LayerTableModifyCmd::clear()
{

}
