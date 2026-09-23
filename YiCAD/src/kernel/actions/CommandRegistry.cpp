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

#include <utility>

#include "ActionSelect.h"
#include "DmDocument.h"
#include "EntityTable.h"

// 注册冲突（重复 ID / 重复 legacy 类型）用返回值报告，不用 assert() 硬中断——
// 这条路径本身就是可测试、可恢复的正常分支（见
// tests/interaction/test_command_registry.cpp 的"重复注册…被拒绝"用例），
// 与 PluginRegistry 用返回值而非 assert 报告注册错误的既有约定一致。

CommandRegistry& CommandRegistry::instance()
{
    static CommandRegistry registry;
    return registry;
}

bool CommandRegistry::registerCommand(const QString& id, CommandFactory factory)
{
    if (id.isEmpty() || !factory)
    {
        return false;
    }
    if (m_commands.find(id) != m_commands.end())
    {
        return false;
    }
    m_commands.emplace(id, std::move(factory));
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

bool CommandRegistry::hasLegacyMapping(DM::ActionType legacyType) const
{
    return m_legacyBridge.find(legacyType) != m_legacyBridge.end();
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
    return it->second(ctx);
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
