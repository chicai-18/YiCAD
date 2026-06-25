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


/// @file ActionDrawCircle2p.h
/// @brief 通过两点绘制圆的交互动作类

#ifndef ACTIONDRAWCIRCLE2P_H
#define ACTIONDRAWCIRCLE2P_H

#include "PreviewActionInterface.h"

class CircleData;

/// @brief 通过给定两个点（直径端点）绘制圆的交互动作类
class ActionDrawCircle2P : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief 交互状态枚举
    enum Status
    {
        SetPoint1, /**< 设置第一个点 */
        SetPoint2  /**< 设置第二个点 */
    };

public:
    ActionDrawCircle2P(DmDocument* doc, GuiDocumentView* docView);
    ~ActionDrawCircle2P() override;

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
    std::unique_ptr<CircleData> data; ///< 已定义的圆数据
    struct Points;
    std::unique_ptr<Points> pPoints;  ///< 两点数据
};

#endif
