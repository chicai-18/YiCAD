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

/// @file ExclusiveCommandBus.cpp
/// @brief ExclusiveCommandBus 的实现

#include "ExclusiveCommandBus.h"

#include <QTimer>

#include "ICommandHost.h"
#include "IEditMode.h"
#include "ViewToolControl.h"

ExclusiveCommandBus::DispatchScope::DispatchScope(ExclusiveCommandBus* bus)
    : m_bus(bus)
{
    if (m_bus)
    {
        m_bus->enterScope();
    }
}

ExclusiveCommandBus::DispatchScope::~DispatchScope()
{
    if (m_bus)
    {
        m_bus->leaveScope();
    }
}

ExclusiveCommandBus::ExclusiveCommandBus(ICommandHost& host)
    : m_host(host)
{
}

ExclusiveCommandBus::~ExclusiveCommandBus()
{
    m_inCallback = false;
    finishActive();
    exitEditMode();
    m_retired.clear();
    m_retiredModes.clear();
}

QString ExclusiveCommandBus::activeCommandId() const
{
    return m_active ? m_active->commandId() : QString();
}

bool ExclusiveCommandBus::start(std::unique_ptr<IExclusiveCommand> command)
{
    if (!command || m_inCallback)
    {
        return false;
    }
    // 先按 5.1 节请当前命令让位，被否决时丢弃新命令；编辑模式不受影响，新命令叠在它上面
    if (!endCommand(CommandEndReason::Replaced))
    {
        return false;
    }

    m_active = std::move(command);
    m_finishPending = false;
    ++m_generation;
    emit commandStarting();
    // 编辑模式里启动的命令叠在模式之上：收起模式的界面，命令结束后恢复
    if (m_mode)
    {
        m_mode->suspendMode();
    }

    // 激活期间请求的结束（已有选择集时直接完成的命令）延迟到激活返回后
    enterScope();
    const bool activated = m_active->activate(m_host) && m_active->isActive();
    if (!activated)
    {
        m_finishPending = true;
    }
    leaveScope();
    return activated;
}

void ExclusiveCommandBus::requestFinish(IExclusiveCommand* command)
{
    if (!command || command != m_active.get())
    {
        return;
    }
    m_finishPending = true;
    if (m_scopeDepth > 0)
    {
        return;
    }
    // 分发范围之外（如选项条按钮）：调用方就是命令自己，不能当场销毁它
    const unsigned generation = m_generation;
    QTimer::singleShot(0, this, [this, generation]()
    {
        if (generation == m_generation && m_finishPending && m_scopeDepth == 0)
        {
            finishActive();
        }
    });
}

bool ExclusiveCommandBus::endCommand(CommandEndReason reason)
{
    if (m_inCallback)
    {
        // 回调里弹出的对话框的事件循环中又请求结束：忽略（5.1 节）
        return false;
    }
    if (!approveCommand(reason))
    {
        return false;
    }
    finishActive();
    return true;
}

bool ExclusiveCommandBus::endAll(CommandEndReason reason)
{
    if (m_inCallback)
    {
        return false;
    }
    // 先问命令、再问模式，都同意后才结束：模式否决时命令也继续（命令在回调里已自己结束的除外）
    if (!approveCommand(reason) || !approveMode(reason))
    {
        return false;
    }
    finishActive();
    exitEditMode();
    return true;
}

bool ExclusiveCommandBus::approveCommand(CommandEndReason reason)
{
    if (!m_active)
    {
        return true;
    }
    bool approved = false;
    {
        // 回调里命令自己请求的结束，延迟到回调返回后
        DispatchScope scope(this);
        m_inCallback = true;
        approved = m_active->onEndRequested(reason);
        m_inCallback = false;
    }
    return approved || reason == CommandEndReason::ViewClosing;
}

bool ExclusiveCommandBus::approveMode(CommandEndReason reason)
{
    if (!m_mode)
    {
        return true;
    }
    bool approved = false;
    {
        DispatchScope scope(this);
        m_inCallback = true;
        approved = m_mode->onEndRequested(reason);
        m_inCallback = false;
    }
    return approved || reason == CommandEndReason::ViewClosing;
}

void ExclusiveCommandBus::enterEditMode(std::unique_ptr<IEditMode> mode)
{
    if (!mode)
    {
        return;
    }
    exitEditMode();
    m_mode = std::move(mode);
    m_exitModePending = false;
    m_host.viewToolControl()->activateAtBottom(m_mode.get());
    if (!m_active)
    {
        m_mode->resumeMode();
    }
}

void ExclusiveCommandBus::requestExitEditMode(IEditMode* mode)
{
    if (!mode || mode != m_mode.get())
    {
        return;
    }
    m_exitModePending = true;
    if (m_scopeDepth > 0)
    {
        return;
    }
    // 分发范围之外（如选项条按钮）：调用方就是模式自己，不能当场销毁它
    QTimer::singleShot(0, this, [this]()
    {
        if (m_exitModePending && m_scopeDepth == 0)
        {
            exitEditMode();
        }
    });
}

void ExclusiveCommandBus::exitEditMode()
{
    m_exitModePending = false;
    if (!m_mode)
    {
        return;
    }
    std::unique_ptr<IEditMode> mode = std::move(m_mode);
    m_host.viewToolControl()->deactivate(mode.get());
    mode->onExit();
    if (m_scopeDepth > 0)
    {
        m_retiredModes.push_back(std::move(mode));
    }
}

void ExclusiveCommandBus::finishActive()
{
    m_finishPending = false;
    if (!m_active)
    {
        return;
    }

    // 先把 m_active 移出再 deactivate()：回调里查询总线时它已不是活动命令
    std::unique_ptr<IExclusiveCommand> command = std::move(m_active);
    command->deactivate();
    emit commandFinished();
    // 回到编辑模式
    if (m_mode)
    {
        m_mode->resumeMode();
    }

    if (m_scopeDepth > 0)
    {
        // 它的工具可能还在这次分发的调用栈上，范围结束时再销毁
        m_retired.push_back(std::move(command));
    }
}

void ExclusiveCommandBus::enterScope()
{
    ++m_scopeDepth;
}

void ExclusiveCommandBus::leaveScope()
{
    if (--m_scopeDepth > 0)
    {
        return;
    }
    if (m_finishPending)
    {
        finishActive();
    }
    if (m_exitModePending)
    {
        exitEditMode();
    }
    m_retired.clear();
    m_retiredModes.clear();
}
