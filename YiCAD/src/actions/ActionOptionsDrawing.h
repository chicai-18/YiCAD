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


/// @file ActionOptionsDrawing.h
/// @brief 绘图选项交互命令头文件

#ifndef ACTIONOPTIONSDRAWING_H
#define ACTIONOPTIONSDRAWING_H

#include "ActionInterface.h"

/// @brief 绘图选项交互命令
///
/// 打开绘图选项设置对话框，用户可在此修改与绘图相关的配置参数。
class ActionOptionsDrawing : public ActionInterface
{
    Q_OBJECT

public:
    ActionOptionsDrawing(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 初始化并立即触发命令
    /// @param [in] status 初始状态，默认为0
    void init(int status = 0) override;

    /// @brief 触发绘图选项对话框显示
    void trigger() override;
};

#endif
