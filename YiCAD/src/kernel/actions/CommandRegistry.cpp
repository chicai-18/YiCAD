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

/// @file CommandRegistry.cpp

#include "CommandRegistry.h"

#include <iterator>
#include <utility>

#include "IExclusiveCommand.h"
#include "TransientViewTool.h"

// 注册冲突（重复 ID / 重复 legacy 类型）用返回值报告，不用 assert() 硬中断——
// 这条路径本身就是可测试、可恢复的正常分支（见
// tests/interaction/test_command_registry.cpp 的"重复注册…被拒绝"用例），
// 与 PluginRegistry 用返回值而非 assert 报告注册错误的既有约定一致。

namespace
{
/// @brief 别名统一去首尾空白、转小写，去掉空项与重复项。
QStringList normalizeAliases(const QStringList& aliases)
{
    QStringList out;
    for (const QString& alias : aliases)
    {
        const QString key = alias.trimmed().toLower();
        if (!key.isEmpty() && !out.contains(key))
        {
            out.append(key);
        }
    }
    return out;
}
}  // namespace

CommandRegistry& CommandRegistry::instance()
{
    static CommandRegistry registry;
    return registry;
}

bool CommandRegistry::addEntry(const QString& id, Entry entry)
{
    if (id.isEmpty())
    {
        return false;
    }
    if (m_commands.find(id) != m_commands.end())
    {
        return false;
    }
    entry.info.aliases = normalizeAliases(entry.info.aliases);
    for (const QString& alias : entry.info.aliases)
    {
        if (m_aliases.find(alias) != m_aliases.end())
        {
            return false;
        }
    }
    for (const QString& alias : entry.info.aliases)
    {
        m_aliases.emplace(alias, id);
    }
    m_commands.emplace(id, std::move(entry));
    return true;
}

bool CommandRegistry::registerExclusiveCommand(const QString& id, ExclusiveCommandFactory factory, CommandInfo info)
{
    if (!factory)
    {
        return false;
    }
    Entry entry;
    entry.kind = CommandKind::Exclusive;
    entry.commandFactory = std::move(factory);
    entry.info = std::move(info);
    return addEntry(id, std::move(entry));
}

bool CommandRegistry::registerExclusiveCommand(DM::ActionType legacyType, const QString& id,
                                               ExclusiveCommandFactory factory)
{
    return addBridged(legacyType, id, [&]() { return registerExclusiveCommand(id, std::move(factory)); });
}

bool CommandRegistry::registerInstantCommand(const QString& id, InstantCommand command, CommandInfo info)
{
    if (!command)
    {
        return false;
    }
    Entry entry;
    entry.kind = CommandKind::Instant;
    entry.instant = std::move(command);
    entry.info = std::move(info);
    return addEntry(id, std::move(entry));
}

bool CommandRegistry::registerInstantCommand(DM::ActionType legacyType, const QString& id, InstantCommand command,
                                             CommandInfo info)
{
    return addBridged(legacyType, id,
                      [&]() { return registerInstantCommand(id, std::move(command), std::move(info)); });
}

bool CommandRegistry::registerViewTool(const QString& id, ViewToolFactory factory, CommandInfo info)
{
    if (!factory)
    {
        return false;
    }
    Entry entry;
    entry.kind = CommandKind::ViewTool;
    entry.viewToolFactory = std::move(factory);
    entry.info = std::move(info);
    return addEntry(id, std::move(entry));
}

bool CommandRegistry::registerViewTool(DM::ActionType legacyType, const QString& id, ViewToolFactory factory)
{
    return addBridged(legacyType, id, [&]() { return registerViewTool(id, std::move(factory)); });
}

bool CommandRegistry::bindLegacyType(DM::ActionType legacyType, const QString& id)
{
    if (m_legacyBridge.find(legacyType) != m_legacyBridge.end() || !hasCommand(id))
    {
        return false;
    }
    m_legacyBridge.emplace(legacyType, id);
    return true;
}

bool CommandRegistry::addBridged(DM::ActionType legacyType, const QString& id,
                                 const std::function<bool()>& registerEntry)
{
    if (m_legacyBridge.find(legacyType) != m_legacyBridge.end())
    {
        return false;
    }
    if (!registerEntry())
    {
        return false;
    }
    m_legacyBridge.emplace(legacyType, id);
    return true;
}

bool CommandRegistry::unregisterCommand(const QString& id)
{
    auto it = m_commands.find(id);
    if (it == m_commands.end())
    {
        return false;
    }
    for (const QString& alias : it->second.info.aliases)
    {
        m_aliases.erase(alias);
    }
    m_commands.erase(it);
    for (auto bridge = m_legacyBridge.begin(); bridge != m_legacyBridge.end();)
    {
        bridge = bridge->second == id ? m_legacyBridge.erase(bridge) : std::next(bridge);
    }
    for (auto editor = m_entityEditors.begin(); editor != m_entityEditors.end();)
    {
        editor = editor->second == id ? m_entityEditors.erase(editor) : std::next(editor);
    }
    return true;
}

bool CommandRegistry::registerEntityEditor(DM::EntityType type, const QString& commandId)
{
    if (kind(commandId) != CommandKind::Exclusive || m_entityEditors.count(type) != 0)
    {
        return false;
    }
    m_entityEditors[type] = commandId;
    return true;
}

QString CommandRegistry::entityEditor(DM::EntityType type) const
{
    auto it = m_entityEditors.find(type);
    return it == m_entityEditors.end() ? QString() : it->second;
}

bool CommandRegistry::hasCommand(const QString& id) const
{
    return m_commands.find(id) != m_commands.end();
}

CommandKind CommandRegistry::kind(const QString& id) const
{
    auto it = m_commands.find(id);
    return it == m_commands.end() ? CommandKind::None : it->second.kind;
}

bool CommandRegistry::hasLegacyMapping(DM::ActionType legacyType) const
{
    return m_legacyBridge.find(legacyType) != m_legacyBridge.end();
}

QString CommandRegistry::commandId(DM::ActionType legacyType) const
{
    auto it = m_legacyBridge.find(legacyType);
    return it == m_legacyBridge.end() ? QString() : it->second;
}

DM::ActionType CommandRegistry::legacyType(const QString& id) const
{
    for (const auto& [type, bridgedId] : m_legacyBridge)
    {
        if (bridgedId == id)
        {
            return type;
        }
    }
    return DM::ActionNone;
}

QString CommandRegistry::commandForAlias(const QString& alias) const
{
    auto it = m_aliases.find(alias.trimmed().toLower());
    return it == m_aliases.end() ? QString() : it->second;
}

QStringList CommandRegistry::aliases() const
{
    QStringList out;
    for (const auto& [alias, id] : m_aliases)
    {
        out.append(alias);
    }
    return out;
}

QString CommandRegistry::description(const QString& id) const
{
    auto it = m_commands.find(id);
    return it == m_commands.end() ? QString() : it->second.info.description;
}

ExclusiveCommandOptionsFactory CommandRegistry::commandOptionsFactory(const QString& id) const
{
    auto it = m_commands.find(id);
    return it == m_commands.end() ? ExclusiveCommandOptionsFactory() : it->second.info.commandOptionsFactory;
}

InstantInterrupt CommandRegistry::instantInterrupt(const QString& id) const
{
    auto it = m_commands.find(id);
    return it == m_commands.end() ? InstantInterrupt::EndUninterruptible : it->second.info.instantInterrupt;
}

std::unique_ptr<IExclusiveCommand> CommandRegistry::createCommand(const QString& id, const CommandContext& ctx) const
{
    auto it = m_commands.find(id);
    if (it == m_commands.end() || it->second.kind != CommandKind::Exclusive)
    {
        return nullptr;
    }
    std::unique_ptr<IExclusiveCommand> command = it->second.commandFactory(ctx);
    if (command)
    {
        command->setCommandId(id);
    }
    return command;
}

std::unique_ptr<TransientViewTool> CommandRegistry::createViewTool(const QString& id, const CommandContext& ctx) const
{
    auto it = m_commands.find(id);
    if (it == m_commands.end() || it->second.kind != CommandKind::ViewTool)
    {
        return nullptr;
    }
    std::unique_ptr<TransientViewTool> tool = it->second.viewToolFactory(ctx);
    if (tool)
    {
        tool->setCommandId(id);
    }
    return tool;
}

bool CommandRegistry::runInstant(const QString& id, const CommandContext& ctx) const
{
    auto it = m_commands.find(id);
    if (it == m_commands.end() || it->second.kind != CommandKind::Instant)
    {
        return false;
    }
    it->second.instant(ctx);
    return true;
}
