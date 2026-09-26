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

/// @file RegionData.cpp
/// @brief 面域数据类实现

#include "RegionData.h"

#include <utility>

RegionData::RegionData()
{
    m_boundary = std::make_shared<DmEntityContainer>(nullptr);
    setEntityType(EEntityType::eRegion);
}

RegionData::RegionData(DmEntityContainerPtr boundary, const std::vector<DmEntityContainerPtr>& holes)
    : m_boundary(boundary)
    , m_holes(holes)
{
    setEntityType(EEntityType::eRegion);
}

DmEntityContainerPtr RegionData::getBoundary() const
{
    return m_boundary;
}

void RegionData::setBoundary(DmEntityContainerPtr b)
{
    m_boundary = b;
}

std::vector<DmEntityContainerPtr> RegionData::getHoles() const
{
    return m_holes;
}

void RegionData::setHoles(const std::vector<DmEntityContainerPtr>& holes)
{
    m_holes = holes;
}