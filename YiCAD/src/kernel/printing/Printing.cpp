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

/// @file Printing.cpp
/// @brief 打印功能实现，纸张格式转换

#include "Printing.h"

/// @brief 将YiCAD纸张格式转换为Qt纸张格式
/// @param f YiCAD纸张格式
/// @return 对应的Qt纸张格式，未匹配时返回Custom
QPrinter::PageSize Printing::rsToQtPaperFormat(const DM::PaperFormat f)
{
	switch (f)
	{
	case DM::A0: return QPrinter::A0;
	case DM::A1: return QPrinter::A1;
	case DM::A2: return QPrinter::A2;
	case DM::A3: return QPrinter::A3;
	case DM::A4: return QPrinter::A4;

	case DM::Letter: return QPrinter::Letter;
	case DM::Legal:  return QPrinter::Legal;
	case DM::Tabloid: return QPrinter::Tabloid;

	case DM::Ansi_C: return QPrinter::AnsiC;
	case DM::Ansi_D: return QPrinter::AnsiD;
	case DM::Ansi_E: return QPrinter::AnsiE;

	case DM::Arch_A: return QPrinter::ArchA;
	case DM::Arch_B: return QPrinter::ArchB;
	case DM::Arch_C: return QPrinter::ArchC;
	case DM::Arch_D: return QPrinter::ArchD;
	case DM::Arch_E: return QPrinter::ArchE;

	default:
		return QPrinter::Custom;
	}
}
