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

#include "EditTool.h"
#include "IEditMode.h"
#include "ISnapService.h"
#include "SelectTool.h"
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

ExclusiveCommandBus::ExclusiveCommandBus(DmDocument* doc, IDocumentView* view, ViewToolControl* tools,
                                         SelectTool* selectTool, EditTool* editTool)
    : m_document(doc)
    , m_view(view)
    , m_tools(tools)
    , m_selectTool(selectTool)
    , m_editTool(editTool)
{
    syncEditTool();
}

ExclusiveCommandBus::~ExclusiveCommandBus()
{
    m_inCallback = false;
    finishActive();
    exitEditMode();
    m_retired.clear();
    m_retiredModes.clear();
    if (m_tools && m_editTool)
    {
        m_tools->deactivate(m_editTool);
    }
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
    m_finishPending = false;
    ++m_generation;
    // 夹点编辑工具移出业务栈，激活的夹点随之取消。拆分前只清除它的预览，命令结束后夹点
    // 接着跟随鼠标，下一次单击会按命令改过的选择集落位（doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.6 节）
    syncEditTool();
    // 先挂起选择层（清除它的预览与捕捉标记），与原先 Action 从空闲态启动时一致
    if (m_selectTool)
    {
        m_selectTool->suspend();
    }
    // 编辑模式里启动的命令叠在模式之上：收起模式的界面，命令结束后恢复
    if (m_mode)
    {
        m_mode->suspendMode();
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

bool ExclusiveCommandBus::approveEndAll(CommandEndReason reason)
{
    if (m_inCallback)
    {
        return false;
    }
    if (!approveEnd(reason))
    {
        return false;
    }
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

void ExclusiveCommandBus::endAll()
{
    if (m_inCallback)
    {
        return;
    }
    finishActive();
    exitEditMode();
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
    m_tools->activateAtBottom(m_mode.get());
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
    m_tools->deactivate(mode.get());
    mode->onExit();
    if (m_scopeDepth > 0)
    {
        m_retiredModes.push_back(std::move(mode));
    }
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
    command->deactivate();

    if (m_selectTool)
    {
        // 命令没有退出选择阶段就结束时，由总线清除约束（第二步第 4 项）
        if (m_selectTool->inSelectionPhase())
        {
            m_selectTool->endSelectionPhase();
        }
        // 恢复选择层（刷新提示，重绘预览与捕捉标记），与原先 Action 栈清空时一致
        m_selectTool->resume();
    }
    // 回到编辑模式
    if (m_mode)
    {
        m_mode->resumeMode();
    }
    // 夹点编辑工具放回业务栈顶，在编辑模式的工具之上
    syncEditTool();

    if (m_scopeDepth > 0)
    {
        // 它的工具可能还在这次分发的调用栈上，范围结束时再销毁
        m_retired.push_back(std::move(command));
    }
}

void ExclusiveCommandBus::syncEditTool()
{
    if (!m_tools || !m_editTool)
    {
        return;
    }
    if (m_active)
    {
        m_tools->deactivate(m_editTool);
    }
    else
    {
        m_tools->activate(m_editTool);
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
