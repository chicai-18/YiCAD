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

/// @file Main.h
/// @brief YiCAD 应用程序主入口头文件
/// @details 定义应用程序启动相关的宏和声明

#ifndef MAIN_H
#define MAIN_H

#include <QStringList>

/// @brief 将宏参数转换为字符串（无法用constexpr替代，因为使用了预处理器字符串化运算符#）
#define STR(x) #x

/// @brief 将宏参数展开后转换为字符串（无法用constexpr替代，因为依赖STR宏的字符串化）
#define XSTR(x) STR(x)

#endif
