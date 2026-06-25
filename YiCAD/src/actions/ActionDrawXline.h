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


/// @file ActionDrawXline.h
/// @brief 构造线绘制 Action 类，处理用户事件以绘制构造线

#ifndef ACTIONDRAWXLINE_H
#define ACTIONDRAWXLINE_H

#include "PreviewActionInterface.h"
#include "DmXline.h"

/// @brief 处理构造线绘制的用户交互 Action
class ActionDrawXline : public PreviewActionInterface
{
public:
    /// @brief Action 状态枚举
    enum Status
    {
        SetBasePoint,  ///< 设置基点
        SetDir         ///< 设置方向
    };

public:
    /// @brief 构造函数
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    ActionDrawXline(DmDocument* doc, GuiDocumentView* docView);
    ~ActionDrawXline() override;

    /// @brief 重置数据
    void reset();

    /// @brief 初始化 Action 状态
    /// @param status 初始状态
    void init(int status = 0) override;
    void trigger() override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void coordinateEvent(GuiCoordinateEvent* e) override;
    void commandEvent(GuiCommandEvent* e) override;
    QStringList getAvailableCommands() override;

    void showOptions() override;
    void hideOptions() override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

protected:
    std::unique_ptr<XLineData> data;
};

#endif // ACTIONDRAWXLINE_H
