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

/// @file LegacyActionTool.h
/// @brief 业务工具适配器：把 GuiEventHandler 的旧版 Action 栈包成
/// ViewToolControl 业务工具栈的一项（阶段2 第5.4节第6项）
///
/// 没有业务 Action 活动时，对全部事件返回 NotHandled，空闲态事件直接落到
/// 选择层 SelectTool（doc/COMMAND_TOOL_MIGRATION_PLAN.md 第一步）。
///
/// 有业务 Action 活动时，GuiEventHandler 的分发语义是"栈顶 Action 总是
/// 处理"，本类原样转发并返回 Handled，只有以下例外：
///   1. 中键按下：永远不属于业务 Action，让给导航层 PanZoomTool；
///   2. 平移进行中（PanZoomTool::isPanning()）的移动/释放：避免业务层抢在
///      导航层之前把事件处理掉，导致平移中断；
///   3. 栈顶 Action 的 ActionInterface::passesToSelection() 对该事件返回
///      true：照常转发给它，然后返回 NotHandled，事件继续落到选择层。
///      这是第一步的临时钩子，取代块编辑、多行文字属性编辑对默认 Action
///      的直调，第四步随 ActionInterface 删除。
///
/// 见 doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段2 5.7 节。

#ifndef LEGACYACTIONTOOL_H
#define LEGACYACTIONTOOL_H

#include "IViewTool.h"

class ActionInterface;
class GuiEventHandler;
class PanZoomTool;
class QEvent;

class LegacyActionTool : public IViewTool
{
public:
    /// @param handler 非持有，GuiDocumentView 拥有的既有事件处理器
    /// @param panTool 非持有，用于查询"导航层是否正在平移中"
    LegacyActionTool(GuiEventHandler* handler, PanZoomTool* panTool);

    ViewToolResult mousePressEvent(QMouseEvent* e) override;
    ViewToolResult mouseReleaseEvent(QMouseEvent* e) override;
    ViewToolResult mouseMoveEvent(QMouseEvent* e) override;
    ViewToolResult mouseDoubleClickEvent(QMouseEvent* e) override;
    ViewToolResult keyPressEvent(QKeyEvent* e) override;
    ViewToolResult keyReleaseEvent(QKeyEvent* e) override;

    /// @brief 鼠标进入画布：恢复栈顶业务 Action
    void enterEvent() override;
    /// @brief 鼠标离开画布：挂起栈顶业务 Action
    void leaveEvent() override;

private:
    /// @brief 事件的去向
    enum class Route
    {
        Skip,           ///< 没有业务 Action：不转发，返回 NotHandled
        Forward,        ///< 转发给栈顶 Action，返回 Handled
        ForwardAndPass  ///< 转发给栈顶 Action，再返回 NotHandled 交给选择层
    };

    /// @brief 转发前决定事件的去向（问栈顶 Action 的 passesToSelection()）
    Route routeOf(const QEvent* e) const;

    /// @brief 已转发时的分发结果
    static ViewToolResult resultOf(Route route);

    GuiEventHandler* m_handler = nullptr;
    PanZoomTool* m_panTool = nullptr;
};

#endif // LEGACYACTIONTOOL_H
