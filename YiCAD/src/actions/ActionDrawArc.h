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


/// @file ActionDrawArc.h
/// @brief 通过圆心、半径和起始/终止角度绘制圆弧的交互动作类

#ifndef ACTIONDRAWARC_H
#define ACTIONDRAWARC_H

#include "PreviewActionInterface.h"

class DmArc;

/// @brief 通过圆心、半径、起始角和终止角绘制简单圆弧的交互动作类
class ActionDrawArc : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief 交互状态枚举
    enum Status
    {
        SetCenter, /**< 设置圆心 */
        SetRadius, /**< 设置半径 */
        SetAngle1, /**< 设置起始角度 */
        ArcAngle   /**< 设置圆弧角度 */
    };

public:
    ActionDrawArc(DmDocument* doc, GuiDocumentView* docView);
    ~ActionDrawArc() override;

    void reset();

    void init(int status = 0) override;
    void trigger() override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void coordinateEvent(GuiCoordinateEvent* e) override;
    void commandEvent(GuiCommandEvent* e) override;

    void hideOptions() override;
    void showOptions() override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

    bool isClockwise() const;
    void setClockwise(bool clockwise);

private:
    /// @brief 设置起始角度
    /// @param mouse 鼠标位置
    void setStartAngle(const DmVector& mouse);

    /// @brief 设置终止角度
    /// @param mouse 鼠标位置
    void setEndAngle(const DmVector& mouse);

protected:
    std::unique_ptr<DmArc> tempArc; ///< 绘制过程中临时的圆弧，其法向根据选项可能是负的
};

#endif
