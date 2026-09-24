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

/// @file DrawCloudLineCommand.cpp
/// @brief 云线命令与工具的实现
///
/// 三个工具的状态机、云线的几何计算从原 Action 原样搬来；原 Action 画完一条云线
/// 就经 init(-1) 结束，这里一样。

#include "DrawCloudLineCommand.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <QKeyEvent>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "DmPolyline.h"
#include "EntityTable.h"
#include "GeometryMethods.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Transaction.h"

namespace
{
// ---- 矩形云线 ----
constexpr double MIN_ARC_LENGTH_THRESHOLD = 1.0; ///< 最小弧长阈值，用于判断线段是否过短
constexpr double HALF_DIVISOR = 2.0;             ///< 半分除数
constexpr double DEFAULT_BULGE = 0.5;            ///< 默认弧线凸度
constexpr int MIN_POLY_POINTS = 3;               ///< 多段线最少顶点数
constexpr int MIN_POLY_POINTS_FOR_TRIM = 4;      ///< 需要裁剪处理的最少顶点数
constexpr int RECT_SIDES = 4;                    ///< 矩形边数
constexpr int MAX_PREVIEW_RATIO = 1000;          ///< 预览点距比值上限

// ---- 多边形云线 ----
constexpr double CLOUD_DEFAULT_BULGE = 0.5;    ///< 默认弧线凸起值
constexpr double CLOUD_MIN_SEGMENT_LEN = 1.0;  ///< 最小线段长度
constexpr int CLOUD_MIN_POLY_PTS = 2;          ///< 最少控制点数
constexpr int CLOUD_CLOSED_THRESHOLD = 3;      ///< 闭合所需最小控制点数
constexpr int CLOUD_RESULT_MIN_PTS = 4;        ///< 结果多边形最小顶点数
constexpr int CLOUD_MAX_RATIO_THRESHOLD = 1000; ///< 总长/最小弧长比阈值（超过则不绘制）

// ---- 自由云线 ----
constexpr double CLOUD_CLOSE_DEVICE_THRESHOLD = 20.0;    ///< 闭合检测的设备坐标阈值
constexpr double CLOUD_CLOSE_MAGNITUDE_THRESHOLD = 10.0; ///< 闭合检测的向量长度阈值
constexpr double CLOUD_MIN_POINT_DISTANCE = 1.0;         ///< 点之间最小距离
constexpr double CLOUD_LARGE_COORD = 1e10;               ///< 大坐标初始值
constexpr int CLOUD_MIN_HISTORY_POINTS = 2;              ///< 最少历史点数

/// @brief 云线工具的公共部分：提交与"回到某一状态"
template <typename Command>
class CloudLineToolBase : public BasePlaceTool
{
public:
    CloudLineToolBase(Command& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

protected:
    /// @brief 丢弃采集的数据（原 reset()）
    virtual void reset() = 0;

    /// @brief 回到某一状态（原 init(status)：先 reset() 再初始化）；status < 0 时结束命令
    void init(int s)
    {
        reset();
        if (s < 0)
        {
            command().finish();
            return;
        }
        restart(s);
    }

    /// @brief 提交云线（原 trigger()）
    void commitPolyline(DmPolyline*& polyline)
    {
        m_command.preview().clear();
        if (!polyline)
        {
            return;
        }
        m_command.commitCloudLine(polyline, Command::tr("Add cloud line"));
        snapper()->deleteSnapper();
        view()->moveRelativeZero(polyline->getEndpoint());
        snapper()->drawSnapper();
        polyline = nullptr;
    }

    Command& m_command;
};

/// @brief 矩形云线工具：两个对角点
class CloudLineRectangleTool : public CloudLineToolBase<DrawCloudLineRectangleCommand>
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetStartPoint, ///< 设置起点（矩形第一角点）
        SetEndPoint    ///< 设置终点（矩形对角点）
    };

    using CloudLineToolBase::CloudLineToolBase;

protected:
    void updateHints() override;
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;
    void onCoordinate(const DmVector& coord) override;

    void reset() override
    {
        m_points.polyline = nullptr;
        m_points.start = {};
        m_points.end = {};
    }

private:
    struct Points
    {
        DmPolyline* polyline{nullptr};
        DmVector start;
        DmVector end;
        bool isError{false}; ///< 是否发生错误导致无法生成云线
    };

    void commit() { commitPolyline(m_points.polyline); }
    DmPolyline* calculateCloudRect(DmEntityContainer* host, DmVector startPt, DmVector endPt);

    Points m_points;
};

/// @brief 多边形云线工具：逐点指定，回车或右键结束
class CloudLinePolygonTool : public CloudLineToolBase<DrawCloudLinePolygonCommand>
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetStartPoint, ///< 设置起点
        SetEndPoint    ///< 设置后续控制点
    };

    using CloudLineToolBase::CloudLineToolBase;

    void undo();

protected:
    void updateHints() override;
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;
    void onKeyPress(QKeyEvent* e) override;
    void onCoordinate(const DmVector& coord) override;

    void reset() override
    {
        m_points.polyline = nullptr;
        m_points.history.clear();
        m_points.isError = false;
    }

private:
    struct Points
    {
        DmPolyline* polyline{nullptr};
        DmVector mousePt;
        std::vector<DmVector> history;
        bool isError{false}; ///< 是否发生错误（如：线太短）导致无法生成云线
    };

    void commit() { commitPolyline(m_points.polyline); }
    DmPolyline* calculateCloudPoly(DmEntityContainer* host, const std::vector<DmVector>& polyPts);
    void drawPoly(DmEntityContainer* host);

    Points m_points;
};

/// @brief 自由云线工具：移动鼠标画路径，回到起点附近自动闭合
class CloudLineFreeTool : public CloudLineToolBase<DrawCloudLineFreeCommand>
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetStartPoint, ///< 设置起点
        SetEndPoint    ///< 设置终点/移动路径
    };

    using CloudLineToolBase::CloudLineToolBase;

protected:
    void updateHints() override;
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;
    void onKeyPress(QKeyEvent* e) override;
    void onCoordinate(const DmVector& coord) override;

    void reset() override
    {
        m_points.polyline = nullptr;
        m_points.history.clear();
        m_points.isClosed = false;
    }

private:
    struct Points
    {
        DmPolyline* polyline{nullptr};
        std::vector<DmVector> history; ///< 自由拾取的所有点
        bool isClosed{false};          ///< 是否已闭合
        bool isError{false};           ///< 是否出错
    };

    void commit() { commitPolyline(m_points.polyline); }
    DmPolyline* getPoly(DmEntityContainer* host);
    void drawPoly(DmEntityContainer* host);

    Points m_points;
};

DmPolyline* CloudLineRectangleTool::calculateCloudRect(DmEntityContainer* host, DmVector startPt, DmVector endPt)
{
    double minX = std::min(startPt.x, endPt.x);
    double maxX = std::max(startPt.x, endPt.x);
    double minY = std::min(startPt.y, endPt.y);
    double maxY = std::max(startPt.y, endPt.y);
    double deltaX = maxX - minX;
    double deltaY = maxY - minY;

    if (deltaX < m_command.getMinLength() && deltaY < m_command.getMinLength())
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
        int num = static_cast<int>(dist / (m_command.getMinLength() + m_command.getMaxLength()));

        if (num > 0)
        {
            for (size_t k = 0; k < num; k++)
            {
                polyPts.push_back(curB + dir * m_command.getMinLength());
                curB = curB + dir * m_command.getMinLength();
                polyPts.push_back(curB + dir * m_command.getMaxLength());
                curB = curB + dir * m_command.getMaxLength();
            }
        }

        if (curB.distanceTo(curE) > m_command.getMinLength())
        {
            polyPts.push_back(curB + dir * m_command.getMinLength());
            curB = curB + dir * m_command.getMinLength();
        }

        // 计算转角处圆弧偏移
        double nextLen = (nextE - nextB).magnitude();
        DmVector nextDir = (nextE - nextB) / (nextE - nextB).magnitude();
        double leftLen = (nextB - curB).magnitude();
        double offsetLen = 0.0;

        if (m_command.getMaxLength() > leftLen)
        {
            // 下一个点相对于起点（nextB）的偏移距离
            offsetLen = std::sqrt(m_command.getMaxLength() * m_command.getMaxLength() - leftLen * leftLen);
            if (offsetLen > nextLen && m_command.getMinLength() > leftLen)
            {
                offsetLen = std::sqrt(m_command.getMinLength() * m_command.getMinLength() - leftLen * leftLen);
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
        if (dist < m_command.getMinLength() / HALF_DIVISOR)
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
        resPoly->setDocument(document());
        resPoly->update();
    }

    if (resPoly && host)
    {
        host->addEntity(resPoly);
    }

    return resPoly;
}

void CloudLineRectangleTool::onMouseMove(QMouseEvent* e)
{
    DmVector mouse = snapper()->snapPoint(e);

    if (status() == SetEndPoint && m_points.start.valid)
    {
        m_command.preview().clear();
        double dist = m_points.start.distanceTo(mouse);

        // 范围不是太大才考虑绘制
        if (!(m_command.getMinLength() > 0 && dist / m_command.getMinLength() > MAX_PREVIEW_RATIO))
        {
            calculateCloudRect(m_command.preview().entities().getEntityContainer(), m_points.start, mouse);
        }
        m_command.preview().draw();
    }
}

void CloudLineRectangleTool::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        DmVector snapped = snapper()->snapPoint(e);
        onCoordinate(snapped);
    }
    else if (e->button() == Qt::RightButton)
    {
        m_command.preview().clear();
        snapper()->deleteSnapper();
        switch (status())
        {
        default:
        case SetStartPoint:
            init(status() - 1);
            break;
        case SetEndPoint:
            init(status() - 2);
            break;
        }
    }
}

void CloudLineRectangleTool::onCoordinate(const DmVector& coord){

    DmVector mouse = coord;

    switch (status())
    {
    case SetStartPoint:
        m_points.start = mouse;
        setStatus(SetEndPoint);
        updateHints();
        break;
    case SetEndPoint:
        m_points.polyline = calculateCloudRect(nullptr, m_points.start, mouse);
        if (m_points.polyline)
        {
            document()->specifyModifiedEntity(m_points.polyline);
            m_command.preview().clear();
            commit();
            init(status() - 2);
            m_points.isError = false;
        }
        else
        {
            m_points.isError = true;
        }
        updateHints();
        break;
    default:
        break;
    }
}

void CloudLineRectangleTool::updateHints()
{
    switch (status())
    {
    case SetStartPoint:
        GUIDIALOGFACTORY->updateMouseWidget(DrawCloudLineRectangleCommand::tr("Specify first point"), DrawCloudLineRectangleCommand::tr("Cancel"));
        break;
    case SetEndPoint:
        if (m_points.isError)
        {
            GUIDIALOGFACTORY->updateMouseWidget(DrawCloudLineRectangleCommand::tr("Can not create cloud line, please select again"), DrawCloudLineRectangleCommand::tr("Cancel"));
        }
        else
        {
            GUIDIALOGFACTORY->updateMouseWidget(DrawCloudLineRectangleCommand::tr("Specify next point"), DrawCloudLineRectangleCommand::tr("Cancel"));
        }
        break;
    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}
DmPolyline* CloudLinePolygonTool::calculateCloudPoly(DmEntityContainer* host, const std::vector<DmVector>& polyPts)
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
        int num = static_cast<int>(dist / (m_command.getMinLength() + m_command.getMaxLength()));
        if (num > 0)
        {
            for (size_t k = 0; k < static_cast<size_t>(num); k++)
            {
                resPolyPts.push_back(curB + dir * m_command.getMinLength());
                curB = curB + dir * m_command.getMinLength();
                resPolyPts.push_back(curB + dir * m_command.getMaxLength());
                curB = curB + dir * m_command.getMaxLength();
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
            int num = static_cast<int>(dist / (m_command.getMinLength() + m_command.getMaxLength()));
            if (num > 0)
            {
                for (size_t k = 0; k < static_cast<size_t>(num); k++)
                {
                    resPolyPts.push_back(curB + dir * m_command.getMinLength());
                    curB = curB + dir * m_command.getMinLength();
                    resPolyPts.push_back(curB + dir * m_command.getMaxLength());
                    curB = curB + dir * m_command.getMaxLength();
                }
            }
            if (curB.distanceTo(curE) > m_command.getMinLength())
            {
                resPolyPts.push_back(curB + dir * m_command.getMinLength());
                curB = curB + dir * m_command.getMinLength();
            }

            // 计算转角处圆弧
            double nextLen = (nextE - nextB).magnitude();
            DmVector nextDir = (nextE - nextB) / (nextE - nextB).magnitude();
            double leftLen = (nextB - curB).magnitude();    // 当前点到下一个起始点的距离
            double offsetLen = 0.0;                         // 下一个点相对于起点（nextB）的偏移距离
            if (m_command.getMaxLength() > leftLen)
            {
                offsetLen = m_command.getMaxLength() - leftLen;
                if (offsetLen > nextLen && m_command.getMinLength() > leftLen)
                {
                    offsetLen = m_command.getMinLength() - leftLen;
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
            if (dist < m_command.getMinLength() / 2.0)
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
        resPoly->setDocument(document());
        resPoly->update();
    }
    if (resPoly && host)
    {
        host->addEntity(resPoly);
    }
    return resPoly;
}

void CloudLinePolygonTool::onMouseMove(QMouseEvent* e)
{
    DmVector mouse = snapper()->snapPoint(e);
    m_points.mousePt = mouse;
    if (status() == SetEndPoint && m_points.history.size() > 0)
    {
        drawPoly(m_command.preview().entities().getEntityContainer());
    }
}

void CloudLinePolygonTool::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        DmVector snapped = snapper()->snapPoint(e);
        onCoordinate(snapped);
    }
    else if (e->button() == Qt::RightButton)
    {
        m_command.preview().clear();
        snapper()->deleteSnapper();
        switch (status())
        {
        default:
        case SetStartPoint:
            init(status() - 1);
            break;
        case SetEndPoint:
            onKeyPress(nullptr);
            break;
        }
    }
}

void CloudLinePolygonTool::onKeyPress(QKeyEvent* e)
{
    if (e != nullptr && e->key() != Qt::Key_Enter)
    {
        e->ignore();
        return;
    }
    if (m_points.history.size() <= CLOUD_MIN_POLY_PTS)
    {
        m_points.isError = true;
        updateHints();
        m_points.isError = false;
        if (e)
        {
            e->ignore();
        }
        return;
    }
    m_points.polyline = calculateCloudPoly(nullptr, m_points.history);
    if (nullptr == m_points.polyline)
    {
        document()->specifyModifiedEntity(m_points.polyline);
        m_points.isError = true;
        updateHints();
        if (status() == SetStartPoint)
        {
            init(status() - 1);
        }
        else if (status() == SetEndPoint)
        {
            init(status() - 2);
        }
    }
    else
    {
        commit();
        init(status() - 2);
    }
    if (e)
    {
        e->ignore();
    }
}

void CloudLinePolygonTool::onCoordinate(const DmVector& coord){
    DmVector mouse = coord;
    switch (status())
    {
    case SetStartPoint:
        m_points.history.push_back(mouse);
        setStatus(SetEndPoint);
        updateHints();
        break;
    case SetEndPoint:
        m_points.history.push_back(mouse);
        drawPoly(m_command.preview().entities().getEntityContainer());
        updateHints();
        break;
    default:
        break;
    }
}

void CloudLinePolygonTool::updateHints()
{
    switch (status())
    {
    case SetStartPoint:
        GUIDIALOGFACTORY->updateMouseWidget(DrawCloudLinePolygonCommand::tr("Specify first point"), DrawCloudLinePolygonCommand::tr("Cancel"));
        break;
    case SetEndPoint:
        if (m_points.isError)
        {
            GUIDIALOGFACTORY->updateMouseWidget(DrawCloudLinePolygonCommand::tr("Can not create cloud line, please select again"), DrawCloudLinePolygonCommand::tr("Cancel"));
        }
        else
        {
            GUIDIALOGFACTORY->updateMouseWidget(DrawCloudLinePolygonCommand::tr("Specify next point"), DrawCloudLinePolygonCommand::tr("Cancel"));
        }
        break;
    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void CloudLinePolygonTool::undo()
{
    if (m_points.history.size() > 1)
    {
        m_points.history.pop_back();
        drawPoly(m_command.preview().entities().getEntityContainer());
    }
    else
    {
        GUIDIALOGFACTORY->commandMessage(DrawCloudLinePolygonCommand::tr("Cannot undo: Not enough entities defined yet."));
    }
}

void CloudLinePolygonTool::drawPoly(DmEntityContainer* host)
{
    m_command.preview().clear();
    std::vector<DmVector> polyPts(m_points.history);
    polyPts.push_back(m_points.mousePt);

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
    if (!(m_command.getMinLength() > 0 && totalLen / m_command.getMinLength() > CLOUD_MAX_RATIO_THRESHOLD))   // 范围不是太大，才考虑绘制
    {
        poly = calculateCloudPoly(host, polyPts);
    }
    Q_UNUSED(poly);
    m_command.preview().draw();
}
void CloudLineFreeTool::onMouseMove(QMouseEvent* e)
{
    DmVector mouse = snapper()->snapPoint(e);
    if (status() == SetEndPoint && !m_points.isClosed)
    {
        DmVector first = m_points.history.front();
        DmVector deltaVec = first - mouse;
        DmVector deviceVec = view()->toGui(first) - view()->toGui(mouse);
        if (m_points.history.back().distanceTo(mouse) > CLOUD_MIN_POINT_DISTANCE)
        {
            m_points.history.push_back(mouse);
        }

        // 计算历史点的范围
        double minX = CLOUD_LARGE_COORD, minY = CLOUD_LARGE_COORD;
        double maxX = -CLOUD_LARGE_COORD, maxY = -CLOUD_LARGE_COORD;
        for (auto& pt : m_points.history)
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
        double deltaDeviceX = view()->toGuiDX(deltaX);
        double deltaY = maxY - minY;
        double deltaDeviceY = view()->toGuiDX(deltaY);
        if ((deltaDeviceX > CLOUD_CLOSE_DEVICE_THRESHOLD || deltaDeviceY > CLOUD_CLOSE_DEVICE_THRESHOLD)
            && deviceVec.magnitude() < CLOUD_CLOSE_MAGNITUDE_THRESHOLD)    // 闭合并结束命令
        {
            m_points.isClosed = true;
            m_points.polyline = getPoly(nullptr);
            if (m_points.polyline)
            {
                document()->specifyModifiedEntity(m_points.polyline);
                commit();
            }
            updateHints();
            init(status() - 2);
        }
        drawPoly(m_command.preview().entities().getEntityContainer());
    }
}

void CloudLineFreeTool::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        DmVector snapped = snapper()->snapPoint(e);
        onCoordinate(snapped);
    }
    else if (e->button() == Qt::RightButton)
    {
        m_command.preview().clear();
        snapper()->deleteSnapper();
        switch (status())
        {
        default:
        case SetStartPoint:
            init(status() - 1);
            break;
        case SetEndPoint:
            onKeyPress(nullptr);
            break;
        }
    }
}

void CloudLineFreeTool::onKeyPress(QKeyEvent* e)
{
    if (e != nullptr && e->key() != Qt::Key_Enter)
    {
        e->ignore();
        return;
    }
    if (m_points.history.size() <= CLOUD_MIN_HISTORY_POINTS)
    {
        m_points.isError = true;
        updateHints();
        m_points.isError = false;
        if (e)
        {
            e->ignore();
        }
        return;
    }
    m_points.polyline = getPoly(nullptr);
    if (m_points.polyline == nullptr)
    {
        m_points.isError = true;
        updateHints();
        if (status() == SetStartPoint)
        {
            init(status() - 1);
        }
        else if (status() == SetEndPoint)
        {
            init(status() - 2);
        }
    }
    else
    {
        document()->specifyModifiedEntity(m_points.polyline);
        commit();
        init(status() - 2);
    }
    if (e)
    {
        e->ignore();
    }
}

void CloudLineFreeTool::onCoordinate(const DmVector& coord){
    DmVector mouse = coord;
    switch (status())
    {
    case SetStartPoint:
        m_points.history.push_back(mouse);
        setStatus(SetEndPoint);
        updateHints();
        break;
    default:
        break;
    }
}

void CloudLineFreeTool::updateHints()
{
    switch (status())
    {
    case SetStartPoint:
        GUIDIALOGFACTORY->updateMouseWidget(DrawCloudLineFreeCommand::tr("Specify first point"), DrawCloudLineFreeCommand::tr("Cancel"));
        break;
    case SetEndPoint:
        if (m_points.isClosed)
        {
            GUIDIALOGFACTORY->updateMouseWidget(DrawCloudLineFreeCommand::tr("Cloud line done!"), DrawCloudLineFreeCommand::tr("Cancel"));
        }
        else
        {
            GUIDIALOGFACTORY->updateMouseWidget(DrawCloudLineFreeCommand::tr("Move cursor to get cloud line path..."), DrawCloudLineFreeCommand::tr("Cancel"));
        }
        break;
    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

DmPolyline* CloudLineFreeTool::getPoly(DmEntityContainer* host)
{
    if (m_points.history.size() <= 1)
    {
        return nullptr;
    }
    bool isClockwise = GeometryMethods::isPtsClockwise(m_points.history);
    double bulge = isClockwise ? -CLOUD_DEFAULT_BULGE : CLOUD_DEFAULT_BULGE;
    if (m_command.getReversed())
    {
        bulge = -bulge;
    }
    DmPolyline* resPoly = nullptr;
    if (m_points.isClosed)
    {
        std::vector<double> bulges(m_points.history.size(), CLOUD_DEFAULT_BULGE);
        std::vector<double> weights(m_points.history.size() * 2, 0.0);
        resPoly = new DmPolyline(host, PolylineData(m_points.history, bulges, weights, true));
    }
    else
    {
        std::vector<double> bulges(m_points.history.size() - 1, CLOUD_DEFAULT_BULGE);
        std::vector<double> weights((m_points.history.size() - 1) * 2, 0.0);
        resPoly = new DmPolyline(host, PolylineData(m_points.history, bulges, weights, false));
    }
    resPoly->setDocument(document());
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

void CloudLineFreeTool::drawPoly(DmEntityContainer* host)
{
    if (host == nullptr)
    {
        return;
    }
    m_command.preview().clear();
    DmPolyline* poly = getPoly(host);
    Q_UNUSED(poly);
    m_command.preview().draw();
}
}  // namespace

void CloudLineCommand::commitCloudLine(DmPolyline* polyline, const QString& transactionName)
{
    Transaction t(transactionName.toStdString(), document());
    t.start();
    document()->getEntityTable()->add(polyline);
    t.commit();
}

void CloudLineCommand::showOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, true);
}

void CloudLineCommand::hideOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, false);
}

std::unique_ptr<BasePlaceTool> DrawCloudLineRectangleCommand::createTool()
{
    return std::make_unique<CloudLineRectangleTool>(*this, document(), view());
}

std::unique_ptr<BasePlaceTool> DrawCloudLinePolygonCommand::createTool()
{
    return std::make_unique<CloudLinePolygonTool>(*this, document(), view());
}

void DrawCloudLinePolygonCommand::undo()
{
    if (placeTool())
    {
        static_cast<CloudLinePolygonTool*>(placeTool())->undo();
    }
}

std::unique_ptr<BasePlaceTool> DrawCloudLineFreeCommand::createTool()
{
    return std::make_unique<CloudLineFreeTool>(*this, document(), view());
}

namespace
{
const bool g_registeredRectangle = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionCloudLineRectangle, QStringLiteral("draw.cloud_line_rectangle"),
    exclusiveCommandFactory<DrawCloudLineRectangleCommand>());

const bool g_registeredPolygon = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionCloudLinePolygon, QStringLiteral("draw.cloud_line_polygon"),
    exclusiveCommandFactory<DrawCloudLinePolygonCommand>());

const bool g_registeredFree = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionCloudLineFree, QStringLiteral("draw.cloud_line_free"), exclusiveCommandFactory<DrawCloudLineFreeCommand>());
}  // namespace
