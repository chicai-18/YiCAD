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


/// @file ActionDrawCloudLineRectangle.cpp
/// @brief 矩形云线（修订云线）绘制交互动作实现

#include "ActionDrawCloudLineRectangle.h"
#include "DmPolyline.h"
#include "DmVector.h"
#include "DmArc.h"
#include "GuiDocumentView.h"
#include "Debug.h"
#include "Preview.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "Tools.h"

#include <algorithm>
#include <initializer_list>
#include <QMouseEvent>
#include "Transaction.h"

namespace
{
    constexpr double MIN_ARC_LENGTH_THRESHOLD = 1.0;   ///< 最小弧长阈值，用于判断线段是否过短
    constexpr double HALF_DIVISOR = 2.0;               ///< 半分除数
    constexpr double DEFAULT_BULGE = 0.5;               ///< 默认弧线凸度
    constexpr int MIN_POLY_POINTS = 3;                 ///< 多段线最少顶点数
    constexpr int MIN_POLY_POINTS_FOR_TRIM = 4;        ///< 需要裁剪处理的最少顶点数
    constexpr int RECT_SIDES = 4;                      ///< 矩形边数
    constexpr int MAX_PREVIEW_RATIO = 1000;            ///< 预览点距比值上限
}

struct ActionDrawCloudLineRectangle::Points
{
    DmPolyline* polyline{ nullptr };
    DmVector start;
    DmVector end;
    bool isError{ false }; ///< 是否发生错误导致无法生成云线
};

ActionDrawCloudLineRectangle::ActionDrawCloudLineRectangle(DmDocument* doc, GuiDocumentView* docView):
    PreviewActionInterface("Draw cloud line rectangle", doc, docView), pPoints(new Points{})
{
    actionType = DM::ActionCloudLineRectangle;
}

ActionDrawCloudLineRectangle::~ActionDrawCloudLineRectangle() = default;

void ActionDrawCloudLineRectangle::reset()
{
    pPoints->polyline = nullptr;
    pPoints->start = {};
    pPoints->end = {};
}

DmPolyline* ActionDrawCloudLineRectangle::calculateCloudRect(DmEntityContainer* host, DmVector startPt, DmVector endPt)
{
    double minX = std::min(startPt.x, endPt.x);
    double maxX = std::max(startPt.x, endPt.x);
    double minY = std::min(startPt.y, endPt.y);
    double maxY = std::max(startPt.y, endPt.y);
    double deltaX = maxX - minX;
    double deltaY = maxY - minY;

    if (deltaX < m_minArcLen && deltaY < m_minArcLen)
    {
        return nullptr;
    }

    // 矩形四个角点，从左下角逆时针回到左下角
    std::vector<DmVector> pts{ {minX, minY}, {maxX, minY}, {maxX, maxY}, {minX, maxY}, {minX, minY} };

    // 计算云线的顶点（用最小长度和最大长度交替生成）
    std::vector<DmVector> polyPts;
    bool curUseMinLength = true;
    bool useLastB = false;
    DmVector lastB;

    for (size_t i = 0; i < RECT_SIDES; i++)
    {
        DmVector curB;
        if (useLastB)
        {
            curB = lastB;
            useLastB = false;
        }
        else
        {
            curB = pts.at(i);
        }

        DmVector curE = pts.at(i + 1);
        DmVector nextB = curE;
        DmVector nextE;

        if (i == RECT_SIDES - 1)
        {
            nextE = pts.at(1);
        }
        else
        {
            nextE = pts.at(i + 2);
        }

        DmVector dir = (curE - curB) / (curE - curB).magnitude();
        double dist = curB.distanceTo(curE);

        if (dist < MIN_ARC_LENGTH_THRESHOLD)
        {
            continue;
        }

        polyPts.push_back(curB);
        int num = static_cast<int>(dist / (m_minArcLen + m_maxArcLen));

        if (num > 0)
        {
            for (size_t k = 0; k < num; k++)
            {
                polyPts.push_back(curB + dir * m_minArcLen);
                curB = curB + dir * m_minArcLen;
                polyPts.push_back(curB + dir * m_maxArcLen);
                curB = curB + dir * m_maxArcLen;
            }
        }

        if (curB.distanceTo(curE) > m_minArcLen)
        {
            polyPts.push_back(curB + dir * m_minArcLen);
            curB = curB + dir * m_minArcLen;
        }

        // 计算转角处圆弧偏移
        double nextLen = (nextE - nextB).magnitude();
        DmVector nextDir = (nextE - nextB) / (nextE - nextB).magnitude();
        double leftLen = (nextB - curB).magnitude();
        double offsetLen = 0.0;

        if (m_maxArcLen > leftLen)
        {
            // 下一个点相对于起点（nextB）的偏移距离
            offsetLen = std::sqrt(m_maxArcLen * m_maxArcLen - leftLen * leftLen);
            if (offsetLen > nextLen && m_minArcLen > leftLen)
            {
                offsetLen = std::sqrt(m_minArcLen * m_minArcLen - leftLen * leftLen);
            }
        }

        // 计算的下一点偏移距离太长，偏移置为0
        if (offsetLen > nextLen)
        {
            offsetLen = 0.0;
        }

        lastB = nextB + nextDir * offsetLen;
        useLastB = true;
    }

    // 左下角处理（小于最小长度的一半则直接延伸，否则另外加一段）
    if (polyPts.size() > MIN_POLY_POINTS_FOR_TRIM)
    {
        double dist = polyPts.back().distanceTo(polyPts.front());
        if (dist < m_minArcLen / HALF_DIVISOR)
        {
            polyPts.erase(polyPts.end() - 1);
        }
    }

    // 由多段线的顶点生成云线
    DmPolyline* resPoly = nullptr;
    if (polyPts.size() > MIN_POLY_POINTS)
    {
        std::vector<double> bulges(polyPts.size(), DEFAULT_BULGE);
        std::vector<double> weights(polyPts.size() * 2, 0.0);
        resPoly = new DmPolyline(host, PolylineData(polyPts, bulges, weights, true));
        resPoly->setDocument(pDocument);
        resPoly->update();
    }

    if (resPoly && host)
    {
        host->addEntity(resPoly);
    }

    return resPoly;
}

void ActionDrawCloudLineRectangle::init(int status /*= 0*/)
{
    reset();
    PreviewActionInterface::init(status);
}

void ActionDrawCloudLineRectangle::trigger()
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

void ActionDrawCloudLineRectangle::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = snapPoint(e);

    if (getStatus() == SetEndPoint && pPoints->start.valid)
    {
        deletePreview();
        double dist = pPoints->start.distanceTo(mouse);

        // 范围不是太大才考虑绘制
        if (!(m_minArcLen > 0 && dist / m_minArcLen > MAX_PREVIEW_RATIO))
        {
            calculateCloudRect(preview->getEntityContainer(), pPoints->start, mouse);
        }
        drawPreview();
    }
}

void ActionDrawCloudLineRectangle::mouseReleaseEvent(QMouseEvent* e)
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
            init(getStatus() - 2);
            break;
        }
    }
}

void ActionDrawCloudLineRectangle::coordinateEvent(GuiCoordinateEvent* e)
{
    if (!e)
    {
        return;
    }

    DmVector mouse = e->getCoordinate();

    switch (getStatus())
    {
    case SetStartPoint:
        pPoints->start = mouse;
        setStatus(SetEndPoint);
        updateMouseButtonHints();
        break;
    case SetEndPoint:
        pPoints->polyline = calculateCloudRect(nullptr, pPoints->start, mouse);
        if (pPoints->polyline)
        {
            pDocument->specifyModifiedEntity(pPoints->polyline);
            deletePreview();
            trigger();
            init(getStatus() - 2);
            pPoints->isError = false;
        }
        else
        {
            pPoints->isError = true;
        }
        updateMouseButtonHints();
        break;
    default:
        break;
    }
}

void ActionDrawCloudLineRectangle::commandEvent(GuiCommandEvent* e)
{
    Q_UNUSED(e);
}

QStringList ActionDrawCloudLineRectangle::getAvailableCommands()
{
    QStringList cmd;
    return cmd;
}

void ActionDrawCloudLineRectangle::showOptions()
{
    ActionInterface::showOptions();

    GUIDIALOGFACTORY->requestOptions(this, true);
}

void ActionDrawCloudLineRectangle::hideOptions()
{
    ActionInterface::hideOptions();

    GUIDIALOGFACTORY->requestOptions(this, false);
}

void ActionDrawCloudLineRectangle::updateMouseButtonHints()
{
    switch (getStatus())
    {
    case SetStartPoint:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Specify first point"), tr("Cancel"));
        break;
    case SetEndPoint:
        if (pPoints->isError)
        {
            GUIDIALOGFACTORY->updateMouseWidget(tr("Can not create cloud line, please select again"), tr("Cancel"));
        }
        else
        {
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify next point"), tr("Cancel"));
        }
        break;
    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void ActionDrawCloudLineRectangle::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}

void ActionDrawCloudLineRectangle::setMaxLength(double maxLength)
{
    m_maxArcLen = maxLength;
}

void ActionDrawCloudLineRectangle::setMinLength(double minLength)
{
    m_minArcLen = minLength;
}

double ActionDrawCloudLineRectangle::getMaxLength() const
{
    return m_maxArcLen;
}

double ActionDrawCloudLineRectangle::getMinLength() const
{
    return m_minArcLen;
}
