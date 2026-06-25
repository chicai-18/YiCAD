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


/// @file DmRegion.cpp
/// @brief DmRegion 区域实体类的实现，包括点判断、三角化、序列化等

#include "DmRegion.h"
#include "DmEntityHelper.h"
#include "DmLine.h"
#include "Information.h"
#include "DmLine.h"
#include "DmArc.h"
#include "DmEllipse.h"
#include "DmSpline.h"
#include "Math2d.h"
#include "FindClosedRegion.h"

using namespace FindClosedRegion;

TYPESYSTEM_SOURCE(DmRegion, DmEntity, 0)

DmRegion::DmRegion(DmEntity* parent, const RegionData& d)
: DmEntity(parent)
, data(d)
{
    calculateBorders();
}

DmEntity* DmRegion::clone() const
{
    DmRegion* r = new DmRegion(*this);
    RegionData d = getCloneData();
    r->setData(d);
    r->m_ulID = DmId();
    r->setSelected(false);
    r->setHighlighted(false);
    return r;
}

DM::EntityType DmRegion::getEntityType() const
{
    return DM::EntityRegion;
}

RegionData DmRegion::getData() const
{
    return data;
}

void DmRegion::setData(const RegionData& d)
{
    data = d;
}

RegionData DmRegion::getCloneData() const
{
    RegionData d;
    auto oldBoundary = data.getBoundary();
    if(oldBoundary)
    {
        DmEntityContainer* cloneBoundary = static_cast<DmEntityContainer*>(oldBoundary->clone());
        DmEntityContainerPtr boundary(cloneBoundary);
        d.setBoundary(boundary);
    }

    std::vector<DmEntityContainerPtr> holes = data.getHoles();
    std::vector<DmEntityContainerPtr> newHoles;
    newHoles.reserve(holes.size());
    for(auto hole:holes)
    {
        DmEntityContainer* cloneHole = static_cast<DmEntityContainer*>(hole->clone());
        DmEntityContainerPtr newHole(cloneHole);
        newHoles.emplace_back(newHole);
    }
    d.setHoles(newHoles);
    return d;
}

bool DmRegion::isContainer() const
{
    return false;
}

void DmRegion::calculateBorders()
{
    resetBorders();
    auto boundary=data.getBoundary();
    for (auto c : *boundary)
    {
        c->calculateBorders();
        minV = DmVector::minimum(c->getMin(), minV);
        maxV = DmVector::maximum(c->getMax(), maxV);
    }
}

std::list<DmEntity*> DmRegion::getSubEntities() const
{
    return std::list<DmEntity*>();
}

int DmRegion::size() const
{
    int hasBoundary = (data.getBoundary() && !data.getBoundary()->isEmpty()) ? 1 : 0;
    return (int)data.getHoles().size() + hasBoundary;
}

void DmRegion::update()
{
    calculateBorders();
}

bool DmRegion::isPointInside(const DmVector& point, bool* onBoundary) const
{
    if (!data.getBoundary())
    {
        return false;
    }

    if (point.x < minV.x || point.x > maxV.x ||
        point.y < minV.y || point.y > maxV.y)
    {
        return false;
    }

    double width = getWidthHeight().x + 1.0;
    auto allEdges = getCombineBoundary();

    // 从point发出射线，求该射线与轮廓的交点。
    // 如果不能确定（如与端点重合的情况），旋转射线一定角度，继续求交点。迭代之多6次
    bool sure;
    int counter;
    int tries = 0;
    double rayAngle = 0.0;
    do {
        sure = true;

        // 创建射线
        DmVector v = DmVector::polar(width * 10.0, rayAngle);
        DmLine ray{ point, point + v };
        counter = 0;
        DmVectorSolutions sol;

        if (onBoundary)
        {
            *onBoundary = false;
        }

        for (DmEntity* e = allEdges->firstEntity(DM::ResolveAll); e; e = allEdges->nextEntity(DM::ResolveAll))
        {
            isPointInside_subroutineForSegment(point, &ray, e, onBoundary, counter, sure);
        }

        rayAngle += 0.02;
        tries++;
    } while (!sure && rayAngle < 2 * M_PI && tries < 6);
    return ((counter % 2) == 1);
}

void DmRegion::isPointInside_subroutineForSegment(const DmVector& point, const DmLine* ray, DmEntity* e, bool* onContour, int& counter, bool& sure) const
{
    // 仅考虑一次二次曲线的交点，所以交点至多2个
    DmVectorSolutions sol = Information::getIntersection(ray, e, true);

    for (int i = 0; i < sol.size(); ++i)
    {
        DmVector p = sol.get(i);

        if (p.valid)
        {
            if (p.distanceTo(point) < 1.0e-5)
            {
                if (onContour)
                {
                    *onContour = true;
                }
            }
            else
            {
                if (e->getEntityType() == DM::EntityLine)
                {
                    DmLine* line = (DmLine*)e;

                    if (p.distanceTo(line->getStartpoint()) < 1.0e-4)
                    {
                        if (Math2d::correctAngle(line->getStartAngle()) < M_PI)
                        {
                            sure = false;
                        }
                    }

                    else if (p.distanceTo(line->getEndpoint()) < 1.0e-4)
                    {
                        if (Math2d::correctAngle(line->getEndAngle()) < M_PI)
                        {
                            sure = false;
                        }
                    }
                    counter++;
                }
                else if (e->getEntityType() == DM::EntityArc)
                {
                    DmArc* arc = (DmArc*)e;

                    if (p.distanceTo(arc->getStartpoint()) < 1.0e-4)
                    {
                        double dir = arc->getDirection1();
                        if ((dir < M_PI && dir >= 1.0e-5) || ((dir > 2 * M_PI - 1.0e-5 || dir < 1.0e-5) && arc->getCenter().y > p.y))
                        {
                            counter++;
                            sure = false;
                        }
                    }
                    else if (p.distanceTo(arc->getEndpoint()) < 1.0e-4)
                    {
                        double dir = arc->getDirection2();
                        if ((dir < M_PI && dir >= 1.0e-5) || ((dir > 2 * M_PI - 1.0e-5 || dir < 1.0e-5) && arc->getCenter().y > p.y))
                        {
                            counter++;
                            sure = false;
                        }
                    }
                    else
                    {
                        counter++;
                    }
                }
                else if (e->getEntityType() == DM::EntityEllipse)
                {
                    DmEllipse* ellipse = static_cast<DmEllipse*>(e);
                    if (p.distanceTo(ellipse->getStartpoint()) < 1.0e-4)
                    {
                        double dir = ellipse->getDirection1();
                        if ((dir < M_PI && dir >= 1.0e-5) || ((dir > 2 * M_PI - 1.0e-5 || dir < 1.0e-5) && ellipse->getCenter().y > p.y))
                        {
                            counter++;
                            sure = false;
                        }
                    }
                    else if (p.distanceTo(ellipse->getEndpoint()) < 1.0e-4)
                    {
                        double dir = ellipse->getDirection2();
                        if ((dir < M_PI && dir >= 1.0e-5) || ((dir > 2 * M_PI - 1.0e-5 || dir < 1.0e-5) && ellipse->getCenter().y > p.y))
                        {
                            counter++;
                            sure = false;
                        }
                    }
                    else
                    {
                        counter++;
                    }
                }
                    // 样条线仅供参考，未验证
                else if(e->getEntityType() == DM::EntitySpline)
                {
                    DmSpline* spline = static_cast<DmSpline*>(e);
                    auto closePtItem = spline->getClosetPt(point);
                    DmVector closetPt = std::get<0>(closePtItem);
                    double closetT = std::get<1>(closePtItem);
                    if(closetPt.valid)
                    {
                        // 相切的情况
                        DmVector deriv = spline->derivative(closetT);
                        double ang = deriv.angle();
                        double angleGap = Math2d::correctAngle(ang - ray->getDirection1());
                        if(angleGap < 1.0e-5 || Math2d::correctAngle(angleGap - M_PI) < 1.0e-5)
                        {
                            counter++;
                            sure = false;
                        }
                        else
                        {
                            counter++;
                        }
                    }
                    else
                    {
                        counter++;
                    }
                }
            }
        }
    }
}

DmEntityContainerPtr DmRegion::getCombineBoundary() const
{
    DmEntityContainerPtr ec(new DmEntityContainer(nullptr, false));
    // 边界
    auto boundary = data.getBoundary();
    if(boundary)
    {
        for(auto e:*boundary)
        {
            ec->addEntity(e);
        }
    }
    // 孔洞
    auto holes = data.getHoles();
    for(auto hole:holes)
    {
        for(auto e:*hole)
        {
            ec->addEntity(e);
        }
    }
    return ec;
}


template<class T>
void DmRegion::getTriangles(std::vector<T>& triangles, bool considerHoles)
{
    //仅支持2种类型实例化
    static_assert(std::is_same_v<T, DmTriangle*> || std::is_same_v<T, DmTrianglePtr>,
            "DmRegion::getTriangles only support DmTriangle* or DmTrianglePtr");

    // 计算边界点
    std::vector<DmVector> bPoints;
    getPointsOfOneBoundary(data.getBoundary(), bPoints);

    std::vector<std::vector<DmVector>> allHolePoints;
    if(considerHoles)
    {
        auto holes = data.getHoles();
        for(auto hole:holes)
        {
            std::vector<DmVector> hPoints;
            getPointsOfOneBoundary(hole, hPoints);
            allHolePoints.emplace_back(hPoints);
        }
    }

    ConstrainedDelaunayTriangulation::trianglulate<T>(bPoints, allHolePoints, triangles);
}
// 显式实例化2种类型
template void DmRegion::getTriangles<DmTriangle*>(std::vector<DmTriangle*>&, bool );
template void DmRegion::getTriangles<DmTrianglePtr>(std::vector<DmTrianglePtr>&, bool );

void DmRegion::getPointsOfOneBoundary(DmEntityContainerPtr boundary, std::vector<DmVector>& points)
{
    std::vector<DmVector> pts;
    for(auto const& edge: *boundary)
    {
        Edge::getPoints(edge,false,pts);
    }
    // 去重，避免三角化内外三角形判断出问题
    DmVectorSolutions sol(pts);
    sol.distinct(DM_TOLERANCE);
    points = sol.getVector();
}

void DmRegion::move(const DmVector& offset)
{
    data.getBoundary()->move(offset);
    auto holes = data.getHoles();
    for(auto h:holes)
    {
        h->move(offset);
    }
}

void DmRegion::rotate(const DmVector& center, const DmVector& angleVector)
{
    data.getBoundary()->rotate(center, angleVector);
    auto holes = data.getHoles();
    for(auto h:holes)
    {
        h->rotate(center, angleVector);
    }
}

void DmRegion::scale(const DmVector& center, const DmVector& factor)
{
    data.getBoundary()->scale(center, factor);
    auto holes = data.getHoles();
    for(auto h:holes)
    {
        h->scale(center, factor);
    }
}

void DmRegion::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    data.getBoundary()->mirror(axisPoint1, axisPoint2);
    auto holes = data.getHoles();
    for(auto h:holes)
    {
        h->mirror(axisPoint1, axisPoint2);
    }
}

DmVector DmRegion::getNearestEndpoint(const DmVector & coord, double* dist /*= nullptr*/) const
{
    return DmVector(false);
}

DmVector DmRegion::getNearestPointOnEntity(const DmVector& coord, bool onEntity /*= true*/, double* dist /*= nullptr*/, DmEntity **entity /*= nullptr*/) const
{
    double minDist = DM_MAXDOUBLE, d= DM_MAXDOUBLE;
    DmVector res(false), temp(false);
    DmVector center(false), normal(false);
    double r = 0.0, startAng = 0.0, endAng = 0.0;

    // 边界取最近点
    auto boundary = data.getBoundary();
    temp = boundary->getNearestPointOnEntity(coord, true, &d);
    if (temp.valid && d < minDist)
    {
        minDist = d;
        res = temp;
    }

    // 孔洞取最近点
    auto holes = data.getHoles();
    for (auto h:holes)
    {
        temp = h->getNearestPointOnEntity(coord, true, &d);
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

DmVector DmRegion::getNearestCenter(const DmVector & coord, double* dist /*= nullptr*/) const
{
    return DmVector(false);
}

DmVector DmRegion::getNearestMiddle(const DmVector & coord, double* dist /*= nullptr*/, int middlePoints /*= 1*/) const
{
    return DmVector(false);
}

void DmRegion::saveStream(OutputStream& wrt) const
{
    DmEntity::saveStream(wrt);

    auto boundary = data.getBoundary();
    wrt << (uint32_t)boundary->size();
    for(auto e:*boundary)
    {
        wrt << (std::string)DmEntityHelper::getEntityNameByType(e->getEntityType());
        e->saveStream(wrt);
    }

    auto holes = data.getHoles();
    wrt << (uint32_t)holes.size();
    for(auto h:holes)
    {
        wrt << (uint32_t)h->size();
        for(auto e: *h)
        {
            wrt << (std::string)DmEntityHelper::getEntityNameByType(e->getEntityType());
            e->saveStream(wrt);
        }
    }
}

void DmRegion::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
    int fileRev = getRevisionId("DmRegion", revs);
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

void DmRegion::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (rev == 0)
    {
    }
    else //big change, e.g. change supper class of DmPolyline
    {
        //step1.
        // read all legacy data one by one
    }
}

void DmRegion::restoreStream(InputStream& rdr)
{
    DmEntity::restoreStream(rdr);

    uint32_t bSize;
    rdr>>(uint32_t&)bSize;
    DmEntityContainerPtr boundary(new DmEntityContainer());
    for(uint32_t i=0;i<bSize;i++)
    {
        std::string type;
        rdr >>(std::string&)type;
        DmEntity* e = DmEntityHelper::createEntityByName(type);
        e->restoreStream(rdr);
        boundary->addEntity(e);
    }

    uint32_t holeSize;
    std::vector<DmEntityContainerPtr> holes;
    rdr>>(uint32_t&)holeSize;
    for(uint32_t i=0;i<holeSize;i++)
    {
        DmEntityContainerPtr hole(new DmEntityContainer());
        uint32_t eSize;
        rdr>>(uint32_t&)eSize;
        for(int j=0;j<eSize;j++)
        {
            std::string type;
            rdr >>(std::string&)type;
            DmEntity* e = DmEntityHelper::createEntityByName(type);
            e->restoreStream(rdr);
            hole->addEntity(e);
        }
        holes.emplace_back(hole);
    }

    RegionData d(boundary, holes);
    data = d;
    update();
}



