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

/// @file EditCommands.h
/// @brief 编辑扩展（src/extensions/edit/）各命令的工厂
///
/// 命令类多数只在自己的 .cpp 里定义，工厂也定义在那里；扩展入口按命令 ID 登记它们
/// （doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.4 节：原 src/actions/ 的内置命令拆进扩展）。

#ifndef EDITCOMMANDS_H
#define EDITCOMMANDS_H

#include "CommandRegistry.h"

namespace EditCommands
{
/// @brief 交互命令 ext.edit.cut（EditCopyCommand.cpp）
ExclusiveCommandFactory cut();
/// @brief 交互命令 ext.edit.copy（EditCopyCommand.cpp）
ExclusiveCommandFactory copy();
/// @brief 交互命令 ext.edit.paste（EditPasteCommand.cpp）
ExclusiveCommandFactory paste();
/// @brief 即时命令 ext.edit.undo（EditUndoCommand.cpp）
InstantCommand undo();
/// @brief 即时命令 ext.edit.redo（EditUndoCommand.cpp）
InstantCommand redo();
}  // namespace EditCommands

#endif  // EDITCOMMANDS_H
