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

/// @file PluginIOInterface.h
/// @brief 插件IO接口类，扩展FilterInterface以支持导入导出格式类型注册

#ifndef PLUGINIOINTERFACE_H
#define PLUGINIOINTERFACE_H

#include "FilterInterface.h"

class QString;

/// @brief 插件IO接口，在过滤器基础上增加了格式类型管理和工厂创建方法
class PluginIOInterface : public FilterInterface
{
public:
    PluginIOInterface();
    ~PluginIOInterface();

    /// @brief 判断给定文件是否支持导入
    /// @param file 待检查的文件路径
    /// @return 若支持导入返回true，否则返回false
    virtual bool canImport(const QString& file) const override;

    /// @brief 判断给定格式类型是否支持导出
    /// @param formatType 格式类型标识
    /// @return 若支持导出返回true，否则返回false
    virtual bool canExport(const QString& formatType) const override;

    /// @brief 导入文件并存储实体到当前实体容器中
    /// @param document 目标文档对象
    /// @param file 待导入的文件路径
    /// @return 导入成功返回true，否则返回false
    virtual bool fileImport(DmDocument& document, const QString& file) override;

    /// @brief 导出指定格式的文件
    /// @param document 源文档对象
    /// @param file 导出目标文件路径
    /// @param formatType 导出格式类型标识
    /// @return 导出成功返回true，否则返回false
    virtual bool fileExport(DmDocument& document, const QString& file, const QString& formatType) override;

    /// @brief 设置支持导入的格式类型列表
    /// @param pImportFormatTypes 导入格式类型列表指针
    void setImportFormatTypes(std::list<QString>* pImportFormatTypes);

    /// @brief 设置支持导出的格式类型列表
    /// @param pExportFormatTypes 导出格式类型列表指针
    void setExportFormatTypes(std::list<QString>* pExportFormatTypes);

    /// @brief 创建过滤器实例（工厂方法）
    /// @return 新创建的PluginIOInterface实例指针
    static FilterInterface* createFilter();

private:
    std::list<QString>* m_pImportFormatTypes = nullptr;   ///< 支持导入的格式类型列表
    std::list<QString>* m_pExportFormatTypes = nullptr;   ///< 支持导出的格式类型列表

};

#endif