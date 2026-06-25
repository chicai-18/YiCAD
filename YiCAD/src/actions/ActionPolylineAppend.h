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


/// @file ActionPolylineAppend.h
/// @brief 多段线追加节点操作，支持在非闭合多段线的首端或末端追加节点

#ifndef ACTIONPOLYLINEAPPEND_H
#define ACTIONPOLYLINEAPPEND_H

#include "PreviewActionInterface.h"

#include "DmPolyline.h"

/// @brief 多段线追加节点操作，支持在非闭合多段线的首端或末端追加节点
class ActionPolylineAppend : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief 操作状态枚举
    enum Status
    {
        SetStartpoint,  ///< 设置起始点（选择多段线）
        SetNextPoint    ///< 设置下一个追加点
    };

public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    ActionPolylineAppend(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 执行追加节点操作
    void trigger() override;

    /// @brief 处理鼠标释放事件
    /// @param [in] e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 处理鼠标移动事件
    /// @param [in] e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 处理坐标输入事件
    /// @param [in] e 坐标事件指针
    void coordinateEvent(GuiCoordinateEvent* e) override;

    /// @brief 更新鼠标按钮提示文本
    void updateMouseButtonHints() override;

private:
    DmPolyline* originalPolyline = nullptr;     ///< 原始多段线指针
    PolylineData data;                          ///< 修改后的多段线数据
    bool prepend = false;                       ///< 是否在首端追加（false为末端追加）
};

#endif
