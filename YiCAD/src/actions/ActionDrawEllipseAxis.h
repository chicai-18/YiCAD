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


/// @file ActionDrawEllipseAxis.h
/// @brief 通过中心和轴端点绘制椭圆/椭圆弧的交互动作类

#ifndef ACTIONDRAWELLIPSEAXIS_H
#define ACTIONDRAWELLIPSEAXIS_H

#include "PreviewActionInterface.h"

/// @brief 通过指定中心点、长轴端点和短轴比绘制椭圆或椭圆弧
class ActionDrawEllipseAxis : public PreviewActionInterface
{
    Q_OBJECT
public:
    /// @brief 动作状态枚举
    enum Status
    {
        SetCenter, ///< 设置中心点
        SetMajor,  ///< 设置长轴端点
        SetMinor,  ///< 设置短轴比
        SetAngle1, ///< 设置起始角度
        SetAngle2  ///< 设置结束角度
    };

public:
    /// @brief 构造函数
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    /// @param isArc 为 true 创建椭圆弧，为 false 创建完整椭圆
    ActionDrawEllipseAxis(DmDocument* doc, GuiDocumentView* docView, bool isArc);
    ~ActionDrawEllipseAxis() override;

    void init(int status = 0) override;
    void trigger() override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void coordinateEvent(GuiCoordinateEvent* e) override;
    void commandEvent(GuiCommandEvent* e) override;
    QStringList getAvailableCommands() override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

protected:
    struct Points;
    std::unique_ptr<Points> pPoints;
};

#endif
