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


/// @file DmRay.cpp
/// @brief 射线实体类实现：几何变换、最近点计算、包围盒、持久化

#include "DmRay.h"
#include "DmRect.h"
#include "GuiDocumentView.h"
#include <cmath>

#include "Writer.h"
#include "Reader.h"
#include "Stream.h"

TYPESYSTEM_SOURCE(DmRay, DmAtomicEntity, 0)

DmRay::DmRay(DmEntity* parent, const RayData& d)
    : DmAtomicEntity(parent)
    , data(d)
    , isModify(true)
{
    calculateBorders();
}

DmRay::~DmRay()
{
}

DmEntity* DmRay::clone() const
{
    DmRay* c = new DmRay(*this);
    c->m_ulID = DmId();
    c->setSelected(false);
    c->setHighlighted(false);
    return c;
}

DM::EntityType DmRay::getEntityType() const
{
    return DM::EntityRay;
}

DmVector DmRay::getBasePoint()
{
    return data.getBasePoint();
}

void DmRay::setBasePoint(const DmVector& pt)
{
    data.setBasePoint(pt);
}

DmVector DmRay::getDirecion()
{
    return data.getDirection();
}

void DmRay::setDirection(const DmVector& vec)
{
    data.setDirection(vec);
}

RayData DmRay::getData() const
{
    return data;
}

double DmRay::getAngle() const
{
    double k = data.getDirection().y / data.getDirection().x;
    return atan(k);
}

DmVectorSolutions DmRay::getRefPoints() const
{
    return DmVectorSolutions({ data.getBasePoint(),
                               data.getBasePoint()
                               + data.getDirection() });
}

DmVector DmRay::getMiddlePoint() const
{
    return DmVector(false);
}

DmVector DmRay::getNearestEndpoint(const DmVector& coord,
                                   double* dist) const
{
    return data.getBasePoint();
}

DmVector DmRay::getNearestPointOnEntity(const DmVector& coord,
                                        bool onEntity, double* dist,
                                        DmEntity** entity) const
{
    if (entity)
    {
        *entity = const_cast<DmRay*>(this);
    }

    DmVector vec(coord - data.getBasePoint());
    double distance = data.getDirection().squared();

    const double t{ DmVector::dotP(vec, data.getDirection()) / distance };
    if (t < 0)
    {
        vec = data.getBasePoint();
    }
    else
    {
        vec = data.getBasePoint() + data.getDirection() * t;
    }

    if (dist)
    {
        *dist = vec.distanceTo(coord);
    }

    return vec;
}

DmVector DmRay::getNearestMiddle(const DmVector& coord, double* dist,
                                 int middlePoints) const
{
    return {};
}

bool DmRay::offset(const DmVector& coord, const double& distance)
{
    DmVector direction = data.getDirection();
    double ds = direction.magnitude();
    if (ds < DM_TOLERANCE)
    {
        return false;
    }

    direction /= ds;
    DmVector vp(coord - getBasePoint());
    direction.set(-direction.y, direction.x);  // rotate pi/2
    if (DmVector::dotP(direction, vp) < 0.0)
    {
        direction *= -1.0;
    }

    direction *= distance;
    move(direction);
    return true;
}

void DmRay::move(const DmVector& offset)
{
    DmVector pt(data.getBasePoint() + data.getDirection());
    pt.move(offset);
    data.getBasePoint().move(offset);
    moveBorders(offset);
}

void DmRay::rotate(const double& angle)
{
    DmVector pt(data.getBasePoint() + data.getDirection());
    DmVector rvp(angle);
    DmVector vec = data.getBasePoint().rotate(rvp);
    data.setBasePoint(vec);
    pt.rotate(rvp);
    data.setDirection(pt - data.getBasePoint());
    isModify = true;
    calculateBorders();
}

void DmRay::rotate(const DmVector& center, const DmVector& angleVector)
{
    DmVector pt(data.getBasePoint() + data.getDirection());
    DmVector vec = data.getBasePoint().rotate(center, angleVector);
    data.setBasePoint(vec);
    pt.rotate(center, angleVector);
    data.setDirection(pt - data.getBasePoint());
    isModify = true;
    calculateBorders();
}

void DmRay::scale(const DmVector& factor)
{
    DmVector pt = data.getBasePoint().scale(factor);
    DmVector dir = data.getDirection().scale(factor);
    data.setBasePoint(pt);
    data.setDirection(dir);
    isModify = true;
    calculateBorders();
}

void DmRay::scale(const DmVector& center, const DmVector& factor)
{
    DmVector pt = data.getBasePoint().scale(center, factor);
    DmVector dir = data.getDirection().scale(center, factor);
    data.setBasePoint(pt);
    data.setDirection(dir);
    isModify = true;
    calculateBorders();
}

void DmRay::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    DmVector pt(data.getBasePoint() + data.getDirection());
    DmVector basePt = data.getBasePoint().mirror(axisPoint1, axisPoint2);
    data.setBasePoint(basePt);
    pt.mirror(axisPoint1, axisPoint2);
    DmVector dir = pt - data.getBasePoint();
    data.setDirection(dir);
    isModify = true;
    calculateBorders();
}

void DmRay::moveRef(const DmVector& ref, const DmVector& offset)
{
}

void DmRay::calculateBorders()
{
    resetBorders();
    DmVector dir = data.getDirection();
    DmVector basePt = data.getBasePoint();
    double minx = 0.0, miny = 0.0, maxx = 0.0, maxy = 0.0;

    if (dir.x == 0.0)
    {
        minx = maxx = basePt.x;
    }
    else if (dir.x > 0.0)
    {
        maxx = DM_MAXDOUBLE;
        minx = basePt.x;
    }
    else
    {
        maxx = basePt.x;
        minx = DM_MINDOUBLE;
    }

    if (dir.y == 0.0)
    {
        miny = maxy = basePt.y;
    }
    else if (dir.y > 0.0)
    {
        maxy = DM_MAXDOUBLE;
        miny = basePt.y;
    }
    else
    {
        maxy = basePt.y;
        miny = DM_MINDOUBLE;
    }

    minV.x = minx;
    minV.y = miny;
    maxV.x = maxx;
    maxV.y = maxy;
}

bool DmRay::pointInPolygon(const DmVector point,
                           const std::vector<DmVector> vec)
{
    int nCross = 0;
    int nCount = vec.size();
    for (int i = 0; i < nCount; i++)
    {
        DmVector p1 = vec[i];                       // 当前节点
        DmVector p2 = vec[(i + 1) % nCount];        // 下一个节点

        if (p1.y == p2.y)  // p1p2 与 y=p0.y平行
        {
            continue;
        }

        if (point.y < std::min(p1.y, p2.y))  // 交点在p1p2延长线上
        {
            continue;
        }

        if (point.y >= std::max(p1.y, p2.y))  // 交点在p1p2延长线上
        {
            continue;
        }

        double x = (double)(point.y - p1.y) * (double)(p2.x - p1.x)
                   / (double)(p2.y - p1.y) + p1.x;

        if (x > point.x)
        {
            nCross++;  // 只统计单边交点
        }
    }

    // 单边交点为偶数，点在多边形之外 ---
    return (nCross % 2 == 1);
}

Quadratic DmRay::getQuadratic() const
{
    std::vector<double> ce(3, 0.0);
    auto dvp = data.getDirection();
    DmVector normal(-dvp.y, dvp.x);
    ce[0] = normal.x;
    ce[1] = normal.y;
    ce[2] = -normal.dotP(data.getBasePoint() + data.getDirection());
    return Quadratic(ce);
}

std::list<DmEntity*> DmRay::getSubEntities() const
{
    return std::list<DmEntity*>();
}

void DmRay::saveStream(OutputStream& wrt) const
{
    DmAtomicEntity::saveStream(wrt);

    auto base = data.getBasePoint();
    auto dir = data.getDirection();

    wrt << (double)base.x << (double)base.y
        << (double)dir.x << (double)dir.y;
}

void DmRay::restoreStream(InputStream& reader,
                          const std::vector<PAIR>& revs)
{
    DmAtomicEntity::restoreStream(reader, revs);

    int fileRev = getRevisionId("DmRay", revs);
    if (revId > fileRev)
    {
        // 老文件格式
        restoreStreamWithRev(reader, fileRev);
    }
    else
    {
        DmVector base(true), dir(true);
        reader >> (double&)base.x >> (double&)base.y
               >> (double&)dir.x >> (double&)dir.y;

        setBasePoint(base);
        setDirection(dir);
        isModify = true;
    }
}

void DmRay::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (rev == 0)
    {
    }
    else // big change, e.g. change supper class of DmRay
    {
        // step1.
        // read all legacy data one by one
    }
}
