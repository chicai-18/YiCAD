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

/// @file PointData.cpp
/// @brief 点数据结构类实现

#include "PointData.h"

/// @brief 默认构造函数
PointData::PointData()
    : EntityData()
    , m_pPosition(0, 0, 0)
{
    setEntityType(EEntityType::ePoint);
}

/// @brief 带位置参数的构造函数
/// @param pos 点坐标
PointData::PointData(const DmVector& pos)
    : EntityData()
    , m_pPosition(pos)
{
    // TODO:  - 此构造函数未设置实体类型，应添加 setEntityType(EEntityType::ePoint)
}

DmVector PointData::getPosition() const
{
    return m_pPosition;
}

void PointData::setPosition(const DmVector& pt)
{
    m_pPosition = pt;
}
