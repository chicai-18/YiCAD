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

/// @file ProxyPermissions.cpp
/// @brief 命令按代理权限过滤实体的实现

#include "ProxyPermissions.h"

#include <QCoreApplication>

#include "DmProxyEntity.h"
#include "GuiDialogFactory.h"

std::vector<DmEntity*> allowedForProxies(const std::vector<DmEntity*>& entities, DmProxyFlags operation)
{
    int skipped = 0;
    std::vector<DmEntity*> allowed = DmProxyEntity::filterAllowed(entities, operation, &skipped);
    reportSkippedProxies(skipped);
    return allowed;
}

void reportSkippedProxies(int count)
{
    if (count <= 0 || !GUIDIALOGFACTORY)
    {
        return;
    }
    GUIDIALOGFACTORY->commandMessage(QCoreApplication::translate(
        "ProxyPermissions", "%n proxy object(s) do not allow this operation and were skipped.", nullptr, count));
}
