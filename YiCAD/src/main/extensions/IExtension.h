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
    /// @param ctx 本次调用期间有效的扩展上下文。
    /// @note `ctx` 参数本身只在调用期间保证有效；扩展需要在调用之后继续
    /// 使用的东西必须自己存成员，不能默认保留 `ctx` 引用/指针到调用结束
    /// 之后。例外：如果调用方交给你的具体 `IExtensionContext` 实现在自己
    /// 的文档里说明了更长的生命周期保证（例如它存活到 `Shutdown()` 为止），
    /// 扩展可以按那份实现自己的保证保留指针——这是否安全取决于具体实现，
    /// 不是本接口的通用契约。
    virtual void OnRegister(IExtensionContext& ctx) = 0;

    /// @brief 关闭期回调，由 `ExtensionManager::Shutdown` 按注册顺序的
    /// 反序调用。默认空实现。
    virtual void OnShutdown() {}

    /// @brief 扩展的稳定标识，建议 "ext." 前缀 + snake_case（如 "ext.ai"）。
    virtual std::string_view Id() const = 0;
};

#endif  // IEXTENSION_H
