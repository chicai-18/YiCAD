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

/// @file DimAngularData.cpp
/// @brief 角度标注(两线)数据类实现

#include "DimAngularData.h"

DimAngularData::DimAngularData()
    : DimensionData()
    , m_ptxLine1Start(DmVector(0, 0, 0))
    , m_ptxLine1End(DmVector(0, 0, 0))
    , m_ptxLine2Start(DmVector(0, 0, 0))
    , m_ptxLine2End(DmVector(0, 0, 0))
{
    setEntityType(EEntityType::eDimangular);
}

DmVector DimAngularData::getXLine1Start() const
{
    return m_ptxLine1Start;
}

void DimAngularData::setXLine1Start(const DmVector& pt)
{
    m_ptxLine1Start = pt;
}

DmVector DimAngularData::getXLine1End() const
{
    return m_ptxLine1End;
}

void DimAngularData::setXLine1End(const DmVector& pt)
{
    m_ptxLine1End = pt;
}

DmVector DimAngularData::getXLine2Start() const
{
    return m_ptxLine2Start;
}

void DimAngularData::setXLine2Start(const DmVector& pt)
{
    m_ptxLine2Start = pt;
}

DmVector DimAngularData::getXLine2End() const
{
    return m_ptxLine2End;
}

void DimAngularData::setXLine2End(const DmVector& pt)
{
    m_ptxLine2End = pt;
}

DmVector DimAngularData::getArcPoint() const
{
    return m_ptArcPoint;
}

void DimAngularData::setArcPoint(const DmVector& pt)
{
    m_ptArcPoint = pt;
}
