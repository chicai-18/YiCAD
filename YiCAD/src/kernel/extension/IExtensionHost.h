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

/// @file IExtensionHost.h
/// @brief 宿主（ApplicationWindow）提供给扩展框架的服务。
///
/// 扩展不直接看到本接口：ExtensionManager 为每个扩展包一层
/// IExtensionContext，由那一层负责命名空间校验与命令的注册/注销，再把
/// 其余调用转给这里。宿主因此不需要知道当前是哪个扩展在调用——唯一的
/// 例外是 ribbonFor()，Ribbon 注册表属于 UI 层，按扩展限定命名空间的
/// 注册入口只能由宿主构造。

#ifndef IEXTENSIONHOST_H
#define IEXTENSIONHOST_H

#include <functional>
#include <string_view>

#include <QString>

class DmDocument;
class GuiDocumentView;
class QWidget;
class UIRibbonRegistrar;

class IExtensionHost
{
public:
    virtual ~IExtensionHost() = default;

    /// @brief 给扩展 extensionId 用的 Ribbon 注册入口，只接受该扩展命名空间
    /// 下的 ID（见 ExtensionNamespace.h）。宿主持有返回对象，同一扩展多次
    /// 调用返回同一个对象。
    virtual UIRibbonRegistrar& ribbonFor(std::string_view extensionId) = 0;

    /// @brief 主窗口，用作扩展弹出对话框时的父控件。
    virtual QWidget* mainWindow() = 0;

    /// @brief 当前活动文档；无打开文档时为 nullptr。
    virtual DmDocument* currentDocument() const = 0;

    /// @brief 当前活动文档视图；无打开文档时为 nullptr。
    virtual GuiDocumentView* currentDocumentView() const = 0;

    /// @brief 在"设置"类目里加一个设置页入口。ID 已由调用方校验。
    /// @return Ribbon 注册表拒绝（如 ID 重复、已冻结）时返回 false。
    virtual bool registerSettingsPage(const QString& id, const QString& title,
                                      const QString& iconPath, std::function<void()> open) = 0;

    /// @brief 按命令 ID 启动 CommandRegistry 里的命令。
    /// @return 命令未注册时返回 false。
    virtual bool activateCommand(const QString& commandId) = 0;
};

#endif  // IEXTENSIONHOST_H
