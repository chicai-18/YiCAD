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


/// @file DmHatch.cpp
/// @brief 填充（Hatch）实体实现，支持实体填充和图案填充

#include <iostream>
#include <cmath>
#include <memory>

#include <QPainterPath>
#include <QBrush>
#include <QString>
#include "GuiDocumentView.h"
#include "GuiDialogFactory.h"
#include "InfoArea.h"
#include "Information.h"
#include "DmPattern.h"
#include "DmPatternList.h"
#include "Math2d.h"
#include "Debug.h"
#include "DmConstructionLine.h"
#include "DmLine.h"
#include "DmArc.h"
#include "DmCircle.h"
#include "DmPoint.h"
#include "DmPolyline.h"
#include "DmEllipse.h"
#include "DmHatch.h"
#include "DmSolid.h"
#include "DmEntityHelper.h"
#include "UIDialogFactory.h"
#include "ConstrainedDelaunayTriangulation.h"
#include "GeometryMethods.h"

TYPESYSTEM_SOURCE(DmHatch, DmEntity, 0);

DmHatch::DmHatch(DmEntity* parent, const HatchData& hatchdata)
    : DmEntity(parent)
    , data(hatchdata)
    , m_filledEntities(new DmEntityContainer())
{
    calculateBorders();
}

DmHatch::DmHatch(DmEntity* parent, DmHatch& hatchdata)
    : DmEntity(parent)
    , m_filledEntities(new DmEntityContainer())
{
    setData(hatchdata.getData());
    calculateBorders();
}

DmHatch::DmHatch(const DmHatch& hatch)
{
    data = hatch.data;
    if (hatch.getBoundary())
    {
        DmRegion* boundary =
            static_cast<DmRegion*>(hatch.data.getBoundary()->clone());
        data.setBoundary(DmRegionPtr(boundary));
    }
    if (hatch.m_filledEntities)
    {
        auto c = static_cast<DmEntityContainer*>(
            hatch.m_filledEntities->clone());
        m_filledEntities = std::make_shared<DmEntityContainer>(c);
    }
}

DmHatch::~DmHatch()
{
}

DmHatch* DmHatch::clone() const
{
    DmHatch* t = new DmHatch(*this);
    t->m_ulID = DmId();
    t->setSelected(false);
    t->setHighlighted(false);
    return t;
}

DM::EntityType DmHatch::getEntityType() const
{
    return DM::EntityHatch;
}

bool DmHatch::isContainer() const
{
    return false;
}

bool DmHatch::isSolid() const
{
    return data.isSolid();
}

void DmHatch::setSolid(bool solid)
{
    data.setIsSolid(solid);
}

void DmHatch::setData(const HatchData& hdata)
{
    data = hdata;
}

bool DmHatch::setSelected(bool select)
{
    bool res = DmEntity::setSelected(select);
    if (m_filledEntities)
    {
        m_filledEntities->setSelected(select);
    }
    return false;
}

QString DmHatch::getPattern() const
{
    return QString::fromStdWString(data.getPatternName());
}

void DmHatch::setPattern(const QString& pattern)
{
    data.setPatternName(pattern.toStdWString());
}

double DmHatch::getScale() const
{
    return data.getPatternScale();
}

void DmHatch::setScale(double scale)
{
    data.setPatternScale(scale);
}

double DmHatch::getAngle() const
{
    return data.getPatternAngle();
}

void DmHatch::setAngle(double angle)
{
    data.setPatternAngle(angle);
}

void DmHatch::setBoundary(DmRegionPtr boundary)
{
    data.setBoundary(boundary);
}

DmRegionPtr DmHatch::getBoundary() const
{
    return data.getBoundary();
}

DmEntityContainerPtr DmHatch::getFilledEntities() const
{
    return m_filledEntities;
}

HatchData& DmHatch::getDataRef()
{
    return data;
}

HatchData DmHatch::getData() const
{
    // 克隆边界信息
    HatchData d = data;
    d.setBoundary(DmRegionPtr()); // 重置边界
    auto boundary = data.getBoundary();
    if (boundary)
    {
        auto boundaryData = boundary->getCloneData();
        boundary->setData(boundaryData);
        d.setBoundary(boundary);
    }
    return d;
}

void DmHatch::calculateBorders()
{
    if (!data.getBoundary())
    {
        return;
    }

    minV = data.getBoundary()->getMin();
    maxV = data.getBoundary()->getMax();
}

void DmHatch::update()
{
    DmRegionPtr boundary = data.getBoundary();
    // 没有轮廓不能创建填充
    if (!boundary || boundary->size() == 0)
    {
        return;
    }
    // 清除原始填充
    if (m_filledEntities)
    {
        m_filledEntities->clear();
    }
    else
    {
        m_filledEntities = std::make_shared<DmEntityContainer>(nullptr);
    }

    // 实体填充
    if (data.isSolid())
    {
        fillSolid();
        calculateBorders();
        return;
    }

    // 线段填充
    calculateBorders();
    DmPen hatch_pen(DmColor(DM::FlagByBlock), DM::Width00,
        DmLineTypeTable::Continuous);
    DmPattern pattern = data.getPattern();
    auto* pat = &pattern;
    if (pat->getPatternData().size() == 0)
    {
        pat = DMPATTERNLIST->requestPattern(
            QString::fromStdWString(data.getPatternName()));
    }
    if (!pat)
    {
        return;
    }

    m_filledEntities.reset(new DmEntityContainer(this));
    m_filledEntities->setPen(hatch_pen);
    m_filledEntities->setLayer(nullptr);
    m_filledEntities->setFlag(DM::FlagTemp);
    pat->scale(data.getPatternScale());
    pat->angle(Math2d::rad2deg(data.getPatternAngle()));
    std::vector<std::vector<double>> pat_data = pat->getPatternData();
    for (int i = 0; i < pat_data.size(); i++)
    {
        // 单独每一项
        std::vector<double> pat_per_data = pat_data[i];
        fillPattern(m_filledEntities, pat_per_data,
            minV.x, maxV.x, minV.y, maxV.y);
    }
}

void DmHatch::fillSolid()
{
    if (m_filledEntities)
    {
        m_filledEntities->clear();
    }
    std::vector<DmTriangle*> triangles;
    data.getBoundary()->getTriangles<DmTriangle*>(triangles, true);
    DmPen pen(DmColor(DM::FlagByBlock), DM::Width00,
        DmLineTypeTable::Continuous);
    for (auto tri : triangles)
    {
        tri->setParent(m_filledEntities.get());
        tri->setPen(pen);
        tri->setLayer(nullptr);
        m_filledEntities->addEntity(tri);
    }
}

void DmHatch::fillPattern(DmEntityContainerPtr parent,
    const std::vector<double>& pat,
    double minX, double maxX, double minY, double maxY)
{
    /** 总体实现方法：
    * 1、用包围框（minX，maxX，minY，maxY），pattern起始点及方向，
    *    求出pattern线迭代范围（minPos，maxPos）；
    * 2、逐条pat线填充——每条pattern线与轮廓求出交点并排序，
    *    每2个相邻交点判断是否在轮廓内，在范围内则按pattern的数据填充。
    **/

    minX -= 1.0;
    maxX += 1.0;
    minY -= 1.0;
    maxY += 1.0;

    double angle_rad = pat[0];
    double angle_deg = Math2d::rad2deg(angle_rad);
    double startX = pat[1];
    double startY = pat[2];
    double deltaX = pat[3];
    double deltaY = pat[4];
    bool isSolidPat = (pat.size() == 5);

    // 计算范围角点
    DmVector boundaryPt1(0.0, 0.0);
    DmVector boundaryPt2(0.0, 0.0);
    double tempAngle_deg = angle_deg;
    if (tempAngle_deg > 180.0)
    {
        tempAngle_deg = (int)tempAngle_deg % 180;
    }
    if (tempAngle_deg < 90.0)
    {
        // 左上，右下
        boundaryPt1.x = minX;
        boundaryPt1.y = maxY;
        boundaryPt2.x = maxX;
        boundaryPt2.y = minY;
    }
    else
    {
        // 左下，右上
        boundaryPt1.x = minX;
        boundaryPt1.y = minY;
        boundaryPt2.x = maxX;
        boundaryPt2.y = maxY;
    }

    // 计算角点对应pat的位置
    DmVector patStart(startX, startY);
    DmVector patDir(Math2d::correctAngle2(angle_rad));
    DmVector offsetVec_local(deltaX, deltaY);
    double offsetVec_local_csc_abs =
        std::abs(offsetVec_local.magnitude() / deltaY);
    DmVector offsetVec = DmVector(offsetVec_local).rotate(angle_rad);
    DmVector offsetVec_yPart = DmVector(0.0, deltaY).rotate(angle_rad);
    DmVector offsetDir = DmVector(offsetVec).normalize();
    double offsetDist = offsetVec.magnitude();
    DmConstructionLine patLine(nullptr,
        DmConstructionLineData(patStart, patStart + patDir));
    double dist1 = patLine.getDistanceToPoint(boundaryPt1);
    DmVector tempVec1 = boundaryPt1 - patStart;
    double dotP1 = offsetVec_yPart.dotP(tempVec1);
    // 角点1（boundaryPt1）相对与起始pat的（法向）偏移值
    double pos1 = dist1;
    if (dotP1 < 0)
    {
        pos1 = -dist1;
    }
    double dist2 = patLine.getDistanceToPoint(boundaryPt2);
    DmVector tempVec2 = boundaryPt2 - patStart;
    double dotP2 = offsetVec_yPart.dotP(tempVec2);
    // 角点2（boundaryPt2）相对与起始pat的（法向）偏移值
    double pos2 = dist2;
    if (dotP2 < 0)
    {
        pos2 = -dist2;
    }

    // 如果需要填充虚线，先获得其数据
    std::vector<double> dashLineLengths;
    dashLineLengths.reserve(pat.size() - 5);
    double curDashLen = 0.0;
    double totalDashLen = 0.0;
    for (auto i = 5; i < pat.size(); i++)
    {
        curDashLen = pat.at(i);
        dashLineLengths.emplace_back(curDashLen);
        totalDashLen += std::abs(curDashLen);
    }
    // 虚线非空段（起点终点）到初始位置距离的组合
    std::vector<std::pair<double, double>> dashPosPairs;
    getDashPositionPairs(dashLineLengths, dashPosPairs);

    // 逐条pat线填充
    double minPos = std::min(pos1, pos2);
    double maxPos = std::max(pos1, pos2);
    double absOffsetY_local = std::abs(offsetVec_local.y);
    // 当前pattern线相对于起始pattern线的位置
    double curPos = std::floor(minPos / absOffsetY_local) * absOffsetY_local;
    std::vector<DmVector> border_nodes;
    DmVector currentPatPt(0.0, 0.0); // 当前pat点
    DmVector patNode1(0.0, 0.0);
    DmVector patNode2(0.0, 0.0);
    bool duplicatePt = false;
    double k = 0.0;
    DmVector intersectPt1(0.0, 0.0);
    DmVector intersectPt2(0.0, 0.0);
    bool bUseYBoundary = false; // y作为边界
    if (tempAngle_deg > 45 && tempAngle_deg < 135)
    {
        // y作为边界
        bUseYBoundary = true;
        k = 1.0 / std::tan(angle_rad);
    }
    else
    {
        // x作为边界
        bUseYBoundary = false;
        k = std::tan(angle_rad);
    }
    DmColor c(DM::FlagByBlock);
    DmPen pen(c, DM::Width00, DmLineTypeTable::Continuous);
    while (curPos < maxPos)
    {
        border_nodes.clear();
        // pat起始点移动到当前pat线的位置
        currentPatPt = patStart + offsetDir * curPos * offsetVec_local_csc_abs;
        // 计算pat的两个端点，用来与轮廓求交
        if (bUseYBoundary)
        {
            // y作为边界
            patNode1.y = minY;
            patNode1.x = k * (minY - currentPatPt.y) + currentPatPt.x;
            patNode2.y = maxY;
            patNode2.x = k * (maxY - currentPatPt.y) + currentPatPt.x;
        }
        else
        {
            // x作为边界
            patNode1.x = minX;
            patNode1.y = k * (minX - currentPatPt.x) + currentPatPt.y;
            patNode2.x = maxX;
            patNode2.y = k * (maxX - currentPatPt.x) + currentPatPt.y;
        }

        // 计算pat线与轮廓的交点，并且排序
        intersectBoundariesWithLine(patNode1, patNode2,
            bUseYBoundary, border_nodes);

        // 按pat填充
        if (border_nodes.size() >= 2)
        {
            for (auto i = 0; i < border_nodes.size() - 1; i++)
            {
                intersectPt1 = border_nodes.at(i);
                intersectPt2 = border_nodes.at(i + 1);

                // 判断该线段是否在轮廓内（抄的原始的）
                std::unique_ptr<DmLine> line =
                    std::make_unique<DmLine>(nullptr,
                        intersectPt1, intersectPt2);
                line->setPen(pen);
                DmVector middlePoint = line->getMiddlePoint();
                DmVector lineDir(line->getStartAngle());
                DmVector middlePoint2 =
                    line->getStartpoint() + lineDir * line->getLength() / 2.1;
                bool isInside = false;
                if (middlePoint.valid)
                {
                    bool onContour = false;
                    if (data.getBoundary()->isPointInside(middlePoint,
                            &onContour)
                        || data.getBoundary()->isPointInside(middlePoint2))
                    {
                        isInside = true;
                    }
                }
                if (!isInside)
                {
                    continue;
                }
                if (isSolidPat)
                {
                    // 实线填充
                    line->setParent(parent.get());
                    parent->addEntity(line.get());
                    line.release();
                }
                else
                {
                    // 虚线填充
                    fillDashBetweenTwoPoints(parent,
                        intersectPt1, intersectPt2,
                        currentPatPt, patDir,
                        totalDashLen, dashPosPairs);
                }
            }
        }

        // 迭代下一条pat线
        curPos += absOffsetY_local;
    }
}

void DmHatch::getDashPositionPairs(
    const std::vector<double>& dashLineLengths,
    std::vector<std::pair<double, double>>& dashPosPairs)
{
    double curPos = 0.0;
    for (auto dashLen : dashLineLengths)
    {
        if (dashLen == 0.0)
        {
            dashPosPairs.emplace_back(std::make_pair(curPos, curPos));
        }
        else if (dashLen > 0.0)
        {
            dashPosPairs.emplace_back(
                std::make_pair(curPos, curPos + dashLen));
            curPos += dashLen;
        }
        else
        {
            curPos += std::abs(dashLen);
        }
    }
}

void DmHatch::intersectBoundariesWithLine(const DmVector& linePt1,
    const DmVector& linePt2, const bool orderByY,
    std::vector<DmVector>& intersectPts)
{
    bool duplicatePt = false;
    constexpr double TOL = 1e-5;

    // 收集所有边界及孔洞
    RegionData d = data.getBoundary()->getData();
    DmEntityContainer ec(nullptr, false);
    for (auto e : *d.getBoundary())
    {
        ec.addEntity(e);
    }
    for (auto h : d.getHoles())
    {
        for (auto e : *h)
        {
            ec.addEntity(e);
        }
    }
    if (ec.size() == 0)
    {
        return;
    }

    // 边界交点集
    for (auto p : ec)
    {
        DmLine tempLine(linePt1, linePt2);
        DmVectorSolutions sol =
            Information::getIntersection(&tempLine, p, true);
        for (const DmVector& vp : sol)
        {
            if (vp.valid)
            {
                // 如果不是重复点，保存之
                duplicatePt = false;
                for (auto borderPt : intersectPts)
                {
                    if (std::abs(borderPt.x - vp.x) < TOL
                        && std::abs(borderPt.y - vp.y) < TOL)
                    {
                        duplicatePt = true;
                        break;
                    }
                }
                if (!duplicatePt)
                {
                    intersectPts.emplace_back(vp);
                }
            }
        }
    }
    // 按坐标轴排序
    if (orderByY)
    {
        std::sort(intersectPts.begin(), intersectPts.end(),
            [](const DmVector& p1, const DmVector& p2)
            {
                return p1.y < p2.y;
            });
    }
    else
    {
        std::sort(intersectPts.begin(), intersectPts.end(),
            [](const DmVector& p1, const DmVector& p2)
            {
                return p1.x < p2.x;
            });
    }
}

void DmHatch::fillDashBetweenTwoPoints(DmEntityContainerPtr parent,
    const DmVector& intersectPt1, const DmVector& intersectPt2,
    const DmVector& currentPatPt, const DmVector& patDir,
    const double totalDashLen,
    const std::vector<std::pair<double, double>>& dashPosPairs)
{
    DmColor c(DM::FlagByBlock);
    DmPen pen(c, DM::Width00, DmLineTypeTable::Continuous);
    // 虚线填充
    DmVector intersectVec1 = intersectPt1 - currentPatPt;
    double intersectLen1 = intersectVec1.magnitude();
    double intersectDotP1 = intersectVec1.dotP(patDir);
    double intersectPos1 =
        intersectDotP1 > 0.0 ? intersectLen1 : -intersectLen1;
    DmVector intersectVec2 = intersectPt2 - currentPatPt;
    double intersectLen2 = intersectVec2.magnitude();
    double intersectDotP2 = intersectVec2.dotP(patDir);
    double intersectPos2 =
        intersectDotP2 > 0.0 ? intersectLen2 : -intersectLen2;
    double minPos = std::min(intersectPos1, intersectPos2);
    double maxPos = std::max(intersectPos1, intersectPos2);
    // 计算起始位置
    double curPos = std::floor(minPos / totalDashLen) * totalDashLen;
    double rangeStart = 0.0;
    double rangeEnd = 0.0;
    DmVector rangeStartPt(0.0, 0.0);
    DmVector rangeEndPt(0.0, 0.0);
    // 0表示不在范围内，1表示起点在范围内（终点不在范围），
    // 2表示终点在范围内（起点不在范围），3表示完全在范围内
    int inRangeType = 0;
    while (curPos < maxPos)
    {
        for (auto posPair : dashPosPairs)
        {
            inRangeType = 0;
            rangeStart = curPos + posPair.first;
            rangeStartPt = currentPatPt + patDir * rangeStart;
            rangeEnd = curPos + posPair.second;
            rangeEndPt = currentPatPt + patDir * rangeEnd;
            if (rangeStart >= minPos && rangeEnd <= maxPos)
            {
                inRangeType = 3;
            }
            else if (rangeStart >= minPos && rangeStart <= maxPos)
            {
                rangeEnd = maxPos;
                rangeEndPt = currentPatPt + patDir * rangeEnd;
                inRangeType = 1;
            }
            else if (rangeEnd >= minPos && rangeEnd <= maxPos)
            {
                rangeStart = minPos;
                rangeStartPt = currentPatPt + patDir * rangeStart;
                inRangeType = 2;
            }
            else
            {
                // 不在范围内
            }
            if (inRangeType != 0)
            {
                if (posPair.first == posPair.second)
                {
                    // 长度为0，填点
                    if (inRangeType == 3)
                    {
                        DmPoint* pPoint = new DmPoint(parent.get(),
                            PointData(rangeStartPt));
                        pPoint->setPen(pen);
                        parent->addEntity(pPoint);
                    }
                }
                else
                {
                    // 长度不为0，填线段
                    DmLine* pLine = new DmLine(parent.get(),
                        LineData(rangeStartPt, rangeEndPt));
                    pLine->setPen(pen);
                    parent->addEntity(pLine);
                }
            }
        }
        curPos += totalDashLen;
    }
}

void DmHatch::move(const DmVector& offset)
{
    data.getBoundary()->move(offset);
    m_filledEntities->move(offset);
    DmEntity::moveBorders(offset);
}

void DmHatch::rotate(const DmVector& center, const DmVector& angleVector)
{
    data.getBoundary()->rotate(center, angleVector);
    m_filledEntities->rotate(center, angleVector);
    data.setPatternAngle(
        Math2d::correctAngle(data.getPatternAngle() + angleVector.angle()));
    calculateBorders();
}

void DmHatch::scale(const DmVector& center, const DmVector& factor)
{
    data.getBoundary()->scale(center, factor);
    m_filledEntities->scale(center, factor);
    calculateBorders();
    data.setPatternScale(data.getPatternScale() * factor.x);
}

void DmHatch::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    data.getBoundary()->mirror(axisPoint1, axisPoint2);
    m_filledEntities->mirror(axisPoint1, axisPoint2);
    calculateBorders();
    double ang = axisPoint1.angleTo(axisPoint2);
    data.setPatternAngle(
        Math2d::correctAngle(data.getPatternAngle() + ang * 2.0));
}

std::list<DmEntity*> DmHatch::getSubEntities() const
{
    std::list<DmEntity*> subEnts = std::list<DmEntity*>();
    // 没有填充实体
    if (!data.getBoundary() || !m_filledEntities)
    {
        return subEnts;
    }
    // 图案填充
    auto listEnts = m_filledEntities->getEntityList();
    if (listEnts.size() > 0)
    {
        auto seSub = std::list<DmEntity*>(listEnts.begin(), listEnts.end());
        subEnts.splice(subEnts.end(), seSub);
    }
    return subEnts;
}

DmVectorSolutions DmHatch::getRefPoints() const
{
    if (!data.getBoundary() || !m_filledEntities)
    {
        return DmVectorSolutions();
    }

    DmVector min = data.getBoundary()->getMin();
    DmVector max = data.getBoundary()->getMax();
    DmVector mid = (min + max) / 2.0;
    return DmVectorSolutions({mid});
}

void DmHatch::moveRef(const DmVector& ref, const DmVector& offset)
{
    move(offset);
    calculateBorders();
}

DmVector DmHatch::getNearestSelectedRef(const DmVector& coord,
    double* dist) const
{
    return DmEntity::getNearestSelectedRef(coord, dist);
}

DmVector DmHatch::getNearestRef(const DmVector& coord, double* dist) const
{
    return DmEntity::getNearestRef(coord, dist);
}

DmVector DmHatch::getNearestEndpoint(const DmVector& coord,
    double* dist) const
{
    return DmVector(false);
}

DmVector DmHatch::getNearestPointOnEntity(const DmVector& coord,
    bool onEntity, double* dist, DmEntity** entity) const
{
    if (entity)
    {
        *entity = const_cast<DmHatch*>(this);
    }
    bool onBoundary = false;
    if (data.getBoundary()->isPointInside(coord, &onBoundary))
    {
        if (dist)
        {
            *dist = 0.0;
            return coord;
        }
    }
    // 区域外
    else
    {
        data.getBoundary()->getNearestPointOnEntity(coord, true, dist, nullptr);
    }

    return DmVector(false);
}

DmVector DmHatch::getNearestCenter(const DmVector& coord,
    double* dist) const
{
    return DmVector(false);
}

DmVector DmHatch::getNearestMiddle(const DmVector& coord,
    double* dist, int middlePoints) const
{
    return DmVector(false);
}

void DmHatch::saveStream(OutputStream& wrt) const
{
    DmEntity::saveStream(wrt);

    auto name = getPattern().toStdString();
    auto angle = getAngle();
    auto scale = getScale();
    bool isSolid = data.isSolid();
    wrt << name << (double)angle << (double)scale << (bool)isSolid;

    // pattern
    auto pattern = data.getPattern().getPatternData();
    auto patternsize = pattern.size();
    wrt << (uint32_t)patternsize;
    for (auto patline : pattern)
    {
        auto patlinesize = patline.size();
        wrt << (uint32_t)patlinesize;
        for (int i = 0; i < patlinesize; i++)
        {
            double dat = patline[i];
            wrt << (double)dat;
        }
    }

    // 边界
    data.getBoundary()->saveStream(wrt);
}

void DmHatch::restoreStream(InputStream& reader,
    const std::vector<PAIR>& revs)
{
    int fileRev = getRevisionId("DmHatch", revs);
    if (revId > fileRev)
    {
        DmEntity::restoreStream(reader, revs);
        // 老文件格式
        restoreStreamWithRev(reader, fileRev);
    }
    else
    {
        restoreStream(reader);
    }
}

void DmHatch::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (0 == rev)
    {
        // 基本版本，无需额外处理
    }
    else // big change, e.g. change super class of DmCircle
    {
        // step1.
        // read all legacy data one by one
    }
}

void DmHatch::restoreStream(InputStream& reader)
{
    DmEntity::restoreStream(reader);

    std::string name;
    double angle = 0.0;
    double scale = 0.0;
    bool isSolid = false;
    reader >> (std::string&)name >> (double&)angle
           >> (double&)scale >> (bool&)isSolid;
    setPattern(QString::fromStdString(name));
    setAngle(angle);
    setScale(scale);
    data.setIsSolid(isSolid);

    // pattern
    std::vector<std::vector<double>> patdata =
        std::vector<std::vector<double>>{};
    uint32_t patternsize = 0;
    reader >> (uint32_t&)patternsize;
    for (int i = 0; i < patternsize; i++)
    {
        std::vector<double> patline = std::vector<double>{};
        uint32_t patlinesize = 0;
        reader >> (uint32_t&)patlinesize;
        for (int j = 0; j < patlinesize; j++)
        {
            double dat = 0.0;
            reader >> (double&)dat;
            patline.emplace_back(dat);
        }
        patdata.emplace_back(patline);
    }

    std::wstring wname(name.begin(), name.end());
    DmPattern pattern(wname);
    pattern.setPatternData(patdata);
    data.setPattern(pattern);

    // 边界
    data.getBoundary()->restoreStream(reader);
}
