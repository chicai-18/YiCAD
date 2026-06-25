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

/// @file DatabaseInterface.h
/// @brief 数据库接口类，提供插件与主程序之间文件导入导出的抽象方法

#ifndef DATABASEINTERFACE_H
#define DATABASEINTERFACE_H

#include <nlohmann/json.hpp>

class PluginIOInterface;
class QString;

/// @brief 数据库接口，定义插件对文件导入导出操作的标准抽象方法
class Database_Interface
{
public:
    Database_Interface() = default;
    virtual ~Database_Interface() = default;

    /// @brief 新增导入扩展类型
    /// @param type 导入类型名称
    /// @param formatType 对应的格式类型标识
    virtual void addImportType(const QString& type, const QString& formatType) = 0;

    /// @brief 新增导出扩展类型
    /// @param type 导出类型名称
    /// @param formatType 对应的格式类型标识
    virtual void addExportType(const QString& type, const QString& formatType) = 0;

    /// @brief 判断给定文件是否支持导入
    /// @param file 待检查的文件路径
    /// @return 若支持导入返回true，否则返回false
    virtual bool canImport(const QString& file) const = 0;

    /// @brief 判断给定格式类型是否支持导出
    /// @param formatType 格式类型标识
    /// @return 若支持导出返回true，否则返回false
    virtual bool canExport(const QString& formatType) const = 0;

    /// @brief 导入文件
    /// @param json 包含导入配置的JSON对象
    /// @return 导入成功返回true，否则返回false
    virtual bool fileImport(const nlohmann::json& json) = 0;

    /// @brief 导出指定格式的文件
    /// @param json 包含导出配置的JSON对象
    /// @param file 导出目标文件路径
    /// @param formatType 导出格式类型标识
    /// @return 导出成功返回true，否则返回false
    virtual bool fileExport(const nlohmann::json& json, const QString& file, const QString& formatType) = 0;

    /// @brief 设置当前导入导出的文件类型
    /// @param type 文件类型标识
    virtual void setCurrentType(const QString& type) = 0;

    /// @brief 输出命令行信息
    /// @param msg 要输出的消息文本
    virtual void outputCmdMessage(const QString& msg) = 0;

};

#endif