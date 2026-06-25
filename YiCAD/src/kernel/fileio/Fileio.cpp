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

/// @file Fileio.cpp
/// @brief 文件导入导出接口类实现

#include "Fileio.h"

#include <cstddef>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <QApplication>

#include "FilterOcdIO.h"
#include "filter/PluginIOInterface.h"
#include "Debug.h"

bool FileIO::fileImport(DmDocument& document, const QString& file)
{
    std::unique_ptr<FilterInterface>&& filter(getImportFilter(file));
    if (filter)
    {
        bool bImported = { filter->fileImport(document, file) };
        return bImported;
    }
    return false;
}

bool FileIO::fileExport(DmDocument& document, const QString& file, const QString& formatType)
{
    std::unique_ptr<FilterInterface>&& filter(getExportFilter(formatType));
    if (filter)
    {
        return filter->fileExport(document, file, formatType);
    }
    return false;
}

FileIO* FileIO::instance()
{
    static FileIO* uniqueInstance = nullptr;
    if (!uniqueInstance)
    {
        uniqueInstance = new FileIO();
    }
    return uniqueInstance;
}

std::unique_ptr<FilterInterface> FileIO::getImportFilter(const QString& file) const
{
    for (auto& f : getFilters())
    {
        std::unique_ptr<FilterInterface> filter(f());
        if (filter && filter->canImport(file))
        {
            return filter;
        }
    }
    return nullptr;
}

std::unique_ptr<FilterInterface> FileIO::getExportFilter(const QString& formatType) const
{
    for (auto& f : getFilters())
    {
        std::unique_ptr<FilterInterface> filter(f());
        if (filter && filter->canExport(formatType))
        {
            return filter;
        }
    }

    QMessageBox::critical(nullptr, QObject::tr("ToolTips"), QObject::tr("Unsupported file format, please use another format to export!"), QMessageBox::Cancel); // 暂不支持的文件格式，请使用其他格式导出
    return nullptr;
}

std::vector<std::function<FilterInterface* ()>> FileIO::getFilters()
{
    return
    {
        PluginIOInterface::createFilter,
        FilterOcdIO::createFilter
    };
}
