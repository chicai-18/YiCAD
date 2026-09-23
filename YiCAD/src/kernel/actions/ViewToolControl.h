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

/// @file ViewToolControl.h
/// @brief 视图工具控制器：维护 IViewTool 的三层栈并统一分发事件
///
/// 参考 E:\dev\DS 的 Application/ViewToolControl.h。内部维护三层：
/// 业务工具栈（后进先出，最高优先级）、选择工具（次优先）、导航工具
/// （栈底，兜底）。事件按此顺序分发，直到某层返回 Handled/Cancel。
///
/// 阶段2（本次落地）只挂载了导航工具（PanZoomTool）；选择工具与业务
/// 工具栈是为 SelectTool、GripEditTool 以及未来把旧版 Action 体系包成
/// 一个业务工具（5.4节第6项的适配器）预留的接口，当前未使用，见
/// doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段2"执行结果"一节的范围说明。
///
/// GuiDocumentView 在事件到达旧版 GuiEventHandler 之前，先把事件交给
/// 本类；任一层返回 Handled，事件到此为止，不再转发给 GuiEventHandler。

#ifndef VIEWTOOLCONTROL_H
#define VIEWTOOLCONTROL_H

#include <optional>
#include <vector>

#include "IViewTool.h"

class IDocumentView;
class QMouseEvent;
class QKeyEvent;
class QWheelEvent;

class ViewToolControl
{
public:
    /// @param docView 光标应用目标；为空则不做光标仲裁
    explicit ViewToolControl(IDocumentView* docView);
    ~ViewToolControl();

    /// @brief 设置导航工具（栈底，兜底之一）。传 nullptr 取消。
    void setNavigationTool(IViewTool* tool);
    /// @brief 设置选择工具（导航之上，兜底之一）。传 nullptr 取消。
    void setSelectionTool(IViewTool* tool);

    /// @brief 激活一个业务工具：追加到栈顶，享有最高优先级
    /// 若已在栈中，不重复添加
    void activate(IViewTool* tool);
    /// @brief 停用一个业务工具：从栈中移除（无论位置）
    void deactivate(IViewTool* tool);
    /// @brief 停用所有业务工具（不影响选择/导航工具）
    void deactivateAll();
    /// @brief 查询工具当前是否处于激活状态（业务栈或选择/导航槽位）
    bool isActive(IViewTool* tool) const;

    ViewToolResult mousePressEvent(QMouseEvent* e);
    ViewToolResult mouseReleaseEvent(QMouseEvent* e);
    ViewToolResult mouseMoveEvent(QMouseEvent* e);
    ViewToolResult mouseDoubleClickEvent(QMouseEvent* e);

    ViewToolResult keyPressEvent(QKeyEvent* e);
    ViewToolResult keyReleaseEvent(QKeyEvent* e);

    ViewToolResult wheelEvent(QWheelEvent* e);

    void enterEvent();
    void leaveEvent();

private:
    template <typename EventFunc>
    ViewToolResult dispatch(EventFunc&& func);

    /// @brief 按与 dispatch 相同的栈序拉取首个有偏好的光标并应用
    /// 全体无偏好时不触碰当前光标——见 IViewTool::getCursor 的说明，
    /// 这是与 DS 参考实现（无偏好即恢复默认箭头）刻意不同之处：
    /// 旧版 Action 体系仍在通过 ActionInterface::updateMouseCursor 直接
    /// 调用 IDocumentView::setMouseCursor，本类不应在它们之上重置默认光标。
    void refreshCursor();

    IDocumentView* m_docView = nullptr;

    IViewTool* m_navigationTool = nullptr;
    IViewTool* m_selectionTool = nullptr;
    std::vector<IViewTool*> m_businessTools;

    std::optional<DM::CursorType> m_lastAppliedCursor;
};

#endif // VIEWTOOLCONTROL_H
