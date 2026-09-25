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

/// @file BaseExclusiveCommand.cpp
/// @brief BaseExclusiveCommand 的实现

#include "BaseExclusiveCommand.h"

#include <QWidget>

#include "CommandRegistry.h"
#include "ExclusiveCommandBus.h"
#include "IDocumentView.h"
#include "SelectTool.h"

bool BaseExclusiveCommand::activate(ExclusiveCommandBus& bus)
{
    m_bus = &bus;
    // 先置为活动：onActivate() 里即可请求结束（已有选择集时直接完成的命令）
    m_active = true;
    if (!onActivate())
    {
        m_active = false;
        m_bus = nullptr;
    }
    return m_active;
}

void BaseExclusiveCommand::deactivate()
{
    if (!m_active)
    {
        return;
    }
    onDeactivate();
    m_active = false;
    m_bus = nullptr;
}

void BaseExclusiveCommand::finish()
{
    if (m_active && m_bus)
    {
        m_bus->requestFinish(this);
    }
}

void BaseExclusiveCommand::replaceWith(const QString& commandId)
{
    replaceWith(commandId, nullptr);
}

void BaseExclusiveCommand::replaceWith(const QString& commandId, DmEntity* entity, const DmVector& point)
{
    if (!m_active || !m_bus)
    {
        return;
    }
    if (std::unique_ptr<IExclusiveCommand> next = CommandRegistry::instance().createCommand(
            commandId, CommandContext{document(), view(), nullptr, entity, point}))
    {
        m_bus->start(std::move(next));
    }
}

DmDocument* BaseExclusiveCommand::document() const
{
    return m_bus ? m_bus->document() : nullptr;
}

IDocumentView* BaseExclusiveCommand::view() const
{
    return m_bus ? m_bus->view() : nullptr;
}

ViewToolControl* BaseExclusiveCommand::viewToolControl() const
{
    return m_bus ? m_bus->viewToolControl() : nullptr;
}

QWidget* BaseExclusiveCommand::dialogParentOf(IDocumentView* view)
{
    QWidget* canvas = view ? qobject_cast<QWidget*>(view->asQObject()) : nullptr;
    return canvas ? canvas->window() : nullptr;
}

void BaseExclusiveCommand::enterSelectionPhase(const EntityTypeList& entityTypes)
{
    if (SelectTool* selectTool = m_bus ? m_bus->selectTool() : nullptr)
    {
        selectTool->beginSelectionPhase(SelectTool::SelectionPhase{entityTypes});
    }
}

void BaseExclusiveCommand::leaveSelectionPhase()
{
    if (SelectTool* selectTool = m_bus ? m_bus->selectTool() : nullptr)
    {
        selectTool->endSelectionPhase();
    }
}

bool BaseExclusiveCommand::inSelectionPhase() const
{
    SelectTool* selectTool = m_bus ? m_bus->selectTool() : nullptr;
    return selectTool && selectTool->inSelectionPhase();
}
