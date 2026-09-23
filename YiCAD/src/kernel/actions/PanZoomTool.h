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

/// @file PanZoomTool.h
/// @brief 平移导航工具：ViewToolControl 的导航层
///
/// 吸收了两处原本各自为政的平移实现：
///   1. GuiDocumentView.cpp 里硬编码的中键平移
///      （`if (e->button()==MiddleButton) setCurrentAction(new ActionZoomPan(...))`，
///       P4 的证据之一：绕过 GuiEventHandler 的分发逻辑）。
///   2. ActionDefault::Panning 状态（Ctrl+左键拖拽，仅在 Neutral 状态下生效）。
///
/// 本类只负责平移的**机制**：给定一次"开始/移动/结束"手势就执行
/// docView->zoomPan(...)。"这次按下算不算平移手势"的**策略**——中键总是算，
/// Ctrl+左键只在没有业务 Action 活动时才算——由调用方（GuiDocumentView）
/// 判断后再决定要不要把事件转发给本工具，本类不依赖 GuiEventHandler，
/// 保持导航层与旧版 Action 体系解耦。
///
/// 显式的"平移"命令（Ribbon 的 Pan 按钮，`ActionZoomPan`）不在本类改动
/// 范围内：那是用户主动进入的模态命令，与这里"任何时候中键一按就能平移"
/// 的导航手势是两回事，语义不同，予以保留。
///
/// 见 doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段2 第5.4节第2项。

#ifndef PANZOOMTOOL_H
#define PANZOOMTOOL_H

#include <Qt>

#include "DmVector.h"
#include "IViewTool.h"

class IDocumentView;

class PanZoomTool : public IViewTool
{
public:
    explicit PanZoomTool(IDocumentView* docView);

    ViewToolResult mousePressEvent(QMouseEvent* e) override;
    ViewToolResult mouseMoveEvent(QMouseEvent* e) override;
    ViewToolResult mouseReleaseEvent(QMouseEvent* e) override;

    std::optional<DM::CursorType> getCursor() const override;

    /// @brief 是否正在平移中
    /// 供 GuiDocumentView 判断：按下手势已被本工具接受后，后续的
    /// mouseMoveEvent/mouseReleaseEvent 即便不满足调用方自己的转发
    /// 策略（比如释放的不是触发平移的那个按钮），也应该先经过本工具。
    bool isPanning() const { return m_panning; }

private:
    IDocumentView* m_docView = nullptr;
    bool m_panning = false;
    Qt::MouseButton m_panButton = Qt::NoButton;  ///< 开始平移的按钮，据此判断何时结束
    DmVector m_lastGuiPos;                        ///< 上一次鼠标位置（GUI 像素坐标）
};

#endif // PANZOOMTOOL_H
