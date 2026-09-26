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

/// @file AttributeData.cpp
/// @brief 属性数据类实现

#include "AttributeData.h"

AttributeData::AttributeData()
    : m_strTag("")
{
    setEntityType(EEntityType::eAttribute);
}

AttributeData::AttributeData(const QString& tag)
    : m_strTag(tag)
{
    setEntityType(EEntityType::eAttribute);
}

QString AttributeData::getTag() const
{
    return m_strTag;
}

void AttributeData::setTag(const QString& tag)
{
    m_strTag = tag;
}
