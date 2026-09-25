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

/// @file InfoTotalLengthCommand.cpp
/// @brief 总长度命令的实现

#include "InfoTotalLengthCommand.h"

#include "CommandRegistry.h"
#include "DmDocument.h"
#include "GuiDialogFactory.h"

bool InfoTotalLengthCommand::onSelectionReady()
{
    DmDocument* doc = document();
    double totalLength = 0.0;
    auto table = doc->getEntityTable();
    for (auto it = table->begin(); it != table->end(); ++it)
    {
        if ((*it)->isSelected())
        {
            totalLength += (*it)->getLength();
        }
    }
    if (totalLength > 0.0)
    {
        QString len = DmUnits::formatLinear(totalLength, doc->getUnit(), doc->getLinearFormat(), doc->getLinearPrecision());
        GUIDIALOGFACTORY->commandMessage(tr("Total Length of selected entities: %1").arg(len));
    }
    else
    {
        GUIDIALOGFACTORY->commandMessage(tr("At least one of the selected entities cannot be measured."));
    }

    finish();
    return true;
}

namespace
{
const bool g_registered = CommandRegistry::instance().registerExclusiveCommand(
    QStringLiteral("info.total_length"), exclusiveCommandFactory<InfoTotalLengthCommand>());
}  // namespace
