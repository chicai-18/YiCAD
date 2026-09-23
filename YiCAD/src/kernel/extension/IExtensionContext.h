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

/// @file IExtensionContext.h
/// @brief 扩展拿到的上下文：Ribbon 注册、命令注册与启动、设置页、当前文档。
///
/// 刻意保持精简（对齐 DS 的 IExtensionContext 设计），不暴露整个
/// ApplicationWindow。每个扩展拿到的是 ExtensionManager 为它单独构造的
/// 实例：扩展自己注册的命令、设置页与 Ribbon 条目，ID 都必须以
/// "<扩展 ID>." 开头（见 ExtensionNamespace.h），否则注册被拒绝并返回
/// false；扩展注册的命令在它的 OnShutdown 之后由 ExtensionManager 注销。

#ifndef IEXTENSIONCONTEXT_H
#define IEXTENSIONCONTEXT_H

#include <functional>
#include <string_view>

#include <QString>

#include "CommandRegistry.h"

class DmDocument;
class GuiDocumentView;
class QWidget;
class UIRibbonRegistrar;

/// @brief 扩展的上下文。
/// @note 本对象由 ExtensionManager 持有，从该扩展的 OnRegister 开始有效，
/// 到它的 OnShutdown 返回为止；扩展可以在这段时间内保留指向它的指针。
class IExtensionContext
{
public:
    virtual ~IExtensionContext() = default;

    /// @brief 本扩展的 ID（即 IExtension::Id()），拼命令 ID 等用。
    virtual std::string_view extensionId() const = 0;

    /// @brief Ribbon 注册入口。只能在 OnRegister 期间注册，之后注册表已冻结。
    virtual UIRibbonRegistrar& ribbon() = 0;

    /// @brief 主窗口，用作扩展弹出对话框时的父控件。
    virtual QWidget* mainWindow() = 0;

    /// @brief 当前活动文档；无打开文档时为 nullptr。
    virtual DmDocument* currentDocument() const = 0;

    /// @brief 当前活动文档视图；无打开文档时为 nullptr。
    virtual GuiDocumentView* currentDocumentView() const = 0;

    /// @brief 在"设置"类目里加一个设置页入口。
    /// @param id 必须在本扩展的命名空间内
    /// @param title 显示名（按钮提示）
    /// @param iconPath 图标路径，可为空（使用宿主的默认图标）
    /// @param open 用户点击该入口时调用
    /// @return id 不在本扩展命名空间内时返回 false。
    virtual bool registerSettingsPage(const QString& id, const QString& title,
                                      const QString& iconPath, std::function<void()> open) = 0;

    /// @brief 注册一条命令（见 CommandRegistry::registerCommand）。
    /// @param id 必须在本扩展的命名空间内，如 "ext.dim.linear"
    /// @return id 不在本扩展命名空间内，或 CommandRegistry 拒绝时返回 false。
    virtual bool registerCommand(const QString& id, CommandFactory factory, CommandInfo info) = 0;

    /// @brief 按命令 ID 启动命令（任意已注册命令，不限本扩展）。
    /// @return 命令未注册时返回 false。
    virtual bool activateCommand(const QString& commandId) = 0;
};

#endif  // IEXTENSIONCONTEXT_H
