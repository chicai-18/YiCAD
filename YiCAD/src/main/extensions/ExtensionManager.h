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

/// @file ExtensionManager.h
/// @brief 扩展的注册、启动与关闭。
///
/// 生命周期约束：注册顺序 == 启动顺序 == 关闭的反序；`BootAll` 只能成功
/// 调用一次；`Shutdown` 幂等。参考 `E:\dev\DS` 的
/// `Application/Framework/ExtensionManager.h`。
/// @note 仅限 UI 主线程访问，无内部同步。

#ifndef EXTENSIONMANAGER_H
#define EXTENSIONMANAGER_H

#include <memory>
#include <string_view>
#include <vector>

class IExtension;
class IExtensionContext;

class ExtensionManager
{
public:
    static ExtensionManager& instance();

    /// @brief 注册一个扩展。
    /// @return 成功返回 true；`Id()` 与已注册的扩展重复，或已经
    /// `BootAll` 过之后再调用，均返回 false。
    bool Register(std::unique_ptr<IExtension> ext);

    /// @brief 按注册顺序调用每个扩展的 `OnRegister(ctx)`。已经启动过时
    /// 再次调用是空操作（不会重新触发任何扩展的 `OnRegister`）。
    void BootAll(IExtensionContext& ctx);

    /// @brief 若已启动，按注册顺序的反序调用每个扩展的 `OnShutdown()`；
    /// 随后清空已持有的扩展，并把管理器恢复到初始状态（可以重新
    /// `Register`/`BootAll`）。幂等——重复调用是空操作。若从未
    /// `BootAll` 过就调用本方法，只清空已注册但从未 `OnRegister`
    /// 过的扩展，不调用它们的 `OnShutdown()`（没有配对的 `OnRegister`）。
    void Shutdown();

    /// @brief 按 Id 查找已注册的扩展；未找到返回 nullptr。
    IExtension* Find(std::string_view id) const;

private:
    ExtensionManager() = default;
    ~ExtensionManager();

    ExtensionManager(const ExtensionManager&) = delete;
    ExtensionManager& operator=(const ExtensionManager&) = delete;

    std::vector<std::unique_ptr<IExtension>> m_extensions;
    bool m_booted = false;
};

#endif  // EXTENSIONMANAGER_H
