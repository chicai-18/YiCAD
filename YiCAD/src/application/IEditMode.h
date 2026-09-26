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

/// @file IEditMode.h
/// @brief 编辑模式接口：进入后常驻在业务栈底部的工具（块编辑）
///
/// doc/COMMAND_TOOL_MIGRATION_PLAN.md 第 5 节"命令并存"：块编辑不是命令，而是
/// "编辑模式"。进入时在业务栈底部常驻一个工具，它处理自己的退出、其余事件让给
/// 选择层；模式里启动的命令叠在它上面，结束后回到模式。由视图的命令总线持有（ExclusiveCommandBus::enterEditMode）。
///
/// 启动命令不影响编辑模式；"结束全部命令"（Esc/空格未被接受、Ribbon 的结束全部、
/// 需要结束全部的即时命令）与视图关闭时，总线先征求命令同意，再征求编辑模式同意（5.1 节）。

#ifndef IEDITMODE_H
#define IEDITMODE_H

#include "IExclusiveCommand.h"
#include "IViewTool.h"

/// @brief 编辑模式
class IEditMode : public IViewTool
{
public:
    /// @brief 结束全部命令或视图关闭前调用：模式在此决定退出时保存还是放弃
    /// @param reason Cancelled（结束全部命令）、Replaced（需要结束全部的即时命令）或 ViewClosing
    /// @return false 表示否决：模式与命令都继续，"清空选择"也不执行；ViewClosing 时
    ///         返回值被忽略
    /// @note 可以弹模态对话框；回调期间总线忽略新的启动与结束请求
    virtual bool onEndRequested(CommandEndReason reason) = 0;

    /// @brief 离开编辑模式：总线已把工具移出业务栈，模式按之前的决定收尾
    virtual void onExit() = 0;

    /// @brief 命令叠到模式之上：收起模式的界面（选项条）
    virtual void suspendMode() = 0;
    /// @brief 叠在上面的命令结束：恢复模式的界面与提示
    virtual void resumeMode() = 0;
};

#endif // IEDITMODE_H
