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


/// @file DmPoint.cpp
/// @brief 点实体类实现：几何变换、最近点计算、持久化

#include "DmPoint.h"

#include <iostream>
#include <cmath>

#include "DmCircle.h"
#include "GuiDocumentView.h"

#include "Writer.h"
#include "Reader.h"
#include "Stream.h"

TYPESYSTEM_SOURCE(DmPoint, DmAtomicEntity, 0)

DmPoint::DmPoint(DmEntity* parent, const PointData& d)
    : DmAtomicEntity(parent)
    , data(d)
    , isModify(true)
{
    calculateBorders();
}

DmEntity* DmPoint::clone() const
{
    DmPoint* p = new DmPoint(*this);
    p->m_ulID = DmId();
    p->setSelected(false);
    p->setHighlighted(false);
    return p;
}

DM::EntityType DmPoint::getEntityType() const
{
    return DM::EntityPoint;
}

void DmPoint::calculateBorders()
{
    minV = maxV = data.getPosition();
}

std::list<DmEntity*> DmPoint::getSubEntities() const
{
    return std::list<DmEntity*>();
}

DmVectorSolutions DmPoint::getRefPoints() const
{
    return DmVectorSolutions{ data.getPosition() };
}

DmVector DmPoint::getStartpoint() const
{
    return data.getPosition();
}

DmVector DmPoint::getEndpoint() const
{
    return data.getPosition();
}

PointData DmPoint::getData() const
{
    return data;
}

DmVector DmPoint::getPos() const
{
    return data.getPosition();
}

DmVector DmPoint::getCenter() const
{
    return data.getPosition();
}

double DmPoint::getRadius() const
{
    return 0.0;
}

bool DmPoint::isTangent(const CircleData& circleData) const
{
    double const dist =
        data.getPosition().distanceTo(circleData.getCenter());
    return fabs(dist - fabs(circleData.getRadius()))
           < 50.0 * DM_TOLERANCE;
}

void DmPoint::setPos(const DmVector& pos)
{
    data.setPosition(pos);
}

DmVector DmPoint::getNearestEndpoint(const DmVector& coord,
                                     double* dist) const
{
    if (dist)
    {
        *dist = data.getPosition().distanceTo(coord);
    }

    return data.getPosition();
}

DmVector DmPoint::getNearestPointOnEntity(const DmVector& coord,
                                          bool /*onEntity*/,
                                          double* dist,
                                          DmEntity** entity) const
{
    if (dist)
    {
        *dist = data.getPosition().distanceTo(coord);
    }

    if (entity)
    {
        *entity = const_cast<DmPoint*>(this);
    }

    return data.getPosition();
}

DmVector DmPoint::getNearestCenter(const DmVector& coord,
                                   double* dist) const
{
    if (dist)
    {
        *dist = data.getPosition().distanceTo(coord);
    }

    return data.getPosition();
}

DmVector DmPoint::getMiddlePoint() const
{
    return data.getPosition();
}

DmVector DmPoint::getNearestMiddle(const DmVector& coord,
                                   double* dist,
                                   const int /*middlePoints*/) const
{
    if (dist)
    {
        *dist = data.getPosition().distanceTo(coord);
    }

    return data.getPosition();
}

double DmPoint::getDistanceToPoint(const DmVector& coord,
                                   DmEntity** entity,
                                   DM::ResolveLevel /*level*/) const
{
    if (entity)
    {
        *entity = const_cast<DmPoint*>(this);
    }

    return data.getPosition().distanceTo(coord);
}

void DmPoint::moveStartpoint(const DmVector& pos)
{
    data.setPosition(pos);
    isModify = true;
    calculateBorders();
}

void DmPoint::move(const DmVector& offset)
{
    data.setPosition(getPos().move(offset));
    isModify = true;
    calculateBorders();
}

void DmPoint::rotate(const DmVector& center, const DmVector& angleVector)
{
    data.setPosition(getPos().rotate(center, angleVector));
    isModify = true;
    calculateBorders();
}

void DmPoint::scale(const DmVector& center, const DmVector& factor)
{
    data.setPosition(getPos().scale(center, factor));
    isModify = true;
    calculateBorders();
}

void DmPoint::mirror(const DmVector& axisPoint1,
                     const DmVector& axisPoint2)
{
    data.setPosition(getPos().mirror(axisPoint1, axisPoint2));
    isModify = true;
    calculateBorders();
}

void DmPoint::saveStream(OutputStream& wrt) const
{
    DmAtomicEntity::saveStream(wrt);

    auto p = getPos();

    wrt << (double)p.x << (double)p.y;
}

void DmPoint::restoreStream(InputStream& reader,
                            const std::vector<PAIR>& revs)
{
    DmAtomicEntity::restoreStream(reader, revs);

    int fileRev = getRevisionId("DmPoint", revs);
    if (revId > fileRev)
    {
        // 老文件格式
        restoreStreamWithRev(reader, fileRev);
    }
    else
    {
        DmVector p(true);
        reader >> (double&)p.x >> (double&)p.y;

        setPos(p);
        isModify = true;
    }
}

void DmPoint::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (rev == 0)
    {
    }
    else // big change, e.g. change supper class of DmPoint
    {
        // step1.
        // read all legacy data one by one
    }
}
