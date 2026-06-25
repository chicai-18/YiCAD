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


/// @file ActionDrawCircle.h
/// @brief 通过圆心和半径绘制圆的交互动作类

#ifndef ACTIONDRAWCIRCLE_H
#define ACTIONDRAWCIRCLE_H

#include "PreviewActionInterface.h"

class CircleData;

/// @brief 通过给定圆心和圆上一点绘制圆的交互动作类
class ActionDrawCircle : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief 交互状态枚举
    enum Status
    {
        SetCenter, /**< 设置圆心 */
        SetRadius  /**< 设置半径 */
    };

public:
    ActionDrawCircle(DmDocument* doc, GuiDocumentView* docView);
    ~ActionDrawCircle() override;

    void reset();

    void init(int status = 0) override;

    void trigger() override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void coordinateEvent(GuiCoordinateEvent* e) override;
    void commandEvent(GuiCommandEvent* e) override;
    QStringList getAvailableCommands() override;

    void hideOptions() override;
    void showOptions() override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

protected:
    std::unique_ptr<CircleData> data; ///< 已定义的圆数据
};

#endif
