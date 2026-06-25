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


/// @file ActionInfoArea.h
/// @brief 面积测量 Action 类，处理用户通过多边形测量面积和周长

#ifndef ACTIONINFOAREA_H
#define ACTIONINFOAREA_H

#include "PreviewActionInterface.h"

class InfoArea;

/// @brief 面积测量 Action，通过定义多边形来测量封闭区域面积和周长
class ActionInfoArea : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief Action 状态枚举
    enum Status
    {
        SetFirstPoint, ///< 设置多边形第一个点
        SetNextPoint   ///< 设置多边形下一个点
    };

public:
    /// @brief 构造函数
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    ActionInfoArea(DmDocument* doc, GuiDocumentView* docView);
    ~ActionInfoArea() override;

    /// @brief 初始化 Action 状态
    /// @param status 初始状态
    void init(int status = 0) override;
    void trigger() override;
    void display();

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void coordinateEvent(GuiCoordinateEvent* e) override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

private:
    std::unique_ptr<InfoArea> ia; ///< 面积信息对象
};

#endif // ACTIONINFOAREA_H
