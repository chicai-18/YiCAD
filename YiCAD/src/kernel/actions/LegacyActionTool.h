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
/// @brief 业务工具适配器：把 GuiEventHandler 的既有 Action 体系包成
/// ViewToolControl 业务工具栈的一项（阶段2 第5.4节第6项）
///
/// GuiEventHandler 管理的 106 个 Action（画线、修改等）与默认 Action
/// （ActionDefault，内部转发给 SelectTool）保持不动——本类不改写它们
/// 一行代码，只是把"要不要把这个事件转发给 GuiEventHandler"这件事，
/// 从 GuiDocumentView 里散落的按钮/修饰键判断，收敛成一个符合
/// IViewTool 约定、可以参与 ViewToolControl 优先级仲裁的对象。
///
/// GuiEventHandler 本身没有"这次事件我不关心"的概念——它的分发语义是
/// "只要有活动 Action（或默认 Action）就总是处理"。本类因此只在两类
/// 场景下主动声明 NotHandled，把事件让给导航层 PanZoomTool：
///   1. 中键：永远不属于任何业务 Action，一律放行给导航层。
///   2. Ctrl/Meta+左键且没有业务 Action 活动：与原 ActionDefault::
///      Panning 状态的适用范围一致（仅在默认/空闲状态下把左键拖拽
///      解释为平移）。
///   3. 平移正在进行中（PanZoomTool::isPanning()）时的移动/释放：
///      避免业务层抢在导航层之前"消费"掉这次事件，导致平移中断。
/// 除此之外的一切事件都原样转发给 GuiEventHandler，返回 Handled——
/// 与转发前的行为完全一致。
///
/// 见 doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段2 5.7 节。

#ifndef LEGACYACTIONTOOL_H
#define LEGACYACTIONTOOL_H

#include "IViewTool.h"

class GuiEventHandler;
class PanZoomTool;

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

private:
    /// @brief 这次按下是否应该转发给 GuiEventHandler（而非让给导航层）
    bool wantsPress(QMouseEvent* e) const;

    GuiEventHandler* m_handler = nullptr;
    PanZoomTool* m_panTool = nullptr;
};

#endif // LEGACYACTIONTOOL_H
