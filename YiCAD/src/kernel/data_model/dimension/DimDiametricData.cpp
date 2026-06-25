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

/// @file DimDiametricData.cpp
/// @brief 直径标注数据类实现

#include "DimDiametricData.h"

DimDiametricData::DimDiametricData()
    : DimensionData()
    , m_ptDiameter1Point(0, 0, 0)
    , m_ptDiameter2Point(0, 0, 0)
    , m_dLeaderLength(0)
{
    setEntityType(EEntityType::eDimdiametric);
}

DmVector DimDiametricData::getDiameter1Point() const
{
    return m_ptDiameter1Point;
}

void DimDiametricData::setDiameter1Point(const DmVector& pt)
{
    m_ptDiameter1Point = pt;
}

DmVector DimDiametricData::getDiameter2Point() const
{
    return m_ptDiameter2Point;
}

void DimDiametricData::setDiameter2Point(const DmVector& pt)
{
    m_ptDiameter2Point = pt;
}

double DimDiametricData::getLeaderLength() const
{
    return m_dLeaderLength;
}

void DimDiametricData::setLeaderLength(const double dLeaderLength)
{
    m_dLeaderLength = dLeaderLength;
}
