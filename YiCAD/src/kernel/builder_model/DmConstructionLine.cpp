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


/// @file DmConstructionLine.cpp
/// @brief 构造线实体类实现

#include "DmConstructionLine.h"

#include "Debug.h"
#include "Quadratic.h"
#include "Math2d.h"

DmConstructionLineData::DmConstructionLineData()
    : point1(false)
    , point2(false)
{
}

DmConstructionLineData::DmConstructionLineData(const DmVector& point1, const DmVector& point2)
    : point1(point1)
    , point2(point2)
{
}

DmConstructionLine::DmConstructionLine(DmEntity* parent, const DmConstructionLineData& d)
    : DmAtomicEntity(parent)
    , data(d)
{
    calculateBorders();
}

DM::EntityType DmConstructionLine::getEntityType() const
{
    return DM::EntityConstructionLine;
}

DmEntity* DmConstructionLine::clone() const
{
    DmConstructionLine* c = new DmConstructionLine(*this);
    c->m_ulID = DmId();
    c->setSelected(false);
    c->setHighlighted(false);
    return c;
}

void DmConstructionLine::calculateBorders()
{
    minV = DmVector::minimum(data.point1, data.point2);
    maxV = DmVector::maximum(data.point1, data.point2);
}

std::list<DmEntity*> DmConstructionLine::getSubEntities() const
{
    return std::list<DmEntity*>();
}

DmVector DmConstructionLine::getNearestEndpoint(const DmVector& coord, double* dist) const
{
    double dist1 = 0.0, dist2 = 0.0;

    dist1 = (data.point1 - coord).squared();
    dist2 = (data.point2 - coord).squared();

    if (dist2 < dist1)
    {
        if (dist)
        {
            *dist = sqrt(dist2);
        }
        return data.point2;
    }
    else
    {
        if (dist)
        {
            *dist = sqrt(dist1);
        }
        return data.point1;
    }
}

DmVector DmConstructionLine::getNearestPointOnEntity(const DmVector& coord, bool /*onEntity*/, double* /*dist*/, DmEntity** entity) const
{
    if (entity)
    {
        *entity = const_cast<DmConstructionLine*>(this);
    }

    constexpr double kMinMagnitude = 1.0e-6;
    DmVector ae = data.point2 - data.point1;
    DmVector ea = data.point1 - data.point2;
    DmVector ap = coord - data.point1;
    if (ae.magnitude() < kMinMagnitude || ea.magnitude() < kMinMagnitude)
    {
        return DmVector(false);
    }

    // Orthogonal projection from both sides:
    DmVector ba = ae * DmVector::dotP(ae, ap) / (ae.magnitude() * ae.magnitude());

    return data.point1 + ba;
}

DmVector DmConstructionLine::getNearestCenter(const DmVector& /*coord*/, double* dist) const
{
    if (dist)
    {
        *dist = DM_MAXDOUBLE;
    }

    return DmVector(false);
}

/// @return Copy of data that defines the line.
DmConstructionLineData const& DmConstructionLine::getData() const
{
    return data;
}

/// @return First definition point.
DmVector const& DmConstructionLine::getPoint1() const
{
    return data.point1;
}

/// @return Second definition point.
DmVector const& DmConstructionLine::getPoint2() const
{
    return data.point2;
}

/// @return Start point of the entity
DmVector DmConstructionLine::getStartpoint() const
{
    return data.point1;
}

/// @return End point of the entity
DmVector DmConstructionLine::getEndpoint() const
{
    return data.point2;
}

/// @return Direction 1. The angle at which the arc starts at the startpoint.
double DmConstructionLine::getDirection1(void) const
{
    return Math2d::correctAngle(data.point1.angleTo(data.point2));
}

/// @return Direction 2. The angle at which the arc starts at the endpoint.
double DmConstructionLine::getDirection2(void) const
{
    return Math2d::correctAngle(data.point2.angleTo(data.point1));
}

Quadratic DmConstructionLine::getQuadratic() const
{
    std::vector<double> ce(3, 0.);
    auto dvp = data.point2 - data.point1;
    DmVector normal(-dvp.y, dvp.x);
    ce[0] = normal.x;
    ce[1] = normal.y;
    ce[2] = -normal.dotP(data.point2);
    return Quadratic(ce);
}

DmVector DmConstructionLine::getMiddlePoint() const
{
    return DmVector(false);
}

DmVector DmConstructionLine::getNearestMiddle(const DmVector& /*coord*/, double* dist, const int /*middlePoints*/)const
{
    if (dist)
    {
        *dist = DM_MAXDOUBLE;
    }
    return DmVector(false);
}

double DmConstructionLine::getDistanceToPoint(const DmVector& coord, DmEntity** entity, DM::ResolveLevel /*level*/) const
{
    if (entity)
    {
        *entity = const_cast<DmConstructionLine*>(this);
    }
    DmVector se = data.point2 - data.point1;
    double d(se.magnitude());
    if (d < DM_TOLERANCE)
    {
        // line too short
        return DM_MAXDOUBLE;
    }
    se.set(se.x / d, -se.y / d); //normalized
    DmVector vpc = coord - data.point1;
    vpc.rotate(se); // rotate to use the line as x-axis, and the distance is fabs(y)
    return (fabs(vpc.y));
}

void DmConstructionLine::move(const DmVector& offset)
{
    data.point1.move(offset);
    data.point2.move(offset);
}

void DmConstructionLine::rotate(const DmVector& center, const DmVector& angleVector)
{
    data.point1.rotate(center, angleVector);
    data.point2.rotate(center, angleVector);
}

void DmConstructionLine::scale(const DmVector& center, const DmVector& factor)
{
    data.point1.scale(center, factor);
    data.point2.scale(center, factor);
}

void DmConstructionLine::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    data.point1.mirror(axisPoint1, axisPoint2);
    data.point2.mirror(axisPoint1, axisPoint2);
}
