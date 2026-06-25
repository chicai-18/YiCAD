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


/// @file DmArc.cpp
/// @brief 圆弧实体类的实现，包括构造、几何计算、捕捉、序列化等功能

#include "DmArc.h"

#include <cmath>

#include "DmLine.h"
#include "Information.h"
#include "Math2d.h"
#include "GuiDocumentView.h"
#include "Quadratic.h"
#include "Debug.h"
#include "DmRect.h"

#include "Writer.h"
#include "Reader.h"
#include "Stream.h"

TYPESYSTEM_SOURCE(DmArc, DmAtomicEntity, 0)

// Default constructor.
DmArc::DmArc(DmEntity* parent, const ArcData& d)
    : DmAtomicEntity(parent)
    , data(d)
    , isModify(true)
{
    calculateBorders();
}

DmEntity* DmArc::clone() const
{
    DmArc* a = new DmArc(*this);
    a->m_ulID = DmId();
    a->setSelected(false);
    a->setHighlighted(false);
    return a;
}

DM::EntityType DmArc::getEntityType() const
{
    return DM::EntityArc;
}

ArcData DmArc::getData() const
{
    return data;
}

ArcData& DmArc::getDataRef()
{
    return data;
}

const ArcData& DmArc::getDataConstRef() const
{
    return data;
}

void DmArc::setData(const ArcData& d)
{
    data = d;
}

DmVector DmArc::getCenter() const
{
    return data.getCenter();
}

void DmArc::setCenter(const DmVector& c)
{
    data.setCenter(c);
}

double DmArc::getRadius() const
{
    return data.getRadius();
}

void DmArc::setRadius(double r)
{
    data.setRadius(r);
}

double DmArc::getStartAngle() const
{
    return data.getStartAngle();
}

void DmArc::setStartAngle(double a1)
{
    data.setStartAngle(a1);
}

double DmArc::getStartAngleNormal() const
{
    if (isClockwise())
    {
        return Math2d::correctAngle(M_PI - getEndAngle());
    }
    else
    {
        return getStartAngle();
    }
}

double DmArc::getEndAngle() const
{
    return data.getEndAngle();
}

void DmArc::setEndAngle(double a2)
{
    data.setEndAngle(a2);
}

double DmArc::getEndAngleNormal() const
{
    if (isClockwise())
    {
        return Math2d::correctAngle(M_PI - getStartAngle());
    }
    else
    {
        return getEndAngle();
    }
}

double DmArc::getAngleNormal(double a) const
{
    if (isClockwise())
    {
        return Math2d::correctAngle(M_PI - a);
    }
    else
    {
        return a;
    }
}

double DmArc::getArcAngle(const DmVector& vp)
{
    return (vp - data.getCenter()).angle();
}

/// @brief 从3个给定点创建圆弧
/// @param p1 第1个点
/// @param p2 第2个点
/// @param p3 第3个点
/// @return 创建成功返回true，否则返回false
bool DmArc::createFrom3P(const DmVector& p1, const DmVector& p2, const DmVector& p3)
{
    // 求出向量p1p2, p1p3的圆心，再平移p1位移
    DmVector vra = p2 - p1;
    DmVector vrb = p3 - p1;
    double ra2 = vra.squared() * 0.5;
    double rb2 = vrb.squared() * 0.5;
    double crossp = vra.x * vrb.y - vra.y * vrb.x;
    if (fabs(crossp) < DM_TOLERANCE2)
    {
        return false;
    }
    crossp = 1. / crossp;
    data.setCenter(DmVector((ra2 * vrb.y - rb2 * vra.y) * crossp, (rb2 * vra.x - ra2 * vrb.x) * crossp));
    data.setRadius(data.getCenter().magnitude());
    data.setCenter(data.getCenter() + p1);

    //计算起始终止角度，法向
    double p1Angle = data.getCenter().angleTo(p1);
    double p3Angle = data.getCenter().angleTo(p3);
    double p2Angle = data.getCenter().angleTo(p2);
    if (Math2d::isAngleBetween(p2Angle, p1Angle, p3Angle))
    {
        data.setStartAngle(p1Angle);
        data.setEndAngle(p3Angle);
        data.setNormal(DmVector(0.0, 0.0, 1.0));
    }
    else
    {
        double startAngle = Math2d::correctAngle(M_PI - p1Angle);
        double endAngle = Math2d::correctAngle(M_PI - p3Angle);
        data.setStartAngle(startAngle);
        data.setEndAngle(endAngle);
        data.setNormal(DmVector(0.0, 0.0, -1.0));
        setClockwise(!isClockwise());//最终法向设置为正
    }
    calculateBorders();
    return true;
}

// Creates an arc from its startpoint, endpoint and bulge.
bool DmArc::createFrom2PBulge(const DmVector& startPoint, const DmVector& endPoint, double bulge)
{
    double alpha = atan(bulge) * 4.0;    //圆弧所对应的弧度角。逆时针为正，顺时针为负

    DmVector middle = (startPoint + endPoint) / 2.0;
    double dist = startPoint.distanceTo(endPoint) / 2.0;    //半个弦长

    // alpha can't be 0.0 at this point
    data.setRadius(fabs(dist / sin(alpha / 2.0)));

    double wu = fabs(Math2d::pow(data.getRadius(), 2.0) - Math2d::pow(dist, 2.0));
    double h = sqrt(wu);
    double angle = startPoint.angleTo(endPoint);    //h对应的向量。指向圆心方向

    if (bulge > 0.0)
    {
        angle += M_PI_2;
    }
    else
    {
        angle -= M_PI_2;
    }

    if (fabs(alpha) > M_PI)
    {
        h *= -1.0;
    }

    DmVector center = getCenter();
    center.setPolar(h, angle);
    data.setCenter(center);
    data.setCenter(data.getCenter() + middle);

    double startAngle = data.getCenter().angleTo(startPoint);
    double endAngle = data.getCenter().angleTo(endPoint);
    if (bulge > 0.0)
    {
        data.setStartAngle(startAngle);
        data.setEndAngle(endAngle);
    }
    else
    {
        data.setStartAngle(endAngle);
        data.setEndAngle(startAngle);
    }
    data.setNormal(DmVector(0.0, 0.0, 1.0));

    isModify = true;
    calculateBorders();

    return true;
}

void DmArc::calculateBorders()
{
    double a1 = 0.0;
    double a2 = 0.0;
    if (isClockwise())
    {
        a1 = Math2d::correctAngle(M_PI - getEndAngle());
        a2 = Math2d::correctAngle(M_PI - getStartAngle());
    }
    else
    {
        a1 = getStartAngle();
        a2 = getEndAngle();
    }
    DmVector const p1 = data.getCenter() + DmVector::polar(data.getRadius(), a1);
    DmVector const p2 = data.getCenter() + DmVector::polar(data.getRadius(), a2);
    DmRect const rect{ p1, p2 };

    double minX = rect.lowerLeftCorner().x;
    double minY = rect.lowerLeftCorner().y;
    double maxX = rect.upperRightCorner().x;
    double maxY = rect.upperRightCorner().y;
    if (Math2d::isAngleBetween(0.5 * M_PI, a1, a2))
    {
        maxY = data.getCenter().y + data.getRadius();
    }
    if (Math2d::isAngleBetween(1.5 * M_PI, a1, a2))
    {
        minY = data.getCenter().y - data.getRadius();
    }
    if (Math2d::isAngleBetween(M_PI, a1, a2))
    {
        minX = data.getCenter().x - data.getRadius();
    }
    if (Math2d::isAngleBetween(0., a1, a2))
    {
        maxX = data.getCenter().x + data.getRadius();
    }

    minV.set(minX, minY);
    maxV.set(maxX, maxY);
}

DmVector DmArc::getStartpoint() const
{
    double angle = 0.0;
    if (isClockwise())
    {
        angle = Math2d::correctAngle(M_PI - getStartAngle());
    }
    else
    {
        angle = data.getStartAngle();
    }
    return data.getCenter() + DmVector::polar(data.getRadius(), angle);
}

/// @return End point of the entity.
DmVector DmArc::getEndpoint() const
{
    double angle = 0.0;
    if (isClockwise())
    {
        angle = Math2d::correctAngle(M_PI - getEndAngle());
    }
    else
    {
        angle = data.getEndAngle();
    }
    return data.getCenter() + DmVector::polar(data.getRadius(), angle);
}

DmVectorSolutions DmArc::getRefPoints() const
{
    // order: start, end, center
    return { getStartpoint(), getEndpoint(), data.getCenter() };
}

double DmArc::getDirection1() const
{
    if (isClockwise())
    {
        double temp = Math2d::correctAngle(M_PI - data.getStartAngle());
        return Math2d::correctAngle(temp - M_PI_2);
    }
    else
    {
        return Math2d::correctAngle(data.getStartAngle() + M_PI_2);
    }
}

double DmArc::getDirection2() const
{
    if (isClockwise())
    {
        double temp = Math2d::correctAngle(M_PI - data.getEndAngle());
        return Math2d::correctAngle(temp + M_PI_2);
    }
    else
    {
        return Math2d::correctAngle(data.getEndAngle() - M_PI_2);
    }
}

bool DmArc::isClockwise() const
{
    return data.getNormal().z < 0.0;
}

void DmArc::setClockwise(const bool& clockwise)
{
    DmVector normal = data.getNormal();
    bool changed = false;
    if (clockwise)
    {
        if (!isClockwise())
        {
            data.setNormal(DmVector(0.0, 0.0, -1.0));
            changed = true;
        }
    }
    else
    {
        if (isClockwise())
        {
            data.setNormal(DmVector(0.0, 0.0, 1.0));
            changed = true;
        }
    }
    if (changed)
    {
        double oldStart = getStartAngle();
        double oldEnd = getEndAngle();
        double newStart = Math2d::correctAngle(M_PI - oldEnd);
        double newEnd = Math2d::correctAngle(M_PI - oldStart);
        data.setStartAngle(newStart);
        data.setEndAngle(newEnd);
    }
}

void DmArc::switchStartEndAngle()
{
    double oldStart = data.getStartAngle();
    double oldEnd = data.getEndAngle();
    data.setStartAngle(oldEnd);
    data.setEndAngle(oldStart);
}

DmVector DmArc::getNormal() const
{
    return data.getNormal();
}

void DmArc::setNormal(const DmVector& normal)
{
    data.setNormal(normal);
}

DmVector DmArc::getNearestEndpoint(const DmVector& coord, double* dist) const
{
    double dist1, dist2;

    auto const startpoint = getStartpoint();
    auto const endpoint = getEndpoint();

    dist1 = coord.squaredTo(startpoint);
    dist2 = coord.squaredTo(endpoint);

    if (dist2 < dist1)
    {
        if (dist)
        {
            *dist = sqrt(dist2);
        }

        return endpoint;
    }
    else
    {
        if (dist)
        {
            *dist = sqrt(dist1);
        }

        return startpoint;
    }
}

// find the tangential points from a given point, i.e., the tangent lines should pass the given point and tangential points
DmVectorSolutions DmArc::getTangentPoint(const DmVector& point) const
{
    DmVectorSolutions ret;
    double r2(getRadius() * getRadius());
    if (r2 < DM_TOLERANCE2)
    {
        return ret; // circle too small
    }
    DmVector vp(point - getCenter());
    double c2(vp.squared());
    if (c2 < r2 - getRadius() * 2. * DM_TOLERANCE)
    {
        //inside point, no tangential point
        return ret;
    }
    if (c2 > r2 + getRadius() * 2. * DM_TOLERANCE)
    {
        //external point
        DmVector vp1(-vp.y, vp.x);
        vp1 *= getRadius() * sqrt(c2 - r2) / c2;
        vp *= r2 / c2;
        vp += getCenter();
        if (vp1.squared() > DM_TOLERANCE2)
        {
            ret.push_back(vp + vp1);
            ret.push_back(vp - vp1);
            return ret;
        }
    }
    ret.push_back(point);
    return ret;
}

DmVector DmArc::getTangentDirection(const DmVector& point) const
{
    DmVector vp(point - getCenter());
    return DmVector(-vp.y, vp.x);

}

DmVector DmArc::getNearestPointOnEntity(const DmVector& coord, bool onEntity, double* dist, DmEntity** entity) const
{
    DmVector vec(false);
    if (entity)
    {
        *entity = const_cast<DmArc*>(this);
    }

    double angle = (coord - data.getCenter()).angle();
    if (!onEntity || Math2d::isAngleBetween(angle, getStartAngleNormal(), getEndAngleNormal()))
    {
        vec.setPolar(data.getRadius(), angle);
        vec += data.getCenter();
    }
    else
    {
        return vec = getNearestEndpoint(coord, dist);
    }
    if (dist)
    {
        *dist = vec.distanceTo(coord);
    }

    return vec;
}

DmVector DmArc::getNearestCenter(const DmVector& coord, double* dist) const
{
    if (dist)
    {
        *dist = coord.distanceTo(data.getCenter());
    }
    return data.getCenter();
}

/// @brief get the nearest equidistant middle points
/// @param coord coordinate
/// @param middlePoints number of equidistant middle points
DmVector DmArc::getNearestMiddle(const DmVector& coord, double* dist, int middlePoints) const
{
#ifndef EMU_C99
    using std::isnormal;
#endif

    double amin = getStartAngleNormal();
    double amax = getEndAngleNormal();
    if (!(isnormal(amin) || isnormal(amax)))
    {
        if (dist)
        {
            *dist = DM_MAXDOUBLE;
        }
        return DmVector(false);
    }
    double da = fmod(amax - amin + 2. * M_PI, 2. * M_PI);
    if (da < DM_TOLERANCE)
    {
        da = 2. * M_PI; // whole circle
    }
    DmVector vp(getNearestPointOnEntity(coord, true, dist));
    double angle = getCenter().angleTo(vp);
    int counts = middlePoints + 1;
    int i(static_cast<int>(fmod(angle - amin + 2. * M_PI, 2. * M_PI) / da * counts + 0.5));
    if (!i)
    {
        i++; // remove end points
    }
    if (i == counts)
    {
        i--;
    }
    angle = amin + da * (double(i) / double(counts));
    vp.setPolar(getRadius(), angle);
    vp.move(getCenter());

    if (dist)
    {
        *dist = vp.distanceTo(coord);
    }
    return vp;
}

DmVector DmArc::getNearestOrthTan(const DmVector& coord, const DmLine& normal, bool onEntity) const
{
    if (!coord.valid)
    {
        return DmVector(false);
    }
    double angle = normal.getStartAngle();
    DmVector vp = DmVector::polar(getRadius(), angle);
    std::vector<DmVector> sol;
    for (int i = 0; i <= 1; i++)
    {
        if (!onEntity || Math2d::isAngleBetween(angle, getStartAngleNormal(), getEndAngleNormal()))
        {
            if (i)
            {
                sol.push_back(-vp);
            }
            else
            {
                sol.push_back(vp);
            }
        }
        angle = Math2d::correctAngle(angle + M_PI);
    }
    switch (sol.size())
    {
    case 0:
        return DmVector(false);
    case 2:
        if (DmVector::dotP(sol[1], coord - getCenter()) > 0.)
        {
            vp = sol[1];
            break;
        }
        // fall-through
    default:
        vp = sol[0];
        break;
    }
    return getCenter() + vp;
}

void DmArc::moveStartpoint(const DmVector& pos)
{
    double bulge = getBulge();
    createFrom2PBulge(pos, getEndpoint(), bulge);
    setClockwise(!(bulge > 0.0));    //createFrom2PBulge会将法向设为正，设置回原始值
    correctAngles();
}

void DmArc::moveEndpoint(const DmVector& pos)
{
    double bulge = getBulge();
    createFrom2PBulge(getStartpoint(), pos, bulge);
    setClockwise(!(bulge > 0.0));    //createFrom2PBulge会将法向设为正，设置回原始值
    correctAngles();
}

/// @brief this function creates offset
/// @param coord, position indicates the direction of offset
/// @param distance, distance of offset
/// @return true, if success, otherwise, false
bool DmArc::offset(const DmVector& coord, const double& distance)
{
    double r0(coord.distanceTo(getCenter()));
    if (r0 > getRadius())
    {
        r0 = getRadius() + fabs(distance);
    }
    else
    {
        r0 = getRadius() - fabs(distance);
        if (r0 < DM_TOLERANCE)
        {
            return false;
        }
    }
    setRadius(r0);
    isModify = true;
    calculateBorders();
    return true;
}

std::vector<DmEntity*> DmArc::offsetTwoSides(const double& distance) const
{
    std::vector<DmEntity*> ret(0, nullptr);
    ret.push_back(new DmArc(nullptr, ArcData(getCenter(), getNormal(), getRadius() + distance, getStartAngle(), getEndAngle())));
    if (getRadius() > distance)
    {
        ret.push_back(new DmArc(nullptr, ArcData(getCenter(), getNormal(), getRadius() - distance, getStartAngle(), getEndAngle())));
    }
    return ret;
}

// make sure angleLength() is not more than 2*M_PI
void DmArc::correctAngles()
{
    data.setEndAngle(data.getStartAngle() + fmod(data.getEndAngle() - data.getStartAngle(), 2. * M_PI));
    if (fabs(getAngleLength()) < DM_TOLERANCE_ANGLE)
    {
        data.setEndAngle(data.getEndAngle() + 2. * M_PI);
    }
}

void DmArc::trimStartpoint(const DmVector& pos)
{
    data.setStartAngle(data.getCenter().angleTo(pos));
    correctAngles();
    isModify = true;
    calculateBorders();
}

void DmArc::trimEndpoint(const DmVector& pos)
{
    data.setEndAngle(data.getCenter().angleTo(pos));
    correctAngles();
    isModify = true;
    calculateBorders();
}

// trimCoord, mouse point
// trimPoint, trim to this intersection point
DM::Ending DmArc::getTrimPoint(const DmVector& trimCoord, const DmVector& /*trimPoint*/)
{
    double angMouse = data.getCenter().angleTo(trimCoord);
    if (fabs(remainder(angMouse - data.getStartAngle(), 2. * M_PI)) < fabs(remainder(angMouse - data.getEndAngle(), 2. * M_PI)))
    {
        return DM::EndingEnd;
    }
    else
    {
        return DM::EndingStart;
    }
}

void DmArc::move(const DmVector& offset)
{
    data.setCenter(data.getCenter().move(offset));
    isModify = true;
    moveBorders(offset);
}

void DmArc::rotate(const DmVector& center, const DmVector& angleVector)
{
    data.setCenter(data.getCenter().rotate(center, angleVector));
    double angle(angleVector.angle());
    data.setStartAngle(Math2d::correctAngle(data.getStartAngle() + angle));
    data.setEndAngle(Math2d::correctAngle(data.getEndAngle() + angle));
    isModify = true;
    calculateBorders();
}

void DmArc::scale(const DmVector& center, const DmVector& factor)
{
    // negative scaling: mirroring
    if (factor.x < 0.0)
    {
        mirror(data.getCenter(), data.getCenter() + DmVector(0.0, 1.0));
    }
    if (factor.y < 0.0)
    {
        mirror(data.getCenter(), data.getCenter() + DmVector(1.0, 0.0));
    }

    data.setCenter(data.getCenter().scale(center, factor));
    data.setRadius(data.getRadius() * factor.x);
    data.setRadius(fabs(data.getRadius()));
    calculateBorders();
}

void DmArc::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    data.setCenter(data.getCenter().mirror(axisPoint1, axisPoint2));
    double a = (axisPoint2 - axisPoint1).angle() * 2;
    double start = getStartAngle();
    double end = getEndAngle();
    // 逆时针
    if (!isClockwise())
    {
        double newStart = a - start;
        double newEnd = a - end;
        setStartAngle(Math2d::correctAngle(M_PI - newStart));
        setEndAngle(Math2d::correctAngle(M_PI - newEnd));
        data.setNormal(DmVector(0.0, 0.0, -1.0));
        setClockwise(!isClockwise());    //最终法向设置为正
    }
    // 顺时针
    else
    {
        start = Math2d::correctAngle(M_PI - start);    //转成相对于x向右的角度
        end = Math2d::correctAngle(M_PI - end);
        double newStart = a - start;
        double newEnd = a - end;
        setStartAngle(Math2d::correctAngle(newStart));
        setEndAngle(Math2d::correctAngle(newEnd));
        data.setNormal(DmVector(0.0, 0.0, 1.0));
    }
    isModify = true;
    correctAngles();
    calculateBorders();
}

void DmArc::moveRef(const DmVector& ref, const DmVector& offset)
{
    auto const refs = getRefPoints();
    double dMin;
    size_t index;
    DmVector const vp = refs.getClosest(ref, &dMin, &index);
    if (dMin >= 1.0e-4)
    {
        return;
    }

    //reference points must be by the order: start, end, center
    switch (index)
    {
    case 0:
        moveStartpoint(vp + offset);
        return;
    case 1:
        moveEndpoint(vp + offset);
        return;
    default:
        move(offset);
        break;
    }
    isModify = true;
    correctAngles(); // make sure angleLength is no more than 2*M_PI
}

std::vector<double> DmArc::calculateVertexs(const DmVector& center, const double radius)
{
    std::vector<double> vertexs;
    double start = 0.;
    double end = 0.;

    // 角度定义：x正轴绕z正轴转向y正轴所经过的角度
    start = data.getStartAngle();
    end = data.getEndAngle();
    if (start > end)
    {
        start -= 2 * DM_PI;
    }

    float delta = std::abs(end - start);
    float angle = 0;

    // z轴由里向外为正，x轴向右为正，y轴向上为正
    if (data.getNormal().z > 0)
    {
        for (int i = 0; i <= DM_CURVE_VERTEXS; i++)
        {
            angle = start + (((float)i) / DM_CURVE_VERTEXS) * (delta);
            vertexs.emplace_back(center.x + radius * cos(angle));
            vertexs.emplace_back(center.y + radius * sin(angle));
        }
    }
    // z轴由外向里为正，x轴向左为正，y轴向上为正
    else
    {
        for (int i = 0; i <= DM_CURVE_VERTEXS; i++)
        {
            angle = -start + M_PI - (((float)i) / DM_CURVE_VERTEXS) * (delta);
            vertexs.emplace_back(center.x + radius * cos(angle));
            vertexs.emplace_back(center.y + radius * sin(angle));
        }
    }
    return vertexs;
}

const std::vector<float>& DmArc::getVerticesRef(int& float_count_per_vertex)
{
    updateVertices();
    float_count_per_vertex = 5;
    return data.getVerticesRef();
}

void DmArc::getPoints(std::vector<DmVector>& pts, bool reverse /*= false*/)
{
    // 计算分段数
    double a = getEndAngle() - getStartAngle();
    a = Math2d::correctAngle(a);
    double delta = 6.0;
    double count = Math2d::rad2deg(a) / delta;    //一圆周60份
    count = std::round(count);
    count = std::max(count, 2.0);
    delta = a / count;

    // 计算点
    int iCount = (int)count;
    DmVector startPt = getStartpoint();
    DmVector center = getCenter();
    double r = getRadius();
    int startIdx = (int)pts.size();
    pts.emplace_back(startPt);
    double startAng = getStartAngle();
    for (int i = 1; i <= iCount; i++)
    {
        double ang = startAng + delta * i;
        ang = getAngleNormal(ang); // 翻正之后的角度
        DmVector vec(ang);
        DmVector p = center + vec * r;
        pts.emplace_back(p);
    }
    if (reverse)
    {
        std::reverse(pts.begin() + startIdx, pts.end());
    }
}

void DmArc::updateVertices()
{
    if (isModify)
    {
        float startAngle = (float)getStartAngleNormal();
        float endAngle = (float)getEndAngleNormal();
        float radius = (float)getRadius();
        float x0 = (float)data.getCenter().x;
        float y0 = (float)data.getCenter().y;

        std::vector<float> vertexs;
        constexpr float _2pi = M_PI * 2.0f;
        float delta_angle = endAngle - startAngle;
        if (delta_angle < 0.0f)
        {
            delta_angle += _2pi;
        }
        float factor = delta_angle / _2pi;
        int segment_count = (int)std::ceil(factor * CIRCLE_SEGMENT_COUNT);
        segment_count = std::max(2, segment_count);

        vertexs.reserve((segment_count + 3) * 5);
        float ang_delta = delta_angle / segment_count;
        float total_length = delta_angle * radius;
        float angle = startAngle;
        float oldAngle = angle;
        for (int i = 0; i < segment_count; i++)
        {
            if (i == 0)
            {
                // 针对GL_LINE_STRIP_ADJACENCY的起始坐标
                vertexs.emplace_back(x0 + radius * std::cos(angle + ang_delta));
                vertexs.emplace_back(y0 + radius * std::sin(angle + ang_delta));
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(total_length);

                // 起始点
                vertexs.emplace_back(x0 + radius * std::cos(angle));
                vertexs.emplace_back(y0 + radius * std::sin(angle));
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(total_length);
            }
            oldAngle = angle;
            angle += ang_delta;
            vertexs.emplace_back((x0 + radius * std::cos(angle)));
            vertexs.emplace_back((y0 + radius * std::sin(angle)));
            vertexs.emplace_back(0.0f);
            vertexs.emplace_back(radius * (angle - startAngle));
            vertexs.emplace_back(total_length);

            if (i == segment_count - 1)
            {
                // 针对GL_LINE_STRIP_ADJACENCY的终止坐标
                vertexs.emplace_back((x0 + radius * std::cos(angle - ang_delta)));
                vertexs.emplace_back((y0 + radius * std::sin(angle - ang_delta)));
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(total_length);
                vertexs.emplace_back(total_length);
            }
        }

        data.setVertices(vertexs);
        isModify = false;
    }
}

void DmArc::update()
{
    isModify = true;
    updateVertices();
}

/// @return Middle point of the entity.
DmVector DmArc::getMiddlePoint() const
{
    double a = getStartAngleNormal();
    double b = getEndAngleNormal();
    a += Math2d::correctAngle(b - a) * 0.5;
    DmVector ret(a);
    return getCenter() + ret * getRadius();
}

double DmArc::getAngleLength() const
{
    double ret;
    double a = getStartAngleNormal();
    double b = getEndAngleNormal();
    ret = Math2d::correctAngle(b - a);
    // full circle:
    if (fabs(remainder(ret, 2. * M_PI)) < DM_TOLERANCE_ANGLE)
    {
        ret = 2 * M_PI;
    }

    return ret;
}

/// @return Length of the arc.
double DmArc::getLength() const
{
    return getAngleLength() * data.getRadius();
}

/// Gets the arc's bulge (tangens of angle length divided by 4).
double DmArc::getBulge() const
{
    double bulge = tan(getAngleLength() / 4.0);
    if (isClockwise())
    {
        bulge *= -1.0;
    }
    return bulge;
}

// return the equation of the entity for quadratic,
// return a vector contains:
// m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0
// for linear:
Quadratic DmArc::getQuadratic() const
{
    std::vector<double> ce(6, 0.);
    ce[0] = 1.;
    ce[2] = 1.;
    ce[5] = -data.getRadius() * data.getRadius();
    Quadratic ret(ce);
    ret.move(data.getCenter());
    return ret;
}

std::list<DmEntity*> DmArc::getSubEntities() const
{
    return std::list<DmEntity*>();
}

void DmArc::saveStream(OutputStream& wrt) const
{
    DmAtomicEntity::saveStream(wrt);

    auto ce = getCenter();
    auto ra = getRadius();
    auto start = getStartAngle();
    auto end = getEndAngle();
    auto normal = getNormal();

    wrt << (double)ce.x << (double)ce.y << (double)ra << (double)start << (double)end << (double)normal.x << (double)normal.y << (double)normal.z;
}

void DmArc::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
    int fileRev = getRevisionId("DmArc", revs);
    if (revId > fileRev)
    {
        DmAtomicEntity::restoreStream(reader, revs);
        // 老文件格式
        restoreStreamWithRev(reader, fileRev);
    }
    else
    {
        restoreStream(reader);
    }
}

void DmArc::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (rev == 0)
    {
    }
    else //big change, e.g. change supper class of DmArc
    {
        //step1.
        // read all legacy data one by one
    }
}

void DmArc::restoreStream(InputStream& rdr)
{
    DmAtomicEntity::restoreStream(rdr);

    DmVector ce(true), normal(true);
    double ra = 0., start = 0., end = 0.;
    rdr >> (double&)ce.x >> (double&)ce.y >> (double&)ra >> (double&)start >> (double&)end >> (double&)normal.x >> (double&)normal.y >> (double&)normal.z;

    setCenter(ce);
    setRadius(ra);
    setStartAngle(start);
    setEndAngle(end);
    setNormal(normal);
    calculateBorders();
    isModify = true;
}
