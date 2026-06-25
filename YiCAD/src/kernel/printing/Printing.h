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

/// @file Printing.h
/// @brief 打印功能命名空间，提供纸张格式转换等打印相关功能

#ifndef PRINTING_H
#define PRINTING_H

#include <QPrinter>
#include "Datamodel.h"

/// @brief 打印相关功能
namespace Printing
{
	/// @brief 将YiCAD纸张格式转换为Qt纸张格式
	/// @param f YiCAD纸张格式
	/// @return 对应的Qt纸张格式
	QPrinter::PageSize rsToQtPaperFormat(DM::PaperFormat f);
}

#endif // PRINTING_H
