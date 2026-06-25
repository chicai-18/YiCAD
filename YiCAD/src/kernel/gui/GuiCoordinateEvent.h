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

/// @file GuiCoordinateEvent.h
/// @brief 坐标事件类，封装坐标输入事件

#ifndef GUICOORDINATEEVENT_H
#define GUICOORDINATEEVENT_H

#include "DmVector.h"

/// @brief 坐标事件
class GuiCoordinateEvent
{
public:
    /// @brief 构造坐标事件
    /// @param pos 坐标位置
    GuiCoordinateEvent(const DmVector& pos) : pos(pos) {}

    /// @brief 获取事件中的实际文档坐标
    /// @return 事件发生时的位置坐标
    DmVector getCoordinate()
    {
        return pos;
    }

protected:
    DmVector pos;
};

#endif
