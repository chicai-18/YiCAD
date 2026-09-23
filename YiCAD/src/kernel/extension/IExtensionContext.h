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
/// @brief 扩展在 `OnRegister` 期间能拿到的、启动期资源门面。
///
/// 刻意保持精简（对齐 `E:\dev\DS` 的 `IExtensionContext` 设计）：只暴露
/// Ribbon 注册入口、主窗口、当前文档/视图访问器、设置页注册，不暴露整个
/// `ApplicationWindow`——扩展需要的其它东西自己去拿，不靠这层门面代为
/// 转发。这个窄接口也让 `ExtensionManager` 自身的单测可以用一个不需要
/// 真正可用的假实现来跑，不需要拉起一整个 `ApplicationWindow`。

#ifndef IEXTENSIONCONTEXT_H
#define IEXTENSIONCONTEXT_H

#include <functional>

#include <QString>

class SARibbonBar;
class QWidget;
class DmDocument;
class GuiDocumentView;

/// @brief 扩展注册期的资源门面。
/// @note 本接口对象本身的生命周期只在 `IExtension::OnRegister` 调用期间
/// 保证有效（见 `IExtension.h` 的说明）；具体实现是否存活更久，由该实现
/// 自己的文档说明。
class IExtensionContext
{
public:
    virtual ~IExtensionContext() = default;

    /// @brief 应用的 Ribbon 栏，用于放置扩展自己的按钮。
    virtual SARibbonBar& ribbon() = 0;

    /// @brief 主窗口，用作扩展弹出对话框时的父widget。
    virtual QWidget* mainWindow() = 0;

    /// @brief 当前活动文档；无打开文档时为 nullptr。
    virtual DmDocument* currentDocument() const = 0;

    /// @brief 当前活动文档视图；无打开文档时为 nullptr。
    virtual GuiDocumentView* currentDocumentView() const = 0;

    /// @brief 注册一个设置页入口。
    /// @param id 稳定标识，建议以扩展自己的 Id() 为前缀。
    /// @param title 显示名。
    /// @param iconPath 图标路径，可为空（使用宿主的默认图标）。
    /// @param open 用户点击该入口时调用；实现通常是构造并 `exec()` 一个
    /// 模态对话框。
    virtual void registerSettingsPage(const QString& id, const QString& title,
                                       const QString& iconPath,
                                       std::function<void()> open) = 0;
};

#endif  // IEXTENSIONCONTEXT_H
