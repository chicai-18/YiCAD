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

/// @file SolidData.cpp
/// @brief 二维填充实体数据类实现

#include "SolidData.h"

SolidData::SolidData()
    : EntityData()
{
    setEntityType(EEntityType::eSolid);
}

SolidData::SolidData(const std::vector<DmVector>& corners)
    : EntityData()
    , m_corners(corners)
{
    setEntityType(EEntityType::eSolid);
}

std::vector<DmVector> SolidData::getCorners() const
{
    return m_corners;
}

void SolidData::setCorners(const std::vector<DmVector>& corners)
{
    m_corners = corners;
}

DmVector SolidData::getCornerAt(const int& index) const
{
    if (index < m_corners.size())
    {
        return m_corners.at(index);
    }
    else
    {
        return DmVector(false);
    }
}

void SolidData::setCornerAt(const int& index, const DmVector& corner)
{
    if (index < m_corners.size())
    {
        m_corners.at(index) = corner;
    }
}

int SolidData::getCornerSize() const
{
    return static_cast<int>(m_corners.size());
}
