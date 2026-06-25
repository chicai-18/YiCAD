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


/// @file Datamodel.cpp
/// @brief 数据模型核心功能的实现，包括线宽转换等工具函数

#include <iostream>
#include <vector>
#include <utility>
#include <climits>

#include "Datamodel.h"

/// @brief 将整数宽度值转换为LineWidth枚举
/// @param w 整数宽度值
/// @return 对应的LineWidth枚举值
DM::LineWidth DM::intToLineWidth(int w)
{
    std::vector<std::pair<int, LineWidth>> const table
    {
        {-2, WidthDefault}, // for w = -3
        {-1, WidthByBlock}, // for w = -2
        {0, WidthByLayer},  // for w = -1
        // for w < 3, return Width00
        {3, Width00},
        {8, Width01},
        {12, Width02},
        {14, Width03},
        {17, Width04},
        {19, Width05},
        {23, Width06},
        {28, Width07},
        {33, Width08},
        {38, Width09},
        {46, Width10},
        {52, Width11},
        {57, Width12},
        {66, Width13},
        {76, Width14},
        {86, Width15},
        {96, Width16},
        {104, Width17},
        {114, Width18},
        {131, Width19},
        {150, Width20},
        {180, Width21},
        {206, Width22},
        {INT_MAX, Width23}
    };

    // binary search
    // assume table size is at least 2
    size_t low = static_cast<size_t>(-1);
    size_t high = table.size() - 1;
    while (low + 1 < high)
    {
        size_t const mid = low + (high - low) / 2;
        if (w >= table.at(mid).first)
        {
            low = mid;
        }
        else
        {
            high = mid;
        }
    }
    return table.at(high).second;
}
