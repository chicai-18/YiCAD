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


/// @file DmBoundingBox.cpp
/// @brief 包围框类实现

#include "DmBoundingBox.h"

DmBoundingBox::DmBoundingBox(const DmVector& pt1, const DmVector& pt2)
{
    add(pt1);
    add(pt2);
}

void DmBoundingBox::add(const DmVector& pt)
{
    if (!min.valid)
    {
        min.valid = true;
    }
    if (!max.valid)
    {
        max.valid = true;
    }
    min = DmVector::minimum(min, pt);
    max = DmVector::maximum(max, pt);
}

bool DmBoundingBox::contains(const DmBoundingBox& box) const
{
    if (min.x > box.min.x + DM_TOLERANCE)
    {
        return false;
    }
    if (min.y > box.min.y + DM_TOLERANCE)
    {
        return false;
    }
    if (max.x < box.max.x - DM_TOLERANCE)
    {
        return false;
    }
    if (max.y < box.max.y - DM_TOLERANCE)
    {
        return false;
    }
    return true;
}

bool DmBoundingBox::isCross(const DmBoundingBox& box) const
{
    auto boxMin = box.getMin();
    auto boxMax = box.getMax();
    if (boxMax.x < min.x - DM_TOLERANCE)
    {
        return false;
    }
    if (boxMax.y < min.y - DM_TOLERANCE)
    {
        return false;
    }
    if (boxMin.x > max.x + DM_TOLERANCE)
    {
        return false;
    }
    if (boxMin.y > max.y + DM_TOLERANCE)
    {
        return false;
    }
    return true;
}

bool DmBoundingBox::isPointInside(const DmVector& pt) const
{
    if (pt.x < min.x - DM_TOLERANCE)
    {
        return false;
    }
    if (pt.y < min.y - DM_TOLERANCE)
    {
        return false;
    }
    if (pt.x > max.x + DM_TOLERANCE)
    {
        return false;
    }
    if (pt.y > max.y + DM_TOLERANCE)
    {
        return false;
    }
    return true;
}

DmVector DmBoundingBox::getMin() const
{
    return min;
}

DmVector DmBoundingBox::getMax() const
{
    return max;
}
