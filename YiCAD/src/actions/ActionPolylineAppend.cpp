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


/// @file ActionPolylineAppend.cpp
/// @brief 多段线追加节点操作实现

#include <QAction>
#include "ActionPolylineAppend.h"

#include <QMouseEvent>

#include "Commands.h"
#include "Debug.h"
#include "DmArc.h"
#include "DmLine.h"
#include "DmPolyline.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Preview.h"
#include "Transaction.h"

/// @brief 构造函数，初始化追加节点操作
/// @param [in] doc 文档指针
/// @param [in] docView 文档视图指针
ActionPolylineAppend::ActionPolylineAppend(DmDocument* doc, GuiDocumentView* docView) :
    PreviewActionInterface("Append polyline node", doc, docView)
    , originalPolyline(nullptr)
    , prepend(false)
{
    actionType = DM::ActionPolylineAppend;
}

/// @brief 执行追加节点操作，将累积的数据写入原始多段线
void ActionPolylineAppend::trigger()
{
    PreviewActionInterface::trigger();

    Transaction t(tr("Append polyline point").toStdString(), pDocument);
    t.start();
    pDocument->getEntityTable()->startModify(originalPolyline);
    originalPolyline->setData(data);
    originalPolyline->update();
    t.commit();

    deleteSnapper();
    docView->moveRelativeZero(DmVector(0.0, 0.0));
    drawSnapper();
    finish();
}

/// @brief 处理鼠标释放事件，选择多段线或设置追加点
/// @param [in] e 鼠标事件指针
void ActionPolylineAppend::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        if (getStatus() == SetStartpoint)
        {
            DmEntity* catchEnt = catchEntity(e);
            if (!catchEnt)
            {
                GUIDIALOGFACTORY->commandMessage(tr("No Entity found."));
                return;
            }
            else if (catchEnt->getEntityType() != DM::EntityPolyline)
            {
                GUIDIALOGFACTORY->commandMessage(tr("Entity must be a polyline."));
                return;
            }
            else
            {
                DmPolyline* poly = static_cast<DmPolyline*>(catchEnt);
                if (poly->isClosed())
                {
                    GUIDIALOGFACTORY->commandMessage(tr("Can not append nodes in a closed polyline."));
                    return;
                }

                originalPolyline = poly;
                prepend = false;

                DmVector start = poly->getStartpoint();
                DmVector end = poly->getEndpoint();
                DmVector mouse = snapPoint(e);
                prepend = start.distanceTo(mouse) < end.distanceTo(mouse);

                setStatus(SetNextPoint);
                docView->moveRelativeZero(prepend ? start : end);
                //updateMouseButtonHints();
            }
        }
        else if (getStatus() == SetNextPoint)
        {
            GuiCoordinateEvent ce(snapPoint(e));
            coordinateEvent(&ce);
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        finish();
    }
}

/// @brief 处理鼠标移动事件，预览追加的节点
/// @param [in] e 鼠标事件指针
void ActionPolylineAppend::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = snapPoint(e);
    if (getStatus() == SetNextPoint)
    {
        deletePreview();

        auto& dataRef = originalPolyline->getDataConstRef();
        if (!prepend)
        {
            double lastBulge = dataRef.getBulgeAt(dataRef.getBulgesCount() - 1);
            double lastWeight1 = 0.0;
            double lastWeight2 = 0.0;
            dataRef.getLineWeightsAt(dataRef.getBulgesCount() - 1, lastWeight1, lastWeight2);
            DmVector lastVertex = dataRef.getVertexAt(dataRef.getVertexCount() - 1);

            std::vector<DmEntity*> ents;
            DmPolyline::getEntitiesByInfo(lastVertex, mouse, lastBulge, lastWeight1, lastWeight2, ents);
            for (auto ent : ents)
            {
                ent->setParent(preview->getEntityContainer());
                ent->setDocument(pDocument);
                preview->addEntity(ent);
            }
        }
        else
        {
            double firstBulge = dataRef.getBulgeAt(0);
            double firstWeight1 = 0.0;
            double firstWeight2 = 0.0;
            dataRef.getLineWeightsAt(0, firstWeight1, firstWeight2);
            DmVector firstVertex = dataRef.getVertexAt(0);

            std::vector<DmEntity*> ents;
            DmPolyline::getEntitiesByInfo(mouse, firstVertex, firstBulge, firstWeight1, firstWeight2, ents);
            for (auto ent : ents)
            {
                ent->setParent(preview->getEntityContainer());
                ent->setDocument(pDocument);
                preview->addEntity(ent);
            }
        }

        drawPreview();
    }
}

/// @brief 处理坐标输入事件，根据坐标追加节点
/// @param [in] e 坐标事件指针
void ActionPolylineAppend::coordinateEvent(GuiCoordinateEvent* e)
{
    if (!e)
    {
        return;
    }

    DmVector mouse = e->getCoordinate();

    switch (getStatus())
    {
        case SetStartpoint:
            // 手动输入时不做任何处理
            updateMouseButtonHints();
            break;

        case SetNextPoint:
        {
            auto data = originalPolyline->getData();
            // 后面追加
            if (!prepend)
            {
                double lastBulge = data.getBulgeAt(data.getBulgesCount() - 1);
                double lastWeight1 = 0.0;
                double lastWeight2 = 0.0;
                data.getLineWeightsAt(data.getBulgesCount() - 1, lastWeight1, lastWeight2);
                data.appendVertex(mouse);
                data.appendBulge(lastBulge);
                data.appendLineWeight(lastWeight1, lastWeight2);
            }
            // 前面追加
            else
            {
                double firstBulge = data.getBulgeAt(0);
                double firstWeight1 = 0.0;
                double firstWeight2 = 0.0;
                data.getLineWeightsAt(0, firstWeight1, firstWeight2);
                data.insertVertex(0, mouse);
                data.insertBulge(0, firstBulge);
                data.insertLineWeight(0, firstWeight1, firstWeight2);
            }

            this->data = data;
            trigger();
        }
            break;

        default:
            break;
    }
}

/// @brief 更新鼠标按钮提示文本
void ActionPolylineAppend::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetStartpoint:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify the polyline somewhere near the beginning or end "
                                                   "point"),
                                                tr("Cancel"));
            break;

        case SetNextPoint:
        {
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify next point"), tr("Back"));
        }
            break;

        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}
