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


/// @file ActionDrawCloudLinePolygon.cpp
/// @brief 通过控制点绘制多边形修订云线的交互动作实现

#include "ActionDrawCloudLinePolygon.h"
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
    constexpr double CLOUD_DEFAULT_BULGE = 0.5;        ///< 默认弧线凸起值
    constexpr double CLOUD_MIN_SEGMENT_LEN = 1.0;       ///< 最小线段长度
    constexpr int CLOUD_MIN_POLY_PTS = 2;               ///< 最少控制点数
    constexpr int CLOUD_CLOSED_THRESHOLD = 3;           ///< 闭合所需最小控制点数
    constexpr int CLOUD_RESULT_MIN_PTS = 4;             ///< 结果多边形最小顶点数
    constexpr int CLOUD_MAX_RATIO_THRESHOLD = 1000;     ///< 总长/最小弧长比阈值（超过则不绘制）
}

struct ActionDrawCloudLinePolygon::Points
{
    DmPolyline* polyline;
    DmVector mousePt;
    std::vector<DmVector> history;
    bool isError{ false }; ///< 是否发生错误（如：线太短）导致无法生成云线
};

ActionDrawCloudLinePolygon::ActionDrawCloudLinePolygon(DmDocument* doc, GuiDocumentView* docView):
    PreviewActionInterface("Draw cloud line polygon", doc, docView), pPoints(new Points{})
{
    actionType = DM::ActionCloudLinePolygon;
}

ActionDrawCloudLinePolygon::~ActionDrawCloudLinePolygon() = default;

void ActionDrawCloudLinePolygon::reset()
{
    pPoints->polyline = nullptr;
    pPoints->history.clear();
    pPoints->isError = false;
}

DmPolyline* ActionDrawCloudLinePolygon::calculateCloudPoly(DmEntityContainer* host, const std::vector<DmVector>& polyPts)
{
    if (polyPts.size() < CLOUD_MIN_POLY_PTS)
    {
        return nullptr;
    }
    bool needClosed = polyPts.size() >= CLOUD_CLOSED_THRESHOLD ? true : false;  // 多段线是否需要闭合（当只有2个点时，不要闭合）
    std::vector<DmVector> copyPts(polyPts);
    bool isClockwise = GeometryMethods::isPtsClockwise(copyPts);
    double bulge = isClockwise ? -CLOUD_DEFAULT_BULGE : CLOUD_DEFAULT_BULGE;
    if (needClosed)
    {
        copyPts.push_back(copyPts.front());
    }

    // 计算云线的顶点
    std::vector<DmVector> resPolyPts;
    bool curUseMinLength = true;
    bool useLastB = false;
    DmVector lastB;
    if (!needClosed) // 只有1根线（不需要转角处理）
    {
        DmVector curB = polyPts.at(0);
        DmVector curE = copyPts.at(1);
        DmVector dir = (curE - curB) / (curE - curB).magnitude();
        double dist = curB.distanceTo(curE);
        if (dist < CLOUD_MIN_SEGMENT_LEN)
        {
            return nullptr;
        }
        resPolyPts.push_back(curB);
        int num = static_cast<int>(dist / (m_minArcLen + m_maxArcLen));
        if (num > 0)
        {
            for (size_t k = 0; k < static_cast<size_t>(num); k++)
            {
                resPolyPts.push_back(curB + dir * m_minArcLen);
                curB = curB + dir * m_minArcLen;
                resPolyPts.push_back(curB + dir * m_maxArcLen);
                curB = curB + dir * m_maxArcLen;
            }
        }
        resPolyPts.push_back(curE);
    }
    else // 有2根以上的线（需要转角处理）
    {
        for (size_t i = 0; i < copyPts.size() - 1; i++)
        {
            DmVector curB;
            if (useLastB)
            {
                curB = lastB;
                useLastB = false;
            }
            else
            {
                curB = copyPts.at(i);
            }
            DmVector curE = copyPts.at(i + 1);
            DmVector nextB = curE;
            DmVector nextE;
            if (i == copyPts.size() - 2) // 最后一条线（这条线使多段线闭合）的起点索引
            {
                nextE = copyPts.at(1);
            }
            else
            {
                nextE = copyPts.at(i + 2);
            }

            DmVector dir = (curE - curB) / (curE - curB).magnitude();
            double dist = curB.distanceTo(curE);
            if (dist < CLOUD_MIN_SEGMENT_LEN)
            {
                continue;
            }
            resPolyPts.push_back(curB);
            int num = static_cast<int>(dist / (m_minArcLen + m_maxArcLen));
            if (num > 0)
            {
                for (size_t k = 0; k < static_cast<size_t>(num); k++)
                {
                    resPolyPts.push_back(curB + dir * m_minArcLen);
                    curB = curB + dir * m_minArcLen;
                    resPolyPts.push_back(curB + dir * m_maxArcLen);
                    curB = curB + dir * m_maxArcLen;
                }
            }
            if (curB.distanceTo(curE) > m_minArcLen)
            {
                resPolyPts.push_back(curB + dir * m_minArcLen);
                curB = curB + dir * m_minArcLen;
            }

            // 计算转角处圆弧
            double nextLen = (nextE - nextB).magnitude();
            DmVector nextDir = (nextE - nextB) / (nextE - nextB).magnitude();
            double leftLen = (nextB - curB).magnitude();    // 当前点到下一个起始点的距离
            double offsetLen = 0.0;                         // 下一个点相对于起点（nextB）的偏移距离
            if (m_maxArcLen > leftLen)
            {
                offsetLen = m_maxArcLen - leftLen;
                if (offsetLen > nextLen && m_minArcLen > leftLen)
                {
                    offsetLen = m_minArcLen - leftLen;
                }
            }
            if (offsetLen > nextLen)
            {
                offsetLen = 0.0;
            }
            lastB = nextB + nextDir * offsetLen;
            useLastB = true;
        }

        // 最后一个转角处理
        if (resPolyPts.size() > CLOUD_RESULT_MIN_PTS)
        {
            double dist = resPolyPts.back().distanceTo(resPolyPts.front());
            if (dist < m_minArcLen / 2.0)
            {
                resPolyPts.erase(resPolyPts.end() - 1);
            }
        }
    }

    // 由多段线的顶点生成云线
    DmPolyline* resPoly = nullptr;
    if (resPolyPts.size() >= CLOUD_MIN_POLY_PTS)
    {
        if (polyPts.size() <= CLOUD_MIN_POLY_PTS)    // 仅有2个点时，不闭合
        {
            std::vector<double> bulges(resPolyPts.size() - 1, CLOUD_DEFAULT_BULGE);
            std::vector<double> weights((resPolyPts.size() - 1) * 2, 0.0);
            resPoly = new DmPolyline(host, PolylineData(resPolyPts, bulges, weights, false));
        }
        else
        {
            std::vector<double> bulges(resPolyPts.size(), CLOUD_DEFAULT_BULGE);
            std::vector<double> weights(resPolyPts.size() * 2, 0.0);
            resPoly = new DmPolyline(host, PolylineData(resPolyPts, bulges, weights, true));
        }
        resPoly->setDocument(pDocument);
        resPoly->update();
    }
    if (resPoly && host)
    {
        host->addEntity(resPoly);
    }
    return resPoly;
}

void ActionDrawCloudLinePolygon::init(int status /*= 0*/)
{
    reset();
    PreviewActionInterface::init(status);
}

void ActionDrawCloudLinePolygon::trigger()
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

void ActionDrawCloudLinePolygon::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = snapPoint(e);
    pPoints->mousePt = mouse;
    if (getStatus() == SetEndPoint && pPoints->history.size() > 0)
    {
        drawPoly(preview->getEntityContainer());
    }
}

void ActionDrawCloudLinePolygon::mouseReleaseEvent(QMouseEvent* e)
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

void ActionDrawCloudLinePolygon::keyPressEvent(QKeyEvent* e)
{
    if (e != nullptr && e->key() != Qt::Key_Enter)
    {
        ActionInterface::keyPressEvent(e);
        return;
    }
    if (pPoints->history.size() <= CLOUD_MIN_POLY_PTS)
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
    pPoints->polyline = calculateCloudPoly(nullptr, pPoints->history);
    if (nullptr == pPoints->polyline)
    {
        pDocument->specifyModifiedEntity(pPoints->polyline);
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
        trigger();
        init(getStatus() - 2);
    }
    if (e)
    {
        ActionInterface::keyPressEvent(e);
    }
}

void ActionDrawCloudLinePolygon::coordinateEvent(GuiCoordinateEvent* e)
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
    case SetEndPoint:
        pPoints->history.push_back(mouse);
        drawPoly(preview->getEntityContainer());
        updateMouseButtonHints();
        break;
    default:
        break;
    }
}

void ActionDrawCloudLinePolygon::commandEvent(GuiCommandEvent* e)
{
    Q_UNUSED(e);
}

QStringList ActionDrawCloudLinePolygon::getAvailableCommands()
{
    QStringList cmd;
    return cmd;
}

void ActionDrawCloudLinePolygon::showOptions()
{
    ActionInterface::showOptions();

    GUIDIALOGFACTORY->requestOptions(this, true);
}

void ActionDrawCloudLinePolygon::hideOptions()
{
    ActionInterface::hideOptions();

    GUIDIALOGFACTORY->requestOptions(this, false);
}

void ActionDrawCloudLinePolygon::updateMouseButtonHints()
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

void ActionDrawCloudLinePolygon::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}

void ActionDrawCloudLinePolygon::setMaxLength(double maxLength)
{
    m_maxArcLen = maxLength;
}

void ActionDrawCloudLinePolygon::setMinLength(double minLength)
{
    m_minArcLen = minLength;
}

double ActionDrawCloudLinePolygon::getMaxLength()
{
    return m_maxArcLen;
}

double ActionDrawCloudLinePolygon::getMinLength()
{
    return m_minArcLen;
}

void ActionDrawCloudLinePolygon::undo()
{
    if (pPoints->history.size() > 1)
    {
        pPoints->history.pop_back();
        drawPoly(preview->getEntityContainer());
    }
    else
    {
        GUIDIALOGFACTORY->commandMessage(tr("Cannot undo: Not enough entities defined yet."));
    }
}

void ActionDrawCloudLinePolygon::drawPoly(DmEntityContainer* host)
{
    deletePreview();
    std::vector<DmVector> polyPts(pPoints->history);
    polyPts.push_back(pPoints->mousePt);

    // 计算总长，范围不是太大，才考虑绘制
    double totalLen = 0.0;
    bool isFirst = true;
    DmVector lastPt;
    for (auto& pt : polyPts)
    {
        if (isFirst)
        {
            isFirst = false;
        }
        else
        {
            double len = pt.distanceTo(lastPt);
            totalLen += len;
        }
        lastPt = pt;
    }
    DmPolyline* poly = nullptr;
    if (!(m_minArcLen > 0 && totalLen / m_minArcLen > CLOUD_MAX_RATIO_THRESHOLD))   // 范围不是太大，才考虑绘制
    {
        poly = calculateCloudPoly(host, polyPts);
    }
    Q_UNUSED(poly);
    drawPreview();
}
