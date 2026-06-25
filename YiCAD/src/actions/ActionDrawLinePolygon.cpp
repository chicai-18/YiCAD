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


/// @file ActionDrawLinePolygon.cpp
/// @brief 中心-角点方式绘制正多边形 Action 实现

#include "ActionDrawLinePolygon.h"

#include <QAction>
#include <QMouseEvent>

#include "Debug.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Preview.h"
#include "DmLine.h"
#include "Transaction.h"

/// 圆周角常量 (2 * pi)
static constexpr double TWO_PI = 2.0 * M_PI;

struct ActionDrawLinePolygonCenCor::Points
{
    DmVector center; ///< 多边形中心点
    DmVector corner; ///< 多边形角点
};

ActionDrawLinePolygonCenCor::ActionDrawLinePolygonCenCor(DmDocument* doc, GuiDocumentView* docView) :
    PreviewActionInterface("Draw Polygons (Center,Corner)", doc, docView), pPoints(new Points{}), number(MIN_POLYGON_EDGES),
    lastStatus(Status::SetCenter)
{
    actionType = DM::ActionDrawLinePolygonCenCor;
}

ActionDrawLinePolygonCenCor::~ActionDrawLinePolygonCenCor() = default;

void ActionDrawLinePolygonCenCor::trigger()
{
    PreviewActionInterface::trigger();

    deletePreview();
    auto ls = createPolygon(nullptr, pPoints->center, pPoints->corner, number);
    if (!ls.empty())
    {
        Transaction t(tr("Create line center corner").toStdString(), pDocument);
        t.start();
        for (auto l : ls)
        {
            pDocument->getEntityTable()->add(l);
        }
        t.commit();
    }
}

void ActionDrawLinePolygonCenCor::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = snapPoint(e);

    switch (getStatus())
    {
        case SetCenter:
            break;

        case SetCorner:
            if (pPoints->center.valid)
            {
                pPoints->corner = mouse;
                deletePreview();
                createPolygon(preview->getEntityContainer(), pPoints->center, pPoints->corner, number);
                drawPreview();
            }
            break;

        default:
            break;
    }
}

void ActionDrawLinePolygonCenCor::mouseReleaseEvent(QMouseEvent* e)
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

void ActionDrawLinePolygonCenCor::coordinateEvent(GuiCoordinateEvent* e)
{
    if (!e)
    {
        return;
    }

    DmVector mouse = e->getCoordinate();

    switch (getStatus())
    {
        case SetCenter:
            pPoints->center = mouse;
            setStatus(SetCorner);
            docView->moveRelativeZero(mouse);
            break;

        case SetCorner:
            pPoints->corner = mouse;
            trigger();
            finishOrthogonal();
            break;

        default:
            break;
    }
}

void ActionDrawLinePolygonCenCor::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetCenter:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify center"), "");
            break;

        case SetCorner:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify a corner"), "");
            break;

        case SetNumber:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Enter number:"), "");
            break;

        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void ActionDrawLinePolygonCenCor::showOptions()
{
    ActionInterface::showOptions();

    GUIDIALOGFACTORY->requestOptions(this, true);
}

void ActionDrawLinePolygonCenCor::hideOptions()
{
    ActionInterface::hideOptions();

    GUIDIALOGFACTORY->requestOptions(this, false);
}

void ActionDrawLinePolygonCenCor::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus())
    {
        case SetCenter:
        case SetCorner:
            if (checkCommand("number", c))
            {
                deletePreview();
                lastStatus = (Status)getStatus();
                setStatus(SetNumber);
            }
            break;

        case SetNumber: {
            bool ok;
            int n = c.toInt(&ok);
            if (ok)
            {
                e->accept();
                if (n > 0 && n < MAX_POLYGON_EDGES)
                {
                    number = n;
                }
                else
                {
                    GUIDIALOGFACTORY->commandMessage(tr("Not a valid number. "
                                                        "Try 1..9999"));
                }
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            GUIDIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

        default:
            break;
    }
}

QStringList ActionDrawLinePolygonCenCor::getAvailableCommands()
{
    QStringList cmd;

    switch (getStatus())
    {
        case SetCenter:
        case SetCorner:
            cmd += command("number");
            break;
        default:
            break;
    }

    return cmd;
}

void ActionDrawLinePolygonCenCor::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}

/// @brief 根据中心点和角点创建正多边形的线段实体
/// @param container 实体容器，可为 nullptr
/// @param center 多边形中心点
/// @param corner 多边形角点
/// @param number 边数
/// @return 创建的线段实体列表
std::vector<DmLine*> ActionDrawLinePolygonCenCor::createPolygon(DmEntityContainer* container, const DmVector& center, const DmVector& corner, int number)
{
    // check given coords / number:
    if (!center.valid || !corner.valid || number < MIN_POLYGON_EDGES)
    {
        return {};
    }

    std::vector<DmLine*> ret;
    double const r = center.distanceTo(corner);
    double const angle0 = center.angleTo(corner);
    double const da = TWO_PI / number;

    for (int i = 0; i < number; ++i)
    {
        DmVector const& c0 = center + DmVector::polar(r, angle0 + i * da);
        DmVector const& c1 = center + DmVector::polar(r, angle0 + ((i + 1) % number) * da);

        DmLine* line = new DmLine(container, c0, c1);
        line->setDocument(pDocument);
        ret.emplace_back(line);

        if (container)
        {
            container->addEntity(line);
        }
    }

    return ret;
}
