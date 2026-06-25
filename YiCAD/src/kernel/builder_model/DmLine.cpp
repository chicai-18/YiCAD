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


/// @file DmLine.cpp
/// @brief 线段实体实现，支持两点定义、变换、求交和顶点渲染

#include "DmLine.h"

#include "Debug.h"
#include "GuiDocumentView.h"
#include "DmDocument.h"
#include "Information.h"
#include "Quadratic.h"
#include "DmCircle.h"
#include "DmRect.h"

#include "Writer.h"
#include "Reader.h"
#include "Stream.h"

TYPESYSTEM_SOURCE(DmLine, DmAtomicEntity, 0)

DmLine::DmLine(DmEntity* parent, const LineData& d)
    : DmAtomicEntity(parent)
    , data(d)
    , isModify(true)
{
    calculateBorders();
}

DmLine::DmLine(DmEntity* parent, const DmVector& pStart,
    const DmVector& pEnd)
    : DmAtomicEntity(parent)
    , isModify(true)
{
    data = LineData(pStart, pEnd);
    calculateBorders();
}

DmLine::DmLine(const DmVector& pStart, const DmVector& pEnd)
    : DmAtomicEntity(nullptr)
    , isModify(true)
{
    data = LineData(pStart, pEnd);
    calculateBorders();
}

DmEntity* DmLine::clone() const
{
    DmLine* l = new DmLine(*this);
    l->m_ulID = DmId();
    l->setSelected(false);
    l->setHighlighted(false);
    return l;
}

DM::EntityType DmLine::getEntityType() const
{
    return DM::EntityLine;
}

void DmLine::calculateBorders()
{
    minV = DmVector::minimum(data.getStartPoint(), data.getEndPoint());
    maxV = DmVector::maximum(data.getStartPoint(), data.getEndPoint());
}

const std::vector<float>& DmLine::getVerticesRef(
    int& float_count_per_vertex)
{
    updateVertices();
    float_count_per_vertex = 5;
    return data.getVerticesRef();
}

void DmLine::updateVertices()
{
    if (isModify)
    {
        std::vector<float> vertexes;
        vertexes.reserve(10);
        float x0 = data.getStartPoint().x;
        float y0 = data.getStartPoint().y;
        float x1 = data.getEndPoint().x;
        float y1 = data.getEndPoint().y;
        float dist = getLength();

        vertexes.emplace_back(x0);
        vertexes.emplace_back(y0);
        vertexes.emplace_back(0.0f);
        vertexes.emplace_back(0.0f);
        vertexes.emplace_back(dist);

        vertexes.emplace_back(x1);
        vertexes.emplace_back(y1);
        vertexes.emplace_back(0.0f);
        vertexes.emplace_back(dist);
        vertexes.emplace_back(dist);

        data.setVertices(vertexes);
        isModify = false;
    }
}

LineData DmLine::getData() const
{
    return data;
}

void DmLine::setData(const LineData& d)
{
    data = d;
}

DmVectorSolutions DmLine::getRefPoints() const
{
    return DmVectorSolutions({data.getStartPoint(), data.getEndPoint()});
}

DmVector DmLine::getStartpoint() const
{
    return data.getStartPoint();
}

DmVector DmLine::getEndpoint() const
{
    return data.getEndPoint();
}

void DmLine::setStartpoint(DmVector s)
{
    data.setStartPoint(s);
    isModify = true;
    calculateBorders();
}

void DmLine::setEndpoint(DmVector e)
{
    data.setEndPoint(e);
    isModify = true;
    calculateBorders();
}

double DmLine::getDirection1() const
{
    return getStartAngle();
}

DmVector DmLine::getNearestEndpoint(const DmVector& coord,
    double* dist) const
{
    double dist1((data.getStartPoint() - coord).squared());
    double dist2((data.getEndPoint() - coord).squared());

    if (dist2 < dist1)
    {
        if (dist)
        {
            *dist = sqrt(dist2);
        }
        return data.getEndPoint();
    }
    else
    {
        if (dist)
        {
            *dist = sqrt(dist1);
        }
        return data.getStartPoint();
    }
}

DmVector DmLine::getNearestPointOnEntity(const DmVector& coord,
    bool onEntity, double* dist, DmEntity** entity) const
{
    if (entity)
    {
        *entity = const_cast<DmLine*>(this);
    }
    DmVector direction(data.getEndPoint() - data.getStartPoint());
    DmVector vpc(coord - data.getStartPoint());
    double a = direction.squared();

    if (a < DM_TOLERANCE2)
    {
        // 线太短，取中点
        vpc = getMiddlePoint();
    }
    else
    {
        // 获得点到直线的投影随影的参数t（0=<t<=1表示范围内），
        // t=cos*c/d，其中cos为两向量夹角，c,d为两向量长度
        const double t = DmVector::dotP(vpc, direction) / a;
        if (onEntity && (t <= -DM_TOLERANCE || t >= 1. + DM_TOLERANCE))
        {
            // 投影在不在直线范围内，获得最近的端点
            return getNearestEndpoint(coord, dist);
        }

        vpc = data.getStartPoint() + direction * t;
    }

    if (dist)
    {
        *dist = vpc.distanceTo(coord);
    }

    return vpc;
}

DmVector DmLine::getMiddlePoint() const
{
    return (getStartpoint() + getEndpoint()) * 0.5;
}

DmVector DmLine::getNearestMiddle(const DmVector& coord,
    double* dist, int middlePoints) const
{
    DmVector dvp(getEndpoint() - getStartpoint());
    double l = dvp.magnitude();
    if (l <= DM_TOLERANCE)
    {
        // line too short
        return const_cast<DmLine*>(this)->getNearestCenter(coord, dist);
    }
    DmVector vp0(getNearestPointOnEntity(coord, true, dist));
    int counts = middlePoints + 1;
    int i(static_cast<int>(
        vp0.distanceTo(getStartpoint()) / l * counts + 0.5));
    if (!i)
    {
        i++; // remove end points
    }
    if (i == counts)
    {
        i--;
    }
    vp0 = getStartpoint() + dvp * (double(i) / double(counts));

    if (dist)
    {
        *dist = vp0.distanceTo(coord);
    }
    return vp0;
}

Quadratic DmLine::getQuadratic() const
{
    // Ax+By+C=0，ce由{A,B,C}构成
    std::vector<double> ce(3, 0.);
    auto dvp = data.getEndPoint() - data.getStartPoint();
    DmVector normal(-dvp.y, dvp.x);
    ce[0] = normal.x;
    ce[1] = normal.y;
    ce[2] = -normal.dotP(data.getEndPoint());
    return Quadratic(ce);
}

std::list<DmEntity*> DmLine::getSubEntities() const
{
    return std::list<DmEntity*>();
}

double DmLine::getDirection2() const
{
    return getEndAngle();
}

DmVector DmLine::getTangentDirection(const DmVector& /*point*/) const
{
    return getEndpoint() - getStartpoint();
}

void DmLine::moveStartpoint(const DmVector& pos)
{
    data.setStartPoint(pos);
    isModify = true;
    calculateBorders();
}

void DmLine::moveEndpoint(const DmVector& pos)
{
    data.setEndPoint(pos);
    isModify = true;
    calculateBorders();
}

DM::Ending DmLine::getTrimPoint(const DmVector& trimCoord,
    const DmVector& trimPoint)
{
    DmVector vp1 = getStartpoint() - trimCoord;
    DmVector vp2 = trimPoint - trimCoord;
    if (DmVector::dotP(vp1, vp2) < 0)
    {
        return DM::EndingEnd;
    }
    else
    {
        return DM::EndingStart;
    }
}

bool DmLine::hasEndpointsWithinWindow(const DmVector& firstCorner,
    const DmVector& secondCorner)
{
    DmVector vLow(std::min(firstCorner.x, secondCorner.x),
        std::min(firstCorner.y, secondCorner.y));
    DmVector vHigh(std::max(firstCorner.x, secondCorner.x),
        std::max(firstCorner.y, secondCorner.y));

    return data.getStartPoint().isInWindowOrdered(vLow, vHigh)
        || data.getEndPoint().isInWindowOrdered(vLow, vHigh);
}

double DmLine::getLength() const
{
    return data.getStartPoint().distanceTo(data.getEndPoint());
}

double DmLine::getStartAngle() const
{
    return data.getStartPoint().angleTo(data.getEndPoint());
}

bool DmLine::offset(const DmVector& coord, const double& distance)
{
    DmVector direction{getEndpoint() - getStartpoint()};
    double ds(direction.magnitude());
    if (ds < DM_TOLERANCE)
    {
        return false;
    }

    direction /= ds;
    DmVector vp(coord - getStartpoint());
    direction.set(-direction.y, direction.x); // rotate pi/2
    if (DmVector::dotP(direction, vp) < 0.)
    {
        direction *= -1.;
    }
    direction *= distance;
    move(direction);
    return true;
}

double DmLine::getEndAngle() const
{
    return data.getEndPoint().angleTo(data.getStartPoint());
}

bool DmLine::isTangent(const CircleData& circleData) const
{
    double d = 0.0;
    getNearestPointOnEntity(circleData.getCenter(), false, &d);
    if (fabs(d - circleData.getRadius()) < 20. * DM_TOLERANCE)
    {
        return true;
    }
    return false;
}

DmVector DmLine::getNormalVector() const
{
    // direction vector
    DmVector vp = data.getEndPoint() - data.getStartPoint();
    double r = vp.magnitude();
    if (r < DM_TOLERANCE)
    {
        return DmVector{false};
    }
    return DmVector(-vp.y, vp.x) / r;
}

std::vector<DmEntity*> DmLine::offsetTwoSides(
    const double& distance) const
{
    std::vector<DmEntity*> ret(0);
    DmVector const& vp = getNormalVector() * distance;
    ret.push_back(
        new DmLine(data.getStartPoint() + vp, data.getEndPoint() + vp));
    ret.push_back(
        new DmLine(data.getStartPoint() - vp, data.getEndPoint() - vp));
    return ret;
}

void DmLine::move(const DmVector& offset)
{
    data.setStartPoint(data.getStartPoint().move(offset));
    data.setEndPoint(data.getEndPoint().move(offset));
    isModify = true;
    moveBorders(offset);
}

void DmLine::rotate(const double& angle)
{
    DmVector rvp(angle);
    data.setStartPoint(data.getStartPoint().rotate(rvp));
    data.setEndPoint(data.getEndPoint().rotate(rvp));
    isModify = true;
    calculateBorders();
}

void DmLine::rotate(const DmVector& center, const DmVector& angleVector)
{
    data.setStartPoint(data.getStartPoint().rotate(center, angleVector));
    data.setEndPoint(data.getEndPoint().rotate(center, angleVector));
    isModify = true;
    calculateBorders();
}

// scale the line around origin (0,0)
void DmLine::scale(const DmVector& factor)
{
    data.setStartPoint(data.getStartPoint().scale(factor));
    data.setEndPoint(data.getEndPoint().scale(factor));
    isModify = true;
    calculateBorders();
}

void DmLine::scale(const DmVector& center, const DmVector& factor)
{
    data.setStartPoint(data.getStartPoint().scale(center, factor));
    data.setEndPoint(data.getEndPoint().scale(center, factor));
    isModify = true;
    calculateBorders();
}

void DmLine::mirror(const DmVector& axisPoint1,
    const DmVector& axisPoint2)
{
    data.setStartPoint(
        data.getStartPoint().mirror(axisPoint1, axisPoint2));
    data.setEndPoint(
        data.getEndPoint().mirror(axisPoint1, axisPoint2));
    isModify = true;
    calculateBorders();
}

void DmLine::moveRef(const DmVector& ref, const DmVector& offset)
{
    constexpr double TOL = 1.0e-4;

    if (fabs(data.getStartPoint().x - ref.x) < TOL
        && fabs(data.getStartPoint().y - ref.y) < TOL)
    {
        moveStartpoint(data.getStartPoint() + offset);
    }
    if (fabs(data.getEndPoint().x - ref.x) < TOL
        && fabs(data.getEndPoint().y - ref.y) < TOL)
    {
        moveEndpoint(data.getEndPoint() + offset);
    }
}

// persistent helper
void DmLine::saveStream(OutputStream& str) const
{
    DmAtomicEntity::saveStream(str);

    auto st = getStartpoint();
    auto ed = getEndpoint();

    str << (double)st.x << (double)st.y
        << (double)ed.x << (double)ed.y;
}

void DmLine::restoreStream(InputStream& str,
    const std::vector<PAIR>& revs)
{
    int fileRev = getRevisionId("DmLine", revs);
    if (revId > fileRev)
    {
        DmAtomicEntity::restoreStream(str, revs);
        // 老文件格式
        restoreStreamWithRev(str, fileRev);
    }
    else
    {
        restoreStream(str);
    }
}

void DmLine::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (0 == rev)
    {
        // 基本版本，无需额外处理
    }
    else // big change, e.g. change super class of DmLine
    {
        // step1.
        // read all legacy data one by one
    }
}

void DmLine::restoreStream(InputStream& rdr)
{
    DmAtomicEntity::restoreStream(rdr);
    DmVector st(true);
    DmVector ed(true);
    rdr >> (double&)st.x >> (double&)st.y
        >> (double&)ed.x >> (double&)ed.y;

    setStartpoint(st);
    setEndpoint(ed);
    calculateBorders();
}

void DmLine::update()
{
    isModify = true;
    calculateBorders();
}
