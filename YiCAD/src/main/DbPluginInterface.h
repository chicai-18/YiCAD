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

/// @file DbPluginInterface.h
/// @brief 插件数据库接口类，封装导入导出格式类型的注册与文件I/O操作

#ifndef DBPLUGININTERFACE_H
#define DBPLUGININTERFACE_H

#include <list>

#include "DatabaseInterface.h"

class QString;

/// @brief 插件数据接口，提供格式注册、文件导入导出、命令行消息输出等功能
class Database_plugin_interface : public Database_Interface
{
public:
    Database_plugin_interface();

    /// @brief 新增导入扩展类型
    /// @param [in] type 类型标识字符串
    /// @param [in] formatType 格式类型字符串
    virtual void addImportType(const QString& type, const QString& formatType) override;

    /// @brief 新增导出扩展类型
    /// @param [in] type 类型标识字符串
    /// @param [in] formatType 格式类型字符串
    virtual void addExportType(const QString& type, const QString& formatType) override;

    /// @brief 判断类型是否支持导入
    /// @param [in] file 文件路径
    /// @return true 如果支持导入该文件
    virtual bool canImport(const QString& file) const override;

    /// @brief 判断类型是否支持导出
    /// @param [in] formatType 格式类型字符串
    /// @return true 如果支持导出该格式
    virtual bool canExport(const QString& formatType) const override;

    /// @brief 导入文件并存储实体到当前实体容器中
    /// @param [in] json 包含导入数据的JSON对象
    /// @return true 如果导入成功
    virtual bool fileImport(const nlohmann::json& json) override;

    /// @brief 导出指定格式的文件
    /// @param [in] json 包含实体数据的JSON对象
    /// @param [in] file 目标文件路径
    /// @param [in] formatType 导出格式类型
    /// @return true 如果导出成功
    virtual bool fileExport(const nlohmann::json& json, const QString& file, const QString& formatType) override;

    /// @brief 设置当前导入导出文件类型
    /// @param [in] type 文件类型字符串
    virtual void setCurrentType(const QString& type) override;

    /// @brief 输出命令行信息
    /// @param [in] msg 要输出的消息字符串
    virtual void outputCmdMessage(const QString& msg) override;

private:
    std::list<QString>* m_pImportFormatTypes = nullptr; ///< 导入格式类型列表
    std::list<QString>* m_pExportFormatTypes = nullptr; ///< 导出格式类型列表
    PluginIOInterface*  m_pluginIoInterface = nullptr;  ///< 插件I/O接口实例
};

#endif
