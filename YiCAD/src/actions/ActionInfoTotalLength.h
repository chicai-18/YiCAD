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


/// @file ActionInfoTotalLength.h
/// @brief 总长度测量 Action 类，计算并显示选中实体的总长度

#ifndef ACTIONINFOTOTALLENGTH_H
#define ACTIONINFOTOTALLENGTH_H

#include "ActionInterface.h"

/// @brief 总长度测量 Action，计算选中实体的总长度
class ActionInfoTotalLength : public ActionInterface
{
    Q_OBJECT
public:
    /// @brief Action 状态枚举
    enum Status
    {
        Acknowledge ///< 确认或取消
    };

public:
    /// @brief 构造函数
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    ActionInfoTotalLength(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 初始化并立即触发
    /// @param status 初始状态
    void init(int status = 0) override;

    /// @brief 触发总长度计算和显示
    void trigger() override;
};

#endif
