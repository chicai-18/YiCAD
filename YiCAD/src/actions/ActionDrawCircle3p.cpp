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


/// @file ActionDrawCircle3p.cpp
/// @brief 通过三个点绘制圆的交互动作实现

#include <QAction>
#include "ActionDrawCircle3p.h"

#include <QMouseEvent>

#include "DmCircle.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Preview.h"
#include "Transaction.h"

struct ActionDrawCircle3P::Points
{
    CircleData data;
    DmVector point1; ///< 第一个点
    DmVector point2; ///< 第二个点
    DmVector point3; ///< 第三个点
};

/// @brief 构造函数
ActionDrawCircle3P::ActionDrawCircle3P(DmDocument* doc, GuiDocumentView* docView) :
    PreviewActionInterface("Draw circles", doc, docView), pPoints(new Points{})
{
    actionType = DM::ActionDrawCircle3P;
}

ActionDrawCircle3P::~ActionDrawCircle3P() = default;

void ActionDrawCircle3P::init(int status)
{
    PreviewActionInterface::init(status);
    pPoints.reset(new Points{});
}

void ActionDrawCircle3P::trigger()
{
    PreviewActionInterface::trigger();

    preparePreview();
    if (pPoints->data.isValid())
    {
        Transaction t(tr("Create Circle3p").toStdString(), pDocument);
        t.start();
        DmCircle* circle = new DmCircle(nullptr, pPoints->data);
        circle->setDocument(pDocument);
        pDocument->getEntityTable()->add(circle);
        t.commit();

        DmVector rz = docView->getRelativeZero();
        docView->moveRelativeZero(rz);
        drawSnapper();

        if (getSnapMode()->restriction == DM::RestrictOrthogonal)
        {
            setStatus(-1);
        }
        else
        {
            setStatus(SetPoint1);
        }
        pPoints.reset(new Points{});
    }
    else
    {
        GUIDIALOGFACTORY->requestWarningDialog(tr("Invalid circle data."));
    }
}

void ActionDrawCircle3P::preparePreview()
{
    pPoints->data = CircleData{};
    if (pPoints->point1.valid && pPoints->point2.valid && pPoints->point3.valid)
    {
        DmCircle circle{nullptr, pPoints->data};
        bool suc = circle.createFrom3P(pPoints->point1, pPoints->point2, pPoints->point3);
        if (suc)
        {
            pPoints->data = circle.getData();
        }
    }
}

void ActionDrawCircle3P::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = snapPoint(e);

    switch (getStatus())
    {
        case SetPoint1:
            pPoints->point1 = mouse;
            break;

        case SetPoint2:
            pPoints->point2 = mouse;
            break;

        case SetPoint3:
            pPoints->point3 = mouse;
            preparePreview();
            if (pPoints->data.isValid())
            {
                DmCircle* circle = new DmCircle(preview->getEntityContainer(), pPoints->data);
                circle->setDocument(pDocument);
                deletePreview();
                preview->addEntity(circle);
                drawPreview();
            }
            break;
    }
}

void ActionDrawCircle3P::mouseReleaseEvent(QMouseEvent* e)
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

void ActionDrawCircle3P::coordinateEvent(GuiCoordinateEvent* e)
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

void ActionDrawCircle3P::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
        return;
    }
}

QStringList ActionDrawCircle3P::getAvailableCommands()
{
    QStringList cmd;
    return cmd;
}

void ActionDrawCircle3P::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetPoint1:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify first point"), tr("Cancel"));
            break;
        case SetPoint2:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify second point"), tr("Back"));
            break;
        case SetPoint3:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify third point"), tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void ActionDrawCircle3P::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}
