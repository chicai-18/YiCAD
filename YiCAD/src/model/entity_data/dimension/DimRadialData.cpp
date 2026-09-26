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

/// @file DimRadialData.cpp
/// @brief 半径标注数据类实现

#include "DimRadialData.h"

DimRadialData::DimRadialData()
    : DimensionData()
    , m_ptCenterPoint(0, 0, 0)
    , m_ptDiameterPoint(0, 0, 0)
    , m_dLeaderLength(0)
{
    setEntityType(EEntityType::eDimradial);
}

DmVector DimRadialData::getCenterPoint() const
{
    return m_ptCenterPoint;
}

void DimRadialData::setCenterPoint(const DmVector& pt)
{
    m_ptCenterPoint = pt;
}

DmVector DimRadialData::getDiameterPoint() const
{
    return m_ptDiameterPoint;
}

void DimRadialData::setDiameterPoint(const DmVector& pt)
{
    m_ptDiameterPoint = pt;
}

double DimRadialData::getLeaderLength() const
{
    return m_dLeaderLength;
}

void DimRadialData::setLeaderLength(const double length)
{
    m_dLeaderLength = length;
}
