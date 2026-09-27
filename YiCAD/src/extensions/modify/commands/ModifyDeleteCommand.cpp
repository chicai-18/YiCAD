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

/// @file ModifyDeleteCommand.cpp
/// @brief 删除命令的实现

#include "ModifyDeleteCommand.h"

#include "ModifyCommands.h"
#include "DmDocument.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "Modification.h"

ModifyDeleteCommand::ModifyDeleteCommand()
    : SelectFirstCommand(SelectionEntry::Always)
{
}

void ModifyDeleteCommand::deleteSelection(DmDocument* doc, IDocumentView* view)
{
    if (!doc || !view)
    {
        return;
    }
    std::vector<DmEntity*> ents;
    for (auto e : *doc->getEntityTable())
    {
        if (e->isSelected())
        {
            ents.push_back(e);
        }
    }
    Modification m(doc);
    m.remove(ents);
    GUIDIALOGFACTORY->updateSelectionWidget(doc->getEntityTable()->countSelect());
}

bool ModifyDeleteCommand::onSelectionReady()
{
    deleteSelection(document(), view());
    finish();
    return true;
}

ExclusiveCommandFactory ModifyCommands::remove()
{
    return exclusiveCommandFactory<ModifyDeleteCommand>();
}

InstantCommand ModifyCommands::deleteSelection()
{
    return [](const CommandContext& ctx) { ModifyDeleteCommand::deleteSelection(ctx.document, ctx.view); };
}
