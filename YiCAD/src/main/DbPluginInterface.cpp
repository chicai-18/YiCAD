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

/// @file DbPluginInterface.cpp
/// @brief 插件数据库接口类实现，管理导入导出格式类型注册与文件I/O调度

#include "DbPluginInterface.h"

#include "DmSystem.h"
#include "Fileio.h"
#include "filter/PluginIOInterface.h"
#include "GuiDialogFactory.h"

Database_plugin_interface::Database_plugin_interface()
    : m_pImportFormatTypes(new std::list<QString>())
    , m_pExportFormatTypes(new std::list<QString>())
    , m_pluginIoInterface(new PluginIOInterface())
{
}

void Database_plugin_interface::addImportType(const QString& type, const QString& formatType)
{
    m_pImportFormatTypes->emplace_back(formatType);
    DMSYSTEM->addImportFormatType(type, formatType);
}

void Database_plugin_interface::addExportType(const QString& type, const QString& formatType)
{
    m_pExportFormatTypes->emplace_back(formatType);
    DMSYSTEM->addExportFormatType(type, formatType);
}

bool Database_plugin_interface::canImport(const QString& file) const
{
    return m_pluginIoInterface->canImport(file);
}

bool Database_plugin_interface::canExport(const QString& formatType) const
{
    return m_pluginIoInterface->canExport(formatType);
}

bool Database_plugin_interface::fileImport(const nlohmann::json& json)
{
    return false;
}

bool Database_plugin_interface::fileExport(const nlohmann::json& json, const QString& file, const QString& formatType)
{
    return false;
}

void Database_plugin_interface::setCurrentType(const QString& type)
{
    DMSYSTEM->setCurrentFormatType(type);
}

void Database_plugin_interface::outputCmdMessage(const QString& msg)
{
    GUIDIALOGFACTORY->commandMessage(msg);
}
