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


/// @file ActionDrawSplinePoints.cpp
/// @brief ActionDrawSplinePoints 类实现——通过控制点绘制样条曲线

#include <QAction>
#include "ActionDrawSplinePoints.h"

#include <QMouseEvent>

#include "Commands.h"
#include "Debug.h"
#include "DmPoint.h"
#include "DmLine.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Preview.h"
#include "Transaction.h"

namespace
{
    /// @brief 默认样条曲线阶数（三次样条）
    constexpr int DEFAULT_SPLINE_DEGREE = 3;

    /// @brief 允许执行撤消操作的最小控制点数
    constexpr int MIN_POINTS_FOR_UNDO = 2;

    /// @brief 允许执行闭合操作的最小控制点数
    constexpr int MIN_POINTS_FOR_CLOSE = 3;
}

ActionDrawSplinePoints::ActionDrawSplinePoints(
    DmDocument* doc, GuiDocumentView* docView)
    : PreviewActionInterface("Draw spline points", doc, docView)
    , pPoints(new Points{})
{
    actionType = DM::ActionDrawSplinePoints;
    setName("DrawSplinePoints");
}

ActionDrawSplinePoints::~ActionDrawSplinePoints() = default;

void ActionDrawSplinePoints::reset()
{
    pPoints->spline.reset();
    pPoints->undoBuffer.clear();
    pPoints->data.setDegree(DEFAULT_SPLINE_DEGREE);
    pPoints->data.setSplineType(ESplineType::eFitPoints);
}

void ActionDrawSplinePoints::init(int status)
{
    PreviewActionInterface::init(status);
    reset();
}

void ActionDrawSplinePoints::trigger()
{
    if (!pPoints->spline || !pPoints->spline->isValid())
    {
        return;
    }

    Transaction t(tr("Add spline points").toStdString(), pDocument);
    t.start();
    pPoints->spline->update();
    pDocument->getEntityTable()->add(pPoints->spline.release());
    t.commit();

    // 更新视图
    DmVector r = docView->getRelativeZero();
    docView->redraw();
    docView->moveRelativeZero(r);

    reset();
}

void ActionDrawSplinePoints::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = snapPoint(e);

    if (getStatus() == SetNextPoint && pPoints->spline)
    {
        deletePreview();
        DmSpline* tmpSpline =
            static_cast<DmSpline*>(pPoints->spline->clone());
        tmpSpline->setDocument(pDocument);
        tmpSpline->setParent(preview->getEntityContainer());
        fitPoints(tmpSpline, mouse);
        preview->addEntity(tmpSpline);

        for (auto const& v : tmpSpline->getControlPoints())
        {
            preview->addEntity(new DmPoint(nullptr, PointData(v)));
        }
        drawPreview();
    }
}

void ActionDrawSplinePoints::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        GuiCoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton)
    {
        if (getStatus() == SetNextPoint && pPoints->spline.get())
        {
            trigger();
        }
        init(getStatus() - 1);
    }
    else
    {
        // 其他鼠标按键暂不处理
    }
}

void ActionDrawSplinePoints::coordinateEvent(GuiCoordinateEvent* e)
{
    if (e == nullptr)
    {
        return;
    }

    DmVector mouse = e->getCoordinate();

    switch (getStatus())
    {
        case SetStartPoint:
            pPoints->undoBuffer.clear();
            pPoints->undoBuffer.emplace_back(mouse);
            if (!pPoints->spline.get())
            {
                pPoints->spline.reset(
                    new DmSpline(nullptr, pPoints->data));
                pPoints->spline->setDocument(pDocument);
                // 仅有一个控制点，暂不绘制样条曲线

                preview->addEntity(
                    new DmPoint(nullptr, PointData(mouse)));
            }
            setStatus(SetNextPoint);
            docView->moveRelativeZero(mouse);
            updateMouseButtonHints();
            break;
        case SetNextPoint:
            docView->moveRelativeZero(mouse);
            pPoints->undoBuffer.emplace_back(mouse);
            if (pPoints->spline.get())
            {
                fitPoints(pPoints->spline.get());

                deletePreview();
                DmSpline* tmpSpline =
                    static_cast<DmSpline*>(pPoints->spline->clone());
                tmpSpline->setDocument(pDocument);
                tmpSpline->setParent(preview->getEntityContainer());
                preview->addEntity(tmpSpline);
                drawPreview();
            }
            updateMouseButtonHints();
            break;
        default:
            break;
    }
}

void ActionDrawSplinePoints::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    switch (getStatus())
    {
        case SetStartPoint:
            if (checkCommand("help", c))
            {
                GUIDIALOGFACTORY->commandMessage(
                    msgAvailableCommands()
                    + getAvailableCommands().join(", "));
                return;
            }
            break;
        case SetNextPoint:
            if (checkCommand("undo", c))
            {
                undo();
                updateMouseButtonHints();
                return;
            }
            break;
        default:
            break;
    }
}

QStringList ActionDrawSplinePoints::getAvailableCommands()
{
    QStringList cmd;

    switch (getStatus())
    {
        case SetStartPoint:
            break;
        case SetNextPoint:
            if (pPoints->undoBuffer.size() >= MIN_POINTS_FOR_UNDO)
            {
                cmd += command("undo");
            }
            if (pPoints->undoBuffer.size() >= MIN_POINTS_FOR_CLOSE)
            {
                cmd += command("close");
            }
            break;
        default:
            break;
    }

    return cmd;
}

void ActionDrawSplinePoints::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetStartPoint:
            GUIDIALOGFACTORY->updateMouseWidget(
                tr("Specify first control point"), tr("Cancel"));
            break;
        case SetNextPoint:
        {
            QString msg = "";

            if (pPoints->undoBuffer.size() >= MIN_POINTS_FOR_CLOSE)
            {
                msg += COMMANDS->command("close");
                msg += "/";
            }
            if (pPoints->undoBuffer.size() > 0)
            {
                msg += COMMANDS->command("undo");
                GUIDIALOGFACTORY->updateMouseWidget(
                    tr("Specify next control point or [%1]")
                        .arg(msg),
                    tr("Back"));
            }
            else
            {
                GUIDIALOGFACTORY->updateMouseWidget(
                    tr("Specify next control point"), tr("Back"));
            }
        }
        break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void ActionDrawSplinePoints::showOptions()
{
    ActionInterface::showOptions();
    GUIDIALOGFACTORY->requestOptions(this, true);
}

void ActionDrawSplinePoints::hideOptions()
{
    ActionInterface::hideOptions();
    GUIDIALOGFACTORY->requestOptions(this, false);
}

void ActionDrawSplinePoints::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}

void ActionDrawSplinePoints::undo()
{
    if (!pPoints->spline.get())
    {
        GUIDIALOGFACTORY->commandMessage(
            tr("Cannot undo: Not enough entities defined yet."));
        return;
    }

    if (pPoints->undoBuffer.size() > 1)
    {
        pPoints->undoBuffer.pop_back();
        deletePreview();
        if (pPoints->spline)
        {
            fitPoints(pPoints->spline.get());
            DmSpline* tmpSpline =
                static_cast<DmSpline*>(pPoints->spline->clone());
            tmpSpline->setDocument(pDocument);
            tmpSpline->setParent(preview->getEntityContainer());
            preview->addEntity(tmpSpline);
            if (!pPoints->undoBuffer.empty())
            {
                DmVector v = pPoints->undoBuffer.back();
                docView->moveRelativeZero(v);
            }
        }
        drawPreview();
    }
    else
    {
        GUIDIALOGFACTORY->commandMessage(
            tr("Cannot undo: Not enough entities defined yet."));
    }
}

void ActionDrawSplinePoints::setClosed(bool c)
{
    pPoints->data.setIsClosed(c);
    if (pPoints->spline.get())
    {
        bool isOriginClosed = pPoints->spline->isClosed();
        if (isOriginClosed == c)
        {
            return;
        }
        pPoints->spline->setClosed(c);
        fitPoints(pPoints->spline.get());
    }
}

bool ActionDrawSplinePoints::isClosed()
{
    return pPoints->data.getIsClosed();
}

void ActionDrawSplinePoints::fitPoints(
    DmSpline* spline, DmVector dynamicPt /*= DmVector(false)*/)
{
    std::vector<DmVector> fitPts{pPoints->undoBuffer};
    if (dynamicPt.valid)
    {
        fitPts.emplace_back(dynamicPt);
    }
    if (spline->isClosed()) // 闭合曲线时，追加起始数据点以形成闭环
    {
        fitPts.emplace_back(fitPts.front());
    }
    spline->setFitPts(fitPts);
    spline->fit();
    spline->update();
}
