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

/// @file LeaderData.cpp
/// @brief 引线数据类实现

#include "LeaderData.h"

LeaderData::LeaderData()
    : EntityData()
    , m_vecVertexs(std::vector<DmVector>())
    , m_isArrow(true)
{
    setEntityType(EEntityType::eLeader);
}

std::vector<DmVector> LeaderData::getVertexs() const
{
    return m_vecVertexs;
}

void LeaderData::setVertexs(const std::vector<DmVector>& vertexs)
{
    m_vecVertexs = vertexs;
}

bool LeaderData::getIsArrow() const
{
    return m_isArrow;
}

void LeaderData::setIsArrow(const bool& isArrow)
{
    m_isArrow = isArrow;
}
