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


/// @file ActionDimRadial.cpp
/// @brief 径向/直径标注交互操作类实现

#include <QAction>
#include "ActionDimRadial.h"

#include <QMouseEvent>

#include "Debug.h"
#include "DmArc.h"
#include "DmCircle.h"
#include "DmDimRadial.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Math2d.h"
#include "Preview.h"
#include "Transaction.h"

ActionDimRadial::ActionDimRadial(DmDocument* doc, GuiDocumentView* docView) :
    ActionDimension("Draw Radial Dimensions", doc, docView), entity(nullptr), pos(new DmVector{}),
    lastStatus(SetEntity)
{
    actionType = DM::ActionDimRadial;
    reset();
}

ActionDimRadial::~ActionDimRadial() = default;

void ActionDimRadial::reset()
{
    ActionDimension::reset();

    edata.reset(new DmDimRadialData{{}, 0.0});
    entity     = nullptr;
    *pos       = {};
    lastStatus = SetEntity;
    //GUIDIALOGFACTORY->requestOptions(this, true, true);
}

void ActionDimRadial::trigger()
{
    ActionDimension::trigger();

    preparePreview();
    if (entity)
    {
        DmDimRadial* newEntity =
                    new DmDimRadial(nullptr, *data, *edata);
        newEntity->setDocument(pDocument);
        newEntity->update();
        Transaction t(tr("Add dimension radial").toStdString(), pDocument);
        t.start();
        pDocument->getEntityTable()->add(newEntity);
        t.commit();

        DmVector rz = docView->getRelativeZero();
        docView->moveRelativeZero(rz);
        Snapper::finish();
    }
}

void ActionDimRadial::preparePreview()
{
    if (entity)
    {
        double dist = data->definitionPoint.distanceTo(*pos);   //圆心到鼠标距离
        double angle  = data->definitionPoint.angleTo(*pos);
        double radius = 0.0;
        if (entity->getEntityType() == DM::EntityArc)
        {
            radius = ((DmArc*)entity)->getRadius();
        }
        else if (entity->getEntityType() == DM::EntityCircle)
        {
            radius = ((DmCircle*)entity)->getRadius();
        }

        edata->endPoint.setPolar(radius, angle);
        edata->endPoint += data->definitionPoint;
        edata->leader =  dist - radius;
        //edata->isInside = dist < radius;
    }
}

void ActionDimRadial::mouseMoveEvent(QMouseEvent* e)
{
    switch (getStatus())
    {
        case SetPos:
            if (entity)
            {
                *pos = snapPoint(e);

                preparePreview();

                DmDimRadial* d = new DmDimRadial(
                            preview->getEntityContainer(), *data, *edata);
                d->setDocument(pDocument);
                d->update();

                deletePreview();
                preview->addEntity(d);
                d->update();
                drawPreview();
            }
            break;

        default:
            break;
    }
}

void ActionDimRadial::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        switch (getStatus())
        {
            case SetEntity: {
                DmEntity* en = catchEntity(e, DM::ResolveAll);
                if (en)
                {
                    if (en->getEntityType() == DM::EntityArc || en->getEntityType() == DM::EntityCircle)
                    {
                        entity = en;
                        if (entity->getEntityType() == DM::EntityArc)
                        {
                            data->definitionPoint = static_cast<DmArc*>(entity)->getCenter();
                        }
                        else if (entity->getEntityType() == DM::EntityCircle)
                        {
                            data->definitionPoint = static_cast<DmCircle*>(entity)->getCenter();
                        }
                        docView->moveRelativeZero(data->definitionPoint);
                        setStatus(SetPos);
                    }
                    else
                    {
                        GUIDIALOGFACTORY->commandMessage(
                                    tr("Not a circle or arc entity"));
                    }
                }
            }
            break;

            case SetPos: {
                GuiCoordinateEvent ce(snapPoint(e));
                coordinateEvent(&ce);
            }
            break;

            default:
                break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        deletePreview();
        init(getStatus() - 1);
    }
}

void ActionDimRadial::coordinateEvent(GuiCoordinateEvent* e)
{
    if (!e)
    {
        return;
    }

    switch (getStatus())
    {
        case SetPos:
            *pos = e->getCoordinate();
            trigger();
            reset();
            setStatus(SetEntity);
            break;

        default:
            break;
    }
}

void ActionDimRadial::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
        return;
    }

    // setting new text label:
    if (getStatus() == SetText)
    {
        setText(c);
        //GUIDIALOGFACTORY->requestOptions(this, true, true);
        docView->enableCoordinateInput();
        setStatus(lastStatus);
        return;
    }

    // command: text
    if (checkCommand("text", c))
    {
        lastStatus = (Status)getStatus();
        docView->disableCoordinateInput();
        setStatus(SetText);
    }

    // setting angle
    if (getStatus() == SetPos)
    {
        bool ok;
        double a = Math2d::eval(c, &ok);
        if (ok)
        {
            pos->setPolar(1.0, Math2d::deg2rad(a));
            *pos += data->definitionPoint;
            trigger();
            reset();
            setStatus(SetEntity);
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        return;
    }
}

QStringList ActionDimRadial::getAvailableCommands()
{
    QStringList cmd;

    switch (getStatus())
    {
        case SetEntity:
        case SetPos:
            cmd += command("text");
            break;

        default:
            break;
    }

    return cmd;
}

void ActionDimRadial::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetEntity:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select arc or circle entity"), tr("Cancel"));
            break;
        case SetPos:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify dimension line position or enter angle:"), tr("Cancel"));
            break;
        case SetText:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Enter dimension text:"), "");
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void ActionDimRadial::showOptions()
{
    ActionInterface::showOptions();
    //GUIDIALOGFACTORY->requestOptions(this, true);
}

void ActionDimRadial::hideOptions()
{
    ActionInterface::hideOptions();
    //GUIDIALOGFACTORY->requestOptions(this, false);
}

// EOF
