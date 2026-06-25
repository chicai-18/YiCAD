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

/// @file RayData.cpp
/// @brief 射线数据结构类实现

#include "RayData.h"

/// @brief 默认构造函数
RayData::RayData()
    : EntityData()
    , m_ptBasePoint(DmVector(0, 0, 0))
    , m_ptDirection(DmVector(0, 0, 0))
{
    setEntityType(EEntityType::eRay);
}

/// @brief 带起始终止点的构造函数
/// @param startPt 起点坐标
/// @param dir 方向向量
RayData::RayData(const DmVector& startPt, const DmVector& dir)
    : EntityData()
    , m_ptBasePoint(startPt)
    , m_ptDirection(dir)
{
    setEntityType(EEntityType::eRay);
}

DmVector RayData::getBasePoint() const
{
    return m_ptBasePoint;
}

void RayData::setBasePoint(const DmVector& pt)
{
    m_ptBasePoint = pt;
}

DmVector RayData::getDirection() const
{
    return m_ptDirection;
}

void RayData::setDirection(const DmVector& pt)
{
    m_ptDirection = pt;
}
