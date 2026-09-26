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

TYPESYSTEM_SOURCE(DmLineStrip, DmEntity, 0)

DmLineStrip::DmLineStrip(DmEntity* parent)
    : DmEntity(parent)
    , isModify(true)
{
    calculateBorders();
}

DmLineStrip::DmLineStrip(DmEntity* parent, const LineStripData& d)
    : DmEntity(parent)
    , data(d)
    , isModify(true)
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
    l->setSelected(false);
    l->setHighlighted(false);
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
    isModify = true;
}

void DmLineStrip::clear()
{
    data.clear();
    isModify = true;
}

bool DmLineStrip::isEmpty() const
{
    return (data.getPointCount() == 0);
}

void DmLineStrip::update()
{
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
    isModify = true;
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
    isModify = true;
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
    isModify = true;
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
    isModify = true;
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

const std::vector<float>& DmLineStrip::getVerticesRef(
    int& float_count_per_vertex)
{
    updateVertices();
    float_count_per_vertex = 5;
    return data.getVerticesRef();
}

void DmLineStrip::updateVertices()
{
    if (isModify)
    {
        // 去除重复点
        auto pts = getPoints();
        std::vector<DmVector> new_pts;
        float lastX = 0.0f;
        float lastY = 0.0f;
        float curX = 0.0f;
        float curY = 0.0f;
        bool isFirst = true;
        for (int i = 0; i < (int)pts.size(); i++)
        {
            if (isFirst)
            {
                lastX = (float)pts.at(0).x;
                lastY = (float)pts.at(0).y;
                new_pts.emplace_back(pts.at(0));
                isFirst = false;
            }
            else
            {
                curX = (float)pts.at(i).x;
                curY = (float)pts.at(i).y;
                if (curX - lastX == 0.0f && curY - lastY == 0.0f)
                {
                    continue;
                }
                new_pts.emplace_back(pts.at(i));
                lastX = curX;
                lastY = curY;
            }
        }
        // 如果闭合，最后的点不能与第一个点重复
        if (isClosed())
        {
            float firstX = (float)pts.at(0).x;
            float firstY = (float)pts.at(0).y;
            if (firstX - lastX == 0.0f && firstY - lastY == 0.0f)
            {
                new_pts.erase(new_pts.end() - 1);
            }
        }
        if (new_pts.size() < 2)
        {
            return;
        }
        int pointCount = (int)new_pts.size();
        std::vector<float> vertexs;
        // 闭合
        if (isClosed())
        {
            vertexs.reserve((pointCount + 3) * 5);
            float total_length = 0.0f;
            float lastX_v = 0.0f;
            float lastY_v = 0.0f;
            float curX_v = 0.0f;
            float curY_v = 0.0f;
            float para = 0.0f;
            for (int i = 0; i < pointCount; i++)
            {
                curX_v = new_pts.at(i).x;
                curY_v = new_pts.at(i).y;
                if (i == 0)
                {
                    // 针对GL_LINE_STRIP_ADJACENCY的起始坐标
                    vertexs.emplace_back(new_pts.back().x);
                    vertexs.emplace_back(new_pts.back().y);
                    vertexs.emplace_back(0.0f);
                    vertexs.emplace_back(0.0f);
                    vertexs.emplace_back(total_length);
                }
                else
                {
                    para += DmVector(lastX_v, lastY_v).distanceTo(
                        DmVector(curX_v, curY_v));
                }

                vertexs.emplace_back(curX_v);
                vertexs.emplace_back(curY_v);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(para);
                vertexs.emplace_back(total_length);
                lastX_v = curX_v;
                lastY_v = curY_v;
                if (i == pointCount - 1)
                {
                    // 最后一段的终点
                    vertexs.emplace_back(new_pts.front().x);
                    vertexs.emplace_back(new_pts.front().y);
                    vertexs.emplace_back(0.0f);
                    vertexs.emplace_back(para);
                    vertexs.emplace_back(total_length);

                    // 针对GL_LINE_STRIP_ADJACENCY的终止坐标
                    vertexs.emplace_back(new_pts.at(1).x);
                    vertexs.emplace_back(new_pts.at(1).y);
                    vertexs.emplace_back(0.0f);
                    vertexs.emplace_back(para);
                    vertexs.emplace_back(total_length);
                }
            }
            // 设置总长
            for (int i = 0; i < pointCount + 3; i++)
            {
                vertexs.at(i * 5 + 4) = para;
            }
        }
        // 不闭合
        else
        {
            vertexs.reserve((pointCount + 2) * 5);
            float total_length = 0.0f;
            float lastX_v = 0.0f;
            float lastY_v = 0.0f;
            float curX_v = 0.0f;
            float curY_v = 0.0f;
            float para = 0.0f;
            for (int i = 0; i < pointCount; i++)
            {
                curX_v = new_pts.at(i).x;
                curY_v = new_pts.at(i).y;
                if (i == 0)
                {
                    // 针对GL_LINE_STRIP_ADJACENCY的起始坐标
                    vertexs.emplace_back(new_pts.at(1).x);
                    vertexs.emplace_back(new_pts.at(1).y);
                    vertexs.emplace_back(0.0f);
                    vertexs.emplace_back(0.0f);
                    vertexs.emplace_back(total_length);
                }
                else
                {
                    para += DmVector(lastX_v, lastY_v).distanceTo(
                        DmVector(curX_v, curY_v));
                }

                vertexs.emplace_back(curX_v);
                vertexs.emplace_back(curY_v);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(para);
                vertexs.emplace_back(total_length);
                lastX_v = curX_v;
                lastY_v = curY_v;
                if (i == pointCount - 1)
                {
                    // 针对GL_LINE_STRIP_ADJACENCY的终止坐标
                    vertexs.emplace_back(
                        new_pts.at(pointCount - 2).x);
                    vertexs.emplace_back(
                        new_pts.at(pointCount - 2).y);
                    vertexs.emplace_back(0.0f);
                    vertexs.emplace_back(para);
                    vertexs.emplace_back(total_length);
                }
            }
            // 设置总长
            for (int i = 0; i < pointCount + 2; i++)
            {
                vertexs.at(i * 5 + 4) = para;
            }
        }

        data.setVertices(vertexs);
        isModify = false;
    }
}

bool DmLineStrip::isClosed()
{
    return data.isClosed();
}

void DmLineStrip::setClosed(bool isClosed)
{
    data.setIsClosed(isClosed);
    isModify = true;
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
