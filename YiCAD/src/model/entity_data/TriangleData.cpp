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

/// @file TriangleData.cpp
/// @brief 三角形数据类实现

#include "TriangleData.h"

TriangleData::TriangleData(const DmVector& p0, const DmVector& p1, const DmVector& p2)
    : m_pts{p0, p1, p2}
{
}

std::array<DmVector, 3> TriangleData::getPoints() const
{
    return m_pts;
}

void TriangleData::setPoints(const std::array<DmVector, 3>& pts)
{
    m_pts = pts;
}

DmVector TriangleData::getPointAt(int i) const
{
    return m_pts[i];
}

void TriangleData::setPointAt(int i, const DmVector& pt)
{
    m_pts[i] = pt;
}
