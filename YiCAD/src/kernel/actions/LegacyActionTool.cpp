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

#include "ActionInterface.h"
#include "GuiEventHandler.h"
#include "PanZoomTool.h"

LegacyActionTool::LegacyActionTool(GuiEventHandler* handler, PanZoomTool* panTool)
    : m_handler(handler)
    , m_panTool(panTool)
{
}

LegacyActionTool::Route LegacyActionTool::routeOf(const QEvent* e) const
{
    if (!m_handler->hasAction())
    {
        return Route::Skip;
    }
    // 与 GuiEventHandler 的分发对象一致：有业务 Action 时即栈顶
    ActionInterface* action = m_handler->getCurrentAction();
    return action->passesToSelection(e) ? Route::ForwardAndPass : Route::Forward;
}

ViewToolResult LegacyActionTool::resultOf(Route route)
{
    return route == Route::ForwardAndPass ? ViewToolResult::NotHandled : ViewToolResult::Handled;
}

ViewToolResult LegacyActionTool::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::MiddleButton)
    {
        return ViewToolResult::NotHandled;
    }
    const Route route = routeOf(e);
    if (route == Route::Skip)
    {
        return ViewToolResult::NotHandled;
    }
    m_handler->mousePressEvent(e);
    return resultOf(route);
}

ViewToolResult LegacyActionTool::mouseReleaseEvent(QMouseEvent* e)
{
    if (m_panTool->isPanning())
    {
        return ViewToolResult::NotHandled;
    }
    // 转发前决定去向：GuiEventHandler 转发释放后会 cleanUp()，
    // 已结束的栈顶 Action 在那时被删除。
    const Route route = routeOf(e);
    if (route == Route::Skip)
    {
        return ViewToolResult::NotHandled;
    }
    m_handler->mouseReleaseEvent(e);
    return resultOf(route);
}

ViewToolResult LegacyActionTool::mouseMoveEvent(QMouseEvent* e)
{
    if (m_panTool->isPanning())
    {
        return ViewToolResult::NotHandled;
    }
    const Route route = routeOf(e);
    if (route == Route::Skip)
    {
        return ViewToolResult::NotHandled;
    }
    m_handler->mouseMoveEvent(e);
    return resultOf(route);
}

ViewToolResult LegacyActionTool::mouseDoubleClickEvent(QMouseEvent* e)
{
    const Route route = routeOf(e);
    if (route == Route::Skip)
    {
        return ViewToolResult::NotHandled;
    }
    m_handler->mouseDoubleClickEvent(e);
    return resultOf(route);
}

ViewToolResult LegacyActionTool::keyPressEvent(QKeyEvent* e)
{
    const Route route = routeOf(e);
    if (route == Route::Skip)
    {
        return ViewToolResult::NotHandled;
    }
    m_handler->keyPressEvent(e);
    return resultOf(route);
}

ViewToolResult LegacyActionTool::keyReleaseEvent(QKeyEvent* e)
{
    const Route route = routeOf(e);
    if (route == Route::Skip)
    {
        return ViewToolResult::NotHandled;
    }
    m_handler->keyReleaseEvent(e);
    return resultOf(route);
}

void LegacyActionTool::enterEvent()
{
    m_handler->mouseEnterEvent();
}

void LegacyActionTool::leaveEvent()
{
    m_handler->mouseLeaveEvent();
}
