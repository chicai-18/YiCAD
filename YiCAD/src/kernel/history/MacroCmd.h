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

/// @file MacroCmd.h
/// @brief 宏命令（组合命令），将多个命令组合成一个原子Undo/Redo单元

#ifndef MACROCMD_H
#define MACROCMD_H

#include <vector>
#include <string>
#include "Cmd.h"

/// @brief 命令类型与对象的元组列表类型
using CmdTypeObjectVector = std::vector<std::tuple<CmdType, DmObject*>>;

/// @brief 宏命令（组合模式）
///
/// 将多个ICmd组合成一个大命令，作为整体执行撤销/重做。
/// 支持迭代器遍历子命令列表。
class MacroCmd : public ICmd
{
public:
    /// @brief 构造宏命令
    /// @param name 命令名称
    MacroCmd(const std::string& name);

    /// @brief 析构，清理所有子命令
    ~MacroCmd();

    /// @brief 获取命令名称
    /// @return 命令名称
    std::string getName() const { return m_name; }

    /// @brief 添加子命令
    /// @param cmd 要添加的命令（所有权转移给MacroCmd）
    void addCmd(ICmd* cmd);

    /// @brief 执行所有子命令
    void execute() override;

    /// @brief 逆序撤销所有子命令
    void undo() override;

    /// @brief 顺序重做所有子命令
    void redo() override;

    /// @brief 回滚：对已执行的子命令执行撤销
    void rollback() override;

    /// @brief 递归获取所有子命令的类型和对象
    /// @param result 输出：命令类型与对象的元组列表
    void getCmdTypes(CmdTypeObjectVector& result) const;

    /// @brief 获取子命令数
    /// @return 子命令数量
    size_t size() const { return m_cmds.size(); }

    /// @brief 提取指定位置的命令（从列表中移除但不删除）
    /// @param index 子命令索引
    /// @return 被提取的命令（调用者需负责释放）
    ICmd* extractCmd(size_t index);

    // 迭代器支持

    /// @brief 获取正向迭代器起始
    std::vector<ICmd*>::iterator begin() { return m_cmds.begin(); }

    /// @brief 获取正向常量迭代器起始
    std::vector<ICmd*>::const_iterator begin() const { return m_cmds.begin(); }

    /// @brief 获取正向迭代器结束
    std::vector<ICmd*>::iterator end() { return m_cmds.end(); }

    /// @brief 获取正向常量迭代器结束
    std::vector<ICmd*>::const_iterator end() const { return m_cmds.end(); }

    /// @brief 获取反向迭代器起始
    std::vector<ICmd*>::reverse_iterator rbegin() { return m_cmds.rbegin(); }

    /// @brief 获取反向常量迭代器起始
    std::vector<ICmd*>::const_reverse_iterator rbegin() const { return m_cmds.rbegin(); }

    /// @brief 获取反向迭代器结束
    std::vector<ICmd*>::reverse_iterator rend() { return m_cmds.rend(); }

    /// @brief 获取反向常量迭代器结束
    std::vector<ICmd*>::const_reverse_iterator rend() const { return m_cmds.rend(); }

    /// @brief 清理所有子命令
    virtual void clear() override;

private:
    std::vector<ICmd*> m_cmds;  ///< 子命令列表
    std::string m_name;         ///< 命令名称
};

#endif //MACROCMD_H
