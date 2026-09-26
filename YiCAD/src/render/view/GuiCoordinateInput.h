/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file GuiCoordinateInput.h
/// @brief 命令行坐标输入的解析：绝对/相对 × 直角/极坐标
///
/// 从 GuiEventHandler::commandEvent 原样抽出（doc/COMMAND_TOOL_MIGRATION_PLAN.md
/// 第二步第 3 项），旧版 Action 栈与命令的放置工具共用：
///   - "x,y"   绝对直角坐标
///   - "@x,y"  相对直角坐标（相对零点）
///   - "r<a"   绝对极坐标，a 为角度（度）
///   - "@r<a"  相对极坐标
/// 各分量是 Math2d::eval 表达式。同时含 ',' 与 '<' 时按直角坐标解析。

#ifndef GUICOORDINATEINPUT_H
#define GUICOORDINATEINPUT_H

#include <QString>

#include "DmVector.h"

/// @brief 命令行坐标输入的解析结果
struct GuiCoordinateInput
{
    /// @brief 解析状态
    enum class Status
    {
        NotCoordinate, ///< 不是坐标格式（既不含 ',' 也不含 '<'），应作为命令文本处理
        Ok,            ///< 坐标，position 有效
        SyntaxError    ///< 是坐标格式但表达式求值失败
    };

    Status status = Status::NotCoordinate; ///< 解析状态
    DmVector position;                     ///< status 为 Ok 时的世界坐标

    /// @brief 解析一段命令行文本
    /// @param cmd 用户输入的文本
    /// @param relativeZero 相对零点，相对坐标以它为基准
    /// @return 解析结果
    static GuiCoordinateInput parse(const QString& cmd, const DmVector& relativeZero);
};

#endif // GUICOORDINATEINPUT_H
