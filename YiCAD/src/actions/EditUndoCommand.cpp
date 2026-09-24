/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file EditUndoCommand.cpp
/// @brief 撤销、重做即时命令的实现与注册

#include "EditUndoCommand.h"

#include <QtGlobal>

#include "CommandRegistry.h"
#include "DmDocument.h"

void EditUndoCommand::run(DmDocument* doc, bool undo)
{
    if (!doc)
    {
        qWarning("undo: pDocument is null");
        return;
    }
    if (undo)
    {
        doc->undo();
    }
    else
    {
        doc->redo();
    }
}

namespace
{
const bool g_registeredUndo = CommandRegistry::instance().registerInstantCommand(
    DM::ActionEditUndo, QStringLiteral("edit.undo"),
    [](const CommandContext& ctx) { EditUndoCommand::run(ctx.document, true); });

const bool g_registeredRedo = CommandRegistry::instance().registerInstantCommand(
    DM::ActionEditRedo, QStringLiteral("edit.redo"),
    [](const CommandContext& ctx) { EditUndoCommand::run(ctx.document, false); });
}  // namespace
