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
#include <QStringList>
#include "FilterInterface.h"

class HostApi;
class PluginManager;
class PluginRegistry;

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

    /// @brief 接入应用持有的新原生插件运行时，不取得所有权。
    void setPluginRuntime(
        PluginRegistry& registry,
        PluginManager& manager,
        HostApi& hostApi) noexcept;

    /// @brief 在插件关闭前断开运行时，避免保留 DLL 回调地址。
    void clearPluginRuntime() noexcept;

    /// @brief 返回当前活动插件声明的导入文件对话框过滤项。
    QStringList pluginImportNameFilters() const;

    /// @brief 返回当前活动插件声明的导出文件对话框过滤项。
    QStringList pluginExportNameFilters() const;

    /// @brief 将导出对话框显示项转换为插件规范格式名。
    QString exportFormatType(const QString& nameFilter) const;

private:
    /// @brief 获取指向创建文件转换器的静态函数的指针列表
    static std::vector<std::function<FilterInterface* ()>> getFilters();

    PluginRegistry* m_pluginRegistry = nullptr;
    PluginManager* m_pluginManager = nullptr;
    HostApi* m_pluginHostApi = nullptr;
};

#endif
