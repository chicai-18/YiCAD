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


/// @file ActionDrawCircle3p.h
/// @brief 通过三个点绘制圆的交互动作类

#ifndef ACTIONDRAWCIRCLE3P_H
#define ACTIONDRAWCIRCLE3P_H

#include "PreviewActionInterface.h"

struct DmCircleData;

/// @brief 通过给定的三个点绘制圆的交互动作
class ActionDrawCircle3P : public PreviewActionInterface
{
    Q_OBJECT
public:
    /// @brief 动作状态枚举
    enum Status
    {
        SetPoint1, ///< 设置第一个点
        SetPoint2, ///< 设置第二个点
        SetPoint3  ///< 设置第三个点
    };

public:
    ActionDrawCircle3P(DmDocument* doc, GuiDocumentView* docView);
    ~ActionDrawCircle3P() override;

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
    /// @brief 已定义的圆数据点
    struct Points;
    std::unique_ptr<Points> pPoints;
};

#endif
