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

/// @file MTextEditCmdManager.h
/// @brief 多行文字编辑器命令管理器，提供Undo/Redo管理和宏命令封装

#ifndef MTEXTEDITCMDMANAGER_H
#define MTEXTEDITCMDMANAGER_H

#include <deque>
#include <QObject>
#include "MacroCmd.h"

/// @brief 多行文字编辑器命令管理器
///
/// 管理多行文字编辑器的Undo/Redo操作，使用双端队列存储命令历史。
/// 支持宏命令模式：通过start/done将一系列连续操作封装成一个MacroCmd统一撤销。
class MTextEditCmdManager : public QObject
{
    Q_OBJECT
public:
    /// @brief 构造命令管理器
    MTextEditCmdManager();

    /// @brief 析构，清理所有Undo/Redo命令
    ~MTextEditCmdManager();

    /// @brief 执行一次撤销操作
    void undo();

    /// @brief 执行一次重做操作
    void redo();

    /// @brief 获取当前可撤销/可重做状态
    /// @param hasUndo 输出：是否有可撤销的命令
    /// @param hasRedo 输出：是否有可重做的命令
    void getCmdData(bool& hasUndo, bool& hasRedo);

    /// @brief 向当前宏命令中添加子命令（不执行）
    /// @param cmd 要添加的命令
    void addToCurrentCmd(ICmd* cmd);

    /// @brief 添加并执行命令
    /// @param cmd 要执行的命令
    void addAndExecuteCmd(ICmd* cmd);

    /// @brief 清空重做列表
    void clearRedo();

    /// @brief 清空撤销列表
    void clearUndo();

    /// @brief 开始一个宏命令（用于将连续操作合并为一个Undo单元）
    /// @param name 宏命令名称
    void start(const std::string& name = "");

    /// @brief 结束当前宏命令，将其提交到撤销列表
    void done();

signals:
    /// @brief 当命令栈发生变化时发出
    void cmdChanged();

private:
    std::deque<ICmd*> m_undoCmds;   ///< 撤销命令队列
    std::deque<ICmd*> m_redoCmds;   ///< 重做命令队列
    MacroCmd* m_currentCmd;         ///< 当前正在构建的宏命令
};

#endif //MTEXTEDITCMDMANAGER_H
