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


/// @file DmPolyline.cpp
/// @brief 多段线实体类实现：段管理、几何变换、子实体生成、持久化

#include <iostream>
#include <cmath>
#include <cassert>
#include "DmPolyline.h"

#include "Debug.h"
#include "DmLine.h"
#include "DmArc.h"
#include "GuiDocumentView.h"
#include "Math2d.h"
#include "Information.h"
#include "DmSolid.h"
#include "GeometryMethods.h"

TYPESYSTEM_SOURCE(DmPolyline, DmEntity, 0);

DmPolyline::DmPolyline(DmEntity* parent)
    : DmEntity(parent)
{
}

DmPolyline::DmPolyline(DmEntity* parent, const PolylineData& d)
    : DmEntity(parent)
    , data(d)
{
}

DmPolyline::DmPolyline(const DmPolyline& poly)
    : DmEntity(poly)  // 复制基类的信息
{
    data = poly.getDataConstRef();
}

DmPolyline::~DmPolyline()
{
    clear();
}

DmEntity* DmPolyline::clone() const
{
    DmPolyline* p = new DmPolyline(*this);
    p->m_ulID = DmId();
    p->setSelected(false);
    p->setHighlighted(false);
    p->entities.clear();
    p->update();
    return p;
}

DM::EntityType DmPolyline::getEntityType() const
{
    return DM::EntityPolyline;
}

double DmPolyline::getLength() const
{
    double total = 0.0;
    int segmentCount = getSegmentCount();
    double bulge = 0.0;
    DmVector pt1(false), pt2(false);

    for (int i = 0; i < segmentCount; i++)
    {
        getSegmentInfoAt(i, bulge, pt1, pt2);

        if (fabs(bulge) < 1e-5)
        {
            total += pt1.distanceTo(pt2);
        }
        else
        {
            DmVector center(false), normal(false);
            double radius = 0.0, startAngle = 0.0, endAngle = 0.0;
            GeometryMethods::getArcInfo(pt1, pt2, bulge, center,
                                        radius, startAngle, endAngle, normal);
            total += radius * fabs(endAngle - startAngle);
        }
    }

    return total;
}

PolylineData DmPolyline::getData() const
{
    return data;
}

PolylineData& DmPolyline::getDataRef()
{
    return data;
}

const PolylineData& DmPolyline::getDataConstRef() const
{
    return data;
}

void DmPolyline::setData(const PolylineData& data)
{
    this->data = data;
}

int DmPolyline::getSegmentCount() const
{
    return data.getBulgesCount();
}

void DmPolyline::insertVertex(int i, const DmVector& v, double bulge,
                              double startWeight, double endWeight)
{
    data.insertVertex(i, v);
    data.insertBulge(i - 1, bulge);  // 设置以该点为终点的线段凸度
    data.insertLineWeight(i - 1, startWeight, endWeight);
}

void DmPolyline::appendVertex(const DmVector& v, double bulge,
                              double startWeight, double endWeight)
{
    // 第一次仅添加点
    if (data.getVertexCount() == 0)
    {
        data.appendVertex(v);
    }
    // 后面则添加所有
    else
    {
        data.appendVertex(v);
        data.appendBulge(bulge);
        data.appendLineWeight(startWeight, endWeight);
    }
}

void DmPolyline::update()
{
    clear();

    if (!isValid())
    {
        bool res = validateData();
        if (!res)
        {
            return;
        }
    }

    // 生成实体
    std::vector<double> bulges = data.getBulges();
    std::vector<DmVector> vertexes = data.getVertexs();
    std::vector<double> weights = data.getLineWeights();
    int bulgeCount = bulges.size();
    double bulge = 0.0;
    DmVector startPt(true), endPt(true), center(true);
    DmVector normal(0.0, 0.0, 1.0);
    double startAngle = 0.0, endAngle = 0.0, radius = 0.0;
    double startWeight = 0.0, endWeight = 0.0;

    for (int i = 0; i < bulgeCount; i++)
    {
        bulge = bulges.at(i);
        startPt = vertexes.at(i);

        // 最后一个实体
        if (i == bulgeCount - 1 && isClosed())
        {
            // 闭合，第一个点作为终点
            endPt = vertexes.front();
        }
        // 不是最后一个实体
        else
        {
            endPt = vertexes.at(i + 1);
        }

        startWeight = weights.at(2 * i);
        endWeight = weights.at(2 * i + 1);

        // 生成实体
        addEntityByInfo(bulge, startPt, endPt, startWeight, endWeight);
    }

    calculateBorders();
}

bool DmPolyline::isValid() const
{
    int bulgeCount = data.getBulgesCount();
    int ptsCount = data.getVertexCount();

    if (ptsCount <= 1)
    {
        return false;
    }

    if (isClosed())
    {
        if (bulgeCount == ptsCount)
        {
            return true;
        }
    }
    else
    {
        if (bulgeCount == ptsCount - 1)
        {
            return true;
        }
    }

    return false;
}

bool DmPolyline::hasEntity() const
{
    return entities.size() != 0;
}

void DmPolyline::addEntityByInfo(double bulge, const DmVector& startPt,
                                 const DmVector& endPt,
                                 double startWeight, double endWeight)
{
    std::vector<DmEntity*> ents;
    getEntitiesByInfo(startPt, endPt, bulge, startWeight, endWeight, ents);
    for (auto e : ents)
    {
        e->setParent(this);
        addEntity(e);
    }
}

DmVector DmPolyline::getStartpoint() const
{
    if (isValid())
    {
        return data.getVertexAt(0);
    }
    else
    {
        return DmVector(false);
    }
}

void DmPolyline::setLayer(const QString& name)
{
    DmEntity::setLayer(name);

    // set layer for sub-entities
    for (auto* e : entities)
    {
        e->setLayer(layer);
    }
}

void DmPolyline::setLayer(DmLayer* l)
{
    layer = l;

    // set layer for sub-entities
    for (auto* e : entities)
    {
        e->setLayer(layer);
    }
}

bool DmPolyline::setSelected(bool select)
{
    if (DmEntity::setSelected(select))
    {
        for (auto e : entities)
        {
            if (e->isVisible())
            {
                e->setSelected(select);
            }
        }

        return true;
    }
    else
    {
        return false;
    }
}

void DmPolyline::setHighlighted(bool highlight)
{
    DmEntity::setHighlighted(highlight);

    for (auto e : entities)
    {
        if (e->isVisible())
        {
            e->setHighlighted(highlight);
        }
    }
}

DmVector DmPolyline::getEndpoint() const
{
    if (isValid())
    {
        if (isClosed())
        {
            return data.getVertexAt(0);
        }
        else
        {
            return data.getVertexAt(data.getVertexCount() - 1);
        }
    }

    return DmVector(false);
}

bool DmPolyline::isClosed() const
{
    return data.getIsClosed();
}

void DmPolyline::setClosed(bool cl)
{
    data.setIsClosed(cl);
}

DmVector DmPolyline::getVertexAt(int i) const
{
    return data.getVertexAt(i);
}

int DmPolyline::getVertexCount() const
{
    return data.getVertexCount();
}

double DmPolyline::getBulgeAt(int i) const
{
    return data.getBulgeAt(i);
}

bool DmPolyline::isContainer() const
{
    return false;
}

DmVector DmPolyline::getNearestEndpoint(const DmVector& coord,
                                        double* dist) const
{
    // 从所有顶点中获得最近点
    double minDist = DM_MAXDOUBLE;    // minimum measured distance
    double curDist = 0.0;             // currently measured distance
    DmVector closestPoint(false);     // closest found endpoint

    std::vector<DmVector> vertexes = getDataConstRef().getVertexs();
    for (DmVector v : vertexes)
    {
        curDist = v.distanceTo(coord);
        if (curDist < minDist)
        {
            closestPoint = v;
            minDist = curDist;
            if (dist)
            {
                *dist = minDist;
            }
        }
    }

    return closestPoint;
}

DmVectorSolutions DmPolyline::getRefPoints() const
{
    DmVectorSolutions ret;

    for (auto& pt : data.getVertexs())
    {
        ret.push_back(pt);
    }

    return ret;
}

DmVector DmPolyline::getNearestMiddle(const DmVector& coord, double* dist,
                                      int middlePoints) const
{
    int segmentCount = data.getBulgesCount();
    double bulge = 0.0;
    DmVector pt1(false), pt2(false);
    double minDist = DM_MAXDOUBLE, d = DM_MAXDOUBLE;
    DmVector res(false), temp(false);
    DmVector center(false), normal(false);
    double r = 0.0, startAng = 0.0, endAng = 0.0;

    for (int i = 0; i < segmentCount; i++)
    {
        getSegmentInfoAt(i, bulge, pt1, pt2);
        if (bulge == 0.0)
        {
            DmLine line(pt1, pt2);
            temp = line.getNearestMiddle(coord, &d, middlePoints);
        }
        else
        {
            GeometryMethods::getArcInfo(pt1, pt2, bulge, center,
                                        r, startAng, endAng, normal);
            DmArc arc(nullptr, ArcData(center, normal, r, startAng,
                                       endAng));
            temp = arc.getNearestMiddle(coord, &d, middlePoints);
        }

        if (temp.valid && d < minDist)
        {
            minDist = d;
            res = temp;
        }
    }

    if (res.valid && dist)
    {
        *dist = minDist;
    }

    return res;
}

DmVector DmPolyline::getNearestRef(const DmVector& coord,
                                   double* dist /*= nullptr*/) const
{
    return DmEntity::getNearestRef(coord, dist);
}

DmVector DmPolyline::getNearestSelectedRef(const DmVector& coord,
                                           double* dist /*= nullptr*/) const
{
    return DmEntity::getNearestSelectedRef(coord, dist);
}

DmVector DmPolyline::getNearestPointOnEntity(const DmVector& coord,
                                             bool onEntity, double* dist,
                                             DmEntity** entity) const
{
    int segmentCount = data.getBulgesCount();
    double bulge = 0.0;
    DmVector pt1(false), pt2(false);
    double minDist = DM_MAXDOUBLE, d = DM_MAXDOUBLE;
    DmVector res(false), temp(false);
    DmVector center(false), normal(false);
    double r = 0.0, startAng = 0.0, endAng = 0.0;

    for (int i = 0; i < segmentCount; i++)
    {
        getSegmentInfoAt(i, bulge, pt1, pt2);
        if (bulge == 0.0)
        {
            DmLine line(pt1, pt2);
            temp = line.getNearestPointOnEntity(coord, true, &d);
        }
        else
        {
            GeometryMethods::getArcInfo(pt1, pt2, bulge, center,
                                        r, startAng, endAng, normal);
            DmArc arc(nullptr, ArcData(center, normal, r, startAng,
                                       endAng));
            temp = arc.getNearestPointOnEntity(coord, true, &d);
        }

        if (temp.valid && d < minDist)
        {
            minDist = d;
            res = temp;
        }
    }

    if (res.valid && dist)
    {
        *dist = minDist;
    }

    return res;
}

DmVector DmPolyline::getNearestCenter(const DmVector& coord,
                                      double* dist) const
{
    int segmentCount = data.getBulgesCount();
    double bulge = 0.0;
    DmVector pt1(false), pt2(false);
    double minDist = DM_MAXDOUBLE, d = DM_MAXDOUBLE;
    DmVector res(false), temp(false);
    DmVector center(false), normal(false);
    double r = 0.0, startAng = 0.0, endAng = 0.0;

    for (int i = 0; i < segmentCount; i++)
    {
        getSegmentInfoAt(i, bulge, pt1, pt2);
        if (bulge != 0.0)
        {
            GeometryMethods::getArcInfo(pt1, pt2, bulge, center,
                                        r, startAng, endAng, normal);
            DmArc arc(nullptr, ArcData(center, normal, r, startAng,
                                       endAng));
            temp = arc.getNearestCenter(coord, &d);
        }

        if (temp.valid && d < minDist)
        {
            minDist = d;
            res = temp;
        }
    }

    if (res.valid && dist)
    {
        *dist = minDist;
    }

    return res;
}

void DmPolyline::calculateBorders()
{
    resetBorders();
    for (auto c : entities)
    {
        c->calculateBorders();
        minV = DmVector::minimum(c->getMin(), minV);
        maxV = DmVector::maximum(c->getMax(), maxV);
    }
}

bool DmPolyline::offset(const DmVector& coord, const double& distance)
{
    // TODO cc: 暂不支持多段线偏移
    // 该操作融合了延申、打断、修剪等操作，跟打断修剪等实现过后可考虑算法复用，
    // 所以留待打断修剪功能实现后再实现此处
    // 需参照AutoCAD的offset命令 1.需考虑直线+圆弧时补圆弧或裁切
    // 2.考虑多段线自相交时处理

    return false;
}

void DmPolyline::move(const DmVector& offset)
{
    auto vertexs = data.getVertexs();
    auto newVertexs = std::vector<DmVector>();
    for (auto& pt : vertexs)
    {
        pt = pt.move(offset);
        newVertexs.emplace_back(pt);
    }

    data.setVertexs(newVertexs);
    update();
}

void DmPolyline::rotate(const DmVector& center,
                        const DmVector& angleVector)
{
    auto vertexs = data.getVertexs();
    auto newVertexs = std::vector<DmVector>();
    for (auto& pt : vertexs)
    {
        pt = pt.rotate(center, angleVector);
        newVertexs.emplace_back(pt);
    }

    data.setVertexs(newVertexs);
    update();
}

void DmPolyline::scale(const DmVector& center, const DmVector& factor)
{
    // TODO : 非等比缩放仅发现在块中有使用，
    // 应该放在DmBlock或插入块的Action中
    auto vertexs = data.getVertexs();
    auto newVertexs = std::vector<DmVector>();
    for (auto& pt : vertexs)
    {
        pt = pt.scale(center, factor);
        newVertexs.emplace_back(pt);
    }

    data.setVertexs(newVertexs);
    update();
}

void DmPolyline::mirror(const DmVector& axisPoint1,
                        const DmVector& axisPoint2)
{
    auto vertexs = data.getVertexs();
    auto newVertexs = std::vector<DmVector>();
    for (auto& pt : vertexs)
    {
        newVertexs.emplace_back(pt.mirror(axisPoint1, axisPoint2));
    }

    data.setVertexs(newVertexs);

    auto bulges = data.getBulges();
    auto newBulges = std::vector<double>();
    for (auto& bulge : bulges)
    {
        newBulges.emplace_back(-bulge);
    }

    data.setBulges(newBulges);

    update();
}

void DmPolyline::moveRef(const DmVector& ref, const DmVector& offset)
{
    auto vertexs = data.getVertexs();
    auto newVertexs = std::vector<DmVector>();
    for (auto& pt : vertexs)
    {
        if (ref.distanceTo(pt) < 1.0e-4)
        {
            pt = pt.move(offset);
        }

        newVertexs.emplace_back(pt);
    }

    data.setVertexs(newVertexs);
    update();
}

std::list<DmEntity*> DmPolyline::getSubEntities() const
{
    // 子实体只有直线、圆弧、DmSolid
    return std::list<DmEntity*>(entities.begin(), entities.end());
}

void DmPolyline::clear()
{
    for (auto e : entities)
    {
        delete e;
    }

    entities.clear();
}

void DmPolyline::addEntity(DmEntity* e)
{
    if (!e)
    {
        return;
    }

    entities.emplace_back(e);
}

bool DmPolyline::validateData()
{
    int ptsCount = data.getVertexCount();
    if (ptsCount <= 1)
    {
        return false;
    }

    int needBulgeCount = 0;
    int needWeightCount = 0;

    // 闭合
    if (data.getIsClosed())
    {
        needBulgeCount = ptsCount;
        needWeightCount = ptsCount * 2;
    }
    // 不闭合
    else
    {
        needBulgeCount = ptsCount - 1;
        needWeightCount = (ptsCount - 1) * 2;
    }

    // 追加或删除数据
    int bulgeCount = data.getBulgesCount();

    // 需要追加凸度
    if (bulgeCount < needBulgeCount)
    {
        double bulge = 0.0;  // 待追加的凸度
        if (bulgeCount > 0)
        {
            bulge = data.getBulgeAt(bulgeCount - 1);
        }

        int addCount = needBulgeCount - bulgeCount;
        for (int i = 0; i < addCount; i++)
        {
            data.appendBulge(bulge);
        }
    }
    // 需要删除凸度
    else if (bulgeCount > ptsCount - 1)
    {
        auto bulges = data.getBulges();
        std::vector<double> subBulges(bulges.begin(),
                                      bulges.begin() + ptsCount - 1);
        data.setBulges(subBulges);
    }

    int weightCount = data.getLineWeightCount();
    auto lineWeights = data.getLineWeights();

    // 需要追加线宽
    if (weightCount < needWeightCount)
    {
        double weight = 0.0;  // 待追加的线宽
        if (weightCount > 0)
        {
            weight = lineWeights.back();
        }

        int addCount = needWeightCount - weightCount;
        for (int i = 0; i < addCount; i++)
        {
            data.appendLineWeight(weight);
        }
    }
    // 需要删除线宽
    else
    {
        auto weights = data.getLineWeights();
        std::vector<double> subWeights(weights.begin(),
                                       weights.begin() + needWeightCount);
        data.setLineWeights(subWeights);
    }

    return true;
}

void DmPolyline::getEntitiesByInfo(const DmVector& startPt,
                                   const DmVector& endPt,
                                   const double bulge,
                                   const double startWeight,
                                   const double endWeight,
                                   std::vector<DmEntity*>& ents)
{
    DmVector center(true), normal(0.0, 0.0, 1.0);
    double startAngle = 0.0, endAngle = 0.0, radius = 0.0;

    // 直线
    if (fabs(bulge) < 1e-5)
    {
        // 不带线宽
        if (startWeight == 0.0 && endWeight == 0.0)
        {
            DmLine* line = new DmLine(nullptr,
                                      LineData(startPt, endPt));
            line->setPen(DmPen(DM::FlagInvalid));
            line->setLayer(nullptr);
            ents.emplace_back(line);
        }
        // 带线宽
        else
        {
            auto vertexs = std::vector<DmVector>();
            DmVector dir = (startPt - endPt).normalize();
            DmVector vDir = DmVector(dir).rotate(M_PI_2);
            auto pt1 = startPt + vDir * startWeight * 0.5;
            auto pt2 = startPt - vDir * startWeight * 0.5;
            auto pt3 = endPt + vDir * endWeight * 0.5;
            auto pt4 = endPt - vDir * endWeight * 0.5;
            vertexs = { pt1, pt2, pt3, pt4 };
            SolidData solidData = SolidData(vertexs);
            DmSolid* s = new DmSolid(nullptr, solidData);
            s->setPen(DmPen(DM::FlagInvalid));
            s->setLayer(nullptr);
            ents.emplace_back(s);
        }
    }
    // 圆弧
    else
    {
        GeometryMethods::getArcInfo(startPt, endPt, bulge, center,
                                    radius, startAngle, endAngle, normal);
        // 无线宽
        if (startWeight == 0.0 && endWeight == 0.0)
        {
            DmArc* arc = new DmArc(nullptr,
                                   ArcData(center, normal, radius,
                                           startAngle, endAngle));
            arc->setPen(DmPen(DM::FlagInvalid));
            arc->setLayer(nullptr);
            ents.emplace_back(arc);
        }
        // 带线宽
        else
        {
            // 由一段段solid拼凑
            // 部分参考DmArc::setDrawData()
            double start = startAngle;
            double end = endAngle;
            if (start > end)
            {
                start -= 2 * DM_PI;
            }

            double weightDelta = endWeight - startWeight;
            float delta = std::abs(end - start);
            float angle = 0.0f;

            DmVector lastPt1(true);
            DmVector lastPt2(true);
            DmVector curPt1(true);
            DmVector curPt2(true);
            double curWeight = 0.0;

            bool first = true;
            for (int i = 0; i <= DM_CURVE_VERTEXS; i++)
            {
                if (normal.z > 0)
                {
                    angle = start
                            + (((float)i) / DM_CURVE_VERTEXS) * (delta);
                }
                else
                {
                    angle = -start + M_PI
                            - (((float)i) / DM_CURVE_VERTEXS) * (delta);
                }

                curWeight = startWeight
                            + (((float)i) / DM_CURVE_VERTEXS)
                                  * (weightDelta);
                curPt1.x = center.x
                           + (radius - curWeight / 2.0) * cos(angle);
                curPt1.y = center.y
                           + (radius - curWeight / 2.0) * sin(angle);
                curPt2.x = center.x
                           + (radius + curWeight / 2.0) * cos(angle);
                curPt2.y = center.y
                           + (radius + curWeight / 2.0) * sin(angle);

                if (first)
                {
                    lastPt1 = curPt1;
                    lastPt2 = curPt2;
                    first = false;
                }
                else
                {
                    std::vector<DmVector> vertexs {
                        lastPt1, lastPt2, curPt2, curPt1
                    };
                    SolidData solidData = SolidData(vertexs);
                    DmSolid* s = new DmSolid(nullptr, solidData);
                    s->setPen(DmPen(DM::FlagInvalid));
                    s->setLayer(nullptr);
                    ents.emplace_back(s);
                    lastPt1 = curPt1;
                    lastPt2 = curPt2;
                }
            }
        }
    }
}

void DmPolyline::getSegmentInfoAt(int i, double& bulge, DmVector& pt1,
                                  DmVector& pt2, double* startWeight,
                                  double* endWeight) const
{
    assert(i < data.getBulgesCount());
    bulge = data.getBulgeAt(i);

    if (data.getIsClosed())
    {
        if (i == data.getBulgesCount() - 1)
        {
            pt1 = data.getVertexAt(i);
            pt2 = data.getVertexAt(0);
            return;
        }
    }

    pt1 = data.getVertexAt(i);
    pt2 = data.getVertexAt(i + 1);

    if (startWeight || endWeight)
    {
        double sWeight = 0.0, eWeight = 0.0;
        data.getLineWeightsAt(i, sWeight, eWeight);
        if (startWeight)
        {
            *startWeight = sWeight;
        }

        if (endWeight)
        {
            *endWeight = eWeight;
        }
    }
}

void DmPolyline::saveStream(OutputStream& wrt) const
{
    DmEntity::saveStream(wrt);

    bool isClosedFlag = data.getIsClosed();
    std::vector<DmVector> vecVertexs = data.getVertexs();
    std::vector<double> vecLineWeights = data.getLineWeights();
    std::vector<double> vecBulges = data.getBulges();

    wrt << (bool)isClosedFlag << (uint32_t)vecVertexs.size();

    for (auto& pt : vecVertexs)
    {
        wrt << (double)pt.x << (double)pt.y;
    }

    wrt << (uint32_t)vecLineWeights.size();
    for (auto& vecLineWeight : vecLineWeights)
    {
        wrt << (double)vecLineWeight;
    }

    wrt << (uint32_t)vecBulges.size();
    for (auto& vecBulge : vecBulges)
    {
        wrt << (double)vecBulge;
    }
}

void DmPolyline::restoreStream(InputStream& reader,
                               const std::vector<PAIR>& revs)
{
    int fileRev = getRevisionId("DmPolyline", revs);
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

void DmPolyline::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (rev == 0)
    {
    }
    else // big change, e.g. change supper class of DmPolyline
    {
        // step1.
        // read all legacy data one by one
    }
}

void DmPolyline::restoreStream(InputStream& reader)
{
    DmEntity::restoreStream(reader);

    bool isClosedFlag = false;
    auto vecVertexs = std::vector<DmVector>();
    auto vecLineWeights = std::vector<double>();
    auto vecBulges = std::vector<double>();
    reader >> (bool&)isClosedFlag;

    uint32_t numVectex = 0;
    reader >> (uint32_t&)numVectex;
    for (uint32_t i = 0; i < numVectex; i++)
    {
        DmVector pt(true);
        reader >> (double&)pt.x >> (double&)pt.y;
        vecVertexs.emplace_back(std::move(pt));
    }

    uint32_t numLineWeight = 0;
    reader >> (uint32_t&)numLineWeight;
    for (uint32_t i = 0; i < numLineWeight; i++)
    {
        double lineWeight = 0.0;
        reader >> (double&)lineWeight;
        vecLineWeights.emplace_back(std::move(lineWeight));
    }

    uint32_t numBulges = 0;
    reader >> (uint32_t&)numBulges;
    for (uint32_t i = 0; i < numBulges; i++)
    {
        double bulge = 0.0;
        reader >> (double&)bulge;
        vecBulges.emplace_back(std::move(bulge));
    }

    data.setVertexs(vecVertexs);
    data.setBulges(vecBulges);
    data.setLineWeights(vecLineWeights);
    data.setIsClosed(isClosedFlag);
    update();
}
