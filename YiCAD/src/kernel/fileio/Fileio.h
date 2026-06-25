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

/// @file Fileio.h
/// @brief 文件导入导出接口类头文件

#ifndef FILEIO_H
#define FILEIO_H

#include <vector>
#include <functional>
#include <memory>
#include "FilterInterface.h"

// TODO destructor for clear filterList
/// @class FileIO
/// @brief 文件导入导出的接口类
class FileIO
{
private:
    // singleton
    FileIO() = default;
    FileIO(FileIO const&) = delete;
    FileIO& operator = (FileIO const&) = delete;
    FileIO(FileIO&&) = delete;
    FileIO& operator = (FileIO&&) = delete;

public:
    /// @brief 实例化文件读写(单例)
    static FileIO* instance();

    /// @brief 获取导入的操作类指针
    std::unique_ptr<FilterInterface> getImportFilter(const QString& file) const;

    /// @brief 获取导出的操作类指针
    std::unique_ptr<FilterInterface> getExportFilter(const QString& formatType) const;

    /// @brief 文件导入
    bool fileImport(DmDocument& document, const QString& file);

    /// @brief 文件导出
    bool fileExport(DmDocument& document, const QString& file, const QString& formatType);

private:
    /// @brief 获取指向创建文件转换器的静态函数的指针列表
    static std::vector<std::function<FilterInterface* ()>> getFilters();
};

#endif