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

/// @file XLineData.cpp
/// @brief 构造线(双向射线)数据类实现

#include "XLineData.h"

XLineData::XLineData()
    : EntityData()
    , m_ptBasePoint(DmVector(0, 0, 0))
    , m_ptDirection(DmVector(0, 0, 0))
{
    setEntityType(EEntityType::eXLine);
}

XLineData::XLineData(const DmVector& startPt, const DmVector& dir)
    : EntityData()
    , m_ptBasePoint(startPt)
    , m_ptDirection(dir)
{
    setEntityType(EEntityType::eXLine);
}

DmVector XLineData::getBasePoint() const
{
    return m_ptBasePoint;
}

void XLineData::setBasePoint(const DmVector& pt)
{
    m_ptBasePoint = pt;
}

DmVector XLineData::getDirection() const
{
    return m_ptDirection;
}

void XLineData::setDirection(const DmVector& pt)
{
    m_ptDirection = pt;
}
