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

#include "ExclusiveCommandBus.h"
#include "ICommandHost.h"

bool BaseExclusiveCommand::activate(ICommandHost& host)
{
    m_host = &host;
    // 先置为活动：onActivate() 里即可请求结束（已有选择集时直接完成的命令）
    m_active = true;
    if (!onActivate())
    {
        m_active = false;
        m_host = nullptr;
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
    m_host = nullptr;
}

void BaseExclusiveCommand::finish()
{
    if (ExclusiveCommandBus* commandBus = m_active ? bus() : nullptr)
    {
        commandBus->requestFinish(this);
    }
}

ExclusiveCommandBus* BaseExclusiveCommand::bus() const
{
    return m_host ? m_host->commandBus() : nullptr;
}

DmDocument* BaseExclusiveCommand::document() const
{
    return m_host ? m_host->document() : nullptr;
}

SelectionSet* BaseExclusiveCommand::selection() const
{
    return m_host ? m_host->selection() : nullptr;
}

IDocumentView* BaseExclusiveCommand::view() const
{
    return m_host ? m_host->view() : nullptr;
}

ViewToolControl* BaseExclusiveCommand::viewToolControl() const
{
    return m_host ? m_host->viewToolControl() : nullptr;
}
