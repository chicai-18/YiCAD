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

/// @file ModifyReverseCommand.cpp
/// @brief 反向命令的实现

#include "ModifyReverseCommand.h"

#include "CommandRegistry.h"
#include "DmArc.h"
#include "DmDocument.h"
#include "DmEllipse.h"
#include "DmLine.h"
#include "DmSpline.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "Transaction.h"

bool ModifyReverseCommand::onSelectionReady()
{
    Transaction t(tr("Reverse").toStdString(), document());
    t.start();
    auto table = document()->getEntityTable();
    int count = 0;
    for (auto ent : *table)
    {
        if (!ent->isSelected())
        {
            continue;
        }
        switch (ent->getEntityType())
        {
            case DM::EntityLine:
            {
                table->startModify(ent);
                DmLine* l = (DmLine*)ent;
                DmVector s = l->getStartpoint();
                DmVector e = l->getEndpoint();
                l->setStartpoint(e);
                l->setEndpoint(s);
                l->update();
                count++;
            }
                break;
            case DM::EntityArc:
            {
                table->startModify(ent);
                DmArc* arc = (DmArc*)ent;
                arc->setClockwise(!arc->isClockwise());
                arc->update();
                count++;
            }
                break;
            case DM::EntityEllipse:
            {
                table->startModify(ent);
                DmEllipse* ell = (DmEllipse*)ent;
                ell->setClockwise(!ell->isClockwise());
                ell->update();
                count++;
            }
                break;
            case DM::EntitySpline:
            {
                table->startModify(ent);
                DmSpline* spline = (DmSpline*)ent;
                spline->reverse();
                spline->update();
                count++;
            }
                break;
            default:
                break;
        }
    }
    t.commit();

    finish();
    if (count == 0)
    {
        GUIDIALOGFACTORY->commandMessage(tr("%1 entities reversed. Only line, arc, ellipse, spline are supported.").arg(count));
    }
    else
    {
        GUIDIALOGFACTORY->commandMessage(tr("%1 entities reversed.").arg(count));
    }
    return true;
}

namespace
{
const bool g_registered = CommandRegistry::instance().registerExclusiveCommand(
    QStringLiteral("modify.reverse"), exclusiveCommandFactory<ModifyReverseCommand>());
}  // namespace
