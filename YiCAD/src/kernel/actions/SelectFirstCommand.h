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

/// @file SelectFirstCommand.h
/// @brief 先选后建命令的基类：没有选择集时先进入"选择对象"阶段
///
/// 取代原 ActionSelect + ActionSelectMultiple 与 makeSelectFirstFactory
/// （doc/COMMAND_TOOL_MIGRATION_PLAN.md 第二步第 4、5 项）。选择阶段里，本类
/// 激活一个选择阶段工具，它只接管确认、取消与按键，鼠标事件返回 NotHandled，
/// 由选择层 SelectTool 按选择阶段约束完成点选与框选。交互与原先一致
/// （doc/INTERACTION_CHECKLIST.md 第 3 节）：
///   - 回车：有选择集时确认，开始真正的命令；没有时无反应；
///   - 右键：结束整个命令；
///   - Esc：不接受，主窗口随后结束全部命令并清空选择；
///   - 其它按键（含空格）与双击：到此为止，不再下传。

#ifndef SELECTFIRSTCOMMAND_H
#define SELECTFIRSTCOMMAND_H

#include <memory>

#include "BaseExclusiveCommand.h"

class SelectionPhaseTool;

/// @brief 先选后建命令的基类
class SelectFirstCommand : public BaseExclusiveCommand
{
public:
    /// @brief 什么时候进入选择阶段
    enum class SelectionEntry
    {
        WhenEmpty, ///< 没有选择集时（多数命令）
        Always     ///< 总是先进入（删除，主计划 7.7 节）
    };

    ~SelectFirstCommand() override;

    /// @brief 选择阶段确认选择：退出选择阶段，开始真正的命令
    /// @return 不在选择阶段或没有选择集时返回 false，什么也不做
    bool confirmSelection();

    /// @brief 旧 Action 叠在上面时挂起：选择阶段停用选择阶段工具，否则交给 onSuspend()
    void suspend() final;
    /// @brief 叠在上面的旧 Action 全部结束后恢复
    void resume() final;

protected:
    explicit SelectFirstCommand(SelectionEntry entry = SelectionEntry::WhenEmpty);

    /// @brief 按选择集决定进入选择阶段，还是直接开始真正的命令
    bool onActivate() final;
    /// @brief 结束真正的命令（onStop()），停用选择阶段工具
    void onDeactivate() final;

    /// @brief 选择集就绪，开始真正的命令
    /// @details 相当于原先选择完成后才构造、初始化的那个 Action。可以直接完成
    ///          并调用 finish()（分解、反向、删除、总长度）。
    /// @return false 表示启动失败，整个命令随即结束
    virtual bool onSelectionReady() = 0;
    /// @brief 结束真正的命令：停用工具、清除预览（只在 onSelectionReady() 成功后调用）
    virtual void onStop() {}
    /// @brief 真正的命令被挂起（旧 Action 叠在上面）
    virtual void onSuspend() {}
    /// @brief 真正的命令恢复
    virtual void onResume() {}

private:
    /// @brief 调用 onSelectionReady() 并记下结果
    bool startWork();

    SelectionEntry m_entry;
    std::unique_ptr<SelectionPhaseTool> m_selectionTool; ///< 选择阶段工具，命令销毁时才释放
    bool m_selecting = false;                            ///< 是否处于选择阶段
    bool m_working = false;                              ///< 真正的命令是否已开始
};

#endif // SELECTFIRSTCOMMAND_H
