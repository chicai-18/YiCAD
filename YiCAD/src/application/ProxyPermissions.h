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

/// @file ProxyPermissions.h
/// @brief 命令按代理权限过滤要处理的实体（RENDER_PLAN.md 第 4.8.3 节）
/// @details 扩展不在时自定义实体读成代理（DmProxyEntity），能做哪些操作由它的代理权限决定（DmProxyFlags）。
///          改动实体的命令在动手之前经这里去掉不允许的代理，并在命令行说明跳过了几个

#ifndef PROXYPERMISSIONS_H
#define PROXYPERMISSIONS_H

#include <vector>

#include "DmCustomEntity.h"

class DmEntity;

/// @brief 去掉 entities 里不允许 operation 的代理，有去掉的就在命令行说明
/// @param operation 命令要做的操作，可以是几种操作的组合（都允许才处理）
std::vector<DmEntity*> allowedForProxies(const std::vector<DmEntity*>& entities, DmProxyFlags operation);

/// @brief 在命令行说明跳过了几个不允许这个操作的代理；count 为 0 时什么也不说
void reportSkippedProxies(int count);

#endif // PROXYPERMISSIONS_H
