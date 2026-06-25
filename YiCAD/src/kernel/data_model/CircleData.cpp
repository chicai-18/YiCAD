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

/// @file CircleData.cpp
/// @brief 圆数据结构类实现

#include "CircleData.h"
#include "Tools.h"

/// @brief 默认构造函数
CircleData::CircleData()
    : EntityData()
    , m_ptCenter(DmVector(0, 0, 0))
    , m_dRadius(1.)
{
    setEntityType(EEntityType::eCircle);
}

/// @brief 带参数构造函数
/// @param center 圆心坐标
/// @param radius 半径
CircleData::CircleData(DmVector const& center, double radius)
    : EntityData()
    , m_ptCenter(center)
    , m_dRadius(radius)
{
    setEntityType(EEntityType::eCircle);
}

/// @brief 判断圆数据是否有效
/// @return 若圆心有效且半径大于容差则返回true
bool CircleData::isValid() const
{
    return (m_ptCenter.valid && m_dRadius > DM_TOLERANCE);
}

/// @brief 比较运算符
/// @param rhs 右侧比较对象
/// @return 若圆心和半径相等则返回true
bool CircleData::operator==(CircleData const& rhs) const
{
    if (!(getCenter().valid && rhs.getCenter().valid))
    {
        return false;
    }
    if (getCenter().squaredTo(rhs.getCenter()) > DM_TOLERANCE2)
    {
        return false;
    }
    return std::fabs(getRadius() - rhs.getRadius()) < DM_TOLERANCE;
}

DmVector CircleData::getCenter() const
{
    return m_ptCenter;
}

void CircleData::setCenter(const DmVector& ptCenter)
{
    m_ptCenter = ptCenter;
}

double CircleData::getRadius() const
{
    return m_dRadius;
}

void CircleData::setRadius(const double& dRadius)
{
    m_dRadius = dRadius;
}

const std::vector<float>& CircleData::getVerticesRef() const
{
    return m_vertices;
}

void CircleData::setVertices(const std::vector<float>& vs)
{
    m_vertices = vs;
}
