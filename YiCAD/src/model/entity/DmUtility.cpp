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


/// @file DmUtility.cpp
/// @brief DmUtility 工具类的实现

#include "DmUtility.h"

#include <QString>

// Converts a double to a string cutting away unnecessary 0's.
// e.g. 2.70000  -> 2.7
QString DmUtility::doubleToString(double value, int precision)
{
	QString ret;

	ret.setNum(value, 'f', precision);

	if (ret.contains('.'))
	{
		// remove trailing zeros:
		while (ret.at(ret.length() - 1) == '0')
		{
			ret.truncate(ret.length() - 1);
		}
		// remove trailing .
		if (ret.at(ret.length() - 1) == '.')
		{
			ret.truncate(ret.length() - 1);
		}
	}
	return ret;
}
