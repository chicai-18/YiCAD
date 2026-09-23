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

/// @file IViewTool.h
/// @brief 视图工具接口：交互层工具化的核心抽象
///
/// 把"选择""平移""捕捉"等画布交互从 Action 体系中剥离为独立、可叠加的
/// 工具层。事件集对齐 Qt（mousePress/Release/Move/DoubleClick、
/// keyPress/Release、wheel、enter/leave），由 ViewToolControl 统一分发。
/// 参考 E:\dev\DS 的 Application/IViewTool.h（HOOPS/HEventInfo 版本），
/// 本接口把事件类型换成 Qt 原生事件，把光标类型换成 YiCAD 自己的
/// DM::CursorType（配合 IDocumentView::setMouseCursor 使用），
/// 去掉了 3D 相关的触摸/定时器事件（YiCAD 是纯 2D 画布）。
/// 见 doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段2 第5.4节第1项。

#ifndef IVIEWTOOL_H
#define IVIEWTOOL_H

#include <optional>

#include "Datamodel.h"

class QMouseEvent;
class QKeyEvent;
class QWheelEvent;

/// @brief 视图工具处理一个事件后的结果
enum class ViewToolResult
{
    NotHandled, ///< 未处理，ViewToolControl 应继续向下一层工具传递
    Handled,    ///< 已处理，分发到此为止
    Cancel      ///< 中止处理（语义上区别于正常完成的 Handled，
                ///< 当前分发行为与 Handled 相同，都会停止继续传递；
                ///< 保留区分是为了未来工具能识别"用户主动取消"这一来源）
};

/// @brief 视图工具接口
/// @details 由 ViewToolControl 按栈序分发；每个回调返回 ViewToolResult::
///          Handled（已处理，停止分发）、NotHandled（未处理，继续下传）
///          或 Cancel（中止）。
class IViewTool
{
public:
    virtual ~IViewTool() = default;

    virtual ViewToolResult mousePressEvent(QMouseEvent*) { return ViewToolResult::NotHandled; }
    virtual ViewToolResult mouseReleaseEvent(QMouseEvent*) { return ViewToolResult::NotHandled; }
    virtual ViewToolResult mouseMoveEvent(QMouseEvent*) { return ViewToolResult::NotHandled; }
    virtual ViewToolResult mouseDoubleClickEvent(QMouseEvent*) { return ViewToolResult::NotHandled; }

    virtual ViewToolResult keyPressEvent(QKeyEvent*) { return ViewToolResult::NotHandled; }
    virtual ViewToolResult keyReleaseEvent(QKeyEvent*) { return ViewToolResult::NotHandled; }

    virtual ViewToolResult wheelEvent(QWheelEvent*) { return ViewToolResult::NotHandled; }

    /// @brief 鼠标进入/离开画布。不参与 Handled/NotHandled 仲裁，
    /// 纯粹是生命周期通知（对应 GuiDocumentView::enterEvent/leaveEvent）。
    virtual void enterEvent() {}
    virtual void leaveEvent() {}

    /// @brief 工具希望显示的光标
    /// @return nullopt   — 无偏好，不改变当前光标（由 ViewToolControl 保证：
    ///                     无偏好时不覆盖旧版 Action 体系自行设置的光标，
    ///                     见 ViewToolControl::refreshCursor）；
    ///         其它取值  — 使用该光标，并截断下层工具的偏好。
    /// @note 仲裁按事件分发优先级进行（业务工具栈后进先出 -> 选择工具 ->
    ///       导航工具），取首个非 nullopt 者。需要读取实时修饰键状态的工具
    ///       应在此处直接查询（如 QGuiApplication::keyboardModifiers()）。
    virtual std::optional<DM::CursorType> getCursor() const { return std::nullopt; }

    /// @brief 工具被压入 ViewToolControl 时调用
    virtual void onActivate() {}
    /// @brief 工具从 ViewToolControl 移除时调用
    virtual void onDeactivate() {}
};

#endif // IVIEWTOOL_H
