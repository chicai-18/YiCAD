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

#include "ActionSelect.h"
#include "DmDocument.h"
#include "EntityTable.h"

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

bool CommandRegistry::registerCommand(const QString& id, CommandFactory factory, CommandInfo info)
{
    if (id.isEmpty() || !factory)
    {
        return false;
    }
    if (m_commands.find(id) != m_commands.end())
    {
        return false;
    }
    info.aliases = normalizeAliases(info.aliases);
    for (const QString& alias : info.aliases)
    {
        if (m_aliases.find(alias) != m_aliases.end())
        {
            return false;
        }
    }
    for (const QString& alias : info.aliases)
    {
        m_aliases.emplace(alias, id);
    }
    m_commands.emplace(id, Entry{std::move(factory), std::move(info)});
    return true;
}

bool CommandRegistry::registerLegacyCommand(DM::ActionType legacyType, const QString& id,
                                             CommandFactory factory)
{
    if (m_legacyBridge.find(legacyType) != m_legacyBridge.end())
    {
        return false;
    }
    if (!registerCommand(id, std::move(factory)))
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
    return true;
}

bool CommandRegistry::hasCommand(const QString& id) const
{
    return m_commands.find(id) != m_commands.end();
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

CommandOptionsFactory CommandRegistry::optionsFactory(const QString& id) const
{
    auto it = m_commands.find(id);
    return it == m_commands.end() ? CommandOptionsFactory() : it->second.info.optionsFactory;
}

ActionInterface* CommandRegistry::create(DM::ActionType legacyType, const CommandContext& ctx) const
{
    auto it = m_legacyBridge.find(legacyType);
    if (it == m_legacyBridge.end())
    {
        return nullptr;
    }
    return create(it->second, ctx);
}

ActionInterface* CommandRegistry::create(const QString& id, const CommandContext& ctx) const
{
    auto it = m_commands.find(id);
    if (it == m_commands.end())
    {
        return nullptr;
    }
    ActionInterface* action = it->second.factory(ctx);
    if (action)
    {
        action->setCommandId(id);
    }
    return action;
}

CommandFactory makeSelectFirstFactory(DM::ActionType noSelectLegacyType, CommandFactory buildReal)
{
    return [noSelectLegacyType, buildReal](const CommandContext& ctx) -> ActionInterface*
    {
        if (!ctx.document->getEntityTable()->hasSelect())
        {
            return new ActionSelect(ctx.handler, ctx.document, ctx.view, noSelectLegacyType);
        }
        return buildReal(ctx);
    };
}
