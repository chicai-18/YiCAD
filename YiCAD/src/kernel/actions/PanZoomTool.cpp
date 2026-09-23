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

/// @file PanZoomTool.cpp
/// @brief PanZoomTool 的实现

#include "PanZoomTool.h"

#include <QMouseEvent>

#include "IDocumentView.h"

/// @brief 触发平移所需的最小像素位移，与原 ActionDefault::Panning 的
/// PAN_SQUARED_THRESHOLD（8px 的平方）保持一致，避免手抖误触发。
constexpr double kPanSquaredThreshold = 64.0;

PanZoomTool::PanZoomTool(IDocumentView* docView)
    : m_docView(docView)
{
}

ViewToolResult PanZoomTool::mousePressEvent(QMouseEvent* e)
{
    // 是否算作一次平移手势由调用方（GuiDocumentView）判断后才会转发到这里，
    // 本类只管接受并记录起点。
    m_panning = true;
    m_panButton = e->button();
    m_lastGuiPos = DmVector(e->x(), e->y());
    return ViewToolResult::Handled;
}

ViewToolResult PanZoomTool::mouseMoveEvent(QMouseEvent* e)
{
    if (!m_panning)
    {
        return ViewToolResult::NotHandled;
    }

    DmVector const target(e->x(), e->y());
    DmVector const delta = target - m_lastGuiPos;
    if (delta.squared() >= kPanSquaredThreshold)
    {
        m_docView->zoomPan(static_cast<int>(delta.x), static_cast<int>(delta.y));
        m_lastGuiPos = target;
    }
    return ViewToolResult::Handled;
}

ViewToolResult PanZoomTool::mouseReleaseEvent(QMouseEvent* e)
{
    if (!m_panning || e->button() != m_panButton)
    {
        return ViewToolResult::NotHandled;
    }

    m_panning = false;
    m_panButton = Qt::NoButton;
    m_docView->redraw();
    return ViewToolResult::Handled;
}

std::optional<DM::CursorType> PanZoomTool::getCursor() const
{
    if (m_panning)
    {
        return DM::ClosedHandCursor;
    }
    return std::nullopt;
}
