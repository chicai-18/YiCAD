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


/// @file DmCircle.cpp
/// @brief 圆实体类实现

#include <cfloat>
#include <QPolygonF>
#include "DmCircle.h"

#include "DmArc.h"
#include "DmLine.h"
#include "Information.h"
#include "GuiDocumentView.h"
#include "Math2d.h"
#include "Quadratic.h"
#include "Debug.h"

#include "Writer.h"
#include "Reader.h"
#include "Stream.h"

TYPESYSTEM_SOURCE(DmCircle, DmAtomicEntity, 0)

DmCircle::DmCircle(DmEntity* parent, const CircleData& d)
    : DmAtomicEntity(parent)
    , data(d)
    , isModify(true)
{
    calculateBorders();
}

DmEntity* DmCircle::clone() const
{
    DmCircle* c = new DmCircle(*this);
    c->m_ulID = DmId();
    c->setSelected(false);
    c->setHighlighted(false);
    return c;
}

DM::EntityType DmCircle::getEntityType() const
{
    return DM::EntityCircle;
}

const CircleData& DmCircle::getData() const
{
    return data;
}

void DmCircle::calculateBorders()
{
    DmVector r(data.getRadius(), data.getRadius());
    minV = data.getCenter() - r;
    maxV = data.getCenter() + r;
}

std::list<DmEntity*> DmCircle::getSubEntities() const
{
    return std::list<DmEntity*>();
}

/// @return The center point of this circle
DmVector DmCircle::getCenter() const
{
    return data.getCenter();
}

// Sets new center.
void DmCircle::setCenter(const DmVector& c)
{
    data.setCenter(c);
}

/// @return The radius of this circle
double DmCircle::getRadius() const
{
    return data.getRadius();
}

// Sets new radius.
void DmCircle::setRadius(double r)
{
    data.setRadius(r);
}

/// @return Angle length in rad.
double DmCircle::getAngleLength() const
{
    return 2 * M_PI;
}

/// @return Length of the circle which is the circumference.
double DmCircle::getLength() const
{
    return 2 * M_PI * data.getRadius();
}

bool DmCircle::isTangent(const CircleData& circleData) const
{
    const double d = circleData.getCenter().distanceTo(data.getCenter());
    const double r0 = fabs(circleData.getRadius());
    const double r1 = fabs(data.getRadius());
    if (fabs(d - fabs(r0 - r1)) < 20. * DM_TOLERANCE || fabs(d - fabs(r0 + r1)) < 20. * DM_TOLERANCE)
    {
        return true;
    }
    return false;
}

/// @brief Creates this circle from two opposite points.
/// @param p1 1st point.
/// @param p2 2nd point.
bool DmCircle::createFrom2P(const DmVector& p1, const DmVector& p2)
{
    double r = 0.5 * p1.distanceTo(p2);
    if (r > DM_TOLERANCE)
    {
        data.setRadius(r);
        data.setCenter((p1 + p2) * 0.5);
        calculateBorders();
        return true;
    }
    else
    {
        return false;
    }
}

/// @brief Creates this circle from 3 given points which define the circle line.
/// @param p1 1st point.
/// @param p2 2nd point.
/// @param p3 3rd point.
bool DmCircle::createFrom3P(const DmVector& p1, const DmVector& p2, const DmVector& p3)
{
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
    data.setCenter(DmVector( (ra2 * vrb.y - rb2 * vra.y) * crossp, (rb2 * vra.x - ra2 * vrb.x) * crossp));
    data.setRadius(data.getCenter().magnitude());
    data.setCenter(getCenter() + p1);
    calculateBorders();
    return true;
}

std::vector<DmEntity* > DmCircle::offsetTwoSides(const double& distance) const
{
    std::vector<DmEntity*> ret(0, nullptr);
    ret.push_back(new DmCircle(nullptr, { getCenter(),getRadius() + distance }));
    if (fabs(getRadius() - distance) > DM_TOLERANCE)
    {
        ret.push_back(new DmCircle(nullptr, { getCenter(),fabs(getRadius() - distance) }));
    }
    return ret;
}

// create a circle of radius r and tangential to two given entities
DmVectorSolutions DmCircle::createTan2(const std::vector<DmAtomicEntity*>& circles, const double& r)
{
    if (circles.size() < 2)
    {
        return false;
    }

    auto e0 = circles[0]->offsetTwoSides(r);
    auto e1 = circles[1]->offsetTwoSides(r);
    DmVectorSolutions centers;
    if (e0.size() && e1.size())
    {
        for (auto it0 = e0.begin(); it0 != e0.end(); it0++)
        {
            for (auto it1 = e1.begin(); it1 != e1.end(); it1++)
            {
                centers.push_back(Information::getIntersection(*it0, *it1));
            }
        }
    }
    for (auto it0 = e0.begin(); it0 != e0.end(); it0++)
    {
        delete* it0;
    }
    for (auto it0 = e1.begin(); it0 != e1.end(); it0++)
    {
        delete* it0;
    }
    return centers;
}

std::vector<DmCircle> DmCircle::createTan3(const std::vector<DmAtomicEntity*>& circles)
{
    std::vector<DmCircle> ret;
    if (circles.size() != 3)
    {
        return ret;
    }

    std::vector<DmCircle> cs;
    for (auto c : circles)
    {
        cs.emplace_back(DmCircle(nullptr, { c->getCenter(),c->getRadius() }));
    }
    unsigned short flags = 0;
    do {
        for (unsigned short j = 0u; j < 3u; ++j)
        {
            if (flags & (1u << j))
            {
                cs[j].setRadius(-fabs(cs[j].getRadius()));
            }
            else
            {
                cs[j].setRadius(fabs(cs[j].getRadius()));
            }
        }
        auto list = solveAppolloniusSingle(cs);
        if (list.size() >= 1)
        {
            for (DmCircle& c0 : list)
            {
                bool addNew = true;
                for (DmCircle& c : ret)
                {
                    if ((c0.getCenter() - c.getCenter()).squared() < DM_TOLERANCE15 && fabs(c0.getRadius() - c.getRadius()) < DM_TOLERANCE)
                    {
                        addNew = false;
                        break;
                    }
                }
                if (addNew)
                {
                    ret.push_back(c0);
                }
            }
        }
    } while (++flags < 8u);

    for (size_t i = 0; i < ret.size();)
    {
        if (ret[i].testTan3(circles) == false)
        {
            ret.erase(ret.begin() + i);
        }
        else
        {
            ++i;
        }
    }
    for (auto e : ret)
    {
        e.calculateBorders();
    }
    return ret;
}

bool DmCircle::testTan3(const std::vector<DmAtomicEntity*>& circles)
{
    if (circles.size() != 3)
    {
        return false;
    }
    for (auto const& c : circles)
    {
        const double r0 = fabs(data.getRadius());
        const double r1 = fabs(c->getRadius());

        const double dist = fabs((data.getCenter() - c->getCenter()).magnitude());

        double const rmax = std::max(r0, r1);
        if (dist < rmax)
        {
            return fabs(dist - fabs(r0 - r1)) <= sqrt(DBL_EPSILON) * rmax;
        }
        else
        {
            return fabs(dist - fabs(r0 + r1)) <= sqrt(DBL_EPSILON) * rmax;
        }
    }
    return true;
}

// solve one of the eight Appollonius Equations
// | Cx - Ci|^2=(Rx+Ri)^2
// with Cx the center of the common tangent circle, Rx the radius. Ci and Ri are the Center and radius of the i-th existing circle
std::vector<DmCircle> DmCircle::solveAppolloniusSingle(const std::vector<DmCircle>& circles)
{
    std::vector<DmCircle> ret;

    std::vector<DmVector> centers;
    std::vector<double> radii;

    for (auto c : circles)
    {
        if (c.getCenter().valid == false)
        {
            return ret;
        }
        centers.push_back(c.getCenter());
        radii.push_back(c.getRadius());
    }

    std::vector<std::vector<double> > mat(2, std::vector<double>(3, 0.));
    mat[0][0] = centers[2].x - centers[0].x;
    mat[0][1] = centers[2].y - centers[0].y;
    mat[1][0] = centers[2].x - centers[1].x;
    mat[1][1] = centers[2].y - centers[1].y;
    if (fabs(mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0]) < DM_TOLERANCE2)
    {
        size_t i0 = 0;
        if (centers[0].distanceTo(centers[1]) <= DM_TOLERANCE || centers[0].distanceTo(centers[2]) <= DM_TOLERANCE)
        {
            i0 = 1;
        }
        Quadratic lc0(&(circles[i0]), &(circles[(i0 + 1) % 3]));
        Quadratic lc1(&(circles[i0]), &(circles[(i0 + 2) % 3]));
        auto c0 = Quadratic::getIntersection(lc0, lc1);
        for (size_t i = 0; i < c0.size(); i++)
        {
            const double dc = c0[i].distanceTo(centers[i0]);
            ret.push_back(DmCircle(nullptr, { c0[i], fabs(dc - radii[i0]) }));
            if (dc > radii[i0])
            {
                ret.push_back(DmCircle(nullptr, { c0[i], dc + radii[i0] }));
            }
        }
        return ret;
    }
    // r^0 term
    mat[0][2] = 0.5 * (centers[2].squared() - centers[0].squared() + radii[0] * radii[0] - radii[2] * radii[2]);
    mat[1][2] = 0.5 * (centers[2].squared() - centers[1].squared() + radii[1] * radii[1] - radii[2] * radii[2]);
    std::vector<double> sm(2, 0.);
    if (Math2d::linearSolver(mat, sm) == false)
    {
        return ret;
    }

    DmVector vp(sm[0], sm[1]);

    // r term
    mat[0][2] = radii[0] - radii[2];
    mat[1][2] = radii[1] - radii[2];
    if (Math2d::linearSolver(mat, sm) == false)
    {
        return ret;
    }
    DmVector vq(sm[0], sm[1]);

    // form quadratic equation for r
    DmVector dcp = vp - centers[0];
    double a = vq.squared() - 1.;
    if (fabs(a) < DM_TOLERANCE * 1e-4)
    {
        return ret;
    }
    std::vector<double> ce(0, 0.);
    ce.push_back(2. * (dcp.dotP(vq) - radii[0]) / a);
    ce.push_back((dcp.squared() - radii[0] * radii[0]) / a);
    std::vector<double>&& vr = Math2d::quadraticSolver(ce);
    for (size_t i = 0; i < vr.size(); i++)
    {
        if (vr.at(i) < DM_TOLERANCE)
        {
            continue;
        }
        ret.emplace_back(DmCircle(nullptr, { vp + vq * vr.at(i),fabs(vr.at(i)) }));
    }

    return ret;
}

DmVectorSolutions DmCircle::getRefPoints() const
{
    DmVector v1(data.getRadius(), 0.0);
    DmVector v2(0.0, data.getRadius());

    return DmVectorSolutions({data.getCenter(),data.getCenter() + v1, data.getCenter() + v2,data.getCenter() - v1, data.getCenter() - v2});
}

/// @brief compute nearest endpoint, intersection with X/Y axis at 0, 90, 180 and 270 degree
///        Use getNearestMiddle() method to compute the nearest circle quadrant endpoints
/// @param coord coordinates to compute, e.g. mouse cursor position
/// @param dist double pointer to return distance between mouse pointer and nearest entity point
/// @return the nearest intersection of the circle with X/Y axis.
DmVector DmCircle::getNearestEndpoint(const DmVector& coord, double* dist /*= nullptr*/) const
{
    return getNearestMiddle(coord, dist, 0);
}

DmVector DmCircle::getNearestPointOnEntity(const DmVector& coord, bool /*onEntity*/, double* dist, DmEntity** entity)const
{
    if (entity)
    {
        *entity = const_cast<DmCircle*>(this);
    }

    DmVector vp(coord - data.getCenter());
    double d(vp.magnitude());
    if (d < DM_TOLERANCE)
    {
        return DmVector(false);
    }
    vp = data.getCenter() + vp * (data.getRadius() / d);

    if (dist)
    {
        *dist = coord.distanceTo(vp);
    }
    return vp;
}

// find the tangential points from a given point, i.e., the tangent lines should pass the given point and tangential points
DmVectorSolutions DmCircle::getTangentPoint(const DmVector& point) const
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

DmVector DmCircle::getTangentDirection(const DmVector& point) const
{
    DmVector vp(point - getCenter());
    return DmVector(-vp.y, vp.x);
}

DmVector DmCircle::getNearestCenter(const DmVector& coord, double* dist) const
{
    if (dist)
    {
        *dist = coord.distanceTo(data.getCenter());
    }
    return data.getCenter();
}

DmVector DmCircle::getMiddlePoint(void)const
{
    return DmVector(false);
}

/// @brief compute middlePoints for each quadrant of a circle
///        0 middlePoints snaps to axis intersection at 0, 90, 180 and 270 degree (getNearestEndpoint)
///        1 middlePoints snaps to 45, 135, 225 and 315 degree
///        2 middlePoints snaps to 30, 60, 120, 150, 210, 240, 300 and 330 degree and so on
/// @param coord coordinates to compute, e.g. mouse cursor position
/// @param dist double pointer to return distance between mouse pointer and nearest entity point
/// @param middlePoints number of middle points to compute per quadrant (0 for endpoints)
/// @return the nearest of equidistant middle points of the circles quadrants.
DmVector DmCircle::getNearestMiddle(const DmVector& coord, double* dist /*= nullptr*/, const int middlePoints /*= 1*/) const
{
    if (data.getRadius() <= DM_TOLERANCE)
    {
        // circle too short
        if (nullptr != dist)
        {
            *dist = DM_MAXDOUBLE;
        }
        return DmVector(false);
    }

    DmVector vPoint(getNearestPointOnEntity(coord, true, dist));
    int iCounts = middlePoints + 1;
    double dAngleSteps = M_PI_2 / iCounts;
    double dAngleToPoint = data.getCenter().angleTo(vPoint);
    int iStepCount = static_cast<int>((dAngleToPoint + 0.5 * dAngleSteps) / dAngleSteps);
    if (0 < middlePoints)
    {
        // for nearest middle eliminate start/endpoints
        int iQuadrant = static_cast<int>(dAngleToPoint / 0.5 / M_PI);
        int iQuadrantStep = iStepCount - iQuadrant * iCounts;
        if (0 == iQuadrantStep)
        {
            ++iStepCount;
        }
        else if (iCounts == iQuadrantStep)
        {
            --iStepCount;
        }
    }

    vPoint.setPolar(data.getRadius(), dAngleSteps * iStepCount);
    vPoint.move(data.getCenter());

    if (dist)
    {
        *dist = vPoint.distanceTo(coord);
    }

    return vPoint;
}

DmVector DmCircle::getNearestOrthTan(const DmVector& coord, const DmLine& normal, bool /*onEntity = false*/) const
{
    if (!coord.valid)
    {
        return DmVector(false);
    }
    DmVector vp0(coord - getCenter());
    DmVector vp1(normal.getStartAngle());
    double d = DmVector::dotP(vp0, vp1);
    if (d >= 0.)
    {
        return getCenter() + vp1 * getRadius();
    }
    else
    {
        return getCenter() - vp1 * getRadius();
    }
}

void DmCircle::move(const DmVector& offset)
{
    data.setCenter(getCenter().move(offset));
    isModify = true;
    moveBorders(offset);
}

/// @brief this function creates offset
/// @param coord, position indicates the direction of offset
/// @param distance, distance of offset
/// @return true, if success, otherwise, false
bool DmCircle::offset(const DmVector& coord, const double& distance)
{
    double r0(coord.distanceTo(getCenter()));
    if (r0 > getRadius())
    {
        // external
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

void DmCircle::rotate(const DmVector& center, const DmVector& angleVector)
{
    data.setCenter(getCenter().rotate(center, angleVector));
    isModify = true;
    calculateBorders();
}

void DmCircle::scale(const DmVector& center, const DmVector& factor)
{
    data.setCenter(getCenter().scale(center, factor));
    // radius always is positive
    data.setRadius(getRadius() * fabs(factor.x));
    isModify = true;
    scaleBorders(center, factor);
}

double DmCircle::getDirection1() const
{
    return M_PI_2;
}

/// @return Direction 2. The angle at which the arc starts at the endpoint.
double DmCircle::getDirection2() const
{
    return M_PI_2 * 3.0;
}

void DmCircle::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    data.setCenter(getCenter().mirror(axisPoint1, axisPoint2));
    isModify = true;
    calculateBorders();
}

/// @brief 计算获得圆的离散化点
/// @param center 圆心坐标
/// @param radius 半径
/// @return 离散化顶点坐标
std::vector<double> DmCircle::calculateVertexs(const DmVector& center, const double radius)
{
    std::vector<double> vertexs;
    auto curve_points = DM_CURVE_VERTEXS;
    float angle = 0;
    for (int i = 0; i <= curve_points; i++)
    {
        angle = (((float)i) / curve_points) * (2 * DM_PI);
        vertexs.emplace_back((center.x + radius * cos(angle)));
        vertexs.emplace_back((center.y + radius * sin(angle)));
    }
    return vertexs;
}

const std::vector<float>& DmCircle::getVerticesRef(int& float_count_per_vertex)
{
    updateVertices();
    float_count_per_vertex = 5;
    return data.getVerticesRef();
}

void DmCircle::updateVertices()
{
    if (isModify)
    {
        float radius = (float)getRadius();
        float x0 = (float)getCenter().x;
        float y0 = (float)getCenter().y;

        std::vector<float> vertexs;
        vertexs.reserve((CIRCLE_SEGMENT_COUNT + 3) * 5);
        constexpr float _2pi = M_PI * 2.0f;
        constexpr float ang_delta = _2pi / CIRCLE_SEGMENT_COUNT;
        float total_length = _2pi * radius;
        float angle = 0.0f;
        float oldAngle = angle;
        for (int i = 0; i < CIRCLE_SEGMENT_COUNT; i++)
        {
            if (i == 0)
            {
                // 针对GL_LINE_STRIP_ADJACENCY的起始坐标
                vertexs.emplace_back((x0 + radius * std::cos(-ang_delta)));
                vertexs.emplace_back((y0 + radius * std::sin(-ang_delta)));
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(total_length);

                // 起始点
                vertexs.emplace_back(x0 + radius);
                vertexs.emplace_back(y0);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(total_length);
            }
            oldAngle = angle;
            angle += ang_delta;
            vertexs.emplace_back((x0 + radius * std::cos(angle)));
            vertexs.emplace_back((y0 + radius * std::sin(angle)));
            vertexs.emplace_back(0.0f);
            vertexs.emplace_back(radius * angle);
            vertexs.emplace_back(total_length);

            if (i == CIRCLE_SEGMENT_COUNT - 1)
            {
                // 针对GL_LINE_STRIP_ADJACENCY的终止坐标
                vertexs.emplace_back((x0 + radius * std::cos(ang_delta)));
                vertexs.emplace_back((y0 + radius * std::sin(ang_delta)));
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(total_length);
                vertexs.emplace_back(total_length);
            }
        }

        data.setVertices(vertexs);
        isModify = false;
    }
}

void DmCircle::update()
{
    isModify = true;
    updateVertices();
}

void DmCircle::moveRef(const DmVector& ref, const DmVector& offset)
{
    constexpr double kRefThreshold = 1.0e-4;
    if (ref.distanceTo(data.getCenter()) < kRefThreshold)
    {
        data.setCenter(getCenter() + offset);
        moveBorders(offset);
        isModify = true;
        return;
    }
    DmVector v1(data.getRadius(), 0.0);
    DmVectorSolutions sol;
    sol.push_back(data.getCenter() + v1);
    sol.push_back(data.getCenter() - v1);
    v1.set(0., data.getRadius());
    sol.push_back(data.getCenter() + v1);
    sol.push_back(data.getCenter() - v1);
    double dist = 0.0;
    v1 = sol.getClosest(ref, &dist);
    if (dist > kRefThreshold)
    {
        return;
    }
    data.setRadius(data.getCenter().distanceTo(v1 + offset));
    calculateBorders();
    isModify = true;
}

/// @brief return the equation of the entity for quadratic
/// return a vector contains:
/// m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0
/// for linear:
/// m0 x + m1 y + m2 =0
Quadratic DmCircle::getQuadratic() const
{
    std::vector<double> ce(6, 0.);
    ce[0] = 1.;
    ce[2] = 1.;
    ce[5] = -data.getRadius() * data.getRadius();
    Quadratic ret(ce);
    ret.move(data.getCenter());
    return ret;
}

void DmCircle::saveStream(OutputStream& wrt) const
{
    DmAtomicEntity::saveStream(wrt);

    auto ce = getCenter();
    auto ra = getRadius();

    wrt << (double)ce.x << (double)ce.y << (double)ra;
}

void DmCircle::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
    int fileRev = getRevisionId("DmCircle", revs);
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

void DmCircle::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (rev == 0)
    {
        // legacy rev 0 format
    }
    else //big change, e.g. change supper class of DmCircle
    {
        //step1.
        // read all legacy data one by one
    }
}

void DmCircle::restoreStream(InputStream& rdr)
{
    DmAtomicEntity::restoreStream(rdr);

    DmVector ce(true);
    double ra = 0.;
    rdr >> (double&)ce.x >> (double&)ce.y >> (double&)ra;

    setCenter(ce);
    setRadius(ra);
    calculateBorders();
    isModify = true;
}
