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


/// @file DmIdManager.cpp
/// @brief 实体ID管理器实现，使用boost::uuid生成唯一ID

#include "DmIdManager.h"

#include "boost/uuid/uuid.hpp"
#include "boost/uuid/uuid_generators.hpp"
#include "boost/uuid/uuid_io.hpp"

DmIdManager::DmIdManager()
    : m_entitiesMap(ENTITIESIDMAP())
{
}

DmId DmIdManager::assignID(DmObject* pObj)
{
    DmId id;
    do
    {
        boost::uuids::uuid uid = boost::uuids::random_generator()();
        std::string uidStr = boost::uuids::to_string(uid);
        id = DmId(uidStr);
    }
    while (isExistGUID(id));

    m_entitiesMap[id] = pObj;
    pObj->setId(id);
    return id;
}

bool DmIdManager::isExistGUID(const DmId& id) const
{
    if (m_entitiesMap.find(id) != m_entitiesMap.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}

void DmIdManager::removeID(const DmId& id)
{
    auto it = m_entitiesMap.find(id);
    if (it != m_entitiesMap.end())
    {
        m_entitiesMap.erase(it);
    }
}

DmObject* DmIdManager::getEntity(const DmId& id)
{
    auto it = m_entitiesMap.find(id);
    if (it != m_entitiesMap.end())
    {
        return m_entitiesMap[id];
    }

    return nullptr;
}

void DmIdManager::clear()
{
    m_entitiesMap.clear();
}

void DmIdManager::assignID(DmObject* pObj, const DmId& id)
{
    m_entitiesMap[id] = pObj;
    pObj->setId(id);
}
