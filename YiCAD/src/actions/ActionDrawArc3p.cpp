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


/// @file ActionDrawArc3p.cpp
/// @brief 通过三点绘制圆弧的交互动作类实现

#include <QAction>
#include "ActionDrawArc3p.h"

#include <QMouseEvent>

#include "ActionDrawArc.h"
#include "Commands.h"
#include "DmArc.h"
#include "DmLine.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Preview.h"
#include "Transaction.h"

struct ActionDrawArc3P::Points
{
    ArcData data;

    DmVector point1;
    DmVector point2;
    DmVector point3;
};

ActionDrawArc3P::ActionDrawArc3P(DmDocument* doc, GuiDocumentView* docView)
    : PreviewActionInterface("Draw arcs 3P", doc, docView)
    , pPoints(new Points())
{
    actionType = DM::ActionDrawArc3P;
    reset();
}

ActionDrawArc3P::~ActionDrawArc3P() = default;

void ActionDrawArc3P::reset()
{
    pPoints.reset(new Points{});
}

void ActionDrawArc3P::init(int status)
{
    PreviewActionInterface::init(status);
}

void ActionDrawArc3P::trigger()
{
    PreviewActionInterface::trigger();

    preparePreview();
    if (pPoints->data.isValid())
    {
        Transaction t(tr("Create Arc").toStdString(), pDocument);
        t.start();
        DmArc* arc = new DmArc(nullptr, pPoints->data);
        arc->setDocument(pDocument);
        pDocument->getEntityTable()->add(arc);
        t.commit();

        docView->moveRelativeZero(arc->getEndpoint());

        if (getSnapMode()->restriction == DM::RestrictOrthogonal)
        {
            setStatus(-1);
        }
        else
        {
            setStatus(SetPoint1);
        }
        reset();
    }
    else
    {
        GUIDIALOGFACTORY->commandMessage(tr("Invalid arc data."));
    }
}

void ActionDrawArc3P::preparePreview()
{
    pPoints->data = {};
    if (pPoints->point1.valid && pPoints->point2.valid && pPoints->point3.valid)
    {
        DmArc arc(nullptr, pPoints->data);
        bool success = arc.createFrom3P(pPoints->point1, pPoints->point2, pPoints->point3);
        if (success)
        {
            pPoints->data = arc.getData();
        }
    }
}

void ActionDrawArc3P::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = snapPoint(e);

    switch (getStatus())
    {
        case SetPoint1:
            pPoints->point1 = mouse;
            break;

        case SetPoint2:
            pPoints->point2 = mouse;
            if (pPoints->point1.valid)
            {
                DmLine* line = new DmLine{preview->getEntityContainer(), pPoints->point1, pPoints->point2};
                line->setDocument(pDocument);
                deletePreview();
                preview->addEntity(line);
                drawPreview();
            }
            break;

        case SetPoint3:
            pPoints->point3 = mouse;
            preparePreview();
            if (pPoints->data.isValid())
            {
                DmArc* arc = new DmArc(preview->getEntityContainer(), pPoints->data);
                arc->setDocument(pDocument);
                deletePreview();
                preview->addEntity(arc);
                drawPreview();
            }
            break;

        default:
            break;
    }
}

void ActionDrawArc3P::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        GuiCoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton)
    {
        deletePreview();
        init(getStatus() - 1);
    }
}

void ActionDrawArc3P::coordinateEvent(GuiCoordinateEvent* e)
{
    if (e == nullptr)
    {
        return;
    }
    DmVector mouse = e->getCoordinate();

    switch (getStatus())
    {
        case SetPoint1:
            pPoints->point1 = mouse;
            docView->moveRelativeZero(mouse);
            setStatus(SetPoint2);
            break;

        case SetPoint2:
            pPoints->point2 = mouse;
            docView->moveRelativeZero(mouse);
            setStatus(SetPoint3);
            break;

        case SetPoint3:
            pPoints->point3 = mouse;
            trigger();
            finishOrthogonal();
            break;

        default:
            break;
    }
}

void ActionDrawArc3P::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
        return;
    }

    if (COMMANDS->checkCommand("center", c, getEntityType()))
    {
        finish(false);
        docView->setCurrentAction(new ActionDrawArc(pDocument, docView));
    }
}

QStringList ActionDrawArc3P::getAvailableCommands()
{
    QStringList cmd;
    return cmd;
}

void ActionDrawArc3P::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetPoint1:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify startpoint or [center]"), tr("Cancel"));
            break;
        case SetPoint2:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify second point"), tr("Back"));
            break;
        case SetPoint3:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify endpoint"), tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void ActionDrawArc3P::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}
