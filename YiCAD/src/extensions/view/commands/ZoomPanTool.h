/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file ZoomPanTool.h
/// @brief 平移命令 ext.view.pan 的临时视图工具，取代原 ActionZoomPan
///
/// 用户从 Ribbon 或命令行进入的平移模式：左键拖动平移，右键退出。不占命令总线，
/// 叠在业务栈顶，退出后其下的命令照常继续（TransientViewTool.h）。
///
/// 与导航层 PanZoomTool 不是一回事：那是任何时候按住中键（空闲态还有 Ctrl+左键）
/// 就能平移的导航手势；本类是用户主动进入、需要退出的模式。平移模式下的中键
/// 平移仍由导航层处理。

#ifndef ZOOMPANTOOL_H
#define ZOOMPANTOOL_H

#include "TransientViewTool.h"

class IDocumentView;

/// @brief 平移模式工具
class ZoomPanTool : public TransientViewTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetPanStart, ///< 等待按下左键
        SetPanning,  ///< 按住左键拖动中
    };

    /// @brief 拖动超过该像素距离才平移
    static constexpr int MIN_PAN_DISTANCE = 7;

    /// @param view 视图
    explicit ZoomPanTool(IDocumentView* view);

    /// @brief 左键按下开始拖动；中键让给导航层；其余按键到此为止
    ViewToolResult mousePressEvent(QMouseEvent* e) override;
    /// @brief 右键退出平移模式，左键释放结束一次拖动；中键让给导航层
    ViewToolResult mouseReleaseEvent(QMouseEvent* e) override;
    /// @brief 拖动中平移视图；中键平移进行中时让给导航层
    ViewToolResult mouseMoveEvent(QMouseEvent* e) override;
    /// @brief 到此为止，不交给其下的各层
    ViewToolResult mouseDoubleClickEvent(QMouseEvent* e) override;
    /// @brief 不接受（Esc/空格随后由主窗口结束全部命令），也不下传
    ViewToolResult keyPressEvent(QKeyEvent* e) override;
    /// @brief 不接受，也不下传
    ViewToolResult keyReleaseEvent(QKeyEvent* e) override;
    /// @brief 命令行坐标：丢弃，不交给其下的命令
    ViewToolResult coordinateEvent(const DmVector& pos) override;
    /// @brief 命令行文本：不接受（随后被当作新命令），也不交给其下的命令
    ViewToolResult commandEvent(GuiCommandEvent* e) override;

    /// @brief 等待时张开的手，拖动中握紧的手
    std::optional<DM::CursorType> getCursor() const override;

    /// @brief 当前交互状态
    Status status() const { return m_status; }

private:
    IDocumentView* m_view = nullptr;
    Status m_status = SetPanStart;
    int m_lastX = 0; ///< 上一次平移时的鼠标位置（GUI 像素坐标）
    int m_lastY = 0;
};

#endif // ZOOMPANTOOL_H
