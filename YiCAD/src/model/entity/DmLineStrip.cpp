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


/// @file DmLineStrip.cpp
/// @brief 可带线型的折线段实体实现，支持多点定义和顶点渲染

#include "DmLineStrip.h"
#include "DmLine.h"
#include "Information.h"
#include "IGiGeometry.h"

TYPESYSTEM_SOURCE(DmLineStrip, DmEntity, 0)

DmLineStrip::DmLineStrip(DmEntity* parent)
    : DmEntity(parent)
{
    calculateBorders();
}

DmLineStrip::DmLineStrip(DmEntity* parent, const LineStripData& d)
    : DmEntity(parent)
    , data(d)
{
    calculateBorders();
}

bool DmLineStrip::isContainer() const
{
    return false;
}

DmEntity* DmLineStrip::clone() const
{
    DmLineStrip* l = new DmLineStrip(*this);
    l->m_ulID = DmId();
    l->update();
    return l;
}

DM::EntityType DmLineStrip::getEntityType() const
{
    return DM::EntityLineStrip;
}

DmVector DmLineStrip::getNearestPointOnEntity(const DmVector& coord,
    bool onEntity, double* dist, DmEntity** entity) const
{
    if (entity)
    {
        *entity = const_cast<DmLineStrip*>(this);
    }

    int count = data.getPointCount();
    DmVector pt;
    DmVector nearestPt;
    double max_dist_square = DM_MAXDOUBLE;
    double dist_square = DM_MAXDOUBLE;
    for (int i = 0; i < count; i++)
    {
        pt = data.getPointAt(i);
        dist_square = pt.squaredTo(coord);
        if (dist_square < max_dist_square)
        {
            max_dist_square = dist_square;
            nearestPt = pt;
        }
    }
    if (dist)
    {
        *dist = nearestPt.distanceTo(coord);
    }
    return nearestPt;
}

DmVector DmLineStrip::getNearestCenter(const DmVector& coord,
    double* dist) const
{
    if (dist)
    {
        *dist = DM_MAXDOUBLE;
    }
    return DmVector(false);
}

DmVector DmLineStrip::getNearestMiddle(const DmVector& coord,
    double* dist, int middlePoints) const
{
    if (dist)
    {
        *dist = DM_MAXDOUBLE;
    }
    return DmVector(false);
}

DmVector DmLineStrip::getStartpoint() const
{
    return data.getPointAt(0);
}

DmVector DmLineStrip::getEndpoint() const
{
    return data.getPointAt(data.getPointCount() - 1);
}

std::vector<DmVector> DmLineStrip::getPoints() const
{
    return data.getPoints();
}

void DmLineStrip::setPoints(const std::vector<DmVector>& pts)
{
    data.setPoints(pts);
}

void DmLineStrip::clear()
{
    data.clear();
}

bool DmLineStrip::isEmpty() const
{
    return (data.getPointCount() == 0);
}

void DmLineStrip::update()
{
    bumpRevision();
    calculateBorders();
}

void DmLineStrip::move(const DmVector& offset)
{
    int count = data.getPointCount();
    for (int i = 0; i < count; i++)
    {
        DmVector pt = data.getPointAt(i);
        pt.move(offset);
        data.setPointAt(i, pt);
    }
    moveBorders(offset);
}

void DmLineStrip::rotate(const DmVector& center,
    const DmVector& angleVector)
{
    int count = data.getPointCount();
    for (int i = 0; i < count; i++)
    {
        DmVector pt = data.getPointAt(i);
        pt.rotate(center, angleVector);
        data.setPointAt(i, pt);
    }
    calculateBorders();
}

void DmLineStrip::scale(const DmVector& center,
    const DmVector& factor)
{
    int count = data.getPointCount();
    for (int i = 0; i < count; i++)
    {
        DmVector pt = data.getPointAt(i);
        pt.scale(center, factor);
        data.setPointAt(i, pt);
    }
    calculateBorders();
}

void DmLineStrip::mirror(const DmVector& axisPoint1,
    const DmVector& axisPoint2)
{
    int count = data.getPointCount();
    for (int i = 0; i < count; i++)
    {
        DmVector pt = data.getPointAt(i);
        pt.mirror(axisPoint1, axisPoint2);
        data.setPointAt(i, pt);
    }
    calculateBorders();
}

std::list<DmEntity*> DmLineStrip::getSubEntities() const
{
    return std::list<DmEntity*>();
}

void DmLineStrip::calculateBorders()
{
    resetBorders();
    auto pts = data.getPoints();
    for (auto pt : pts)
    {
        minV = DmVector::minimum(minV, pt);
        maxV = DmVector::maximum(maxV, pt);
    }
}

bool DmLineStrip::isClosed()
{
    return data.isClosed();
}

void DmLineStrip::setClosed(bool isClosed)
{
    data.setIsClosed(isClosed);
}

void DmLineStrip::saveStream(OutputStream& wrt) const
{
    DmEntity::saveStream(wrt);
    int count = data.getPointCount();
    wrt << (int32_t)count;
    for (int i = 0; i < count; i++)
    {
        DmVector pt = data.getPointAt(i);
        wrt << (double)pt.x << (double)pt.y;
    }
}

void DmLineStrip::restoreStream(InputStream& reader,
    const std::vector<PAIR>& revs)
{
    DmEntity::restoreStream(reader, revs);

    int fileRev = getRevisionId("DmLineStrip", revs);
    if (revId > fileRev)
    {
        // 老文件格式
        restoreStreamWithRev(reader, fileRev);
    }
    else
    {
        data.clear();
        int32_t count = 0;
        reader << (int32_t&)count;
        DmVector pt(true);
        for (int i = 0; i < (int)count; i++)
        {
            reader >> (double&)pt.x >> (double&)pt.y;
            data.appendPoint(pt);
        }
        calculateBorders();
    }
}

void DmLineStrip::restoreStreamWithRev(InputStream& rdr, int rev)
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

void DmLineStrip::worldDraw(IGiWorldDraw& wd) const
{
    // 离散后的曲线：整条连续计算弧长
    const std::vector<DmVector> points = data.getPoints();
    GiPolylineFlags flags = GiPolylineFlags::ContinuousLinetype;
    if (data.isClosed())
    {
        flags = flags | GiPolylineFlags::Closed;
    }
    wd.geometry().polyline(points, {}, {}, flags);
}
