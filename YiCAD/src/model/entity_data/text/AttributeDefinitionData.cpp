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

/// @file AttributeDefinitionData.cpp
/// @brief 属性定义数据类实现

#include "AttributeDefinitionData.h"

AttributeDefinitionData::AttributeDefinitionData()
    : m_strTag("")
    , m_strPrompt("")
{
    setEntityType(EEntityType::eAttributeDefinition);
}

AttributeDefinitionData::AttributeDefinitionData(const QString& tag, const QString& prompt)
    : m_strTag(tag)
    , m_strPrompt(prompt)
{
    setEntityType(EEntityType::eAttributeDefinition);
}

QString AttributeDefinitionData::getTag() const
{
    return m_strTag;
}

void AttributeDefinitionData::setTag(const QString& tag)
{
    m_strTag = tag;
}

QString AttributeDefinitionData::getPrompt() const
{
    return m_strPrompt;
}

void AttributeDefinitionData::setPrompt(const QString& prompt)
{
    m_strPrompt = prompt;
}
