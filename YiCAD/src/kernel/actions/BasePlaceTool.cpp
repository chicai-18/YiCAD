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

/// @file BasePlaceTool.cpp
/// @brief BasePlaceTool 的实现

#include "BasePlaceTool.h"

#include <QKeyEvent>
#include <QMouseEvent>

#include "BaseExclusiveCommand.h"
#include "CommandPreview.h"
#include "GuiCommandEvent.h"
#include "Snapper.h"

BasePlaceTool::BasePlaceTool(BaseExclusiveCommand& command, DmDocument* doc, IDocumentView* view)
    : m_command(command)
    , m_document(doc)
    , m_view(view)
    , m_snapper(std::make_unique<Snapper>(doc, view))
{
    m_snapper->init();
}

BasePlaceTool::~BasePlaceTool() = default;

ISnapService* BasePlaceTool::snapper() const
{
    return m_snapper.get();
}

void BasePlaceTool::finishSession()
{
    m_snapper->finish();
    if (m_resetsOrthogonal)
    {
        m_snapper->resetOrthogonal();
    }
}

void BasePlaceTool::setStatus(int status)
{
    if (m_status == status)
    {
        return;
    }
    m_status = status;
    updateHints();
}

void BasePlaceTool::restart(int status)
{
    setStatus(status);
    m_snapper->init();
}

void BasePlaceTool::stepBack()
{
    if (m_status <= 0)
    {
        m_command.finish();
        return;
    }
    restart(m_status - 1);
}

ViewToolResult BasePlaceTool::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::MiddleButton)
    {
        // 中键平移属于导航层
        return ViewToolResult::NotHandled;
    }
    onMousePress(e);
    return ViewToolResult::Handled;
}

ViewToolResult BasePlaceTool::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::MiddleButton)
    {
        return ViewToolResult::NotHandled;
    }
    onMouseRelease(e);
    return ViewToolResult::Handled;
}

ViewToolResult BasePlaceTool::mouseMoveEvent(QMouseEvent* e)
{
    if (e->buttons() & Qt::MiddleButton)
    {
        // 中键平移进行中：让给导航层，否则平移会被打断
        return ViewToolResult::NotHandled;
    }
    onMouseMove(e);
    return ViewToolResult::Handled;
}

ViewToolResult BasePlaceTool::mouseDoubleClickEvent(QMouseEvent*)
{
    return ViewToolResult::Handled;
}

ViewToolResult BasePlaceTool::keyPressEvent(QKeyEvent* e)
{
    // 与原 ActionInterface::keyPressEvent 一样不接受：Esc/空格随后由主窗口结束全部命令
    e->ignore();
    return ViewToolResult::Handled;
}

ViewToolResult BasePlaceTool::keyReleaseEvent(QKeyEvent* e)
{
    e->ignore();
    return ViewToolResult::Handled;
}

ViewToolResult BasePlaceTool::coordinateEvent(const DmVector& pos)
{
    onCoordinate(pos);
    return ViewToolResult::Handled;
}

ViewToolResult BasePlaceTool::commandEvent(GuiCommandEvent* e)
{
    onCommand(e);
    return e->isAccepted() ? ViewToolResult::Handled : ViewToolResult::NotHandled;
}

void BasePlaceTool::enterEvent()
{
    updateHints();
    m_snapper->resume();
    if (m_preview)
    {
        m_preview->draw();
    }
}

void BasePlaceTool::leaveEvent()
{
    m_snapper->suspend();
    if (m_preview)
    {
        m_preview->clear();
    }
}

void BasePlaceTool::onActivate()
{
    enterEvent();
}

void BasePlaceTool::onDeactivate()
{
    leaveEvent();
}

std::optional<DM::CursorType> BasePlaceTool::getCursor() const
{
    return DM::CadCursor;
}
