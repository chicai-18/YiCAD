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

/// @file MLeaderData.cpp
/// @brief 多重引线数据类实现

#include "MLeaderData.h"

MLeaderData::MLeaderData()
    : EntityData()
    , m_vecVertexs(std::vector<std::vector<DmVector>>())
    , m_isArrow(true)
{
    setEntityType(EEntityType::eMLeader);
}

std::vector<std::vector<DmVector>> MLeaderData::getVertexs() const
{
    return m_vecVertexs;
}

void MLeaderData::setVertexs(const std::vector<std::vector<DmVector>>& vertexs)
{
    m_vecVertexs = vertexs;
}

bool MLeaderData::getIsArrow() const
{
    return m_isArrow;
}

void MLeaderData::setIsArrow(const bool& isArrow)
{
    m_isArrow = isArrow;
}
