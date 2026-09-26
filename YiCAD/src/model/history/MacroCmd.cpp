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

/// @file MacroCmd.cpp
/// @brief 宏命令（组合命令）实现

#include "MacroCmd.h"

/// @brief 构造宏命令
MacroCmd::MacroCmd(const std::string &name)
    : m_name(name)
{
}

/// @brief 析构，清理所有子命令
MacroCmd::~MacroCmd()
{
    clear();
}

/// @brief 添加子命令
void MacroCmd::addCmd(ICmd* cmd)
{
    m_cmds.emplace_back(cmd);
}

/// @brief 执行所有子命令
void MacroCmd::execute()
{
    for (auto cmd : m_cmds)
    {
        cmd->execute();
    }
    m_isExecuted = true;
}

/// @brief 逆序撤销所有子命令
void MacroCmd::undo()
{
    //clion reverse iterator异常?
    //for(auto it = m_cmds.rbegin(); it != m_cmds.rend(); ++it)
    //{
    //    (*it)->undo();
    //}

    for (auto it = m_cmds.end(); it != m_cmds.begin();)
    {
        --it; // 先递减迭代器
        (*it)->undo();
    }
    m_isExecuted = false;
}

/// @brief 顺序重做所有子命令
void MacroCmd::redo()
{
    for (auto cmd : m_cmds)
    {
        cmd->redo();
    }
    m_isExecuted = true;
}

/// @brief 回滚：对已执行的子命令执行撤销
void MacroCmd::rollback()
{
    for (auto it = m_cmds.end(); it != m_cmds.begin();)
    {
        --it;
        if ((*it)->isExecuted())
        {
            (*it)->undo();
        }
    }
    m_isExecuted = false;
}

/// @brief 清理所有子命令
void MacroCmd::clear()
{
    for (auto cmd : m_cmds)
    {
        cmd->clear();
        delete cmd;
    }
    m_cmds.clear();
}

/// @brief 提取指定位置的命令（从列表中移除但不删除）
ICmd* MacroCmd::extractCmd(size_t index)
{
    if (index >= m_cmds.size())
    {
        return nullptr;
    }
    ICmd* cmd = m_cmds[index];
    m_cmds.erase(m_cmds.begin() + index);
    return cmd;
}

/// @brief 递归获取所有子命令的类型和对象
void MacroCmd::getCmdTypes(CmdTypeObjectVector& result) const
{
    for (auto cmd : m_cmds)
    {
        MacroCmd* subMacroCmd = dynamic_cast<MacroCmd*>(cmd);
        if (subMacroCmd)
        {
            subMacroCmd->getCmdTypes(result);
        }
        else
        {
            auto t = std::make_tuple(cmd->getCmdType(), cmd->getObject());
            result.emplace_back(t);
        }
    }
}
