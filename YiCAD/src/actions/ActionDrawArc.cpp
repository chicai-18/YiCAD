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


/// @file ActionDrawArc.cpp
/// @brief 通过圆心、半径和起始/终止角度绘制圆弧的交互动作类实现

#include <cmath>
#include "ActionDrawArc.h"

#include <QAction>
#include <QMouseEvent>

#include "Commands.h"
#include "Debug.h"
#include "DmArc.h"
#include "DmCircle.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Math2d.h"
#include "Preview.h"
#include "Transaction.h"

namespace
{
    constexpr double DEFAULT_ARC_ANGLE = M_PI / 3.0; ///< 默认圆弧角度（60度）
}

ActionDrawArc::ActionDrawArc(DmDocument* doc, GuiDocumentView* docView)
    : PreviewActionInterface("Draw arcs", doc, docView)
    , tempArc(new DmArc())
{
    actionType = DM::ActionDrawArc;

    reset();
}

ActionDrawArc::~ActionDrawArc() = default;

void ActionDrawArc::reset()
{
    tempArc.reset(new DmArc());
}

void ActionDrawArc::init(int status)
{
    PreviewActionInterface::init(status);

    reset();
}

void ActionDrawArc::trigger()
{
    PreviewActionInterface::trigger();

    Transaction t(tr("Create Arc").toStdString(), pDocument);
    t.start();
    DmArc* arc = new DmArc(nullptr, tempArc->getData());
    arc->setDocument(pDocument);
    // 如果法向为负，切换为正的
    if (arc->isClockwise())
    {
        arc->setClockwise(!arc->isClockwise());
    }
    pDocument->getEntityTable()->add(arc);
    t.commit();

    docView->moveRelativeZero(arc->getCenter());

    if (getSnapMode()->restriction == DM::RestrictOrthogonal)
    {
        setStatus(-1);
    }
    else
    {
        setStatus(SetCenter);
    }
    reset();
}

void ActionDrawArc::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = snapPoint(e);
    switch (getStatus())
    {
        case SetCenter:
            tempArc->setCenter(mouse);
            break;

        case SetRadius:
            if (tempArc->getCenter().valid)
            {
                tempArc->setRadius(tempArc->getCenter().distanceTo(mouse));
                deletePreview();
                auto c = new DmCircle(preview->getEntityContainer(), { tempArc->getCenter(), tempArc->getRadius() });
                c->setDocument(pDocument);
                preview->addEntity(c);
                drawPreview();
            }
            break;

        case SetAngle1:
        {
            setStartAngle(mouse);
            double endAngle = Math2d::correctAngle(tempArc->getStartAngle() + DEFAULT_ARC_ANGLE);
            tempArc->setEndAngle(endAngle);
            deletePreview();
            auto a = new DmArc(preview->getEntityContainer(), tempArc->getData());
            a->setDocument(pDocument);
            preview->addEntity(a);
            drawPreview();
        }
            break;

        case ArcAngle:
        {
            setEndAngle(mouse);
            deletePreview();
            auto a = new DmArc(preview->getEntityContainer(), tempArc->getData());
            a->setDocument(pDocument);
            preview->addEntity(a);
            drawPreview();
        }
            break;

        default:
            break;
    }
}

void ActionDrawArc::mouseReleaseEvent(QMouseEvent* e)
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

void ActionDrawArc::coordinateEvent(GuiCoordinateEvent* e)
{
    if (!e)
    {
        return;
    }

    DmVector mouse = e->getCoordinate();

    switch (getStatus())
    {
        case SetCenter:
            tempArc->setCenter(mouse);
            docView->moveRelativeZero(mouse);
            setStatus(SetRadius);
            break;

        case SetRadius:
            if (tempArc->getCenter().valid)
            {
                tempArc->setRadius(tempArc->getCenter().distanceTo(mouse));
            }
            setStatus(SetAngle1);
            break;

        case SetAngle1:
        {
            setStartAngle(mouse);
            setStatus(ArcAngle);
        }
            break;

        case ArcAngle:
        {
            setEndAngle(mouse);
            trigger();
            finishOrthogonal(); // 干啥用的？有效果吗？上面trigger()不是实现了吗？
        }
            break;

        default:
            break;
    }
}

void ActionDrawArc::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (COMMANDS->checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus())
    {
        case SetRadius:
        {
            bool ok = false;
            double r = Math2d::eval(c, &ok);
            if (ok)
            {
                tempArc->setRadius(r);
                setStatus(SetAngle1);
                e->accept();
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
            break;

        case SetAngle1:
        {
            bool ok = false;
            double a = Math2d::eval(c, &ok);
            if (ok)
            {
                double a_r = Math2d::deg2rad(a);
                if (tempArc->isClockwise())
                {
                    tempArc->setStartAngle(Math2d::correctAngle(M_PI - a_r));
                }
                else
                {
                    tempArc->setStartAngle(a_r);
                }
                e->accept();
                setStatus(ArcAngle);
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
            break;

        case ArcAngle:
        {
            bool ok = false;
            double a = Math2d::eval(c, &ok);
            if (ok)
            {
                double a_r = Math2d::deg2rad(a); // 间隔角度
                double startAngle = tempArc->getStartAngle();
                tempArc->setEndAngle(Math2d::correctAngle(startAngle + a_r));
                e->accept();
                trigger();
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
            break;

        default:
            break;
    }
}

void ActionDrawArc::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetCenter:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify center"), tr("Cancel"));
            break;
        case SetRadius:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify radius"), tr("Back"));
            break;
        case SetAngle1:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify start angle:"), tr("Back"));
            break;
        case ArcAngle:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify arc angle"), tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void ActionDrawArc::showOptions()
{
    ActionInterface::showOptions();

    GUIDIALOGFACTORY->requestOptions(this, true);
}

void ActionDrawArc::hideOptions()
{
    ActionInterface::hideOptions();

    GUIDIALOGFACTORY->requestOptions(this, false);
}

void ActionDrawArc::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}

bool ActionDrawArc::isClockwise() const
{
    return tempArc->isClockwise();
}

void ActionDrawArc::setClockwise(bool clockwise)
{
    bool oldClockwise = isClockwise();
    tempArc->setClockwise(clockwise);

    // 顺逆时针发生变化
    if (oldClockwise != clockwise)
    {
        // 在设置终止角度时，起始终止角度发生变换，要切换回来
        if (getStatus() == ArcAngle)
        {
            tempArc->switchStartEndAngle();
        }
    }
}

void ActionDrawArc::setStartAngle(const DmVector& mouse)
{
    double angle = tempArc->getCenter().angleTo(mouse);
    if (tempArc->isClockwise())
    {
        tempArc->setStartAngle(Math2d::correctAngle(-angle + M_PI));
    }
    else
    {
        tempArc->setStartAngle(angle);
    }
}

void ActionDrawArc::setEndAngle(const DmVector& mouse)
{
    double angle = tempArc->getCenter().angleTo(mouse);
    if (tempArc->isClockwise())
    {
        tempArc->setEndAngle(Math2d::correctAngle(-angle + M_PI));
    }
    else
    {
        tempArc->setEndAngle(angle);
    }
}
