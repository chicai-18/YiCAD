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


/// @file ActionZoomPan.h
/// @brief 平移缩放Action，支持鼠标拖拽平移视图

#ifndef ACTIONZOOMPAN_H
#define ACTIONZOOMPAN_H

#include "ActionInterface.h"

/// @brief 平移缩放Action类，处理鼠标拖拽平移视图
class ActionZoomPan : public ActionInterface
{
    Q_OBJECT

public:
    /// @brief 平移操作状态枚举
    enum Status
    {
        SetPanStart, /**< 设置平移起始点 */
        SetPanning,  /**< 正在平移中 */
        SetPanEnd,   /**< 平移结束 */
    };

    /// @brief 拖拽触发的最小像素距离
    static constexpr int MIN_PAN_DISTANCE = 7;

public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    ActionZoomPan(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 初始化操作状态
    /// @param [in] status 初始状态值，默认为0
    void init(int status = 0) override;

    /// @brief 执行平移操作
    void trigger() override;

    /// @brief 处理鼠标移动事件
    /// @param [in] e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 处理鼠标按下事件
    /// @param [in] e 鼠标事件指针
    void mousePressEvent(QMouseEvent* e) override;

    /// @brief 处理鼠标释放事件
    /// @param [in] e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

    /// @brief 更新鼠标按钮提示文本
    void updateMouseButtonHints() override;

    /// @brief 是否为视图操作命令
    /// @return true，平移缩放属于视图操作
    bool isViewAction() override { return true; }

protected:
    int x1 = 0; ///< 拖拽起始X坐标（GUI像素）
    int y1 = 0; ///< 拖拽起始Y坐标（GUI像素）
    int x2 = 0; ///< 拖拽当前X坐标（GUI像素）
    int y2 = 0; ///< 拖拽当前Y坐标（GUI像素）
};

#endif // ACTIONZOOMPAN_H
