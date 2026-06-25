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


/// @file ActionDrawPoint.cpp
/// @brief 绘制点动作类的实现

#include "ActionDrawPoint.h"

#include <QMouseEvent>

#include "DmPoint.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Transaction.h"

ActionDrawPoint::ActionDrawPoint(DmDocument* doc, GuiDocumentView* docView) :
    PreviewActionInterface("Draw Points", doc, docView), m_pt(new DmVector{})
{
    actionType = DM::ActionDrawPoint;
}

ActionDrawPoint::~ActionDrawPoint() = default;

void ActionDrawPoint::trigger()
{
    if (m_pt->valid)
    {
        Transaction t(tr("Create Point").toStdString(), pDocument);
        t.start();
        DmPoint* point = new DmPoint(nullptr, PointData(*m_pt));
        point->setDocument(pDocument);
        pDocument->getEntityTable()->add(point);
        t.commit();
        docView->moveRelativeZero(*m_pt);
    }
}

void ActionDrawPoint::mouseMoveEvent(QMouseEvent* e)
{
    snapPoint(e);
}

void ActionDrawPoint::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        GuiCoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton)
    {
        init(getStatus() - 1);
    }
    else
    {
        // TODO: 处理其他鼠标按钮事件
    }
}

void ActionDrawPoint::coordinateEvent(GuiCoordinateEvent* e)
{
    if (e == nullptr)
    {
        return;
    }

    DmVector mouse = e->getCoordinate();

    *m_pt = mouse;
    trigger();
}

void ActionDrawPoint::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
        return;
    }
    else
    {
        // TODO: 处理其他命令
    }
}

QStringList ActionDrawPoint::getAvailableCommands()
{
    return {};
}

void ActionDrawPoint::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case 0:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify location"), tr("Cancel"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void ActionDrawPoint::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}
