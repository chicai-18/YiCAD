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

/// @file EditExtension.cpp

#include "EditExtension.h"

#include "DmSystem.h"
#include "EditCommands.h"
#include "IExtensionContext.h"

void EditExtension::OnRegister(IExtensionContext& ctx)
{
    // 本扩展的翻译包（src/extensions/edit/ts/）
    DMSYSTEM->loadExtensionTranslation(QStringLiteral("edit"));

    ctx.registerExclusiveCommand(QStringLiteral("ext.edit.copy"), EditCommands::copy(), {});
    ctx.registerExclusiveCommand(QStringLiteral("ext.edit.cut"), EditCommands::cut(), {});
    ctx.registerExclusiveCommand(QStringLiteral("ext.edit.paste"), EditCommands::paste(), {});
    ctx.registerInstantCommand(QStringLiteral("ext.edit.undo"), EditCommands::undo(), {});
    ctx.registerInstantCommand(QStringLiteral("ext.edit.redo"), EditCommands::redo(), {});
}
