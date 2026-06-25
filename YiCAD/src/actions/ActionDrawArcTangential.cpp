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


/// @file ActionDrawArcTangential.cpp
/// @brief 通过基实体和终止角度绘制切向圆弧的交互动作类实现

#include <cmath>
#include "ActionDrawArcTangential.h"

#include <QAction>
#include <QMouseEvent>

#include "Debug.h"
#include "DmArc.h"
#include "GeometryMethods.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Math2d.h"
#include "Preview.h"
#include "Transaction.h"
#include "ui_UIArcTangentialOptions.h"

namespace
{
    constexpr double DEFAULT_LOCK_RADIUS = 100.0; ///< 默认锁定半径
}

ActionDrawArcTangential::ActionDrawArcTangential(DmDocument* doc, GuiDocumentView* docView)
    : PreviewActionInterface("Draw arcs tangential", doc, docView)
    , point(new DmVector{}), tempArc(new DmArc{})
    , m_bLockAngle(false), m_dLockAngle(0.0)
    , m_bLockRadius(false), m_dLockRadius(DEFAULT_LOCK_RADIUS)
{
    actionType = DM::ActionDrawArcTangential;
    reset();
}

ActionDrawArcTangential::~ActionDrawArcTangential() = default;

void ActionDrawArcTangential::reset()
{
    baseEntity   = nullptr;
    isStartPoint = false;
    *point       = {};
}

void ActionDrawArcTangential::init(int status)
{
    PreviewActionInterface::init(status);

    reset();
}

void ActionDrawArcTangential::trigger()
{
    PreviewActionInterface::trigger();

    if (!(point->valid && baseEntity))
    {
        return;
    }

    preparePreview();
    if (!tempArc->getData().isValid())
    {
        GUIDIALOGFACTORY->commandMessage(tr("Invalid input!"));
    }
    else
    {
        Transaction t(tr("Create ArcTangential").toStdString(), pDocument);
        t.start();
        DmArc* arc = new DmArc(nullptr, tempArc->getData());
        arc->setDocument(pDocument);
        pDocument->getEntityTable()->add(arc);
        t.commit();

        docView->moveRelativeZero(arc->getCenter());
    }
    init(SetBaseEntity);
}

void ActionDrawArcTangential::preparePreview()
{
    if (baseEntity && point->valid)
    {
        DmVector startPoint;
        double direction = 0.0;
        if (isStartPoint)
        {
            startPoint = baseEntity->getStartpoint();
            direction  = Math2d::correctAngle(baseEntity->getDirection1() + M_PI);
        }
        else
        {
            startPoint = baseEntity->getEndpoint();
            direction  = Math2d::correctAngle(baseEntity->getDirection2() + M_PI);
        }

        bool res = false;
        double radius = 0.0, startAngle = 0.0, endAngle = 0.0;
        DmVector normal(true), center(true);
        // 角度半径均未锁定，由鼠标点及切点为弦生成圆弧
        if (!m_bLockAngle && !m_bLockRadius)
        {
            res = GeometryMethods::createArcInfoTangentialFree(startPoint, DmVector(direction), *point, center, normal, radius, startAngle, endAngle);
        }
        // 仅角度锁定
        else if (m_bLockAngle && !m_bLockRadius)
        {
            res = GeometryMethods::createArcInfoTangentialLockAngle(startPoint, DmVector(direction), *point, m_dLockAngle, center, normal, radius, startAngle, endAngle);
        }
        // 仅半径锁定
        else if (!m_bLockAngle && m_bLockRadius)
        {
            res = GeometryMethods::createArcInfoTangentialLockRadius(startPoint, DmVector(direction), *point, m_dLockRadius, center, normal, radius, startAngle, endAngle);
        }
        // 锁定角度及半径
        else
        {
            res = GeometryMethods::createArcInfoTangentialLockRadiusAngle(startPoint, DmVector(direction), *point, m_dLockRadius, m_dLockAngle, center, normal, radius, startAngle, endAngle);
        }
        if (res)
        {
            DmArc arc(nullptr, ArcData(center, normal, radius, startAngle, endAngle));
            tempArc->setData(arc.getData());
            double endAngleNormal = arc.getEndAngleNormal();
            GUIDIALOGFACTORY->updateArcTangentialOptions(radius, m_bLockRadius, Math2d::rad2deg(endAngleNormal), m_bLockAngle);
        }
        else
        {
            tempArc->setRadius(0.0); // 设为无效
        }
    }
}

void ActionDrawArcTangential::mouseMoveEvent(QMouseEvent* e)
{
    if (getStatus() == SetEndAngle)
    {
        *point = snapPoint(e);
        updatePreview();
    }
}

void ActionDrawArcTangential::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        switch (getStatus())
        {
            case SetBaseEntity:
            {
                DmVector coord   = docView->toGraph(e->x(), e->y());
                DmEntity* entity = catchEntity(coord, DM::ResolveAll);
                if (entity)
                {
                    if (entity->getEntityType() == DM::EntityArc
                        || entity->getEntityType() == DM::EntityLine
                        || entity->getEntityType() == DM::EntityPolyline)
                    {
                        if (!entity->isContainer())
                        {
                            baseEntity = static_cast<DmAtomicEntity*>(entity);
                            if (baseEntity->getStartpoint().distanceTo(coord) < baseEntity->getEndpoint().distanceTo(coord))
                            {
                                isStartPoint = true;
                            }
                            else
                            {
                                isStartPoint = false;
                            }
                            setStatus(SetEndAngle);
                            updateMouseButtonHints();
                        }
                    }
                    else
                    {
                        GUIDIALOGFACTORY->commandMessage(tr("This type does not support tangent arcs!"));
                    }
                }
            }
                break;

            case SetEndAngle:
            {
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

void ActionDrawArcTangential::coordinateEvent(GuiCoordinateEvent* e)
{
    if (e == nullptr)
    {
        return;
    }
    DmVector mouse = e->getCoordinate();

    switch (getStatus())
    {
        case SetBaseEntity:
            break;

        case SetEndAngle:
            *point = mouse;
            trigger();
            break;

        default:
            break;
    }
}

void ActionDrawArcTangential::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
        return;
    }
}

QStringList ActionDrawArcTangential::getAvailableCommands()
{
    QStringList cmd;
    return cmd;
}

void ActionDrawArcTangential::showOptions()
{
    ActionInterface::showOptions();

    GUIDIALOGFACTORY->requestOptions(this, true);
    updateMouseButtonHints();
}

void ActionDrawArcTangential::hideOptions()
{
    ActionInterface::hideOptions();

    GUIDIALOGFACTORY->requestOptions(this, false);
}

void ActionDrawArcTangential::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetBaseEntity:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify base entity"), tr("Cancel"));
            break;
        case SetEndAngle:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify end point"), tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void ActionDrawArcTangential::updateMouseCursor()
{
    docView->setMouseCursor(DM::SelectCursor);
}

void ActionDrawArcTangential::updatePreview()
{
    if (getStatus() == SetEndAngle)
    {
        preparePreview();
        if (tempArc->getData().isValid())
        {
            DmArc* arc = new DmArc(preview->getEntityContainer(), tempArc->getData());
            arc->setDocument(pDocument);
            deletePreview();
            preview->addEntity(arc);
            drawPreview();
        }
    }
}

double ActionDrawArcTangential::getRadius() const
{
    return tempArc->getRadius();
}

double ActionDrawArcTangential::getAngle() const
{
    return tempArc->getAngleLength();
}

bool ActionDrawArcTangential::isLockRadius() const
{
    return m_bLockRadius;
}

void ActionDrawArcTangential::setIsLockRadius(bool lock)
{
    m_bLockRadius = lock;
}

double ActionDrawArcTangential::lockRadius() const
{
    return m_dLockRadius;
}

void ActionDrawArcTangential::setLockRadius(double radius)
{
    m_dLockRadius = radius;
}

bool ActionDrawArcTangential::isLockAngle() const
{
    return m_bLockAngle;
}

void ActionDrawArcTangential::setIsLockAngle(bool lock)
{
    m_bLockAngle = lock;
}

double ActionDrawArcTangential::lockAngle() const
{
    return m_dLockAngle;
}

void ActionDrawArcTangential::setLockAngle(double angle)
{
    m_dLockAngle = angle;
}
