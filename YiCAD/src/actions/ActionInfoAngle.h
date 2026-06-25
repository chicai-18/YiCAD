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


/// @file ActionInfoAngle.h
/// @brief 角度测量 Action 类，处理用户测量两条线之间角度的交互事件

#ifndef ACTIONINFOANGLE_H
#define ACTIONINFOANGLE_H

#include "PreviewActionInterface.h"

/// @brief 角度测量 Action，通过选择两条线来计算夹角
class ActionInfoAngle : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief Action 状态枚举
    enum Status
    {
        SetEntity1, ///< 选择第一条线
        SetEntity2  ///< 选择第二条线
    };

public:
    /// @brief 构造函数
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    ActionInfoAngle(DmDocument* doc, GuiDocumentView* docView);
    ~ActionInfoAngle() override;

    /// @brief 初始化 Action 状态
    /// @param status 初始状态
    void init(int status) override;

    void trigger() override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

private:
    DmEntity* entity1;                  ///< 第一条线实体
    DmEntity* entity2;                  ///< 第二条线实体
    DmEntity* prevHighlighted = nullptr; ///< 上次高亮的实体

    void unhighlightEntity();           ///< 取消当前高亮实体

    struct Points;
    std::unique_ptr<Points> pPoints;    ///< 交点数据
};

#endif // ACTIONINFOANGLE_H
