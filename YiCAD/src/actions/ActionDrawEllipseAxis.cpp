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


/// @file ActionDrawEllipseAxis.cpp
/// @brief 通过中心和轴端点绘制椭圆/椭圆弧的交互动作实现

#include <cmath>
#include "ActionDrawEllipseAxis.h"

#include <QAction>
#include <QMouseEvent>

#include "Debug.h"
#include "DmEllipse.h"
#include "DmLine.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Math2d.h"
#include "Preview.h"
#include "Transaction.h"

namespace
{
    constexpr double ELLIPSE_DEFAULT_RATIO = 0.5;       ///< 默认短轴/长轴比
    constexpr double ELLIPSE_DEFAULT_ANGLE = 0.0;       ///< 默认角度（弧度）
    constexpr double ELLIPSE_FULL_CIRCLE = 2.0 * M_PI;  ///< 完整椭圆的角度范围
    constexpr double ELLIPSE_MAJOR_LENGTH = 1.0;        ///< 命令模式下的默认长轴长度
    constexpr double ELLIPSE_Z_COORD = 0.0;             ///< Z轴坐标分量
    constexpr double ELLIPSE_ANGLE_PREVIEW = 1.0;       ///< 角度预览增量
    constexpr double ELLIPSE_LINE_HALF = 2.0;           ///< 线长除半因子
}

struct ActionDrawEllipseAxis::Points
{
    DmVector center;          ///< 椭圆中心点
    DmVector m_vMajorP;       ///< 长轴端点向量
    double ratio{ ELLIPSE_DEFAULT_RATIO }; ///< 短轴/长轴比
    double startAngle{ ELLIPSE_DEFAULT_ANGLE }; ///< 起始角度
    double endAngle{ ELLIPSE_DEFAULT_ANGLE };   ///< 结束角度
    bool isArc{ false };      ///< 为 true 创建椭圆弧，为 false 创建完整椭圆
    DmVector mouse;           ///< 鼠标当前位置
};

ActionDrawEllipseAxis::ActionDrawEllipseAxis(DmDocument* doc, GuiDocumentView* docView, bool isArc) :
    PreviewActionInterface("Draw ellipse with axis", doc, docView),
    pPoints(new Points{{}, {}, ELLIPSE_DEFAULT_RATIO, ELLIPSE_DEFAULT_ANGLE,
            isArc ? ELLIPSE_FULL_CIRCLE : ELLIPSE_DEFAULT_ANGLE, isArc})
{
    actionType = isArc ? DM::ActionDrawEllipseArcAxis : DM::ActionDrawEllipseAxis;
}

ActionDrawEllipseAxis::~ActionDrawEllipseAxis() = default;

void ActionDrawEllipseAxis::init(int status)
{
    PreviewActionInterface::init(status);

    if (status == SetCenter)
    {
        pPoints->center = {};
    }
    if (status <= SetMajor)
    {
        pPoints->m_vMajorP = {};
    }
    if (status <= SetMinor)
    {
        pPoints->ratio = ELLIPSE_DEFAULT_RATIO;
    }
    if (status <= SetAngle1)
    {
        pPoints->startAngle = ELLIPSE_DEFAULT_ANGLE;
    }
    if (status <= SetAngle2)
    {
        pPoints->endAngle = ELLIPSE_DEFAULT_ANGLE;
    }
}

void ActionDrawEllipseAxis::trigger()
{
    PreviewActionInterface::trigger();

    EllipseData ed(pPoints->center, pPoints->m_vMajorP,
                   DmVector(ELLIPSE_Z_COORD, ELLIPSE_Z_COORD, ELLIPSE_MAJOR_LENGTH),
                   pPoints->ratio, !pPoints->isArc, pPoints->startAngle, pPoints->endAngle);
    DmEllipse* ellipse = new DmEllipse(nullptr, ed);
    ellipse->setDocument(pDocument);

    if (pPoints->ratio > 1.0)
    {
        ellipse->switchMajorMinor();
    }

    Transaction t(tr("Add ellipse").toStdString(), pDocument);
    t.start();
    pDocument->getEntityTable()->add(ellipse);
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
        setStatus(SetCenter);
    }
}

void ActionDrawEllipseAxis::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = snapPoint(e);
    pPoints->mouse = mouse;

    switch (getStatus())
    {
    case SetMajor:
        if (pPoints->center.valid)
        {
            deletePreview();
            EllipseData ed(pPoints->center, mouse - pPoints->center,
                           DmVector(ELLIPSE_Z_COORD, ELLIPSE_Z_COORD, ELLIPSE_MAJOR_LENGTH),
                           ELLIPSE_DEFAULT_RATIO, !pPoints->isArc,
                           ELLIPSE_DEFAULT_ANGLE,
                           pPoints->isArc ? ELLIPSE_FULL_CIRCLE : ELLIPSE_DEFAULT_ANGLE);
            auto ellipsePtr = new DmEllipse(preview->getEntityContainer(), ed);
            ellipsePtr->setDocument(pDocument);
            preview->addEntity(ellipsePtr);
            drawPreview();
        }
        break;

    case SetMinor:
        if (pPoints->center.valid && pPoints->m_vMajorP.valid)
        {
            deletePreview();
            DmLine line{pPoints->center - pPoints->m_vMajorP, pPoints->center + pPoints->m_vMajorP};
            double d = line.getDistanceToPoint(mouse);
            pPoints->ratio = d / (line.getLength() / ELLIPSE_LINE_HALF);
            EllipseData ed(pPoints->center, pPoints->m_vMajorP,
                           DmVector(ELLIPSE_Z_COORD, ELLIPSE_Z_COORD, ELLIPSE_MAJOR_LENGTH),
                           pPoints->ratio, !pPoints->isArc,
                           ELLIPSE_DEFAULT_ANGLE,
                           pPoints->isArc ? ELLIPSE_FULL_CIRCLE : ELLIPSE_DEFAULT_ANGLE);
            auto ellipsePtr = new DmEllipse(preview->getEntityContainer(), ed);
            ellipsePtr->setDocument(pDocument);
            preview->addEntity(ellipsePtr);
            drawPreview();
        }
        break;

    case SetAngle1:
        if (pPoints->center.valid && pPoints->m_vMajorP.valid)
        {
            deletePreview();

            DmVector m = mouse;
            m.rotate(pPoints->center, -pPoints->m_vMajorP.angle());
            DmVector v = m - pPoints->center;
            v.y /= pPoints->ratio;
            pPoints->startAngle = v.angle();

            preview->addEntity(new DmLine(nullptr, pPoints->center, mouse));
            EllipseData ed(pPoints->center, pPoints->m_vMajorP,
                           DmVector(ELLIPSE_Z_COORD, ELLIPSE_Z_COORD, ELLIPSE_MAJOR_LENGTH),
                           pPoints->ratio, false, pPoints->startAngle,
                           pPoints->startAngle + ELLIPSE_ANGLE_PREVIEW);
            auto ellipsePtr = new DmEllipse(preview->getEntityContainer(), ed);
            ellipsePtr->setDocument(pDocument);
            preview->addEntity(ellipsePtr);
            drawPreview();
        }
        break;

    case SetAngle2:
        if (pPoints->center.valid && pPoints->m_vMajorP.valid)
        {
            deletePreview();

            DmVector m = mouse;
            m.rotate(pPoints->center, -pPoints->m_vMajorP.angle());
            DmVector v = m - pPoints->center;
            v.y /= pPoints->ratio;
            pPoints->endAngle = v.angle();

            preview->addEntity(new DmLine(nullptr, pPoints->center, mouse));
            EllipseData ed(pPoints->center, pPoints->m_vMajorP,
                           DmVector(ELLIPSE_Z_COORD, ELLIPSE_Z_COORD, ELLIPSE_MAJOR_LENGTH),
                           pPoints->ratio, false, pPoints->startAngle, pPoints->endAngle);
            auto ellipsePtr = new DmEllipse(preview->getEntityContainer(), ed);
            ellipsePtr->setDocument(pDocument);
            preview->addEntity(ellipsePtr);
            drawPreview();
        }

    default:
        break;
    }
}

void ActionDrawEllipseAxis::mouseReleaseEvent(QMouseEvent* e)
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

void ActionDrawEllipseAxis::coordinateEvent(GuiCoordinateEvent* e)
{
    if (!e)
    {
        return;
    }

    DmVector const& mouse = e->getCoordinate();

    switch (getStatus())
    {
    case SetCenter:
        pPoints->center = mouse;
        docView->moveRelativeZero(mouse);
        setStatus(SetMajor);
        break;

    case SetMajor:
        pPoints->m_vMajorP = mouse - pPoints->center;
        setStatus(SetMinor);
        break;

    case SetMinor:
        {
            DmLine line{pPoints->center + pPoints->m_vMajorP, pPoints->center - pPoints->m_vMajorP};
            double d = line.getDistanceToPoint(mouse);
            pPoints->ratio = d / (line.getLength() / ELLIPSE_LINE_HALF);
            if (!pPoints->isArc)
            {
                trigger();
                finishOrthogonal();
            }
            else
            {
                setStatus(SetAngle1);
            }
        }
        break;

    case SetAngle1:
        {
            DmVector m = mouse;
            m.rotate(pPoints->center, -pPoints->m_vMajorP.angle());
            DmVector v = m - pPoints->center;
            v.y /= pPoints->ratio;
            pPoints->startAngle = v.angle();
            setStatus(SetAngle2);
        }
        break;

    case SetAngle2:
        {
            DmVector m = mouse;
            m.rotate(pPoints->center, -pPoints->m_vMajorP.angle());
            DmVector v = m - pPoints->center;
            v.y /= pPoints->ratio;
            pPoints->endAngle = v.angle();
            trigger();
            docView->getCurrentAction()->finish();
        }
        break;

    default:
        break;
    }
}

void ActionDrawEllipseAxis::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();
    bool ok = false;
    double m = Math2d::eval(c, &ok);

    if (checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus())
    {
    case SetMajor:
        {
            if (ok)
            {
                e->accept();
                DmVector vec(ELLIPSE_MAJOR_LENGTH, ELLIPSE_DEFAULT_ANGLE);
                if (pPoints->mouse)
                {
                    vec = (pPoints->mouse - pPoints->center).normalize();
                }
                pPoints->m_vMajorP = -vec * m;
                setStatus(SetMinor);

                // 重绘预览
                deletePreview();
                EllipseData ed(pPoints->center, pPoints->m_vMajorP,
                               DmVector(ELLIPSE_Z_COORD, ELLIPSE_Z_COORD, ELLIPSE_MAJOR_LENGTH),
                               ELLIPSE_DEFAULT_RATIO, !pPoints->isArc,
                               ELLIPSE_DEFAULT_ANGLE,
                               pPoints->isArc ? ELLIPSE_FULL_CIRCLE : ELLIPSE_DEFAULT_ANGLE);
                preview->addEntity(new DmEllipse(nullptr, ed));
                drawPreview();
            }
        }
        break;

    case SetMinor:
        {
            if (ok)
            {
                e->accept();
                pPoints->ratio = m / pPoints->m_vMajorP.magnitude();
                if (!pPoints->isArc)
                {
                    trigger();
                }
                else
                {
                    setStatus(SetAngle1);
                }
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
        break;

    case SetAngle1:
        {
            bool angleOk = false;
            double a = Math2d::eval(c, &angleOk);
            if (angleOk)
            {
                e->accept();
                pPoints->startAngle = Math2d::deg2rad(a);
                setStatus(SetAngle2);
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
        break;

    case SetAngle2:
        {
            bool angleOk = false;
            double a = Math2d::eval(c, &angleOk);
            if (angleOk)
            {
                e->accept();
                pPoints->endAngle = Math2d::deg2rad(a);
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

QStringList ActionDrawEllipseAxis::getAvailableCommands()
{
    QStringList cmd;
    return cmd;
}

void ActionDrawEllipseAxis::updateMouseButtonHints()
{
    switch (getStatus())
    {
    case SetCenter:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Specify ellipse center"), tr("Cancel"));
        break;

    case SetMajor:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Specify endpoint of major axis"), tr("Back"));
        break;

    case SetMinor:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Specify endpoint or length of minor axis:"), tr("Back"));
        break;

    case SetAngle1:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Specify start angle"), tr("Back"));
        break;

    case SetAngle2:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Specify end angle"), tr("Back"));
        break;

    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void ActionDrawEllipseAxis::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}
