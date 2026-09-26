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

/// @file FilterRegistry.cpp
/// @brief 文件格式注册表实现

#include "FilterRegistry.h"

#include <algorithm>
#include <utility>

#include "FilterInterface.h"

FilterRegistry& FilterRegistry::instance()
{
    static FilterRegistry registry;
    return registry;
}

int FilterRegistry::addImport(const QString& nameFilter, Factory factory)
{
    const int id = m_nextId++;
    m_entries.push_back({id, true, QString(), nameFilter, std::move(factory)});
    return id;
}

int FilterRegistry::addExport(const QString& formatType, const QString& nameFilter, Factory factory)
{
    const int id = m_nextId++;
    m_entries.push_back({id, false, formatType, nameFilter, std::move(factory)});
    return id;
}

void FilterRegistry::remove(int id)
{
    m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
                                   [id](const Entry& entry) { return entry.id == id; }),
                    m_entries.end());
}

std::unique_ptr<FilterInterface> FilterRegistry::importFilter(const QString& file) const
{
    for (const Entry& entry : m_entries)
    {
        if (!entry.isImport)
        {
            continue;
        }
        std::unique_ptr<FilterInterface> filter = entry.factory();
        if (filter && filter->canImport(file))
        {
            return filter;
        }
    }
    return nullptr;
}

std::unique_ptr<FilterInterface> FilterRegistry::exportFilter(const QString& formatType) const
{
    for (const Entry& entry : m_entries)
    {
        if (entry.isImport)
        {
            continue;
        }
        std::unique_ptr<FilterInterface> filter = entry.factory();
        if (filter && filter->canExport(formatType))
        {
            return filter;
        }
    }
    return nullptr;
}

QStringList FilterRegistry::importNameFilters() const
{
    QStringList filters;
    for (const Entry& entry : m_entries)
    {
        if (entry.isImport)
        {
            filters.append(entry.nameFilter);
        }
    }
    return filters;
}

QStringList FilterRegistry::exportNameFilters() const
{
    QStringList filters;
    for (const Entry& entry : m_entries)
    {
        if (!entry.isImport)
        {
            filters.append(entry.nameFilter);
        }
    }
    return filters;
}

QString FilterRegistry::exportFormatType(const QString& nameFilter) const
{
    for (const Entry& entry : m_entries)
    {
        if (!entry.isImport && entry.nameFilter == nameFilter)
        {
            return entry.formatType;
        }
    }
    return nameFilter;
}
