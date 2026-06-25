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


/// @file DmTriangle.cpp
/// @brief DmTriangle 三角形实体类的实现

#include "DmTriangle.h"
#include "DmLine.h"
#include "Information.h"

TYPESYSTEM_SOURCE(DmTriangle, DmEntity, 0)

DmTriangle::DmTriangle(DmEntity* parent, const TriangleData& d)
: DmEntity(parent)
, data(d)
, isModify(true)
{
    calculateBorders();
}

DmEntity* DmTriangle::clone() const
{
    DmTriangle* s = new DmTriangle(*this);
    s->m_ulID = DmId();
    s->setSelected(false);
    s->setHighlighted(false);
    return s;
}

DM::EntityType DmTriangle::getEntityType() const
{
    return DM::EntityTriangle;
}

TriangleData const& DmTriangle::getData() const
{
    return data;
}

void DmTriangle::setData(const TriangleData& d)
{
    data = d;
}

DmVector DmTriangle::getPointAt(int i) const
{
    return data.getPointAt(i);
}

DmVector DmTriangle::getNearestEndpoint(const DmVector& coord, double* dist /*= nullptr*/) const
{
    double minDist = DM_MAXDOUBLE;
    double curDist = 0.0;
    DmVector ret;
    for (int i = 0; i < POINT_SIZE; i++)
    {
        if (data.getPointAt(i).valid)
        {
            curDist = data.getPointAt(i).distanceTo(coord);
            if (curDist < minDist)
            {
                ret = data.getPointAt(i);
                minDist = curDist;
            }
        }
    }

    if(dist)
    {
        *dist = minDist;
    }
    return ret;
}

DmVector DmTriangle::getNearestPointOnEntity(const DmVector& coord, bool onEntity /*= true*/, double* dist /*= nullptr*/, DmEntity** entity /*= nullptr*/) const
{
    // 抄自DmSolid::getNearestPointOnEntity
    // 点在三角形内
    if(isPointInside(coord))
    {
        if(dist)
        {
            *dist = 0.0;
        }
        return coord;
    }

    // 三角形外，从三条边查找最近距离
    if (nullptr != entity)
    {
        *entity = const_cast<DmTriangle*>(this);
    }
    DmVector ret(false);
    double curDist = DM_MAXDOUBLE;
    double tmpDist = 0.0;
    for (int i = 0, next = i + 1; i < POINT_SIZE; ++i, ++next)
    {
        // 最后闭合那条边
        if (next == POINT_SIZE)
        {
            next = 0;
        }
        DmVector direction = data.getPointAt(next) - data.getPointAt(i);
        DmVector vpc = coord - data.getPointAt(i);
        double a = direction.squared();
        if (a < DM_TOLERANCE2)
        {
            // 线太短
            vpc = data.getPointAt(i);
        }
        else
        {
            // 计算投影
            vpc = data.getPointAt(i) + direction * DmVector::dotP(vpc, direction) / a;
        }
        tmpDist = vpc.distanceTo(coord);
        if (tmpDist < curDist)
        {
            curDist = tmpDist;
            ret = vpc;
        }
    }

    // 确保结果在有效范围
    if (onEntity && !ret.isInWindowOrdered(minV, maxV))
    {
        // 投影点不在有效范围，取有效的端点
        ret = getNearestEndpoint(coord, dist);
        curDist = ret.distanceTo(coord);
    }

    if(dist)
    {
        *dist = curDist;
    }
    return ret;
}

DmVector DmTriangle::getNearestCenter(const DmVector& coord, double* dist /*= nullptr*/) const
{
    if(dist)
    {
        *dist = DM_MAXDOUBLE;
    }
    return DmVector(false);
}

DmVector DmTriangle::getNearestMiddle(const DmVector& coord, double* dist /*= nullptr*/, int middlePoints /*= 1*/) const
{
    if(dist)
    {
        *dist = DM_MAXDOUBLE;
    }
    return DmVector(false);
}

double DmTriangle::getDistanceToPoint(const DmVector& coord, DmEntity** entity /*= nullptr*/, DM::ResolveLevel level /*= DM::ResolveNone*/) const
{
    if (nullptr != entity)
    {
        *entity = const_cast<DmTriangle*>(this);
    }

    double ret = 0.;
    getNearestPointOnEntity(coord, true, &ret, entity);
    return ret;
}

void DmTriangle::move(const DmVector& offset)
{
    for (int i = 0; i < POINT_SIZE; ++i)
    {
        if (data.getPointAt(i).valid)
        {
            data.setPointAt(i, data.getPointAt(i).move(offset));
        }
    }
    moveBorders(offset);
    isModify = true;
}

void DmTriangle::rotate(const DmVector& center, const DmVector& angleVector)
{
    for (int i = 0; i < POINT_SIZE; ++i)
    {
        if (data.getPointAt(i).valid)
        {
            data.setPointAt(i, data.getPointAt(i).rotate(center, angleVector));
        }
    }
    calculateBorders();
    isModify = true;
}

void DmTriangle::scale(const DmVector& center, const DmVector& factor)
{
    for (int i = 0; i < POINT_SIZE; ++i)
    {
        if (data.getPointAt(i).valid)
        {
            data.setPointAt(i, data.getPointAt(i).scale(center, factor));
        }
    }
    calculateBorders();
    isModify = true;
}

void DmTriangle::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    for (int i = 0; i < POINT_SIZE; ++i)
    {
        if (data.getPointAt(i).valid)
        {
            data.setPointAt(i, data.getPointAt(i).mirror(axisPoint1, axisPoint2));
        }
    }
    calculateBorders();
    isModify = true;
}

void DmTriangle::calculateBorders()
{
    resetBorders();
    DmVector p1;
    for (int i = 0; i < 3; i++)
    {
        p1 = data.getPointAt(i);
        minV = DmVector::minimum(minV, p1);
        maxV = DmVector::maximum(maxV, p1);
    }
}

std::list<DmEntity*> DmTriangle::getSubEntities() const
{
    return std::list<DmEntity*>();
}

bool DmTriangle::isInCrossWindow(const DmVector& v1, const DmVector& v2) const
{
//    // 包围框不相交
//    DmBoundingBox box(v1, v2);
//    DmBoundingBox box2(minV, maxV);
//    if(!box.isCross(box2))
//        return false;

    DmVector vBL(true);	//要设置为true
    DmVector vTR(true);
    DmVectorSolutions sol;

    // sort input vectors to BottomLeft & TopRight
    if (v1.x < v2.x)
    {
        vBL.x = v1.x;
        vTR.x = v2.x;
    }
    else
    {
        vBL.x = v2.x;
        vTR.x = v1.x;
    }
    if (v1.y < v2.y)
    {
        vBL.y = v1.y;
        vTR.y = v2.y;
    }
    else
    {
        vBL.y = v2.y;
        vTR.y = v1.y;
    }

    // Check if entity is out of window
    if (getMin().x > vTR.x || getMax().x < vBL.x || getMin().y > vTR.y || getMax().y < vBL.y)
    {
        return false;
    }

    DmVector p1 = data.getPointAt(0);
    DmVector p2 = data.getPointAt(1);
    DmVector p3 = data.getPointAt(2);
    std::vector<DmLine> border{{p1,p2}, {p2, p3}, {p3,p1}};

    // 判断边界是否与窗体相交
    if (getMax().x > vBL.x && getMin().x < vBL.x)
    {    // left
        DmLine edge{ vBL, {vBL.x, vTR.y} };
        for (auto const& line : border)
        {
            sol = Information::getIntersection(&edge, &line, true);
            if (sol.hasValid())
            {
                return true;
            }
        }
    }
    if (getMax().x > vTR.x && getMin().x < vTR.x)
    {    // right
        DmLine edge{ {vTR.x, vBL.y}, vTR };
        for (auto const& line : border)
        {
            sol = Information::getIntersection(&edge, &line, true);
            if (sol.hasValid())
            {
                return true;
            }
        }
    }
    if (getMax().y > vBL.y && getMin().y < vBL.y)
    {    // bottom
        DmLine edge{ vBL, {vTR.x, vBL.y} };
        for (auto const& line : border)
        {
            sol = Information::getIntersection(&edge, &line, true);
            if (sol.hasValid())
            {
                return true;
            }
        }
    }
    if (getMax().y > vTR.y
        && getMin().y < vTR.y)
    { // top
        DmLine edge{ {vBL.x, vTR.y}, vTR };
        for (auto const& line : border)
        {
            sol = Information::getIntersection(&edge, &line, true);
            if (sol.hasValid())
            {
                return true;
            }
        }
    }

    return false;
}

bool DmTriangle::isPointInside(const DmVector& pt) const
{
    // 判断三边叉乘是否同号
    DmVector a1 = data.getPointAt(1) - data.getPointAt(0);
    DmVector a2 = data.getPointAt(2) - data.getPointAt(1);
    DmVector a3 = data.getPointAt(0) - data.getPointAt(2);
    DmVector vp1 = pt - data.getPointAt(0);
    DmVector vp2 = pt - data.getPointAt(1);
    DmVector vp3 = pt - data.getPointAt(2);
    double c1 = DmVector::crossP(a1, vp1).z;
    double c2 = DmVector::crossP(a2, vp2).z;
    double c3 = DmVector::crossP(a3, vp3).z;
    bool has_neg = c1 < -DM_TOLERANCE || c2 < -DM_TOLERANCE || c3 < -DM_TOLERANCE;
    bool has_pos = c1 > DM_TOLERANCE || c2 > DM_TOLERANCE || c3 > DM_TOLERANCE;
    // 如果既有正又有负，说明点不在三角形内
    return !(has_neg && has_pos);
}

bool DmTriangle::isContainer() const
{
    return false;
}

void DmTriangle::saveStream(OutputStream& wrt) const
{
    DmEntity::saveStream(wrt);

    auto corners = data.getPoints();
    for (auto& corner : corners)
    {
        wrt << (double)corner.x << (double)corner.y;
    }
}

void DmTriangle::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
    int fileRev = getRevisionId("DmTriangle", revs);
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

void DmTriangle::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (rev == 0)
    {
    }
    else //big change, e.g. change supper class of DmSolid
    {
        //step1.
        // read all legacy data one by one

    }
}

void DmTriangle::restoreStream(InputStream& reader)
{
    DmEntity::restoreStream(reader);

    auto corners = std::array<DmVector, 3>();
    for (uint32_t i = 0; i < POINT_SIZE; i++)
    {
        double pt_x, pt_y;
        reader >> (double&)pt_x >> (double&)pt_y;
        corners[i] = DmVector(pt_x, pt_y);
    }
    data.setPoints(corners);
    calculateBorders();
    isModify = true;
}