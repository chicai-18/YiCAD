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


/// @file ActionDrawLineFree.h
/// @brief 自由手绘线交互动作类头文件

#ifndef ACTIONDRAWLINEFREE_H
#define ACTIONDRAWLINEFREE_H

#include "PreviewActionInterface.h"

class DmPolyline;

/// @brief 自由手绘线交互动作类，处理用户鼠标事件以绘制自由手绘线。
class ActionDrawLineFree : public PreviewActionInterface
{
    Q_OBJECT

public:

    /// @brief 动作状态枚举
    enum Status
    {
        SetStartpoint, ///< 设置起点
        Dragging       ///< 拖拽中
    };

    /// @brief 构造函数
    /// @param doc 文档对象指针
    /// @param docView 文档视图指针
    ActionDrawLineFree(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 析构函数
    ~ActionDrawLineFree() override;

    /// @brief 触发动作完成，结束当前绘制并将有效的折线添加到文档
    void trigger() override;

    /// @brief 鼠标移动事件处理，拖拽时根据鼠标位置追加顶点
    /// @param e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 鼠标按下事件处理，开始新的自由手绘
    /// @param e 鼠标事件指针
    void mousePressEvent(QMouseEvent* e) override;

    /// @brief 鼠标释放事件处理，结束当前绘制
    /// @param e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 更新鼠标按钮提示信息
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

protected:
    std::unique_ptr<DmVector> vertex;     ///< 上一个点
    std::unique_ptr<DmPolyline> polyline; ///< 自由手绘折线对象
};

#endif
