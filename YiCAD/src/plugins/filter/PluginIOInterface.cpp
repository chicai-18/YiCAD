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

/// @file PluginIOInterface.cpp
/// @brief 插件IO接口实现，提供文件导入导出格式检测与过滤器创建

#include "PluginIOInterface.h"

#include <QFileInfo>

PluginIOInterface::PluginIOInterface()
    : m_pImportFormatTypes(new std::list<QString>())
    , m_pExportFormatTypes(new std::list<QString>())
{
}

PluginIOInterface::~PluginIOInterface()
{
    // TODO: 当前未释放 m_pImportFormatTypes 和 m_pExportFormatTypes，
    // 由于外部可能通过 setImportFormatTypes/setExportFormatTypes 传入指针，
    // 需理清内存所有权后再决定是否在此析构
}

bool PluginIOInterface::canImport(const QString& file) const
{
    for (auto i = m_pImportFormatTypes->begin(); i != m_pImportFormatTypes->end(); ++i)
    {
        // TODO: 当前在循环体内遇到不匹配即返回false，仅检查了第一个格式类型，
        // 可能需改为遍历完全部类型后再决定返回值
        auto split = i->split("*.");
        auto suffix = split.at(split.size() - 1);
        QFileInfo fileInfo = QFileInfo(file);
        auto duffix = fileInfo.suffix();
        if (duffix == suffix)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    return false;
}

bool PluginIOInterface::canExport(const QString& formatType) const
{
    for (auto i = m_pExportFormatTypes->begin(); i != m_pExportFormatTypes->end(); ++i)
    {
        if ((*i) == formatType)
        {
            return true;
        }
        else
        {
            continue;
        }
    }

    return false;
}

bool PluginIOInterface::fileImport(DmDocument& document, const QString& file)
{
    // TODO: 当前为桩实现，仅返回true，需要完成实际的导入逻辑
    return true;
}

bool PluginIOInterface::fileExport(DmDocument& document, const QString& file, const QString& formatType)
{
    // TODO: 当前为桩实现，仅返回true，需要完成实际的导出逻辑
    return true;
}

void PluginIOInterface::setImportFormatTypes(std::list<QString>* pImportFormatTypes)
{
    m_pImportFormatTypes = pImportFormatTypes;
}

void PluginIOInterface::setExportFormatTypes(std::list<QString>* pExportFormatTypes)
{
    m_pExportFormatTypes = pExportFormatTypes;
}

FilterInterface* PluginIOInterface::createFilter()
{
    return new PluginIOInterface();
}
