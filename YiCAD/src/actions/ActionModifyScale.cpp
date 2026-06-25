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


/// @file ActionModifyScale.cpp
/// @brief 缩放实体交互命令实现

#include "ActionModifyScale.h"

#include <QAction>
#include <QMouseEvent>

#include "Debug.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Modification.h"
#include "Preview.h"
#include "Transaction.h"
#include "GuiCommandEvent.h"
#include "Math2d.h"

/// @brief 缩放计算中使用的常量因子
constexpr double SCALE_DISTANCE_FACTOR = 2.0;

struct ActionModifyScale::Points
{
    ScaleData data;
    DmVector referencePoint;
};

ActionModifyScale::ActionModifyScale(DmDocument* doc, GuiDocumentView* docView) :
    PreviewActionInterface("Scale Entities", doc, docView), pPoints(new Points{})
{
    actionType = DM::ActionModifyScale;
    initBox();
}

ActionModifyScale::~ActionModifyScale() = default;

/// @brief 计算选中实体的包围框，获取宽高较大值
void ActionModifyScale::initBox()
{
    DmEntityContainer ec(nullptr, false);
    auto table = pDocument->getEntityTable();

    for (auto e : *table)
    {
        if (e->isSelected())
        {
            ec.addEntity(e);
        }
    }

    DmVector deltaXY = ec.getMax() - ec.getMin();
    m_boxRange = (deltaXY.x > deltaXY.y) ? deltaXY.x : deltaXY.y;
}

void ActionModifyScale::init(int status)
{
    ActionInterface::init(status);
}

/// @brief 执行缩放操作
void ActionModifyScale::trigger()
{
    Transaction t(tr("Scale").toStdString(), pDocument);
    t.start();

    auto table = pDocument->getEntityTable();
    DmVector scaleVec(pPoints->data.factor, pPoints->data.factor);

    for (auto e : *table)
    {
        if (e->isSelected())
        {
            table->startModify(e);
            e->scale(pPoints->referencePoint, scaleVec);
        }
    }

    t.commit();

    GUIDIALOGFACTORY->updateSelectionWidget(pDocument->getEntityTable()->countSelect());
}

void ActionModifyScale::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = snapPoint(e);

    switch (getStatus())
    {
        case SetReferencePoint:
        {
            pPoints->referencePoint = mouse;
        }
            break;

        case SetScale:
        {
            double dist = mouse.distanceTo(pPoints->referencePoint);
            pPoints->data.factor = SCALE_DISTANCE_FACTOR * dist / m_boxRange;
            DmVector scaleVec(pPoints->data.factor, pPoints->data.factor);
            deletePreview();
            preview->addSelectionFromDocument();
            preview->getEntityContainer()->scale(pPoints->referencePoint, scaleVec);
            drawPreview();
        }
            break;

        default:
            break;
    }
}

void ActionModifyScale::mouseReleaseEvent(QMouseEvent* e)
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
    else
    {
        // 其他按钮不做处理
    }
}

void ActionModifyScale::coordinateEvent(GuiCoordinateEvent* e)
{
    DmVector mouse = e->getCoordinate();

    switch (getStatus())
    {
        case SetReferencePoint:
        {
            pPoints->referencePoint = mouse;
            pPoints->data.referencePoint = mouse;
            setStatus(SetScale);
        }
            break;

        case SetScale:
        {
            double dist = mouse.distanceTo(pPoints->referencePoint);
            pPoints->data.factor = SCALE_DISTANCE_FACTOR * dist / m_boxRange;
            trigger();
            finish();
        }
            break;

        default:
            break;
    }
}

void ActionModifyScale::commandEvent(GuiCommandEvent* e)
{
    if (getStatus() == SetScale)
    {
        QString c = e->getCommand().toLower();
        bool ok = false;
        double r = Math2d::eval(c, &ok);

        if (ok && (r > 0))
        {
            pPoints->data.factor = r;
        }
        else
        {
            GUIDIALOGFACTORY->updateMouseWidget(tr("Input invalid"), tr("Back"));
        }

        e->accept();

        // 结束命令
        trigger();
        finish();
    }
    else
    {
        e->accept();
    }
}

void ActionModifyScale::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetReferencePoint:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify reference point"), tr("Cancel"));
            break;

        case SetScale:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Input scale"), tr("Cancel"));
            break;

        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void ActionModifyScale::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}
