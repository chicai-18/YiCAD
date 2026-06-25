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


/// @file ActionModifyReverse.cpp
/// @brief 反向修改操作——将选中实体的方向反转

#include "ActionModifyReverse.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "DmLine.h"
#include "DmArc.h"
#include "DmSpline.h"
#include "DmEllipse.h"
#include "Transaction.h"

ActionModifyReverse::ActionModifyReverse(DmDocument* doc, GuiDocumentView* docView)
    : ActionInterface("Reverse Entities", doc, docView)
{
    actionType = DM::ActionModifyReverse;
}

/// @brief 初始化并直接执行反向操作
/// @param [in] status 初始状态值
void ActionModifyReverse::init(int status)
{
    ActionInterface::init(status);

    trigger();
}

/// @brief 执行反向操作，反转所有选中实体的方向
void ActionModifyReverse::trigger()
{
    Transaction t(tr("Reverse").toStdString(), pDocument);
    t.start();
    auto table = pDocument->getEntityTable();
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

    finish(false);
    if (count == 0)
    {
        GUIDIALOGFACTORY->commandMessage(tr("%1 entities reversed. Only line, arc, ellipse, spline are supported.").arg(count));
    }
    else
    {
        GUIDIALOGFACTORY->commandMessage(tr("%1 entities reversed.").arg(count));
    }
}

/// @brief 更新鼠标按钮提示信息
void ActionModifyReverse::updateMouseButtonHints()
{
    GUIDIALOGFACTORY->updateMouseWidget();
}
