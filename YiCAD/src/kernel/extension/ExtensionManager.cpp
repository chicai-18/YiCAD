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

/// @file ExtensionManager.cpp

#include "ExtensionManager.h"

#include <algorithm>
#include <string>
#include <utility>

#include <QDebug>

#include "CommandRegistry.h"
#include "ExtensionNamespace.h"
#include "IExtension.h"
#include "IExtensionContext.h"
#include "IExtensionHost.h"

/// @brief 单个扩展的上下文：命名空间校验、命令登记，其余转给宿主。
///
/// 不放进匿名命名空间：ExtensionManager.h 里 `class ExtensionScopedContext;`
/// 的前置声明要与这里是同一个类型。
class ExtensionScopedContext final : public IExtensionContext
{
public:
    ExtensionScopedContext(IExtensionHost& host, std::string_view extensionId)
        : m_host(host), m_extensionId(extensionId)
    {
    }

    std::string_view extensionId() const override { return m_extensionId; }
    UIRibbonRegistrar& ribbon() override { return m_host.ribbonFor(m_extensionId); }
    QWidget* mainWindow() override { return m_host.mainWindow(); }
    DmDocument* currentDocument() const override { return m_host.currentDocument(); }
    GuiDocumentView* currentDocumentView() const override { return m_host.currentDocumentView(); }

    bool registerSettingsPage(const QString& id, const QString& title, const QString& iconPath,
                              std::function<void()> open) override
    {
        if (!owns("settings page", id))
        {
            return false;
        }
        return m_host.registerSettingsPage(id, title, iconPath, std::move(open));
    }

    bool registerCommand(const QString& id, CommandFactory factory, CommandInfo info) override
    {
        if (!owns("command", id))
        {
            return false;
        }
        if (!CommandRegistry::instance().registerCommand(id, std::move(factory), std::move(info)))
        {
            qWarning("ExtensionManager: %s: command '%s' rejected by CommandRegistry (duplicate id or alias)",
                     m_extensionId.c_str(), qUtf8Printable(id));
            return false;
        }
        m_commands.push_back(id);
        return true;
    }

    bool activateCommand(const QString& commandId) override { return m_host.activateCommand(commandId); }

    /// @brief 注销本扩展注册过的全部命令，在其 OnShutdown 之后调用。
    void releaseCommands()
    {
        for (const QString& id : m_commands)
        {
            CommandRegistry::instance().unregisterCommand(id);
        }
        m_commands.clear();
    }

private:
    bool owns(const char* kind, const QString& id) const
    {
        if (isInExtensionNamespace(m_extensionId, id))
        {
            return true;
        }
        qWarning("ExtensionManager: %s: %s '%s' is outside namespace '%s.'; rejected",
                 m_extensionId.c_str(), kind, qUtf8Printable(id), m_extensionId.c_str());
        return false;
    }

    IExtensionHost& m_host;
    std::string m_extensionId;
    std::vector<QString> m_commands;
};

ExtensionManager& ExtensionManager::instance()
{
    static ExtensionManager manager;
    return manager;
}

ExtensionManager::~ExtensionManager()
{
    // 防御性兜底：正常路径下调用方应显式调用 Shutdown()（见
    // ApplicationWindow::~ApplicationWindow() 的插入点）。如果走到这里时
    // 还有未关闭的扩展，说明调用方漏调了，发出警告并补上，避免扩展的
    // OnShutdown() 完全不执行。Shutdown() 本身幂等，正常路径下这里
    // m_extensions 已经是空的，不会重复触发 OnShutdown()。
    if (!m_extensions.empty())
    {
        qWarning("ExtensionManager: 析构时仍有未 Shutdown() 的扩展，自动补调");
        Shutdown();
    }
}

bool ExtensionManager::Register(std::unique_ptr<IExtension> ext)
{
    if (!ext)
    {
        qWarning("ExtensionManager::Register: 扩展指针为空");
        return false;
    }
    if (m_booted)
    {
        qWarning("ExtensionManager::Register: BootAll 之后不能再注册新扩展（%s）",
                 std::string(ext->Id()).c_str());
        return false;
    }
    const std::string_view id = ext->Id();
    if (Find(id) != nullptr)
    {
        qWarning("ExtensionManager::Register: 重复的扩展 Id（%s）", std::string(id).c_str());
        return false;
    }
    m_extensions.push_back(Slot{std::move(ext), nullptr});
    return true;
}

void ExtensionManager::BootAll(IExtensionHost& host)
{
    if (m_booted)
    {
        qWarning("ExtensionManager::BootAll: 已经启动过，本次调用被忽略");
        return;
    }
    m_booted = true;
    for (auto& slot : m_extensions)
    {
        slot.context = std::make_unique<ExtensionScopedContext>(host, slot.extension->Id());
        slot.extension->OnRegister(*slot.context);
    }
}

void ExtensionManager::Shutdown()
{
    // 只有真正 BootAll 过的扩展才配对调用 OnShutdown()——从未
    // OnRegister() 过的扩展不该收到 OnShutdown()。清空 + 复位
    // m_booted 让管理器回到初始状态，可以重新 Register/BootAll
    // （单测需要这个；生产环境下 Shutdown 只在进程退出前调用一次，
    // 不存在真的"重新启动"场景，复位状态对生产行为没有影响）。
    // 扩展注册的命令在它自己的 OnShutdown 之后注销，OnShutdown 期间
    // 仍然可以按 ID 启动自己的命令。
    if (m_booted)
    {
        for (auto it = m_extensions.rbegin(); it != m_extensions.rend(); ++it)
        {
            it->extension->OnShutdown();
            if (it->context)
            {
                it->context->releaseCommands();
            }
        }
    }
    m_extensions.clear();
    m_booted = false;
}

IExtension* ExtensionManager::Find(std::string_view id) const
{
    auto it = std::find_if(m_extensions.begin(), m_extensions.end(),
                            [id](const Slot& slot) { return slot.extension->Id() == id; });
    return it == m_extensions.end() ? nullptr : it->extension.get();
}
