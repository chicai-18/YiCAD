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

/// @file BlocksEditCommand.cpp
/// @brief 编辑块命令的实现

#include "BlocksEditCommand.h"

#include <memory>

#include <QMessageBox>

#include "BlockEditTool.h"
#include "CommandRegistry.h"
#include "DmBlockReference.h"
#include "DmDocument.h"
#include "EntityTable.h"
#include "ExclusiveCommandBus.h"
#include "GuiDialogFactory.h"
#include "ICommandHost.h"

bool BlocksEditCommand::onSelectionReady()
{
    DmBlockReference* selectedRef = nullptr;
    for (auto e : *document()->getEntityTable())
    {
        if (e && e->isSelected() && e->getEntityType() == DM::EntityBlockReference)
        {
            selectedRef = static_cast<DmBlockReference*>(e);
            break;
        }
    }
    if (!selectedRef)
    {
        GUIDIALOGFACTORY->commandMessage(tr("No block reference selected. Command cancelled."));
        return false;
    }

    auto mode = std::make_unique<BlockEditTool>(*host());
    if (!mode->prepare(selectedRef))
    {
        return false;
    }
    // 先把模式交给总线再进入：进入的事务会触发撤销栈变化通知，
    // UIActionHandler 据此判断是否要"重新进入"块编辑，这时要能看到模式已经存在
    BlockEditTool* blockEdit = mode.get();
    bus()->enterEditMode(std::move(mode));
    blockEdit->beginEditing(selectedRef);
    finish();
    return true;
}

std::unique_ptr<IExclusiveCommand> BlocksEditCommand::create(const CommandContext& ctx)
{
    if (ctx.document->getEditingBlock() != nullptr)
    {
        QMessageBox::warning(nullptr, BlocksEditCommand::tr("Block Edit"),
                             BlocksEditCommand::tr("Cannot edit block references while already editing a block."));
        return nullptr;
    }
    return std::make_unique<BlocksEditCommand>();
}
