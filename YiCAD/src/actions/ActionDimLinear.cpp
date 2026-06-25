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


/// @file ActionDimLinear.cpp
/// @brief 线性标注交互操作类实现

#include <cmath>
#include "ActionDimLinear.h"

#include <QAction>
#include <QMouseEvent>

#include "Debug.h"
#include "DmConstructionLine.h"
#include "DmDimLinear.h"
#include "DmLine.h"
#include "GeometryMethods.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Math2d.h"
#include "Preview.h"
#include <utility>
#include "Transaction.h"

ActionDimLinear::ActionDimLinear(DmDocument* doc, GuiDocumentView* docView,
                                  DM::ActionType /*type*/) :
    ActionDimension("Draw linear dimensions", doc, docView),
    edata(new DmDimLinearData(DmVector(0., 0.), DmVector(0., 0.))),
    lastStatus(SetExtPoint1)
{
    actionType = DM::ActionDimLinear;
    reset();
}

ActionDimLinear::~ActionDimLinear() = default;

void ActionDimLinear::reset()
{
    ActionDimension::reset();

    edata.reset(new DmDimLinearData(DmVector(false), DmVector(false)));

    GUIDIALOGFACTORY->requestOptions(this, true, true);
}

void ActionDimLinear::trigger()
{
    ActionDimension::trigger();

    preparePreview();

    DmDimLinear* dim = new DmDimLinear(nullptr, *data, *edata);
    dim->setDocument(pDocument);
    dim->update();

    Transaction t(tr("Add dimension linear").toStdString(), pDocument);
    t.start();
    pDocument->getEntityTable()->add(dim);
    t.commit();

    DmVector rz = docView->getRelativeZero();
    docView->moveRelativeZero(rz);
}

void ActionDimLinear::preparePreview()
{
    DmVector dirV = DmVector::polar(100., data->angle - M_PI_2);
    DmConstructionLine cl(nullptr,
                DmConstructionLineData(edata->extensionPoint2,
                            edata->extensionPoint2 + dirV));
    data->definitionPoint = cl.getNearestPointOnEntity(data->definitionPoint);
}

void ActionDimLinear::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = snapPoint(e);

    switch (getStatus())
    {
        case SetExtPoint1:
            break;

        case SetExtPoint2:
            if (edata->extensionPoint1.valid)
            {
                deletePreview();
                preview->addEntity(new DmLine(nullptr, edata->extensionPoint1, mouse));
                drawPreview();
            }
            break;

        case SetDefPoint:
            if (edata->extensionPoint1.valid && edata->extensionPoint2.valid)
            {
                deletePreview();
                data->definitionPoint = mouse;

                // 判断鼠标位置情况，确定标注方向（有2个相互垂直的方向）
                double angle = Math2d::correctAngle(data->angle);
                double angle90 = Math2d::correctAngle(data->angle + M_PI_2);
                DmVector angleDir(angle);
                DmVector angle90Dir(angle90);
                int angleSide1 = GeometryMethods::toLeftTest(
                            edata->extensionPoint1,
                            edata->extensionPoint1 + angleDir,
                            data->definitionPoint);
                int angleSide2 = GeometryMethods::toLeftTest(
                            edata->extensionPoint2,
                            edata->extensionPoint2 + angleDir,
                            data->definitionPoint);

                // 是否在angle方向，2个拾取点之间的无限长条区域内
                bool isInAngleRegion = angleSide1 * angleSide2 <= 0;
                int angle90Side1 = GeometryMethods::toLeftTest(
                            edata->extensionPoint1,
                            edata->extensionPoint1 + angle90Dir,
                            data->definitionPoint);
                int angle90Side2 = GeometryMethods::toLeftTest(
                            edata->extensionPoint2,
                            edata->extensionPoint2 + angle90Dir,
                            data->definitionPoint);

                // 是否在angle+90度方向，2个拾取点之间的无限长条区域内
                bool isInAngle90Region = angle90Side1 * angle90Side2 <= 0;

                if (!isInAngleRegion && isInAngle90Region)
                {
                    // data->angle代表标注线的方向，与长条角度区域相差PI/2
                    data->angle = Math2d::correctAngle(angle90 - M_PI_2);
                }
                else if (!isInAngle90Region && isInAngleRegion)
                {
                    data->angle = Math2d::correctAngle(angle - M_PI_2);
                }
                else
                {
                    // 其他情况不变
                }

                preparePreview();

                DmDimLinear* dim = new DmDimLinear(
                            nullptr, *data, *edata);
                preview->addEntity(dim);
                dim->update();
                drawPreview();
            }
            break;
    }
}

void ActionDimLinear::mouseReleaseEvent(QMouseEvent* e)
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

void ActionDimLinear::coordinateEvent(GuiCoordinateEvent* e)
{
    if (!e)
    {
        return;
    }

    DmVector pos = e->getCoordinate();

    switch (getStatus())
    {
        case SetExtPoint1:
            edata->extensionPoint1 = pos;
            docView->moveRelativeZero(pos);
            setStatus(SetExtPoint2);
            break;

        case SetExtPoint2:
            edata->extensionPoint2 = pos;
            docView->moveRelativeZero(pos);
            setStatus(SetDefPoint);
            break;

        case SetDefPoint:
            data->definitionPoint = pos;
            trigger();
            finishOrthogonal();
            reset();
            setStatus(SetExtPoint1);
            break;

        default:
            break;
    }
}

double ActionDimLinear::getAngle() const
{
    return data->angle;
}

void ActionDimLinear::setAngle(double a)
{
    data->angle = a;
}

void ActionDimLinear::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(
                    msgAvailableCommands()
                    + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus())
    {
        case SetText:
            setText(c);
            // TODO: GUI 选项请求暂时禁用，待确认后启用
            // GUIDIALOGFACTORY->requestOptions(this, true, true);
            docView->enableCoordinateInput();
            setStatus(lastStatus);
            break;

        case SetAngle:
        {
            bool ok;
            double a = Math2d::eval(c, &ok);
            if (ok)
            {
                setAngle(Math2d::deg2rad(a));
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(
                            tr("Not a valid expression"));
            }
            // TODO: GUI 选项请求暂时禁用，待确认后启用
            // GUIDIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

        default:
            lastStatus = (Status)getStatus();
            deletePreview();
            if (checkCommand("text", c))
            {
                docView->disableCoordinateInput();
                setStatus(SetText);
                return;
            }
            else if (checkCommand("angle", c))
            {
                setStatus(SetAngle);
            }
            break;
    }
}

QStringList ActionDimLinear::getAvailableCommands()
{
    QStringList cmd;

    switch (getStatus())
    {
        case SetExtPoint1:
        case SetExtPoint2:
        case SetDefPoint:
            cmd += command("text");
            cmd += command("angle");
            break;

        default:
            break;
    }

    return cmd;
}

void ActionDimLinear::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetExtPoint1:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify first extension line origin"), tr("Cancel"));
            break;
        case SetExtPoint2:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify second extension line origin"), tr("Back"));
            break;
        case SetDefPoint:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify dimension line location"), tr("Back"));
            break;
        case SetText:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Enter dimension text:"), "");
            break;
        case SetAngle:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Enter dimension line angle:"), "");
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void ActionDimLinear::showOptions()
{
    ActionInterface::showOptions();

    GUIDIALOGFACTORY->requestOptions(this, true, true);
}

void ActionDimLinear::hideOptions()
{
    ActionInterface::hideOptions();

    GUIDIALOGFACTORY->requestOptions(this, false);
}

// EOF
