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


/// @file DmPen.cpp
/// @brief 画笔类实现：构造函数和析构函数

#include "DmPen.h"
#include "DmLineTypeTable.h"

#include <iostream>

DmPen::DmPen()
    : DmFlags()
{
    setColor(DmColor(0, 0, 0));
    setWidth(DM::Width00);
    setLineType(DmLineTypeTable::Continuous);
}

DmPen::DmPen(const DmColor& c, DM::LineWidth w, DmLineType* t)
    : DmFlags()
{
    setColor(c);
    setWidth(w);
    setLineType(t);
}

DmPen::DmPen(unsigned int f)
    : DmFlags(f)
{
    setColor(DmColor(0, 0, 0));
    setWidth(DM::Width00);
    setLineType(DmLineTypeTable::Continuous);
}

DmPen::~DmPen()
{
}
