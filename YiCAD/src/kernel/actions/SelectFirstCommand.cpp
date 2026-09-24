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

/// @file SelectFirstCommand.cpp
/// @brief SelectFirstCommand 与选择阶段工具的实现

#include "SelectFirstCommand.h"

#include <QKeyEvent>
#include <QMouseEvent>

#include "DmDocument.h"
#include "EntityTable.h"
#include "IViewTool.h"
#include "ViewToolControl.h"

/// @brief 选择阶段工具：接管确认、取消与按键，鼠标选择交给选择层
/// @details 逐项对照原 ActionSelectMultiple（经 LegacyActionTool 转发时，它
///          收到的事件都到此为止）与 ActionSelect 的处理。
class SelectionPhaseTool : public IViewTool
{
public:
    explicit SelectionPhaseTool(SelectFirstCommand& command)
        : m_command(command)
    {
    }

    ViewToolResult mousePressEvent(QMouseEvent* e) override
    {
        // 右键按下原先到 ActionSelectMultiple 为止（它不处理），选择层收不到，
        // 进行中的框选不会被右键按下复位；结束命令的是随后的右键释放。
        return e->button() == Qt::RightButton ? ViewToolResult::Handled : ViewToolResult::NotHandled;
    }

    ViewToolResult mouseReleaseEvent(QMouseEvent* e) override
    {
        if (e->button() == Qt::RightButton)
        {
            // 右键结束整个命令（原 ActionSelectMultiple 连同 ActionSelect 一起结束）
            m_command.finish();
            return ViewToolResult::Handled;
        }
        return ViewToolResult::NotHandled;
    }

    ViewToolResult mouseDoubleClickEvent(QMouseEvent*) override
    {
        // 原 ActionSelectMultiple 不处理双击，事件也不下传：选择阶段双击无反应
        return ViewToolResult::Handled;
    }

    ViewToolResult keyPressEvent(QKeyEvent* e) override
    {
        switch (e->key())
        {
        case Qt::Key_Escape:
            // 原 ActionSelect 不接受 Esc：主窗口随后结束全部命令并清空选择
            e->ignore();
            return ViewToolResult::Cancel;
        case Qt::Key_Enter:
            // 主键盘与小键盘回车都经主窗口合成为 Key_Enter；没有选择集时无反应
            m_command.confirmSelection();
            return ViewToolResult::Handled;
        default:
            // 原 ActionSelectMultiple 不处理其它按键，但也不忽略：空格因此不结束命令
            return ViewToolResult::Handled;
        }
    }

    ViewToolResult keyReleaseEvent(QKeyEvent*) override
    {
        return ViewToolResult::Handled;
    }

private:
    SelectFirstCommand& m_command;
};

SelectFirstCommand::SelectFirstCommand(SelectionEntry entry)
    : m_entry(entry)
{
}

SelectFirstCommand::~SelectFirstCommand() = default;

bool SelectFirstCommand::onActivate()
{
    const bool hasSelection = document()->getEntityTable()->hasSelect();
    if (m_entry == SelectionEntry::Always || !hasSelection)
    {
        m_selecting = true;
        enterSelectionPhase();
        m_selectionTool = std::make_unique<SelectionPhaseTool>(*this);
        viewToolControl()->activate(m_selectionTool.get());
        return true;
    }
    return startWork();
}

void SelectFirstCommand::onDeactivate()
{
    if (m_working)
    {
        m_working = false;
        onStop();
    }
    if (m_selecting)
    {
        m_selecting = false;
        viewToolControl()->deactivate(m_selectionTool.get());
        leaveSelectionPhase();
    }
}

bool SelectFirstCommand::confirmSelection()
{
    if (!m_selecting || !document()->getEntityTable()->hasSelect())
    {
        return false;
    }
    m_selecting = false;
    // 选择阶段工具可能正在调用栈上（回车），只停用、不释放
    viewToolControl()->deactivate(m_selectionTool.get());
    leaveSelectionPhase();
    if (!startWork())
    {
        finish();
    }
    return true;
}

void SelectFirstCommand::suspend()
{
    if (m_selecting)
    {
        viewToolControl()->deactivate(m_selectionTool.get());
    }
    else if (m_working)
    {
        onSuspend();
    }
}

void SelectFirstCommand::resume()
{
    if (m_selecting)
    {
        viewToolControl()->activate(m_selectionTool.get());
    }
    else if (m_working)
    {
        onResume();
    }
}

bool SelectFirstCommand::startWork()
{
    m_working = onSelectionReady();
    return m_working;
}
