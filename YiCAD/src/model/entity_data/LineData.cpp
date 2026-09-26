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

/// @file LineData.cpp
/// @brief 直线数据结构类实现

#include "LineData.h"

/// @brief 默认构造函数
LineData::LineData()
    : EntityData()
    , m_startPoint(DmVector(0, 0, 0))
    , m_endPoint(DmVector(0, 0, 0))
{
    setEntityType(EEntityType::eLine);
}

/// @brief 带起始终止点的构造函数
/// @param startPt 起点坐标
/// @param endPt 终点坐标
LineData::LineData(const DmVector& startPt, const DmVector& endPt)
    : EntityData()
    , m_startPoint(startPt)
    , m_endPoint(endPt)
{
    setEntityType(EEntityType::eLine);
}

DmVector LineData::getStartPoint() const
{
    return m_startPoint;
}

void LineData::setStartPoint(const DmVector& pt)
{
    m_startPoint = pt;
}

DmVector LineData::getEndPoint() const
{
    return m_endPoint;
}

void LineData::setEndPoint(const DmVector& pt)
{
    m_endPoint = pt;
}

const std::vector<float>& LineData::getVerticesRef() const
{
    return m_vertices;
}

void LineData::setVertices(const std::vector<float>& vs)
{
    m_vertices = vs;
}
