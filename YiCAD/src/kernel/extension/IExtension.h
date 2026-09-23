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

/// @file IExtension.h
/// @brief 进程内扩展接口。
///
/// 见 doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段4 第7.3/7.4节。这是给内部功能
/// 拆分用的进程内 C++ 接口，不是给第三方插件用的稳定 C ABI——那是
/// `plugin_runtime/` 已经有的 `YiCadPluginAbi.h`/`PluginRegistry`，两套机制
/// 边界清晰、不合并（"C ABI 对外，IExtension 对内"）。参考
/// `E:\dev\DS` 的 `Application/Framework/IExtension.h`。

#ifndef IEXTENSION_H
#define IEXTENSION_H

#include <string_view>

class IExtensionContext;

/// @brief 一个进程内扩展。`ExtensionManager` 持有并管理其生命周期。
class IExtension
{
public:
    virtual ~IExtension() = default;

    /// @brief 注册期回调，由 `ExtensionManager::BootAll` 按注册顺序调用。
    /// @param ctx 本扩展专属的上下文，由 ExtensionManager 持有，保证有效到
    /// 本扩展的 `OnShutdown()` 返回为止——扩展可以保留指向它的指针，供点击
    /// 回调等在 OnRegister 之后使用。Ribbon 注册只能在本回调期间进行。
    virtual void OnRegister(IExtensionContext& ctx) = 0;

    /// @brief 关闭期回调，由 `ExtensionManager::Shutdown` 按注册顺序的
    /// 反序调用。默认空实现。
    virtual void OnShutdown() {}

    /// @brief 扩展的稳定标识，建议 "ext." 前缀 + snake_case（如 "ext.ai"）。
    virtual std::string_view Id() const = 0;
};

#endif  // IEXTENSION_H
