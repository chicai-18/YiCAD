/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file ZoomPanTool.cpp
/// @brief 平移模式工具的实现

#include "ZoomPanTool.h"

#include <cstdlib>

#include <QKeyEvent>
#include <QMouseEvent>

#include "ViewCommands.h"
#include "IDocumentView.h"

ZoomPanTool::ZoomPanTool(IDocumentView* view)
    : m_view(view)
{
}

ViewToolResult ZoomPanTool::mousePressEvent(QMouseEvent* e)
{
    switch (e->button())
    {
    case Qt::MiddleButton:
        // 中键平移属于导航层（原先 LegacyActionTool 同样让路）
        return ViewToolResult::NotHandled;
    case Qt::LeftButton:
        m_lastX = e->x();
        m_lastY = e->y();
        m_status = SetPanning;
        return ViewToolResult::Handled;
    default:
        return ViewToolResult::Handled;
    }
}

ViewToolResult ZoomPanTool::mouseReleaseEvent(QMouseEvent* e)
{
    switch (e->button())
    {
    case Qt::MiddleButton:
        return ViewToolResult::NotHandled;
    case Qt::RightButton:
        m_view->redraw();
        finish();
        return ViewToolResult::Handled;
    default:
        m_status = SetPanStart;
        return ViewToolResult::Handled;
    }
}

ViewToolResult ZoomPanTool::mouseMoveEvent(QMouseEvent* e)
{
    if (e->buttons() & Qt::MiddleButton)
    {
        // 中键平移进行中：让给导航层，否则平移会被打断
        return ViewToolResult::NotHandled;
    }
    if (m_status == SetPanning
        && (std::abs(e->x() - m_lastX) > MIN_PAN_DISTANCE || std::abs(e->y() - m_lastY) > MIN_PAN_DISTANCE))
    {
        m_view->zoomPan(e->x() - m_lastX, e->y() - m_lastY);
        m_lastX = e->x();
        m_lastY = e->y();
    }
    return ViewToolResult::Handled;
}

ViewToolResult ZoomPanTool::mouseDoubleClickEvent(QMouseEvent*)
{
    return ViewToolResult::Handled;
}

ViewToolResult ZoomPanTool::keyPressEvent(QKeyEvent* e)
{
    e->ignore();
    return ViewToolResult::Handled;
}

ViewToolResult ZoomPanTool::keyReleaseEvent(QKeyEvent* e)
{
    e->ignore();
    return ViewToolResult::Handled;
}

ViewToolResult ZoomPanTool::coordinateEvent(const DmVector&)
{
    return ViewToolResult::Handled;
}

ViewToolResult ZoomPanTool::commandEvent(GuiCommandEvent*)
{
    return ViewToolResult::Handled;
}

std::optional<DM::CursorType> ZoomPanTool::getCursor() const
{
    return m_status == SetPanning ? DM::ClosedHandCursor : DM::OpenHandCursor;
}

ViewToolFactory ViewCommands::pan()
{
    return [](const CommandContext& ctx) -> std::unique_ptr<TransientViewTool>
    { return std::make_unique<ZoomPanTool>(ctx.view); };
}
