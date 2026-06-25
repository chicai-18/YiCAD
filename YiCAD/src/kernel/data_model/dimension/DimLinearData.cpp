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

/// @file DimLinearData.cpp
/// @brief 线性标注数据类实现

#include "DimLinearData.h"

DimLinearData::DimLinearData()
    : DimensionData()
    , m_dAngle(0)
    , m_ptXLine1Point(DmVector(0, 0, 0))
    , m_ptXLine2Point(DmVector(0, 0, 0))
    , m_ptMidLinePoint(DmVector(0, 0, 0))
    , m_dOblique(0)
{
    setEntityType(EEntityType::eDimlinear);
}

double DimLinearData::getAngle() const
{
    return m_dAngle;
}

void DimLinearData::setAngle(const double& dAngle)
{
    m_dAngle = dAngle;
}

DmVector DimLinearData::getXLine1Point() const
{
    return m_ptXLine1Point;
}

void DimLinearData::setXLine1Point(const DmVector& pt)
{
    m_ptXLine1Point = pt;
}

DmVector DimLinearData::getXLine2Point() const
{
    return m_ptXLine2Point;
}

void DimLinearData::setXLine2Point(const DmVector& pt)
{
    m_ptXLine2Point = pt;
}

DmVector DimLinearData::getMidLinePoint() const
{
    return m_ptMidLinePoint;
}

void DimLinearData::setMidLinePoint(const DmVector& pt)
{
    m_ptMidLinePoint = pt;
}

double DimLinearData::getOblique() const
{
    return m_dOblique;
}

void DimLinearData::setOblique(const double& dOblique)
{
    m_dOblique = dOblique;
}
