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

/// @file PlaceCommand.h
/// @brief 放置命令的基类：一个放置工具、命令的预览与选项条
///
/// 原 PreviewActionInterface 派生、不先选后建的交互 Action（绘图、修改、查询等）
/// 迁移后的公共部分（doc/COMMAND_TOOL_MIGRATION_PLAN.md 第三步）。生命周期与
/// 原先经 GuiEventHandler 运行时一致：
///   - 启动：构造预览与放置工具（相当于原 init()），显示选项条，激活工具（工具被
///     激活时刷新提示、恢复捕捉、重绘预览，相当于原先压栈后 cleanUp() 里的 resume()）；
///   - 结束：停用工具并结束捕捉会话，收起选项条，清除预览（原 finish()）；
///   - 临时视图工具（平移模式）叠上来时停用工具、收起选项条，结束后重新激活、显示
///     选项条（原先被挂起时的 suspend()/hideOptions() 与恢复时的 resume()/showOptions()）。
///
/// 分工：工具持有交互状态机与这次交互采集的点，命令持有选项条的参数、预览与提交；
/// 选项条上作用于交互状态的按钮（撤销、闭合等）由命令转给工具。

#ifndef PLACECOMMAND_H
#define PLACECOMMAND_H

#include <memory>

#include "BaseExclusiveCommand.h"

class BasePlaceTool;
class CommandPreview;

/// @brief 放置命令的基类
class PlaceCommand : public BaseExclusiveCommand
{
public:
    ~PlaceCommand() override;

    /// @brief 平移模式叠在上面时挂起：停用工具，收起选项条
    void suspend() final;
    /// @brief 叠在上面的平移模式结束后恢复：重新激活工具，显示选项条
    void resume() final;

    /// @brief 放置工具的捕捉器
    ISnapService* snapService() const override;

    /// @brief 命令的预览；只在活动期间有效
    CommandPreview& preview() const { return *m_preview; }

protected:
    PlaceCommand();

    /// @brief 构造预览与工具，显示选项条，激活工具
    bool onActivate() final;
    /// @brief 停用工具并结束捕捉会话，收起选项条，清除预览
    void onDeactivate() final;

    /// @brief 构造放置工具（相当于原 Action 的构造与 init()）
    /// @return 空表示启动失败
    virtual std::unique_ptr<BasePlaceTool> createTool() = 0;
    /// @brief 工具激活之后调用（原 Action 在 init() 里直接完成的工作放在这里）
    /// @return false 表示启动失败
    virtual bool onStarted() { return true; }
    /// @brief 显示选项条（原 showOptions()）；默认没有选项条
    virtual void showOptions() {}
    /// @brief 收起选项条（原 hideOptions()）
    virtual void hideOptions() {}

    /// @brief 放置工具；只在活动期间有效
    BasePlaceTool* placeTool() const { return m_tool.get(); }

private:
    std::unique_ptr<CommandPreview> m_preview;
    std::unique_ptr<BasePlaceTool> m_tool; ///< 命令销毁时才释放：结束时它可能还在调用栈上
    bool m_suspended = false;
};

#endif // PLACECOMMAND_H
