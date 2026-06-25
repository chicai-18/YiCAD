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

/// @file EntityData.cpp
/// @brief 实体数据基类实现

#include "EntityData.h"

/// @brief 默认构造函数，初始化为未知类型
EntityData::EntityData()
    : m_eType(EEntityType::eUnknown)
{
}

EEntityType EntityData::getEntityType() const
{
    return m_eType;
}

void EntityData::setEntityType(const EEntityType& entType)
{
    m_eType = entType;
}
