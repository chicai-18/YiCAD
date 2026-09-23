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

/// @file ViewToolControl.cpp
/// @brief ViewToolControl 的实现

#include "ViewToolControl.h"

#include <algorithm>

#include "IDocumentView.h"

namespace
{
bool containsTool(std::vector<IViewTool*> const& tools, IViewTool* tool)
{
    return std::find(tools.begin(), tools.end(), tool) != tools.end();
}
}  // namespace

ViewToolControl::ViewToolControl(IDocumentView* docView)
    : m_docView(docView)
{
}

ViewToolControl::~ViewToolControl()
{
    deactivateAll();
    if (m_selectionTool) m_selectionTool->onDeactivate();
    if (m_navigationTool) m_navigationTool->onDeactivate();
}

void ViewToolControl::setNavigationTool(IViewTool* tool)
{
    if (m_navigationTool == tool) return;
    if (m_navigationTool) m_navigationTool->onDeactivate();
    m_navigationTool = tool;
    if (m_navigationTool) m_navigationTool->onActivate();
    refreshCursor();
}

void ViewToolControl::setSelectionTool(IViewTool* tool)
{
    if (m_selectionTool == tool) return;
    if (m_selectionTool) m_selectionTool->onDeactivate();
    m_selectionTool = tool;
    if (m_selectionTool) m_selectionTool->onActivate();
    refreshCursor();
}

void ViewToolControl::activate(IViewTool* tool)
{
    if (!tool || isActive(tool))
    {
        return;
    }
    m_businessTools.push_back(tool);
    tool->onActivate();
    refreshCursor();
}

void ViewToolControl::deactivate(IViewTool* tool)
{
    if (!tool)
    {
        return;
    }
    auto it = std::find(m_businessTools.begin(), m_businessTools.end(), tool);
    if (it == m_businessTools.end())
    {
        return;
    }
    tool->onDeactivate();
    m_businessTools.erase(it);
    refreshCursor();
}

void ViewToolControl::deactivateAll()
{
    for (auto it = m_businessTools.rbegin(); it != m_businessTools.rend(); ++it)
    {
        (*it)->onDeactivate();
    }
    m_businessTools.clear();
    refreshCursor();
}

bool ViewToolControl::isActive(IViewTool* tool) const
{
    if (!tool)
    {
        return false;
    }
    return tool == m_navigationTool || tool == m_selectionTool || containsTool(m_businessTools, tool);
}

template <typename EventFunc>
ViewToolResult ViewToolControl::dispatch(EventFunc&& func)
{
    // 1. 业务工具栈：后进先出，最高优先级
    auto snapshot = m_businessTools;
    for (auto it = snapshot.rbegin(); it != snapshot.rend(); ++it)
    {
        ViewToolResult result = func(*it);
        if (result != ViewToolResult::NotHandled)
        {
            return result;
        }
    }

    // 2. 选择工具
    if (m_selectionTool)
    {
        ViewToolResult result = func(m_selectionTool);
        if (result != ViewToolResult::NotHandled)
        {
            return result;
        }
    }

    // 3. 导航工具：兜底
    if (m_navigationTool)
    {
        ViewToolResult result = func(m_navigationTool);
        if (result != ViewToolResult::NotHandled)
        {
            return result;
        }
    }

    return ViewToolResult::NotHandled;
}

void ViewToolControl::refreshCursor()
{
    if (!m_docView)
    {
        return;
    }

    auto query = [](IViewTool* tool) -> std::optional<DM::CursorType>
    {
        return tool ? tool->getCursor() : std::nullopt;
    };

    // 与 dispatch 同序：业务工具栈（后进先出）-> 选择工具 -> 导航工具。
    // 取首个有偏好者。
    std::optional<DM::CursorType> resolved;
    auto snapshot = m_businessTools;
    for (auto it = snapshot.rbegin(); it != snapshot.rend(); ++it)
    {
        if (auto cursor = query(*it))
        {
            resolved = cursor;
            break;
        }
    }
    if (!resolved)
    {
        resolved = query(m_selectionTool);
    }
    if (!resolved)
    {
        resolved = query(m_navigationTool);
    }

    // 全体无偏好：不触碰当前光标，见头文件说明——旧版 Action 体系可能
    // 正通过 ActionInterface::updateMouseCursor 维护自己的光标。
    if (!resolved)
    {
        return;
    }

    if (resolved == m_lastAppliedCursor)
    {
        return;
    }
    m_lastAppliedCursor = resolved;
    m_docView->setMouseCursor(*resolved);
}

ViewToolResult ViewToolControl::mousePressEvent(QMouseEvent* e)
{
    ViewToolResult result = dispatch([&](IViewTool* t) { return t->mousePressEvent(e); });
    refreshCursor();
    return result;
}

ViewToolResult ViewToolControl::mouseReleaseEvent(QMouseEvent* e)
{
    ViewToolResult result = dispatch([&](IViewTool* t) { return t->mouseReleaseEvent(e); });
    refreshCursor();
    return result;
}

ViewToolResult ViewToolControl::mouseMoveEvent(QMouseEvent* e)
{
    ViewToolResult result = dispatch([&](IViewTool* t) { return t->mouseMoveEvent(e); });
    refreshCursor();
    return result;
}

ViewToolResult ViewToolControl::mouseDoubleClickEvent(QMouseEvent* e)
{
    ViewToolResult result = dispatch([&](IViewTool* t) { return t->mouseDoubleClickEvent(e); });
    refreshCursor();
    return result;
}

ViewToolResult ViewToolControl::keyPressEvent(QKeyEvent* e)
{
    ViewToolResult result = dispatch([&](IViewTool* t) { return t->keyPressEvent(e); });
    refreshCursor();
    return result;
}

ViewToolResult ViewToolControl::keyReleaseEvent(QKeyEvent* e)
{
    ViewToolResult result = dispatch([&](IViewTool* t) { return t->keyReleaseEvent(e); });
    refreshCursor();
    return result;
}

ViewToolResult ViewToolControl::wheelEvent(QWheelEvent* e)
{
    ViewToolResult result = dispatch([&](IViewTool* t) { return t->wheelEvent(e); });
    refreshCursor();
    return result;
}

void ViewToolControl::enterEvent()
{
    if (m_navigationTool) m_navigationTool->enterEvent();
    if (m_selectionTool) m_selectionTool->enterEvent();
    for (auto* tool : m_businessTools) tool->enterEvent();
    refreshCursor();
}

void ViewToolControl::leaveEvent()
{
    if (m_navigationTool) m_navigationTool->leaveEvent();
    if (m_selectionTool) m_selectionTool->leaveEvent();
    for (auto* tool : m_businessTools) tool->leaveEvent();
    refreshCursor();
}
