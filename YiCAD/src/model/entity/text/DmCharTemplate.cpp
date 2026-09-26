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


/// @file DmCharTemplate.cpp
/// @brief 文字模板类实现

#include "DmCharTemplate.h"
#include "DmChar.h"
#include "DmCircle.h"
#include "DmArc.h"
#include "DmPolyline.h"
#include "DmFont.h"
#include "DmSpline.h"
#include "DmTriangle.h"
#include "Math2d.h"
#include "GeometryMethods.h"

// 圆弧分段数
constexpr int ARC_SEGMENT_COUNT = 12;
// 三角形顶点数
constexpr int TRIANGLE_VERTEX_COUNT = 3;
// 默认宽高比
constexpr double DEFAULT_WIDTH_HEIGHT_FACTOR = 1.0;
// 无倾斜角度
constexpr double NO_SLASH_ANGLE = 0.0;

DmCharTemplate::DmCharTemplate(DmEntity* parent, const QString& name, DmCharTemplateList* owner)
    : DmEntity(parent)
    , m_name(name)
    , m_dWidthHeightFactor(DEFAULT_WIDTH_HEIGHT_FACTOR)
    , m_charTemplateList(owner)
    , m_dAscenderFactor(DEFAULT_WIDTH_HEIGHT_FACTOR)
    , m_dHeight(0.0)
{
    if (owner != nullptr)
    {
        owner->add(name, this);
    }
}

DmEntity* DmCharTemplate::clone() const
{
    // 不支持克隆
    return nullptr;
}

DmCharTemplate::~DmCharTemplate()
{
    for (auto e : entities)
    {
        delete e;
    }
    entities.clear();
}

DM::EntityType DmCharTemplate::getEntityType() const
{
    return DM::EntityCharTemplate;
}

DmChar* DmCharTemplate::generateChar(const double& widthFactor, const double& slashAngle)
{
    DmChar* c = new DmChar(this);
    c->setHeight(getHeight());
    c->setWidth(getWHFactor() * getHeight() * widthFactor);
    c->setWidthFactor(widthFactor);
    c->setSlashAngle(slashAngle);
    c->setLayer(nullptr);

    if (widthFactor == DEFAULT_WIDTH_HEIGHT_FACTOR && slashAngle == NO_SLASH_ANGLE)
    {
        // 不需要做倾斜处理
        for (auto stroke : entities)
        {
            DmEntity* newEnt = stroke->clone();
            newEnt->setParent(c);
            c->addEntity(newEnt);
        }
    }
    else
    {
        // 遍历每个笔画，做倾斜及宽度放缩
        for (auto stroke : entities)
        {
            if (stroke->getEntityType() == DM::EntityPolyline)
            {
                DmPolyline* poly = static_cast<DmPolyline*>(stroke);
                getShearedEntityForPolyline(*poly, c, DmVector(0.0, 0.0, 0.0), DM::X, slashAngle, widthFactor);
            }
            else if (stroke->getEntityType() == DM::EntityCircle)
            {
                DmCircle* pCicle = static_cast<DmCircle*>(stroke);
                getShearedEntityForCircle(*pCicle, c, DmVector(0.0, 0.0, 0.0), DM::X, slashAngle, widthFactor);
            }
            else if (stroke->getEntityType() == DM::EntityTriangle)
            {
                DmTriangle* pTriangle = static_cast<DmTriangle*>(stroke);
                getShearedEntityForTriangle(*pTriangle, c, DmVector(0.0, 0.0, 0.0), DM::X, slashAngle, widthFactor);
            }
        }
    }
    c->calculateBorders();
    return c;
}

double DmCharTemplate::getWHFactor() const
{
    return m_dWidthHeightFactor;
}

void DmCharTemplate::setWHFactor(double factor)
{
    m_dWidthHeightFactor = factor;
}

double DmCharTemplate::getHeight() const
{
    return m_dHeight;
}

void DmCharTemplate::setHeight(const double height)
{
    m_dHeight = height;
}

double DmCharTemplate::getAscenderFactor() const
{
    return m_dAscenderFactor;
}

void DmCharTemplate::setAscenderFactor(const double& ascenderFactor)
{
    m_dAscenderFactor = ascenderFactor;
}

DmCharTemplateList* DmCharTemplate::getOwner() const
{
    return m_charTemplateList;
}

void DmCharTemplate::setOwner(DmCharTemplateList* owner)
{
    m_charTemplateList = owner;
}

QString DmCharTemplate::getName() const
{
    return m_name;
}

void DmCharTemplate::addEntity(DmEntity* e)
{
    if (!e)
    {
        return;
    }
    entities.emplace_back(e);
}

bool DmCharTemplate::isEmpty() const
{
    return entities.size() == 0;
}

bool DmCharTemplate::isContainer() const
{
    return false;
}

void DmCharTemplate::calculateBorders()
{
    resetBorders();
    for (auto c : entities)
    {
        c->calculateBorders();
        minV = DmVector::minimum(c->getMin(), minV);
        maxV = DmVector::maximum(c->getMax(), maxV);
    }
}

DmVector DmCharTemplate::getNearestEndpoint(const DmVector& coord, double* dist) const
{
    return DmVector(false);
}

DmVector DmCharTemplate::getNearestPointOnEntity(const DmVector&, bool onEntity, double* dist, DmEntity** entity) const
{
    return DmVector(false);
}

DmVector DmCharTemplate::getNearestCenter(const DmVector& coord, double* dist) const
{
    return DmVector(false);
}

DmVector DmCharTemplate::getNearestMiddle(const DmVector& coord, double* dist, int middlePoints) const
{
    return DmVector(false);
}

void DmCharTemplate::rotate(const DmVector& center, const DmVector& angleVector)
{
    for (auto e : entities)
    {
        e->rotate(center, angleVector);
    }
    calculateBorders();
}

void DmCharTemplate::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    if (axisPoint1.distanceTo(axisPoint2) > DM_TOLERANCE)
    {
        for (auto e : entities)
        {
            e->mirror(axisPoint1, axisPoint2);
        }
    }
    calculateBorders();
}

void DmCharTemplate::scale(const DmVector& center, const DmVector& factor)
{
    for (auto e : entities)
    {
        e->scale(center, factor);
    }
    m_dHeight *= factor.y;
    calculateBorders();
}

void DmCharTemplate::move(const DmVector& offset)
{
    for (auto e : entities)
    {
        e->move(offset);
    }
    moveBorders(offset);
}

std::list<DmEntity*> DmCharTemplate::getSubEntities() const
{
    return std::list<DmEntity*>();
}

DmPolyline* DmCharTemplate::getShearedEntityForArc(const DmArc& arc, DmChar* c, const DmVector& shearOrigin, DM::ShearDirection shearDirection, double angle, double widthScale)
{
    double tana = tan(angle);
    PolylineData data;
    DmPolyline* poly = new DmPolyline(c, data);
    // todoRao: 仅针对文字的切变，待完善
    poly->setPen(arc.getPen(false));
    poly->setLayer(arc.getLayer(false));

    double startAngle = arc.getStartAngleNormal();
    double endAngle = arc.getEndAngleNormal();
    if (endAngle < startAngle)
    {
        endAngle += M_PI * 2;
    }
    double deltaAngle = endAngle - startAngle;
    double delta = deltaAngle / static_cast<double>(ARC_SEGMENT_COUNT);
    double r = arc.getRadius();

    if (shearDirection == DM::X)
    {
        for (int i = 0; i <= ARC_SEGMENT_COUNT; i++)
        {
            double ang = startAngle + i * delta;
            double sina = sin(ang);
            double cosa = cos(ang);
            DmVector pt(cosa * r + arc.getCenter().x, sina * r + arc.getCenter().y, 0.0);
            double dY = pt.y - shearOrigin.y;
            double deltaX = dY * tana;
            DmVector pt_offet(pt.x + deltaX, pt.y, 0.0);
            poly->appendVertex(pt_offet);
        }
    }
    else
    {
        for (int i = 0; i < ARC_SEGMENT_COUNT; i++)
        {
            // todo:待修改
            double ang = arc.getStartAngle() + i * delta;
            double ang_r = Math2d::deg2rad(ang);
            double sina = sin(ang_r);
            double cosa = cos(ang_r);
            DmVector pt(cosa + arc.getCenter().x, sina + arc.getCenter().y, 0.0);
            double dX = pt.x - shearOrigin.x;
            double deltaY = pt.y + dX * tana;
            DmVector pt_offet(pt.x, pt.y + deltaY, 0.0);
            poly->appendVertex(pt_offet);
        }
    }
    // x方向拉伸
    poly->scale(shearOrigin, DmVector(widthScale, 1.0));
    poly->update();
    c->addEntity(poly);
    return poly;
}

DmPolyline* DmCharTemplate::getShearedEntityForCircle(const DmCircle& circle, DmChar* c, const DmVector& shearOrigin, DM::ShearDirection shearDirection, double angle, double widthScale)
{
    double tana = tan(angle);
    DmVector center = circle.getCenter();
    double r = circle.getRadius();

    ArcData data;
    data.setCenter(center);
    data.setStartAngle(0.0);
    data.setEndAngle(M_PI * 2.0);
    data.setRadius(r);
    data.setNormal(DmVector(0.0, 0.0, 1.0));
    DmArc* pArc = new DmArc(c, data);
    pArc->setPen(circle.getPen(false));
    pArc->setLayer(circle.getLayer(false));
    DmPolyline* poly = getShearedEntityForArc(*pArc, c, shearOrigin, shearDirection, angle, widthScale);
    return poly;
}

DmLine* DmCharTemplate::getShearedEntityForLine(const DmLine& line, const DmVector& shearOrigin, DM::ShearDirection shearDirection, double angle, double widthScale)
{
    double tana = tan(angle);
    DmVector start = line.getStartpoint();
    DmVector end = line.getEndpoint();

    if (shearDirection == DM::X)
    {
        double deltaY_start = start.y - shearOrigin.y;
        double deltaX_start = deltaY_start * tana;
        DmVector start_offet(start.x + deltaX_start, start.y, 0.0);
        double deltaY_end = end.y - shearOrigin.y;
        double deltaX_end = deltaY_end * tana;
        DmVector end_offet(end.x + deltaX_end, end.y, 0.0);
        DmLine* pLine = new DmLine(start_offet, end_offet);
        pLine->scale(shearOrigin, DmVector(widthScale, 1.0));
        // todoRao: 仅针对文字的切变，待完善
        pLine->setPen(line.getPen(false));
        pLine->setLayer(line.getLayer(false));
        return pLine;
    }
    else
    {
        // todo：待修改
        double deltaX_start = start.x - shearOrigin.x;
        double deltaY_start = start.y + deltaX_start * tana;
        DmVector start_offet(start.x, start.y + deltaY_start, 0.0);
        double deltaX_end = end.x - shearOrigin.x;
        double deltaY_end = end.y + deltaX_end * tana;
        DmVector end_offet(end.x, end.y + deltaY_end, 0.0);
        DmLine* pLine = new DmLine(start_offet, end_offet);
        DmPen pen(DmColor(DM::FlagByBlock), DM::WidthByBlock, DmLineTypeTable::ByBlock);
        pLine->setPen(pen);
        pLine->setLayer(nullptr);
        return pLine;
    }
}

std::vector<DmEntity*> DmCharTemplate::getShearedEntityForPolyline(const DmPolyline& poly, DmChar* c, const DmVector& shearOrigin, DM::ShearDirection shearDirection, double angle, double widthScale)
{
    // 文字里的多段线没有线宽
    std::vector<DmEntity*> ents;
    int bulgeSize = poly.getDataConstRef().getBulgesCount();
    double bulge = 0.0;
    DmVector pt1(true), pt2(true);
    DmVector center(true);
    double radius = 0.0;
    double startAngle = 0.0;
    double endAngle = 0.0;
    DmVector normal(true);

    for (int i = 0; i < bulgeSize; i++)
    {
        poly.getSegmentInfoAt(i, bulge, pt1, pt2);
        if (bulge == 0.0)
        {
            DmLine line(nullptr, LineData(pt1, pt2));
            DmLine* pShearLine = getShearedEntityForLine(line, shearOrigin, shearDirection, angle, widthScale);
            pShearLine->setParent(c);
            c->addEntity(pShearLine);
            ents.push_back(pShearLine);
        }
        else
        {
            GeometryMethods::getArcInfo(pt1, pt2, bulge, center, radius, startAngle, endAngle, normal);
            DmArc arc(nullptr, ArcData(center, normal, radius, startAngle, endAngle));
            DmPolyline* sharedArc = getShearedEntityForArc(arc, c, shearOrigin, shearDirection, angle, widthScale);
            if (sharedArc)
            {
                ents.push_back(sharedArc);
            }
        }
    }
    return ents;
}

DmTriangle* DmCharTemplate::getShearedEntityForTriangle(const DmTriangle& triangle, DmChar* c, const DmVector& shearOrigin, DM::ShearDirection shearDirection, double angle, double widthScale)
{
    std::array<DmVector, TRIANGLE_VERTEX_COUNT> corners = triangle.getData().getPoints();
    double tana = tan(angle);

    if (shearDirection == DM::X)
    {
        std::array<DmVector, TRIANGLE_VERTEX_COUNT> newCorners;
        for (int i = 0; i < TRIANGLE_VERTEX_COUNT; i++)
        {
            DmVector corner = corners[i];
            double deltaY = corner.y - shearOrigin.y;
            double deltaX = deltaY * tana;
            DmVector pt(corner.x + deltaX, corner.y, 0.0);
            newCorners[i] = pt;
        }
        TriangleData data;
        data.setPoints(newCorners);
        DmTriangle* newTriangle = new DmTriangle(c, data);
        c->addEntity(newTriangle);
        newTriangle->scale(shearOrigin, DmVector(widthScale, 1.0));
        newTriangle->setPen(triangle.getPen(false));
        newTriangle->setLayer(triangle.getLayer(false));
        return newTriangle;
    }
    return nullptr;
}
