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

/// @file DmCustomEntityRegistry.cpp
/// @brief 自定义实体类注册表的实现

#include "DmCustomEntityRegistry.h"

DmCustomEntityRegistry& DmCustomEntityRegistry::instance()
{
    static DmCustomEntityRegistry registry;
    return registry;
}

bool DmCustomEntityRegistry::registerClass(const DmCustomEntityClass& entityClass)
{
    if (entityClass.name.isEmpty() || m_classes.count(entityClass.name) != 0)
    {
        return false;
    }
    if (!entityClass.factory)
    {
        if (entityClass.type.isBad() || !entityClass.type.isDerivedFrom(DmCustomEntity::getClassTypeId()))
        {
            return false;
        }
        // 抽象类建不出实例（TYPESYSTEM_SOURCE_ABSTRACT 的 create 返回空），登记了读盘也用不上
        std::unique_ptr<DmCustomEntity> probe(static_cast<DmCustomEntity*>(Type(entityClass.type).createInstance()));
        if (!probe)
        {
            return false;
        }
    }
    m_classes.emplace(entityClass.name, entityClass);
    return true;
}

bool DmCustomEntityRegistry::unregisterClass(const QString& name)
{
    return m_classes.erase(name) != 0;
}

const DmCustomEntityClass* DmCustomEntityRegistry::find(const QString& name) const
{
    auto it = m_classes.find(name);
    return it == m_classes.end() ? nullptr : &it->second;
}

DmCustomEntity* DmCustomEntityRegistry::create(const QString& name) const
{
    const DmCustomEntityClass* entityClass = find(name);
    if (!entityClass)
    {
        return nullptr;
    }
    if (entityClass->factory)
    {
        return entityClass->factory();
    }
    return static_cast<DmCustomEntity*>(Type(entityClass->type).createInstance());
}

std::vector<QString> DmCustomEntityRegistry::classNames() const
{
    std::vector<QString> names;
    names.reserve(m_classes.size());
    for (const auto& [name, entityClass] : m_classes)
    {
        names.push_back(name);
    }
    return names;
}
