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


/// @file ActionDrawCloudLineFree.cpp
/// @brief 自由绘制云线（修订云线）的交互动作实现

#include "ActionDrawCloudLineFree.h"
#include "DmPolyline.h"
#include "DmVector.h"
#include "DmArc.h"
#include "GuiDocumentView.h"
#include "Debug.h"
#include "Preview.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GeometryMethods.h"

#include <initializer_list>
#include <QMouseEvent>
#include <QList>
#include "Transaction.h"

namespace
{
    constexpr double CLOUD_CLOSE_DEVICE_THRESHOLD = 20.0;   ///< 闭合检测的设备坐标阈值
    constexpr double CLOUD_CLOSE_MAGNITUDE_THRESHOLD = 10.0; ///< 闭合检测的向量长度阈值
    constexpr double CLOUD_MIN_POINT_DISTANCE = 1.0;         ///< 点之间最小距离
    constexpr double CLOUD_LARGE_COORD = 1e10;               ///< 大坐标初始值
    constexpr double CLOUD_DEFAULT_BULGE = 0.5;              ///< 默认弧线凸起值
    constexpr int CLOUD_MIN_HISTORY_POINTS = 2;              ///< 最少历史点数
}

struct ActionDrawCloudLineFree::Points
{
    DmPolyline* polyline;
    std::vector<DmVector> history; ///< 自由拾取的所有点
    bool isClosed{ false };        ///< 是否已闭合
    bool isError{ false };         ///< 是否出错
};

ActionDrawCloudLineFree::ActionDrawCloudLineFree(DmDocument* doc, GuiDocumentView* docView):
    PreviewActionInterface("Draw cloud line free", doc, docView), pPoints(new Points{})
{
    actionType = DM::ActionCloudLineFree;
}

ActionDrawCloudLineFree::~ActionDrawCloudLineFree() = default;

void ActionDrawCloudLineFree::reset()
{
    pPoints->polyline = nullptr;
    pPoints->history.clear();
    pPoints->isClosed = false;
}

void ActionDrawCloudLineFree::init(int status /*= 0*/)
{
    reset();
    PreviewActionInterface::init(status);
}

void ActionDrawCloudLineFree::trigger()
{
    PreviewActionInterface::trigger();
    if (!pPoints->polyline)
    {
        return;
    }

    Transaction t(tr("Add cloud line").toStdString(), pDocument);
    t.start();
    pDocument->getEntityTable()->add(pPoints->polyline);
    t.commit();

    deleteSnapper();
    docView->moveRelativeZero(pPoints->polyline->getEndpoint());
    drawSnapper();
    pPoints->polyline = nullptr;
}

void ActionDrawCloudLineFree::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = snapPoint(e);
    if (getStatus() == SetEndPoint && !pPoints->isClosed)
    {
        DmVector first = pPoints->history.front();
        DmVector deltaVec = first - mouse;
        DmVector deviceVec = docView->toGui(first) - docView->toGui(mouse);
        if (pPoints->history.back().distanceTo(mouse) > CLOUD_MIN_POINT_DISTANCE)
        {
            pPoints->history.push_back(mouse);
        }

        // 计算历史点的范围
        double minX = CLOUD_LARGE_COORD, minY = CLOUD_LARGE_COORD;
        double maxX = -CLOUD_LARGE_COORD, maxY = -CLOUD_LARGE_COORD;
        for (auto& pt : pPoints->history)
        {
            if (pt.x < minX)
            {
                minX = pt.x;
            }
            if (pt.x > maxX)
            {
                maxX = pt.x;
            }
            if (pt.y < minY)
            {
                minY = pt.y;
            }
            if (pt.y > maxY)
            {
                maxY = pt.y;
            }
        }
        double deltaX = maxX - minX;
        double deltaDeviceX = docView->toGuiDX(deltaX);
        double deltaY = maxY - minY;
        double deltaDeviceY = docView->toGuiDX(deltaY);
        if ((deltaDeviceX > CLOUD_CLOSE_DEVICE_THRESHOLD || deltaDeviceY > CLOUD_CLOSE_DEVICE_THRESHOLD)
            && deviceVec.magnitude() < CLOUD_CLOSE_MAGNITUDE_THRESHOLD)    // 闭合并结束命令
        {
            pPoints->isClosed = true;
            pPoints->polyline = getPoly(nullptr);
            if (pPoints->polyline)
            {
                pDocument->specifyModifiedEntity(pPoints->polyline);
                trigger();
            }
            updateMouseButtonHints();
            init(getStatus() - 2);
        }
        drawPoly(preview->getEntityContainer());
    }
}

void ActionDrawCloudLineFree::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        DmVector snapped = snapPoint(e);
        GuiCoordinateEvent ce(snapped);
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton)
    {
        deletePreview();
        deleteSnapper();
        switch (getStatus())
        {
        default:
        case SetStartPoint:
            init(getStatus() - 1);
            break;
        case SetEndPoint:
            keyPressEvent(nullptr);
            break;
        }
    }
}

void ActionDrawCloudLineFree::keyPressEvent(QKeyEvent* e)
{
    if (e != nullptr && e->key() != Qt::Key_Enter)
    {
        ActionInterface::keyPressEvent(e);
        return;
    }
    if (pPoints->history.size() <= CLOUD_MIN_HISTORY_POINTS)
    {
        pPoints->isError = true;
        updateMouseButtonHints();
        pPoints->isError = false;
        if (e)
        {
            ActionInterface::keyPressEvent(e);
        }
        return;
    }
    pPoints->polyline = getPoly(nullptr);
    if (pPoints->polyline == nullptr)
    {
        pPoints->isError = true;
        updateMouseButtonHints();
        if (getStatus() == SetStartPoint)
        {
            init(getStatus() - 1);
        }
        else if (getStatus() == SetEndPoint)
        {
            init(getStatus() - 2);
        }
    }
    else
    {
        pDocument->specifyModifiedEntity(pPoints->polyline);
        trigger();
        init(getStatus() - 2);
    }
    if (e)
    {
        ActionInterface::keyPressEvent(e);
    }
}

void ActionDrawCloudLineFree::coordinateEvent(GuiCoordinateEvent* e)
{
    if (!e)
    {
        return;
    }
    DmVector mouse = e->getCoordinate();
    switch (getStatus())
    {
    case SetStartPoint:
        pPoints->history.push_back(mouse);
        setStatus(SetEndPoint);
        updateMouseButtonHints();
        break;
    default:
        break;
    }
}

void ActionDrawCloudLineFree::commandEvent(GuiCommandEvent* e)
{
    Q_UNUSED(e);
}

QStringList ActionDrawCloudLineFree::getAvailableCommands()
{
    QStringList cmd;
    return cmd;
}

void ActionDrawCloudLineFree::updateMouseButtonHints()
{
    switch (getStatus())
    {
    case SetStartPoint:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Specify first point"), tr("Cancel"));
        break;
    case SetEndPoint:
        if (pPoints->isClosed)
        {
            GUIDIALOGFACTORY->updateMouseWidget(tr("Cloud line done!"), tr("Cancel"));
        }
        else
        {
            GUIDIALOGFACTORY->updateMouseWidget(tr("Move cursor to get cloud line path..."), tr("Cancel"));
        }
        break;
    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void ActionDrawCloudLineFree::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}

void ActionDrawCloudLineFree::showOptions()
{
    ActionInterface::showOptions();

    GUIDIALOGFACTORY->requestOptions(this, true);
}

void ActionDrawCloudLineFree::hideOptions()
{
    ActionInterface::hideOptions();

    GUIDIALOGFACTORY->requestOptions(this, false);
}

DmPolyline* ActionDrawCloudLineFree::getPoly(DmEntityContainer* host)
{
    if (pPoints->history.size() <= 1)
    {
        return nullptr;
    }
    bool isClockwise = GeometryMethods::isPtsClockwise(pPoints->history);
    double bulge = isClockwise ? -CLOUD_DEFAULT_BULGE : CLOUD_DEFAULT_BULGE;
    if (m_isReversed)
    {
        bulge = -bulge;
    }
    DmPolyline* resPoly = nullptr;
    if (pPoints->isClosed)
    {
        std::vector<double> bulges(pPoints->history.size(), CLOUD_DEFAULT_BULGE);
        std::vector<double> weights(pPoints->history.size() * 2, 0.0);
        resPoly = new DmPolyline(host, PolylineData(pPoints->history, bulges, weights, true));
    }
    else
    {
        std::vector<double> bulges(pPoints->history.size() - 1, CLOUD_DEFAULT_BULGE);
        std::vector<double> weights((pPoints->history.size() - 1) * 2, 0.0);
        resPoly = new DmPolyline(host, PolylineData(pPoints->history, bulges, weights, false));
    }
    resPoly->setDocument(pDocument);
    if (resPoly)
    {
        resPoly->update();
        if (host)
        {
            host->addEntity(resPoly);
        }
    }
    return resPoly;
}

void ActionDrawCloudLineFree::drawPoly(DmEntityContainer* host)
{
    if (host == nullptr)
    {
        return;
    }
    deletePreview();
    DmPolyline* poly = getPoly(host);
    Q_UNUSED(poly);
    drawPreview();
}
