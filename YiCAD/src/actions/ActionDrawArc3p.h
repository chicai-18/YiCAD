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


/// @file ActionDrawArc3p.h
/// @brief 通过三点绘制圆弧的交互动作类

#ifndef ACTIONDRAWARC3P_H
#define ACTIONDRAWARC3P_H

#include "PreviewActionInterface.h"

/// @brief 通过三个给定点绘制圆弧的交互动作类
class ActionDrawArc3P : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief 交互状态枚举
    enum Status
    {
        SetPoint1, /**< 设置第一个点 */
        SetPoint2, /**< 设置第二个点 */
        SetPoint3  /**< 设置第三个点 */
    };

public:
    ActionDrawArc3P(DmDocument* doc, GuiDocumentView* docView);
    ~ActionDrawArc3P() override;

    void reset();

    void init(int status = 0) override;

    void trigger() override;
    void preparePreview();

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void coordinateEvent(GuiCoordinateEvent* e) override;
    void commandEvent(GuiCommandEvent* e) override;
    QStringList getAvailableCommands() override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

protected:
    /// @brief 定义圆弧的三点数据
    struct Points;
    std::unique_ptr<Points> pPoints; ///< 三点数据指针
};

#endif
