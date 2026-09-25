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

/// @file ExtensionNamespace.h
/// @brief 扩展命名空间规则：扩展注册的命令、设置页、Ribbon 条目的 ID 必须以
/// "<扩展 ID>." 开头，如扩展 "ext.dim" 的命令 "ext.dim.linear"。
///
/// 前缀后必须紧跟 '.'，这样 "ext.ai" 不会误认领 "ext.aim.x"。命令 ID 由
/// ExtensionManager 校验，Ribbon 条目由 UIRibbonScopedRegistrar 校验，
/// 两处共用这一条规则。

#ifndef EXTENSIONNAMESPACE_H
#define EXTENSIONNAMESPACE_H

#include <string_view>

#include <QString>

/// @brief id 是否落在扩展 extensionId 的命名空间内。
/// @return id 以 "<extensionId>." 开头且其后还有内容时返回 true。
inline bool isInExtensionNamespace(std::string_view extensionId, const QString& id)
{
    if (extensionId.empty())
    {
        return false;
    }
    const QString prefix =
        QString::fromUtf8(extensionId.data(), static_cast<int>(extensionId.size())) + QLatin1Char('.');
    return id.size() > prefix.size() && id.startsWith(prefix);
}

#endif  // EXTENSIONNAMESPACE_H
