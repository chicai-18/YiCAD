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

/// @file DimAlignedData.cpp
/// @brief 对齐标注数据类实现

#include "DimAlignedData.h"

DimAlignedData::DimAlignedData()
    : DimensionData()
    , m_dAngle(0)
    , m_ptXLine1Point(DmVector(0, 0, 0))
    , m_ptXLine2Point(DmVector(0, 0, 0))
    , m_ptMidLinePoint(DmVector(0, 0, 0))
    , m_dOblique(0)
{
    setEntityType(EEntityType::eDimaligned);
}

double DimAlignedData::getAngle() const
{
    return m_dAngle;
}

void DimAlignedData::setAngle(const double& dAngle)
{
    m_dAngle = dAngle;
}

DmVector DimAlignedData::getXLine1Point() const
{
    return m_ptXLine1Point;
}

void DimAlignedData::setXLine1Point(const DmVector& pt)
{
    m_ptXLine1Point = pt;
}

DmVector DimAlignedData::getXLine2Point() const
{
    return m_ptXLine2Point;
}

void DimAlignedData::setXLine2Point(const DmVector& pt)
{
    m_ptXLine2Point = pt;
}

DmVector DimAlignedData::getMidLinePoint() const
{
    return m_ptMidLinePoint;
}

void DimAlignedData::setMidLinePoint(const DmVector& pt)
{
    m_ptMidLinePoint = pt;
}

double DimAlignedData::getOblique() const
{
    return m_dOblique;
}

void DimAlignedData::setOblique(const double& dOblique)
{
    m_dOblique = dOblique;
}
