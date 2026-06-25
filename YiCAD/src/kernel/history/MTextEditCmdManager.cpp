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

/// @file MTextEditCmdManager.cpp
/// @brief 多行文字编辑器命令管理器实现

#include "MTextEditCmdManager.h"

/// @brief 构造命令管理器
MTextEditCmdManager::MTextEditCmdManager()
    : m_currentCmd(nullptr)
{
}

/// @brief 析构，清理所有Undo/Redo命令
MTextEditCmdManager::~MTextEditCmdManager()
{
    clearRedo();
    clearUndo();
}

/// @brief 执行一次撤销操作
void MTextEditCmdManager::undo()
{
    if (m_undoCmds.size() == 0)
    {
        return;
    }
    m_undoCmds.back()->undo();
    m_redoCmds.emplace_back(m_undoCmds.back());
    m_undoCmds.pop_back();
    emit cmdChanged();
}

/// @brief 执行一次重做操作
void MTextEditCmdManager::redo()
{
    if (m_redoCmds.size() == 0)
    {
        return;
    }
    m_redoCmds.back()->redo();
    m_undoCmds.emplace_back(m_redoCmds.back());
    m_redoCmds.pop_back();
    emit cmdChanged();
}

/// @brief 获取当前可撤销/可重做状态
void MTextEditCmdManager::getCmdData(bool &undoable, bool &redoable)
{
    redoable = (m_redoCmds.size() != 0);
    undoable = (m_undoCmds.size() != 0);
}

/// @brief 向当前宏命令中添加子命令（不执行）
void MTextEditCmdManager::addToCurrentCmd(ICmd *cmd)
{
    m_currentCmd->addCmd(cmd);
}

/// @brief 添加并执行命令
void MTextEditCmdManager::addAndExecuteCmd(ICmd* cmd)
{
    cmd->execute();
    m_currentCmd->addCmd(cmd);
}

/// @brief 清空重做列表
void MTextEditCmdManager::clearRedo()
{
    // 最后的是最近的undo的命令，所以从前往后清理
    for (auto cmd : m_redoCmds)
    {
        cmd->clear();
        delete cmd;
    }
    m_redoCmds.clear();
}

/// @brief 清空撤销列表
void MTextEditCmdManager::clearUndo()
{
    // 最后的是最近提交的命令，所以从后往前清理
    for (auto it = m_undoCmds.end(); it != m_undoCmds.begin();)
    {
        --it; // 先递减迭代器
        (*it)->clear();
        delete *it;
    }
    m_undoCmds.clear();
}

/// @brief 开始一个宏命令
void MTextEditCmdManager::start(const std::string& name)
{
    if (!m_currentCmd)
    {
        m_currentCmd = new MacroCmd(name);
    }
}

/// @brief 结束当前宏命令
void MTextEditCmdManager::done()
{
    m_undoCmds.emplace_back(m_currentCmd);
    m_currentCmd = nullptr;
    //清空redo列表
    clearRedo();
    emit cmdChanged();
}
