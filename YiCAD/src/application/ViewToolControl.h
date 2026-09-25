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
/// 交互视图 UIView（kernel/interaction/UIView.h）持有本类并挂载三层：业务层（命令的工具、
/// 编辑模式与临时视图工具）、选择层 SelectTool、导航层 PanZoomTool。画布的鼠标、
/// 双击、滚轮后的补发移动与进入/离开事件都经本类分发；键盘事件由主窗口经
/// GuiDocumentView::processKeyEvent() 转交（doc/COMMAND_TOOL_MIGRATION_PLAN.md
/// 第一步）。右键释放与 XButton1 仍由 UIView 直接处理（主计划 5.7 节）。

#ifndef VIEWTOOLCONTROL_H
#define VIEWTOOLCONTROL_H

#include <optional>
#include <vector>

#include "IViewTool.h"

class DmVector;
class GuiCommandEvent;
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
    /// @brief 在业务栈底部常驻一个工具：优先级低于栈里已有的全部业务工具
    /// @details 用于编辑模式（块编辑）：模式里启动的命令叠在它上面。
    ///          若已在栈中，不重复添加。
    void activateAtBottom(IViewTool* tool);
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

    /// @brief 命令行坐标：只沿业务工具栈分发
    ViewToolResult coordinateEvent(const DmVector& pos);
    /// @brief 命令行文本：只沿业务工具栈分发
    ViewToolResult commandEvent(GuiCommandEvent* e);

    void enterEvent();
    void leaveEvent();

private:
    template <typename EventFunc>
    ViewToolResult dispatch(EventFunc&& func);

    /// @brief 只沿业务工具栈（后进先出）分发，不经过选择层与导航层
    template <typename EventFunc>
    ViewToolResult dispatchBusiness(EventFunc&& func);

    /// @brief 按与 dispatch 相同的栈序拉取首个有偏好的光标并应用
    /// 全体无偏好时不触碰当前光标——见 IViewTool::getCursor 的说明，
    /// 这是与 DS 参考实现（无偏好即恢复默认箭头）刻意不同之处：
    /// 块编辑模式、多行文字编辑等仍直接调用 IDocumentView::setMouseCursor/
    /// setCursor，本类不应在它们之上重置默认光标。
    void refreshCursor();

    IDocumentView* m_docView = nullptr;

    IViewTool* m_navigationTool = nullptr;
    IViewTool* m_selectionTool = nullptr;
    std::vector<IViewTool*> m_businessTools;

    std::optional<DM::CursorType> m_lastAppliedCursor;
};

#endif // VIEWTOOLCONTROL_H
