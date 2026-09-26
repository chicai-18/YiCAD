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

/// @file LineStripData.cpp
/// @brief 线段串数据结构类实现

#include "LineStripData.h"

/// @brief 默认构造函数
LineStripData::LineStripData()
    : m_isClosed(false)
{
    setEntityType(EEntityType::eLineStrip);
}

/// @brief 带顶点集合的构造函数
/// @param pts 顶点坐标列表
/// @param isClosed 是否闭合
LineStripData::LineStripData(const std::vector<DmVector>& pts, bool isClosed)
{
    m_points = pts;
    m_isClosed = isClosed;
    setEntityType(EEntityType::eLineStrip);
}

std::vector<DmVector> LineStripData::getPoints() const
{
    return m_points;
}

void LineStripData::setPoints(const std::vector<DmVector>& pts)
{
    m_points = pts;
}

void LineStripData::appendPoint(const DmVector& pt)
{
    m_points.emplace_back(pt);
}

int LineStripData::getPointCount() const
{
    return (int)m_points.size();
}

DmVector LineStripData::getPointAt(int i) const
{
    return m_points.at(i);
}

void LineStripData::setPointAt(int i, const DmVector& pt)
{
    m_points.at(i) = pt;
}

void LineStripData::clear()
{
    m_points.clear();
}

bool LineStripData::isClosed() const
{
    return m_isClosed;
}

void LineStripData::setIsClosed(bool isClosed)
{
    m_isClosed = isClosed;
}

const std::vector<float>& LineStripData::getVerticesRef() const
{
    return m_vertices;
}

void LineStripData::setVertices(const std::vector<float>& vs)
{
    m_vertices = vs;
}
