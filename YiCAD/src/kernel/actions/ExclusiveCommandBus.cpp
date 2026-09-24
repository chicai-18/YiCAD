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

#include "ISnapService.h"
#include "SelectTool.h"

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

ExclusiveCommandBus::ExclusiveCommandBus(DmDocument* doc, IDocumentView* view, ViewToolControl* tools,
                                         SelectTool* selectTool)
    : m_document(doc)
    , m_view(view)
    , m_tools(tools)
    , m_selectTool(selectTool)
{
}

ExclusiveCommandBus::~ExclusiveCommandBus()
{
    m_inCallback = false;
    finishActive();
    m_retired.clear();
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
    if (m_active)
    {
        if (!approveEnd(CommandEndReason::Replaced))
        {
            return false;
        }
        end();
    }

    m_active = std::move(command);
    m_suspended = false;
    m_finishPending = false;
    ++m_generation;
    // 与旧 Action 从空闲态启动时一致：先挂起选择层（清除它的预览与捕捉标记）
    if (m_selectTool)
    {
        m_selectTool->suspend();
    }

    // 激活期间请求的结束（已有选择集时直接完成的命令）延迟到激活返回后
    enterScope();
    const bool activated = m_active->activate(*this) && m_active->isActive();
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

bool ExclusiveCommandBus::approveEnd(CommandEndReason reason)
{
    if (m_inCallback)
    {
        // 回调里弹出的对话框的事件循环中又请求结束：忽略（5.1 节）
        return false;
    }
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

void ExclusiveCommandBus::end()
{
    if (m_inCallback)
    {
        return;
    }
    finishActive();
}

void ExclusiveCommandBus::suspend()
{
    if (!m_active || m_suspended)
    {
        return;
    }
    m_suspended = true;
    m_active->suspend();
}

void ExclusiveCommandBus::resume()
{
    if (!m_active || !m_suspended)
    {
        return;
    }
    m_suspended = false;
    m_active->resume();
}

void ExclusiveCommandBus::setSnapMode(const SnapMode& snapMode)
{
    if (ISnapService* snapper = m_active ? m_active->snapService() : nullptr)
    {
        snapper->setSnapMode(snapMode);
    }
}

void ExclusiveCommandBus::setSnapRestriction(DM::SnapRestriction restriction)
{
    if (ISnapService* snapper = m_active ? m_active->snapService() : nullptr)
    {
        snapper->setSnapRestriction(restriction);
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
    m_suspended = false;
    command->deactivate();

    if (m_selectTool)
    {
        // 命令没有退出选择阶段就结束时，由总线清除约束（第二步第 4 项）
        if (m_selectTool->inSelectionPhase())
        {
            m_selectTool->endSelectionPhase();
        }
        // 与旧 Action 栈清空时一致：恢复选择层（刷新提示，重绘预览与捕捉标记）
        m_selectTool->resume();
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
    m_retired.clear();
}
