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

/// @file LegacyActionTool.cpp
/// @brief LegacyActionTool 的实现

#include "LegacyActionTool.h"

#include <QMouseEvent>

#include "GuiEventHandler.h"
#include "PanZoomTool.h"

LegacyActionTool::LegacyActionTool(GuiEventHandler* handler, PanZoomTool* panTool)
    : m_handler(handler)
    , m_panTool(panTool)
{
}

bool LegacyActionTool::wantsPress(QMouseEvent* e) const
{
    if (e->button() == Qt::MiddleButton)
    {
        return false;
    }
    if (e->button() == Qt::LeftButton
        && (e->modifiers() & (Qt::ControlModifier | Qt::MetaModifier))
        && !m_handler->hasAction())
    {
        return false;
    }
    return true;
}

ViewToolResult LegacyActionTool::mousePressEvent(QMouseEvent* e)
{
    if (!wantsPress(e))
    {
        return ViewToolResult::NotHandled;
    }
    m_handler->mousePressEvent(e);
    return ViewToolResult::Handled;
}

ViewToolResult LegacyActionTool::mouseReleaseEvent(QMouseEvent* e)
{
    if (m_panTool->isPanning())
    {
        return ViewToolResult::NotHandled;
    }
    m_handler->mouseReleaseEvent(e);
    return ViewToolResult::Handled;
}

ViewToolResult LegacyActionTool::mouseMoveEvent(QMouseEvent* e)
{
    if (m_panTool->isPanning())
    {
        return ViewToolResult::NotHandled;
    }
    m_handler->mouseMoveEvent(e);
    return ViewToolResult::Handled;
}

ViewToolResult LegacyActionTool::mouseDoubleClickEvent(QMouseEvent* e)
{
    m_handler->mouseDoubleClickEvent(e);
    return ViewToolResult::Handled;
}

ViewToolResult LegacyActionTool::keyPressEvent(QKeyEvent* e)
{
    m_handler->keyPressEvent(e);
    return ViewToolResult::Handled;
}

ViewToolResult LegacyActionTool::keyReleaseEvent(QKeyEvent* e)
{
    m_handler->keyReleaseEvent(e);
    return ViewToolResult::Handled;
}
