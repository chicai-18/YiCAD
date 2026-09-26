/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


/// @file DmCharTemplateList.cpp
/// @brief 文字模板列表类实现

#include "DmCharTemplateList.h"
#include "DmCharTemplate.h"

DmCharTemplateList::~DmCharTemplateList()
{
    // 释放文字模板
    for (auto item : m_fontChars)
    {
        delete item.second;
        item.second = nullptr;
    }
}

void DmCharTemplateList::add(const QString& name, DmCharTemplate* letter)
{
    m_fontChars[name] = letter;
    letter->setOwner(this);
}

DmCharTemplate* DmCharTemplateList::find(const QString& name) const
{
    auto it = m_fontChars.find(name);
    if (it == m_fontChars.end())
    {
        return nullptr;
    }
    return it->second;
}

DmFont* DmCharTemplateList::getFont() const
{
    return m_pFont;
}

void DmCharTemplateList::setFont(DmFont* f)
{
    m_pFont = f;
}
