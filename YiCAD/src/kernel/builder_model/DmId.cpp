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

/// @file DmId.cpp
/// @brief 实体唯一标识符类实现

#include "DmId.h"

DmId::DmId()
    : m_idStr("0")
{
}

DmId::DmId(const DmId& id)
{
    m_idStr = id.m_idStr;
}

DmId::DmId(const std::string& idStr)
{
    m_idStr = idStr;
}

bool DmId::isValid() const
{
    return m_idStr != "0";
}

std::string DmId::asString() const
{
    return m_idStr;
}

DmId& DmId::operator=(const DmId& id)
{
    if (this != &id) // 避免自我赋值
    {
        m_idStr = id.m_idStr;
    }
    return *this;
}

bool DmId::operator==(const DmId& id) const
{
    return id.m_idStr == m_idStr;
}

bool DmId::operator!=(const DmId& id) const
{
    return !operator==(id);
}

bool DmId::operator<(const DmId& id) const
{
    return m_idStr < id.m_idStr;
}

std::ostream& operator<<(std::ostream& os, const DmId& id)
{
    os << id.asString();
    return os;
}
