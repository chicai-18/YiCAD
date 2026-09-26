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

/// @file BuiltinExtensions.h
/// @brief 随程序发布的进程内扩展的注册表

#ifndef BUILTINEXTENSIONS_H
#define BUILTINEXTENSIONS_H

class ExtensionManager;

/// @brief 注册随程序发布的进程内扩展（src/extensions/<扩展>/），注册顺序即启动顺序、关闭的反序。
///
/// 每个扩展是独立的 OBJECT 库 YiCadExt_<扩展>，只有可执行文件链接它们；本文件与 Main.cpp
/// 一样只编进可执行文件，内核与主窗口不引用任何扩展（主计划 7.11 节）。由 main() 交给
/// ApplicationWindow，在内置 Ribbon 类目注册之后调用。
/// @param manager 扩展管理器
void registerBuiltinExtensions(ExtensionManager& manager);

#endif // BUILTINEXTENSIONS_H
