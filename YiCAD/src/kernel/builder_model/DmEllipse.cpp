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


/// @file DmEllipse.cpp
/// @brief 椭圆/椭圆弧实体类实现

#include "DmEllipse.h"

#include "DmCircle.h"
#include "DmLine.h"
#include "DmDocument.h"
#include "GuiDocumentView.h"
#include "Information.h"
#include "Math2d.h"
#include <cmath>
#include  "Quadratic.h"
#include "QuarticEquation.h"
#include "Debug.h"

#ifndef Q_MOC_RUN
#include <boost/version.hpp>
#include <boost/math/tools/roots.hpp>
#include <boost/math/special_functions/ellint_2.hpp>
#if BOOST_VERSION > 104500
#include <boost/math/tools/tuple.hpp>
#endif
#endif

#ifndef EMU_C99
using std::isnormal;
#endif

#include "Writer.h"
#include "Reader.h"
#include "Stream.h"

TYPESYSTEM_SOURCE(DmEllipse, DmAtomicEntity, 0)

//functor to solve for distance, used by snapDistance
class EllipseDistanceFunctor
{
public:
    EllipseDistanceFunctor(DmEllipse const* ellipse, double const& target)
        : distance(target)
        , e(ellipse)
        , ra(e->getMajorRadius())
        , k2(1. - e->getRatio() * e->getRatio())
        , k2ra(k2* ra)
    {
    }

    void setDistance(const double& target)
    {
        distance = target;
    }

#if BOOST_VERSION > 104500
    boost::math::tuple<double, double, double> operator()(double const& z) const
    {
#else
    boost::fusion::tuple<double, double, double> operator()(double const& z) const
    {
#endif
        double const cz = cos(z);
        double const sz = sin(z);
        //delta amplitude
        double const d = sqrt(1 - k2 * sz * sz);
        // return f(x), f'(x) and f''(x)
#if BOOST_VERSION > 104500
        return boost::math::make_tuple(
#else
        return boost::fusion::make_tuple(
#endif
            e->getEllipseLength(z) - distance, ra * d, k2ra * sz * cz / d);
    }

private:
    double distance;
    DmEllipse const* const e;
    const double ra;
    const double k2;
    const double k2ra;
};

/// @brief getNearestDistHelper find end point after trimmed by amount
/// @param e ellipse which is not reversed, assume ratio (a/b) >= 1
/// @param trimAmount the length of the trimmed is increased by this amount
/// @param coord current mouse position
/// @param dist if this pointer is not nullptr, save the distance from the new end point to mouse position coord
/// @return the new end point of the trimmed. Only one end of the entity is trimmed
DmVector getNearestDistHelper(DmEllipse const& e, double trimAmount, DmVector const& coord, double* dist = nullptr)
{
    double const x1 = e.getStartParam();

    double const guess = x1 + M_PI;
    int const digits = std::numeric_limits<double>::digits;

    double const wholeLength = e.getEllipseLength(0, 0); // start/end angle 0 is used for whole ellipses

    double trimmed = e.getLength() + trimAmount;

    // choose the end to trim by the mouse position coord
    bool const trimEnd = coord.squaredTo(e.getStartpoint()) <= coord.squaredTo(e.getEndpoint());

    if (trimEnd)
    {
        trimmed = trimAmount > 0 ? wholeLength - trimAmount : -trimAmount;
    }

    // solve equation of the distance by second order newton_raphson
    EllipseDistanceFunctor X(&e, trimmed);
    using namespace boost::math::tools;
    double const sol = halley_iterate<EllipseDistanceFunctor, double>(X, guess, x1, x1 + 2 * M_PI - DM_TOLERANCE_ANGLE, digits);

    DmVector const vp = e.getEllipsePoint(sol);
    if (dist)
    {
        *dist = vp.distanceTo(coord);
    }
    return vp;
}

DmEllipse::DmEllipse(DmEntity* parent, const EllipseData& d)
    : DmAtomicEntity(parent)
    , data(d)
    , isModify(true)
{
    calculateBorders();
}

DmEntity* DmEllipse::clone() const
{
    DmEllipse* e = new DmEllipse(*this);
    e->m_ulID = DmId();
    e->setSelected(false);
    e->setHighlighted(false);
    return e;
}

DM::EntityType DmEllipse::getEntityType() const
{
    return DM::EntityEllipse;
}

// Calculates the boundary box of this ellipse.
void DmEllipse::calculateBorders()
{
    DmVector startpoint = getStartpoint();
    DmVector endpoint = getEndpoint();

    double minX = std::min(startpoint.x, endpoint.x);   //对于椭圆，坐标为0
    double minY = std::min(startpoint.y, endpoint.y);
    double maxX = std::max(startpoint.x, endpoint.x);
    double maxY = std::max(startpoint.y, endpoint.y);

    // 椭圆的包围框计算，参考： https://stackoverflow.com/questions/87734/how-do-you-calculate-the-axis-aligned-bounding-box-of-an-ellipse 第一个回答
    // 计算椭圆的X,Y最大最小值
    DmVector majorDir = getMajorP().normalize();
    double cos_phi = majorDir.x;
    double sin_phi = majorDir.y;
    double a = getMajorP().magnitude();
    double b = getRatio() * a;
    double t_maxX = atan2(-sin_phi * getRatio(), cos_phi);       //x取最大值时的参数，对应于公式[3]的t
    t_maxX = Math2d::correctAngle(t_maxX);
    double t_minX = Math2d::correctAngle(t_maxX + M_PI);
    double dx = a * cos(t_maxX) * cos_phi - b * sin(t_maxX) * sin_phi;       //x最大值相对于中心的差值，对应于公式[1]的部分

    double t_maxY = atan2(cos_phi * getRatio(), sin_phi);        //y取最大值时的参数，对应于公式[4]的t
    t_maxY = Math2d::correctAngle(t_maxY);
    double t_minY = Math2d::correctAngle(t_maxY + M_PI);
    double dy = b * sin(t_maxY) * cos_phi + a * cos(t_maxY) * sin_phi;       //y最大值相对于中心的差值，对应于公式[2]的部分

    // 椭圆
    if (isClosed())
    {
        maxX = data.getCenter().x + abs(dx);
        minX = data.getCenter().x - abs(dx);
        maxY = data.getCenter().y + abs(dy);
        minY = data.getCenter().y - abs(dy);
    }
    // 椭圆弧
    else
    {
        double startParamNormal = getStartParamNormal();
        double endParamNormal = getEndParamNormal();
        if (Math2d::isAngleBetween(t_maxX, startParamNormal, endParamNormal))
        {
            maxX = data.getCenter().x + abs(dx);
        }
        if (Math2d::isAngleBetween(t_minX, startParamNormal, endParamNormal))
        {
            minX = data.getCenter().x - abs(dx);
        }
        if (Math2d::isAngleBetween(t_maxY, startParamNormal, endParamNormal))
        {
            maxY = data.getCenter().y + abs(dy);
        }
        if (Math2d::isAngleBetween(t_minY, startParamNormal, endParamNormal))
        {
            minY = data.getCenter().y - abs(dy);
        }
    }
    this->minV = DmVector(minX, minY);
    this->maxV = DmVector(maxX, maxY);
}

std::list<DmEntity*> DmEllipse::getSubEntities() const
{
    return std::list<DmEntity*>();
}

DmVectorSolutions DmEllipse::getFoci() const
{
    DmEllipse e = *this;
    if (getRatio() > 1.)
    {
        e.switchMajorMinor();
    }
    DmVector vp(e.getMajorP() * sqrt(1. - e.getRatio() * e.getRatio()));  //长轴方向（MajorP），c长度的矢量
    return DmVectorSolutions({ getCenter() + vp, getCenter() - vp });
}

DmVectorSolutions DmEllipse::getRefPoints() const
{
    DmVectorSolutions ret;
    if (!isClosed())
    {
        //no start/end point for whole ellipse
        ret.push_back(getStartpoint());
        ret.push_back(getEndpoint());
    }
    ret.push_back(data.getCenter());
    //ret.push_back(getFoci()); //不显示焦点
    ret.push_back(getMajorPoint());
    ret.push_back(getMinorPoint());
    return ret;
}

DmVector DmEllipse::getNormal() const
{
    return data.getNormal();
}

void DmEllipse::setNormal(const DmVector& normal)
{
    data.setNormal(normal);
}

bool DmEllipse::isClockwise() const
{
    return getNormal().z < 0.0;
}

void DmEllipse::setClockwise(const bool& clockwise)
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
        // todo :待验证
        double oldStart = getStartParam();
        double oldEnd = getEndParam();
        double newStart = Math2d::correctAngle(M_PI * 2.0 - oldEnd);
        double newEnd = Math2d::correctAngle(M_PI * 2.0 - oldStart);
        data.setStartParam(newStart);
        data.setEndParam(newEnd);
    }
}

void DmEllipse::switchStartEndParam()
{
    double oldStart = data.getStartParam();
    double oldEnd = data.getEndParam();
    data.setStartParam(oldEnd);
    data.setEndParam(oldStart);
}

DmVector DmEllipse::getNearestEndpoint(const DmVector& coord, double* dist)const
{
    double dist1 = 0.0, dist2 = 0.0;
    DmVector startpoint = getStartpoint();
    DmVector endpoint = getEndpoint();

    dist1 = (startpoint - coord).squared();
    dist2 = (endpoint - coord).squared();

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
DmVectorSolutions DmEllipse::getTangentPoint(const DmVector& point) const
{
    DmVector point2(point);
    point2.move(-getCenter());
    DmVector aV(-getAngle());
    point2.rotate(aV);
    DmVectorSolutions sol;
    double a = getMajorRadius();
    if (a < DM_TOLERANCE || getRatio() < DM_TOLERANCE)
    {
        return sol;
    }
    DmCircle c(nullptr, CircleData(DmVector(0., 0.), a));
    point2.y /= getRatio();
    sol = c.getTangentPoint(point2);
    sol.scale(DmVector(1., getRatio()));
    aV.y *= -1.;
    sol.rotate(aV);
    sol.move(getCenter());
    return sol;
}

DmVector DmEllipse::getTangentDirection(const DmVector& point) const
{
    DmVector vp(point - getCenter());
    DmVector aV(-getAngle());
    vp.rotate(aV);
    vp.y /= getRatio();
    double a = getMajorRadius();
    if (a < DM_TOLERANCE || getRatio() < DM_TOLERANCE)
    {
        return DmVector(false);
    }
    DmCircle c(nullptr, CircleData(DmVector(0., 0.), a));
    DmVector direction = c.getTangentDirection(vp);
    direction.y *= getRatio();
    aV.y *= -1.;
    direction.rotate(aV);
    return direction;
}

// find total length of the ellipse (arc)
double DmEllipse::getLength() const
{
    DmEllipse e(nullptr, data);
    if (e.getRatio() > 1.)
    {
        e.switchMajorMinor();
    }
    if (e.isClockwise())
    {
        double start = e.getStartParamNormal();
        double end = e.getEndParamNormal();
        e.data.setStartParam(end);
        e.data.setEndParam(start);
    }
    return e.getEllipseLength(e.data.getStartParam(), e.data.getEndParam());
}

double DmEllipse::getEllipseLength(double x1, double x2) const
{
    double a(getMajorRadius()), k(getRatio());
    k = 1 - k * k;//elliptic modulus, or eccentricity
    x1 = Math2d::correctAngle(x1);
    x2 = Math2d::correctAngle(x2);
    if (x2 < x1 + DM_TOLERANCE_ANGLE)
    {
        x2 += 2. * M_PI;
    }
    double ret = 0.;
    if (x2 >= M_PI)
    {
        ret = (static_cast<int>((x2 + DM_TOLERANCE_ANGLE) / M_PI) - (static_cast<int>((x1 + DM_TOLERANCE_ANGLE) / M_PI))) * 2;
        ret *= boost::math::ellint_2<double>(k);
    }
    x1 = fmod(x1, M_PI);
    x2 = fmod(x2, M_PI);
    if (fabs(x2 - x1) > DM_TOLERANCE_ANGLE)
    {
        ret += Math2d::ellipticIntegral_2(k, x2) - Math2d::ellipticIntegral_2(k, x1);
    }
    return a * ret;
}

// arc length from start point (startAngle)
double DmEllipse::getEllipseLength(double x2) const
{
    return getEllipseLength(getStartParam(), x2);
}

bool DmEllipse::switchMajorMinor(void)
{
    if (fabs(data.getRatio()) < DM_TOLERANCE)
    {
        return false;
    }
    DmVector vp_start = getStartpoint();
    DmVector vp_end = getEndpoint();
    DmVector vp = getMajorP();
    setMajorP(DmVector(-data.getRatio() * vp.y, data.getRatio() * vp.x)); // direction pi/2 relative to old MajorP;
    setRatio(1. / data.getRatio());
    if (!isClosed())
    {
        // only reset start/end points for ellipse arcs, i.e., startAngle endAngle are not both zero
        setStartParam(getEllipseParam(vp_start));
        setEndParam(getEllipseParam(vp_end));
    }
    return true;
}

DmVector  DmEllipse::getStartpoint() const
{
    if (!isClosed())
    {
        return getEllipsePoint(data.getStartParam());
    }
    return DmVector(false);
}

DmVector  DmEllipse::getEndpoint() const
{
    if (!isClosed())
    {
        return getEllipsePoint(data.getEndParam());
    }
    return DmVector(false);
}

DmVector  DmEllipse::getEllipsePoint(const double& a) const
{
    double ra = getMajorRadius();
    double x = 0.0;
    double y = 0.0;
    // 逆时针
    if (!isClockwise())
    {
        x = ra * std::cos(a);
        y = ra * getRatio() * std::sin(a);
    }
    // 顺时针
    else
    {
        x = ra * std::cos(-a);
        y = ra * getRatio() * std::sin(-a);
    }
    DmVector point(x, y);
    point.rotate(getAngle());
    point.move(getCenter());
    return point;
}

DmVector DmEllipse::getNearestPointOnEntity(const DmVector& coord, bool onEntity, double* dist, DmEntity** entity)const
{
    DmVector ret(false);

    if (!coord.valid)
    {
        if (dist)
        {
            *dist = DM_MAXDOUBLE;
        }
        return ret;
    }

    if (entity)
    {
        *entity = const_cast<DmEllipse*>(this);
    }

    // 最近点不能像下面这样求，这样求的是中心到coord延长线与椭圆弧的交点
    //double para = getEllipseParam(coord);
    //ret = getEllipsePoint(para);

    // 步骤：
    // （1）将椭圆旋转为长轴与X轴对齐，中心移动到(0,0)点；同步将coord旋转移动。
    // （2）根据"切点和coord的连线与最近点的切线垂直"，求解方程，具体如下：
    //      椭圆参数方程为：x=acos(theta), y=bsin(theta); 切线方程为：x'=-asin(theta), y=bcos(theta)；
    //      设coord为P(x0,y0)，待求椭圆上一点为Pa(xa, ya)。 向量PPa(xa-x0, ya-y0),它与切向量点积为：
    //      dot = x' * (xa - x0) + y' * (ya - y0) = 0;
    //      => -asin(theta) * (acos(theta) - x0) + bsin(theta) * (bsin(theta) - y0) = 0
    //      => (b^2-a^2)*sin(theta)cos(theta) + a*x0*sin(theta) - b*y0*cos(theta) = 0
    //      => (b^2-a^2)*sin(theta) + a*x0*tan(theta) - b*y0 = 0
    //      采用三角万能公式
    //      => (b^2-a^2) * ( 2tan(theta/2)/(1+tan(theta/2)^2)) + a*x0*2tan(theta/2)/(1-tan(theta/2)^2) - b*y0 = 0
    //      设A=(b^2-a^2)， B=a*x0, C=-b*y0, m=tan(theta/2)
    //      => -C*m^4 + 2(B-A)m^3 + 2(B-A)m + C = 0
    //      令a' = -C, b'= 2(B-A), d' = 2(B+A), e'=C
    //      => a'*m^4 + b'*m^3 + d'*m + e' = 0
    //      求解出m，反求theta，从而得到xa,ya

    DmVector coord_copy = coord;
    coord_copy.move(-getCenter());
    coord_copy.rotate(-getMajorP().angle());
    double x0 = coord_copy.x;
    double y0 = coord_copy.y;
    double a = getMajorRadius();
    double b = a * getRatio();
    double A = b * b - a * a;
    double B = a * x0;
    double C = -b * y0;
    double a_ = -C; //a'
    double b_ = 2.0 * (B - A); //b'
    double d_ = 2.0 * (B + A); //d'
    double e_ = C;//e'
    QuarticEquation equation(a_, b_, 0.0, d_, e_);
    std::vector<double> res = equation.getResults();
    double startParam = getStartParamNormal();
    double endParam = getEndParamNormal();
    double minDist = DM_MAXDOUBLE;
    bool found = false;
    DmVector closetPt(true);
    for (int i = 0; i < (int)res.size(); i++)
    {
        double mres = res[i];
        double theta = std::atan(mres) * 2.0;
        theta = Math2d::correctAngle(theta);
        if (onEntity)
        {
            if (!Math2d::isAngleBetween(theta, startParam, endParam))
            {
                continue;
            }
        }
        double xa = a * std::cos(theta);
        double ya = b * std::sin(theta);
        DmVector tanPt(xa, ya);
        double distance = coord_copy.distanceTo(tanPt);
        if (distance < minDist)
        {
            minDist = distance;
            closetPt = tanPt;
            found = true;
        }
    }

    if (!found)
    {
        return ret;
    }
    DmVector closetPt_truePos = closetPt;
    closetPt_truePos.rotate(getMajorP().angle());
    closetPt_truePos.move(getCenter());
    ret = closetPt_truePos;

    if (dist)
    {
        *dist = minDist;
    }
    return ret;
}

bool DmEllipse::isPointOnEntity(const DmVector& coord, double tolerance) const
{
    // 将点转到以majorP为X轴，椭圆中心为原点的坐标系，判断椭圆方程左边的值是否接近1。
    double t = fabs(tolerance);
    double a = getMajorRadius();
    double b = a * getRatio();
    DmVector vp((coord - getCenter()).rotate(-getAngle()));
    vp.scale(DmVector(1. / a, 1. / b));
    if (fabs(vp.squared() - 1.) > t)
    {
        return false;
    }
    return Math2d::isAngleBetween(vp.angle(), getStartParam(), getEndParam());
}

bool DmEllipse::isPointInside(const DmVector& coord, double tolerance) const
{
    // 将点转到以majorP为X轴，椭圆中心为原点的坐标系，判断椭圆方程左边的值是否<=1。
    double t = fabs(tolerance);
    double a = getMajorRadius();
    double b = a * getRatio();
    DmVector vp((coord - getCenter()).rotate(-getAngle()));
    vp.scale(DmVector(1. / a, 1. / b));
    if (vp.squared() <= t + 1.0)
    {
        return true;
    }
    return false;
}

DmVector DmEllipse::getLocalOfPoint(const DmVector& coord) const
{
    DmVector vp((coord - getCenter()).rotate(-getAngle()));
    return vp;
}

DmVector DmEllipse::getNearestCenter(const DmVector& coord, double* dist) const
{
    DmVector vCenter = data.getCenter();
    double distCenter = coord.distanceTo(data.getCenter());

    DmVectorSolutions  vsFoci = getFoci();
    DmVector const& vFocus1 = vsFoci.get(0);
    DmVector const& vFocus2 = vsFoci.get(1);

    double distFocus1 = coord.distanceTo(vFocus1);
    double distFocus2 = coord.distanceTo(vFocus2);

    // if (distFocus1 < distCenter) is true then (distFocus1 < distFocus2) must be true too and vice versa no need to check this
    if (distFocus1 < distCenter)
    {
        vCenter = vFocus1;
        distCenter = distFocus1;
    }
    else if (distFocus2 < distCenter)
    {
        vCenter = vFocus2;
        distCenter = distFocus2;
    }

    if (dist)
    {
        *dist = distCenter;
    }
    return vCenter;
}

/// @brief create from quadratic form:
/// dn[0] x^2 + dn[1] xy + dn[2] y^2 =1
/// keep the ellipse center before calling this function
bool DmEllipse::createFromQuadratic(const std::vector<double>& dn)
{
    if (dn.size() != 3)
    {
        return false;
    }
    double a = dn[0];
    const double c = dn[1];
    double b = dn[2];

    //Eigen system
    const double d = a - b;
    const double s = hypot(d, c);

    // eigenvalues are required to be positive for ellipses
    if (s >= a + b)
    {
        return false;
    }
    if (a >= b)
    {
        setMajorP(DmVector(atan2(d + s, -c)) / sqrt(0.5 * (a + b - s)));
    }
    else
    {
        setMajorP(DmVector(atan2(-c, s - d)) / sqrt(0.5 * (a + b - s)));
    }
    setRatio(sqrt((a + b - s) / (a + b + s)));

    // start/end angle at 0. means a whole ellipse, instead of an elliptic arc
    setStartParam(0.);
    setEndParam(0.);
    calculateBorders();
    return true;
}

bool DmEllipse::createFromQuadratic(const Quadratic& q)
{
    if (!q.isQuadratic())
    {
        return false;
    }
    auto  const& mQ = q.getQuad();
    double const& a = mQ(0, 0);
    double const& c = 2. * mQ(0, 1);
    double const& b = mQ(1, 1);
    auto  const& mL = q.getLinear();
    double const& d = mL(0);
    double const& e = mL(1);
    double determinant = c * c - 4. * a * b;
    if (determinant >= -DBL_EPSILON)
    {
        return false;
    }

    const DmVector eCenter = DmVector(2. * b * d - e * c, 2. * a * e - d * c) / determinant;
    // generate centered quadratic
    Quadratic qCentered = q;
    qCentered.move(-eCenter);
    if (qCentered.constTerm() >= -DBL_EPSILON)
    {
        return false;
    }
    const auto& mq2 = qCentered.getQuad();
    const double factor = -1. / qCentered.constTerm();
    // quadratic terms
    if (!createFromQuadratic({ mq2(0,0) * factor, 2. * mq2(0,1) * factor, mq2(1,1) * factor }))
    {
        return false;
    }

    // move back to center
    move(eCenter);
    return true;
}

// create Ellipse inscribed in a quadrilateral finding the tangential points and ellipse center
bool DmEllipse::createInscribeQuadrilateral(const std::vector<DmLine*>& lines)
{
    if (lines.size() != 4)
    {
        return false; // only do 4 lines
    }
    std::vector<std::unique_ptr<DmLine> > quad(4);
    {
        // form quadrilateral from intersections
        DmEntityContainer c0(nullptr, false);
        for (DmLine* const p : lines)
        {
            // copy the line pointers
            c0.addEntity(p);
        }
        DmVectorSolutions const& s0 = Information::createQuadrilateral(c0);
        if (s0.size() != 4)
        {
            return false;
        }
        for (size_t i = 0; i < 4; ++i)
        {
            quad[i].reset(new DmLine{ s0[i], s0[(i + 1) % 4] });
        }
    }

    // center of original square projected, intersection of diagonal
    DmVector centerProjection;
    {
        std::vector<DmLine> diagonal;
        diagonal.emplace_back(quad[0]->getStartpoint(), quad[1]->getEndpoint());
        diagonal.emplace_back(quad[1]->getStartpoint(), quad[2]->getEndpoint());
        DmVectorSolutions const& sol = Information::getIntersectionLineLine(&diagonal[0], &diagonal[1]);
        if (sol.getNumber() == 0)
        {
            //this should not happen
            return false;
        }
        centerProjection = sol.get(0);
    }
    std::vector<DmVector> tangent;//holds the tangential points on edges, in the order of edges: 1 3 2 0
    int parallel = 0;
    int parallel_index = 0;
    for (int i = 0; i <= 1; ++i)
    {
        DmVectorSolutions const& sol1 = Information::getIntersectionLineLine(quad[i].get(), quad[(i + 2) % 4].get());
        DmVector direction;
        if (sol1.getNumber() == 0)
        {
            direction = quad[i]->getEndpoint() - quad[i]->getStartpoint();
            ++parallel;
            parallel_index = i;
        }
        else
        {
            direction = sol1.get(0) - centerProjection;
        }
        DmLine l(centerProjection, centerProjection + direction);
        for (int k = 1; k <= 3; k += 2)
        {
            DmVectorSolutions sol2 = Information::getIntersectionLineLine(&l, quad[(i + k) % 4].get());
            if (sol2.size())
            {
                tangent.push_back(sol2.get(0));
            }
        }
    }

    if (tangent.size() < 3)
    {
        return false;
    }

    //find ellipse center by projection
    DmVector ellipseCenter;
    {
        DmLine cl0(quad[1]->getEndpoint(), (tangent[0] + tangent[2]) * 0.5);
        DmLine cl1(quad[2]->getEndpoint(), (tangent[1] + tangent[2]) * 0.5);
        DmVectorSolutions const& sol = Information::getIntersection(&cl0, &cl1, false);
        if (sol.getNumber() == 0)
        {
            return false;
        }
        ellipseCenter = sol.get(0);
    }
    if (parallel == 1)
    {
        // trapezoid
        DmLine* l0 = quad[parallel_index].get();
        DmLine* l1 = quad[(parallel_index + 2) % 4].get();
        DmVector centerPoint = (l0->getMiddlePoint() + l1->getMiddlePoint()) * 0.5;
        // not symmetric, no inscribed ellipse
        if (fabs(centerPoint.distanceTo(l0->getStartpoint()) - centerPoint.distanceTo(l0->getEndpoint())) > DM_TOLERANCE)
        {
            return false;
        }
        // symmetric
        double d = l0->getDistanceToPoint(centerPoint);
        double l = ((l0->getLength() + l1->getLength())) * 0.25;
        double k = 4. * d / fabs(l0->getLength() - l1->getLength());
        double theta = d / (l * k);
        if (theta >= 1. || d < DM_TOLERANCE)
        {
            return false;
        }
        theta = asin(theta);

        // major axis
        double a = d / (k * tan(theta));
        setCenter(DmVector(0., 0.));
        setMajorP(DmVector(a, 0.));
        setRatio(d / a);
        rotate(l0->getStartAngle());
        setCenter(centerPoint);
        return true;
    }
    std::vector<double> dn(3);
    DmVector angleVector(false);

    for (size_t i = 0; i < tangent.size(); i++)
    {
        tangent[i] -= ellipseCenter; // relative to ellipse center
    }
    std::vector<std::vector<double> > mt;
    mt.clear();
    const double symTolerance = 20. * DM_TOLERANCE;
    for (const DmVector& vp : tangent)
    {
        // form the linear equation
        // need to remove duplicated {x^2, xy, y^2} terms due to symmetry (x => -x, y=> -y)
        // i.e. rotation of 180 degrees around ellipse center
        std::vector<double> mtRow;
        mtRow.push_back(vp.x * vp.x);
        mtRow.push_back(vp.x * vp.y);
        mtRow.push_back(vp.y * vp.y);
        const double l = hypot(hypot(mtRow[0], mtRow[1]), mtRow[2]);
        bool addRow(true);
        for (const auto& v : mt)
        {
            DmVector const dv{ v[0] - mtRow[0], v[1] - mtRow[1], v[2] - mtRow[2] };
            if (dv.magnitude() < symTolerance * l)
            {
                // symmetric
                addRow = false;
                break;
            }
        }
        if (addRow)
        {
            mtRow.push_back(1.);
            mt.push_back(mtRow);
        }
    }
    switch (mt.size())
    {
    case 2:
    {
        // fixme, need to handle degenerate case better
        DmVector majorP(tangent[0]);
        double dx(majorP.magnitude());
        if (dx < DM_TOLERANCE2)
        {
            return false; // refuse to return zero size ellipse
        }
        angleVector.set(majorP.x / dx, -majorP.y / dx);
        for (size_t i = 0; i < tangent.size(); i++)
        {
            tangent[i].rotate(angleVector);
        }

        DmVector minorP(tangent[2]);
        double dy2(minorP.squared());
        if (fabs(minorP.y) < DM_TOLERANCE || dy2 < DM_TOLERANCE2)
        {
            return false; // refuse to return zero size ellipse
        }

        double ia2 = 1. / (dx * dx);
        double ib2 = 1. / (minorP.y * minorP.y);
        dn[0] = ia2;
        dn[1] = -2. * ia2 * minorP.x / minorP.y;
        dn[2] = ib2 * ia2 * minorP.x * minorP.x + ib2;
    }
    break;
    case 4:
        mt.pop_back(); // only 3 points needed to form the qudratic form
        if (!Math2d::linearSolver(mt, dn))
        {
            return false;
        }
        break;
    default:
        return false; // invalid quadrilateral
    }

    if (!createFromQuadratic(dn))
    {
        return false;
    }
    setCenter(ellipseCenter);

    if (angleVector.valid)
    {
        // need to rotate back, for the parallelogram case
        angleVector.y *= -1.;
        rotate(ellipseCenter, angleVector);
    }
    calculateBorders();
    return true;
}

// a naive implementation of middle point
// to accurately locate the middle point from arc length is possible by using elliptic integral to find the total arc length, then, using elliptic function to find the half length point
DmVector DmEllipse::getMiddlePoint()const
{
    return getNearestMiddle(getCenter());
}

// get Nearest equidistant point
DmVector DmEllipse::getNearestMiddle(const DmVector& coord, double* dist, int middlePoints) const
{
    if (isClosed())
    {
        // no middle point for whole ellipse, startAngle=endAngle=0
        if (dist)
        {
            *dist = DM_MAXDOUBLE;
        }
        return DmVector(false);
    }
    double ra(getMajorRadius());
    double rb(getRatio() * ra);
    if (ra < DM_TOLERANCE || rb < DM_TOLERANCE)
    {
        // zero radius, return the center
        DmVector vp(getCenter());
        if (dist)
        {
            *dist = vp.distanceTo(coord);
        }
        return vp;
    }
    double amin = getCenter().angleTo(getStartpoint());
    double amax = getCenter().angleTo(getEndpoint());
    if (isClockwise())
    {
        std::swap(amin, amax);
    }
    double da = fmod(amax - amin + 2. * M_PI, 2. * M_PI);
    if (da < DM_TOLERANCE)
    {
        da = 2. * M_PI; //whole ellipse
    }

    //获得距离coord最近的分段点
    DmVector vp(getNearestPointOnEntity(coord, true, dist));
    double a = getCenter().angleTo(vp);
    int counts(middlePoints + 1);
    int i(static_cast<int>(fmod(a - amin + 2. * M_PI, 2. * M_PI) / da * counts + 0.5));
    if (!i)
    {
        i++; // remove end points
    }
    if (i == counts)
    {
        i--;
    }
    a = amin + da * (double(i) / double(counts)) - getAngle();
    vp.set(a);
    DmVector vp2(vp);
    vp2.scale(DmVector(1. / ra, 1. / rb));
    vp.scale(1. / vp2.magnitude());
    vp.rotate(getAngle());
    vp.move(getCenter());

    if (dist)
    {
        *dist = vp.distanceTo(coord);
    }
    return vp;
}

/// @brief get the tangential point of a tangential line orthogonal to a given line
/// @param coord current cursor position
/// @param normal the given line
/// @param onEntity should the tangential be required to on entity of the elliptic arc
DmVector DmEllipse::getNearestOrthTan(const DmVector& coord, const DmLine& normal, bool onEntity) const
{
    if (!coord.valid)
    {
        return DmVector(false);
    }
    DmVector direction = normal.getEndpoint() - normal.getStartpoint();
    if (direction.squared() < DM_TOLERANCE15)
    {
        // undefined direction
        return DmVector(false);
    }
    // scale to ellipse angle
    DmVector aV(-getAngle());
    direction.rotate(aV);
    double angle = direction.scale(DmVector(1., getRatio())).angle();
    double ra(getMajorRadius());
    direction.set(ra * cos(angle), getRatio() * ra * sin(angle)); // relative to center
    std::vector<DmVector> sol;
    for (int i = 0; i < 2; i++)
    {
        if (!onEntity || Math2d::isAngleBetween(angle, getStartParam(), getEndParam()))
        {
            if (i)
            {
                sol.push_back(-direction);
            }
            else
            {
                sol.push_back(direction);
            }
        }
        angle = Math2d::correctAngle(angle + M_PI);
    }
    if (sol.size() < 1)
    {
        return DmVector(false);
    }
    aV.y *= -1.;
    for (auto& v : sol)
    {
        v.rotate(aV);
    }
    DmVector vp;
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
        // fallthrough for case 2 if dotP <= 0
    default:
        vp = sol[0];
        break;
    }
    return getCenter() + vp;
}

void DmEllipse::move(const DmVector& offset)
{
    DmVector vec = data.getCenter().move(offset);
    data.setCenter(vec);
    isModify = true;
    moveBorders(offset);
}

void DmEllipse::rotate(const DmVector& center, const DmVector& angleVector)
{
    DmVector cen = data.getCenter().rotate(center, angleVector);
    DmVector maj = data.getMajorP().rotate(angleVector);
    data.setCenter(cen);
    data.setMajorP(maj);
    isModify = true;
    calculateBorders();
}

void DmEllipse::rotate(const double& angle)
{
    DmVector aV(angle);
    DmVector cen = data.getCenter().rotate(aV);
    DmVector maj = data.getMajorP().rotate(aV);
    data.setCenter(cen);
    data.setMajorP(maj);
    isModify = true;
    calculateBorders();
}

void DmEllipse::rotate(const DmVector& angleVector)
{
    DmVector cen = data.getCenter().rotate(angleVector);
    DmVector maj = data.getMajorP().rotate(angleVector);
    data.setCenter(cen);
    data.setMajorP(maj);
    isModify = true;
    calculateBorders();
}

// make sure angleLength() is not more than 2*M_PI
void DmEllipse::correctAngles()
{
    double pa1 = data.getStartParam();
    double pa2 = data.getEndParam();
    pa2 = pa1 + fmod(pa2 - pa1, 2. * M_PI);
    if (fabs(data.getStartParam() - data.getEndParam()) < DM_TOLERANCE_ANGLE)
    {
        pa2 += 2. * M_PI;
    }
    data.setStartParam(pa2);
    data.setEndParam(pa1);
}

void DmEllipse::moveStartpoint(const DmVector& pos)
{
    data.setStartParam(getEllipseParam(pos));
    correctAngles(); // make sure angleLength is no more than 2*M_PI
    isModify = true;
    calculateBorders();
}

void DmEllipse::moveEndpoint(const DmVector& pos)
{
    data.setEndParam(getEllipseParam(pos));
    correctAngles(); // make sure angleLength is no more than 2*M_PI
    isModify = true;
    calculateBorders();
}

DM::Ending DmEllipse::getTrimPoint(const DmVector& trimCoord, const DmVector& /*trimPoint*/)
{
    double angM = getEllipseParam(trimCoord);
    if (Math2d::getAngleDifference(angM, data.getStartParam()) > Math2d::getAngleDifference(data.getEndParam(), angM))
    {
        return DM::EndingStart;
    }
    else
    {
        return DM::EndingEnd;
    }
}

double DmEllipse::getEllipseParam(const DmVector& pos) const
{
    // 计算：中心到pos的角度所代表的参数。

    DmVector m = pos - data.getCenter();
    // 逆时针
    if (!isClockwise())
    {
        m.rotate(-data.getMajorP().angle());
        m.x *= data.getRatio(); //等效于m.y /= data.getRatio();
    }
    // 顺时针
    else
    {
        m.rotate(-data.getMajorP().angle());
        m.x *= data.getRatio();
        m.y *= -1.0;    //计算顺时针角度时y得取反
    }

    return m.angle();
}

double DmEllipse::paramToAngle(double param) const
{
    double a = getMajorRadius();
    double b = getRatio() * a;
    double y = std::sin(param) * b;
    double x = std::cos(param) * a;
    double angle = std::atan2(y, x);    //[-pi, +pi]
    angle = Math2d::correctAngle(angle);
    return angle;
}

double DmEllipse::angleToParam(double angle) const
{
    angle = Math2d::correctAngle(angle);
    // 特殊角度处理
    if (std::abs(angle - M_PI_2) < DM_TOLERANCE_ANGLE || std::abs(angle - M_PI_2 * 3.0) < DM_TOLERANCE_ANGLE)
    {
        return angle;
    }
    // 简化的计算方法：tan(angle) = tan(param) * ratio
    double tan_angle = std::tan(angle);
    double theta = std::atan(tan_angle / getRatio());
    double angle_correct = Math2d::correctAngle2(angle);
    if (angle_correct > M_PI_2 || angle_correct < -M_PI_2)
    {
        theta = Math2d::correctAngle(M_PI + theta);
    }
    else
    {
        theta = Math2d::correctAngle(theta);
    }
    return theta;
}

EllipseData DmEllipse::getData() const
{
    return data;
}

void DmEllipse::setData(const EllipseData& d)
{
    data = d;
}

void DmEllipse::scale(const DmVector& center, const DmVector& factor)
{
    if (factor.x < 0.0)
    {
        mirror(data.getCenter(), data.getCenter() + DmVector(0.0, 1.0));
    }
    if (factor.y < 0.0)
    {
        mirror(data.getCenter(), data.getCenter() + DmVector(1.0, 0.0));
    }

    data.setCenter(data.getCenter().scale(center, factor));
    data.setMajorP(data.getMajorP() * fabs(factor.x));
    data.setRatio(fabs(getRatio() * factor.y / factor.x));
    isModify = true;
    calculateBorders();
}

// mirror by the axis of the line axisPoint1 and axisPoint2
void DmEllipse::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    DmVector center = getCenter();
    DmVector majorp = center + getMajorP();
    DmVector startpoint, endpoint;
    if (!isClosed())
    {
        startpoint = getStartpoint();
        endpoint = getEndpoint();
    }

    center.mirror(axisPoint1, axisPoint2);
    majorp.mirror(axisPoint1, axisPoint2);

    setCenter(center);
    // TODO : 屏蔽reverse
    //setReversed(!isReversed());
    setMajorP(majorp - center);
    if (!isClosed())
    {
        // only reset start/end points for ellipse arcs, i.e., startAngle endAngle are not both zero
        startpoint.mirror(axisPoint1, axisPoint2);
        endpoint.mirror(axisPoint1, axisPoint2);
        setStartParam(getEllipseParam(startpoint));
        setEndParam(getEllipseParam(endpoint));
    }
    //correctAngles();//avoid extra 2.*M_PI in angles
    isModify = true;
    calculateBorders();
}

double DmEllipse::getDirection1() const
{
    DmVector vp;
    if (isClockwise())
    {
        vp.set(-sin(getStartParam()), -getRatio() * cos(getStartParam()));
    }
    else
    {
        vp.set(-sin(getStartParam()), getRatio() * cos(getStartParam()));
    }
    return vp.angle() + getAngle();
}

// getDirection2 for end point
double DmEllipse::getDirection2() const
{
    DmVector vp;
    if (isClockwise())
    {
        vp.set(sin(getEndParam()), getRatio() * cos(getEndParam()));
    }
    else
    {
        vp.set(sin(getEndParam()), -getRatio() * cos(getEndParam()));
    }
    return vp.angle() + getAngle();
}

void DmEllipse::moveRef(const DmVector& ref, const DmVector& offset)
{
    //椭圆弧移动端点
    if (!isClosed())
    {
        DmVector startpoint = getStartpoint();
        DmVector endpoint = getEndpoint();
        if ((ref - startpoint).squared() < DM_TOLERANCE_ANGLE)
        {
            moveStartpoint(startpoint + offset);
            correctAngles();// avoid extra 2.*M_PI in angles
            calculateBorders();
            return;
        }
        if ((ref - endpoint).squared() < DM_TOLERANCE_ANGLE)
        {
            moveEndpoint(endpoint + offset);
            correctAngles();// avoid extra 2.*M_PI in angles
            calculateBorders();
            return;
        }
    }

    //移动中心
    if ((ref - getCenter()).squared() < DM_TOLERANCE_ANGLE)
    {
        // move center
        setCenter(getCenter() + offset);
        moveBorders(offset);
        return;
    }

    if (data.getRatio() > 1.)
    {
        switchMajorMinor();
    }

    // 移动长轴端点
    if ((ref - getMajorPoint()).squared() < DM_TOLERANCE_ANGLE)
    {
        DmVector majorP = getMajorP() + offset;
        double r = majorP.magnitude();
        if (r < DM_TOLERANCE)
        {
            return;
        }
        double ratio = getRatio() * getMajorRadius() / r;
        setMajorP(majorP);
        setRatio(ratio);
        if (data.getRatio() > 1.)
        {
            switchMajorMinor();
        }
        calculateBorders();
        return;
    }

    //移动短轴端点
    if ((ref - getMinorPoint()).squared() < DM_TOLERANCE_ANGLE)
    {
        DmVector minorP = getMinorPoint() + offset;     // 移动后的短轴端点
        double r2 = getMajorP().squared();
        if (r2 < DM_TOLERANCE2)
        {
            return;
        }
        DmVector projected = getCenter() + getMajorP() * getMajorP().dotP(minorP - getCenter()) / r2;   //移动后的短轴端点在长轴的投影点。求向量b到向量a的投影向量p：p = a/|a| * |b|*cos_theta = a*a*b/|a|^2
        double r = (minorP - projected).magnitude();    //短轴长度为0，这一般是移动短轴端点时吸附了中心点，这种情况椭圆不变
        if (r < DM_TOLERANCE)
        {
            return;
        }
        double ratio = getRatio() * r / getMinorRadius();
        setRatio(ratio);
        if (data.getRatio() > 1.)
        {
            switchMajorMinor();
        }
        calculateBorders();
        return;
    }
}

Quadratic DmEllipse::getQuadratic() const
{
    // 参考：https://chat.deepseek.com/share/btkje5jq173hhdlexp
    std::vector<double> ce(6, 0.);
    ce[0] = data.getMajorP().squared(); // a^2
    ce[2] = data.getRatio() * data.getRatio() * ce[0]; // b^2
    if (ce[0] < DM_TOLERANCE2 || ce[2] < DM_TOLERANCE2)
    {
        return Quadratic();
    }
    ce[0] = 1. / ce[0]; // A = 1/a^2
    ce[2] = 1. / ce[2]; // C =  1/b^2
    ce[5] = -1.;        // F = -1
    Quadratic ret(ce);
    ret.rotate(getAngle());
    ret.move(data.getCenter());
    return ret;
}

double DmEllipse::getAngle() const
{
    return data.getMajorP().angle();
}

double DmEllipse::getParam(double angle) const
{
    DmVector center = data.getCenter();
    DmVector pos = center + DmVector(angle);
    double param = getEllipseParam(pos);
    return param;
}

double DmEllipse::getAngle(double param) const
{
    double a = getMajorP().magnitude();
    double b = a * getRatio();
    double x_a = a * cos(param);
    double y_a = a * sin(param);
    double y_b = b * sin(param);
    double tan_alpha = y_b / x_a;
    double alpha = atan(tan_alpha); //摆正时的角度
    double mAngle = getMajorP().angle();
    double ret = 0.0;
    if (!isClockwise())
    {
        ret = Math2d::correctAngle(alpha + mAngle); //加上主轴的角度
    }
    else
    {
        ret = Math2d::correctAngle(-alpha + mAngle);
    }
    return ret;
}

double DmEllipse::getStartParam() const
{
    return data.getStartParam();
}

double DmEllipse::getStartParamNormal() const
{
    if (isClockwise())
    {
        return Math2d::correctAngle(M_PI * 2.0 - getEndParam());
    }
    else
    {
        return getStartParam();
    }
}

void DmEllipse::setStartParam(double a1)
{
    data.setStartParam(a1);
    isModify = true;
}

double DmEllipse::getEndParam() const
{
    return data.getEndParam();
}

double DmEllipse::getEndParamNormal() const
{
    if (isClockwise())
    {
        return Math2d::correctAngle(M_PI * 2.0 - getStartParam());
    }
    else
    {
        return getEndParam();
    }
}

void DmEllipse::setEndParam(double a2)
{
    data.setEndParam(a2);
    isModify = true;
}

double DmEllipse::getParaNormal(double a)
{
    if (isClockwise())
    {
        return Math2d::correctAngle(M_PI * 2.0 - a);
    }
    else
    {
        return a;
    }
}

double DmEllipse::getStartAngle() const
{
    double angle = paramToAngle(getStartParam());
    return angle;
}

void DmEllipse::setStartAngle(double startAngle)
{
    double param = angleToParam(startAngle);
    setStartParam(param);
}

double DmEllipse::getEndAngle() const
{
    double angle = paramToAngle(getEndParam());
    return angle;
}

void DmEllipse::setEndAngle(double endAngle)
{
    double param = angleToParam(endAngle);
    setEndParam(param);
}

DmVector DmEllipse::getCenter() const
{
    return data.getCenter();
}

void DmEllipse::setCenter(const DmVector& c)
{
    data.setCenter(c);
    isModify = true;
}

DmVector DmEllipse::getMajorP() const
{
    return data.getMajorP();
}

void DmEllipse::setMajorP(const DmVector& p)
{
    data.setMajorP(p);
    isModify = true;
}

double DmEllipse::getRatio() const
{
    return data.getRatio();
}

void DmEllipse::setRatio(double r)
{
    data.setRatio(r);
    isModify = true;
}

bool DmEllipse::isClosed() const
{
    return data.getIsClosed();
}

void DmEllipse::setClosed(bool closed)
{
    data.setIsClosed(closed);
}

double DmEllipse::getAngleLength() const
{
    double ret = Math2d::correctAngle(data.getEndParam() - data.getStartParam());
    if (ret < DM_TOLERANCE_ANGLE)
    {
        ret = 2. * M_PI;
    }
    return ret;
}

double DmEllipse::getMajorRadius() const
{
    return data.getMajorP().magnitude();
}

DmVector DmEllipse::getMajorPoint() const
{
    return data.getCenter() + data.getMajorP();
}

DmVector DmEllipse::getMinorPoint() const
{
    return data.getCenter() + DmVector(-data.getMajorP().y, data.getMajorP().x) * data.getRatio();
}

double DmEllipse::getMinorRadius() const
{
    return data.getMajorP().magnitude() * data.getRatio();
}

std::vector<double> DmEllipse::calculateVertexs(const DmVector& center, const double radius)
{
    std::vector<double> vertexs;
    double mAngle = 0.;
    mAngle = getAngle();
    double start = 0.;  //此参数代表绘制时，按逆时针方向的起始角度
    double end = 0.;    //此参数代表绘制时，按逆时针方向的终止角度

    if (isClosed())
    {
        start = 0.0;
        end = M_PI * 2;
    }
    // 顺时针
    else if (isClockwise())
    {
        start = Math2d::correctAngle(M_PI * 2.0 - getEndParam());
        end = Math2d::correctAngle(M_PI * 2.0 - getStartParam());
    }
    // 逆时针
    else
    {
        start = getStartParam();
        end = getEndParam();
    }

    float delta = std::abs(end < start ? (end - start + DM_PI * 2) : (end - start));
    float EA = 0;
    float tx = 0.0f, ty = 0.0f, TX = 0.0f, TY = 0.0f;

    for (int i = 0; i <= DM_CURVE_VERTEXS; i++)
    {
        EA = (((float)i) / DM_CURVE_VERTEXS) * delta + start;

        tx = radius * cos(EA); // parametric equation
        ty = getRatio() * radius * sin(EA);

        TX = (tx * cos(mAngle) - ty * sin(mAngle)) + center.x; // first rotate then shift origin
        TY = (tx * sin(mAngle) + ty * cos(mAngle)) + center.y;

        vertexs.emplace_back(TX);
        vertexs.emplace_back(TY);
    }
    return vertexs;
}

const std::vector<float>& DmEllipse::getVerticesRef(int& float_count_per_vertex)
{
    updateVertices();
    float_count_per_vertex = 5;
    return data.getVerticesRef();
}

void DmEllipse::updateVertices()
{
    if (isModify)
    {
        std::vector<float> vertexs;
        float majorx = (float)getMajorP().x;
        float majory = (float)getMajorP().y;
        float ratio = (float)getRatio();
        float centerx = (float)getCenter().x;
        float centery = (float)getCenter().y;

        // 不闭合
        if (!isClosed())
        {
            float startParam = (float)getStartParamNormal();
            float endParam = (float)getEndParamNormal();

            constexpr float _2pi = M_PI * 2.0f;
            float delta_angle = endParam - startParam;
            if (delta_angle <= 0.0f)
            {
                delta_angle += _2pi;
            }
            float factor = delta_angle / _2pi;
            int segment_count = (int)std::ceil(factor * ELLIPSE_SEGMENT_COUNT);
            constexpr int kMinSegmentCount = 10;
            segment_count = std::max(kMinSegmentCount, segment_count);
            vertexs.reserve((segment_count + 3) * 5);

            float ang_delta = delta_angle / segment_count;
            float radius = std::sqrt(majorx * majorx + majory * majory);
            float majorLen = std::sqrt(majorx * majorx + majory * majory);
            float b = ratio * majorLen;
            float cosa = majorx / majorLen;
            float sina = majory / majorLen;
            float param = startParam;
            float oldParam = param;
            float total_length = 0.0f;

            auto func = [cosa, sina, radius, ratio, centerx, centery](float ea, float& x, float& y)
            {
                float tx = radius * std::cos(ea); // parametric equation
                float ty = ratio * radius * std::sin(ea);
                x = (tx * cosa - ty * sina) + centerx; // first rotate then shift origin
                y = (tx * sina + ty * cosa) + centery;
            };

            float tempx = 0.0f, tempy = 0.0f;
            float para = 0.0f;
            float lastX = 0.0f, lastY = 0.0f;

            for (int i = 0; i < segment_count; i++)
            {
                if (i == 0)
                {
                    func(param + ang_delta, tempx, tempy);
                    // 针对GL_LINE_STRIP_ADJACENCY的起始坐标
                    vertexs.emplace_back(tempx);
                    vertexs.emplace_back(tempy);
                    vertexs.emplace_back(0.0f);
                    vertexs.emplace_back(0.0f);
                    vertexs.emplace_back(total_length);

                    // 起始点
                    func(param, tempx, tempy);
                    vertexs.emplace_back(tempx);
                    vertexs.emplace_back(tempy);
                    vertexs.emplace_back(0.0f);
                    vertexs.emplace_back(0.0f);
                    vertexs.emplace_back(total_length);
                    lastX = tempx;
                    lastY = tempy;
                }
                oldParam = param;
                param += ang_delta;
                func(param, tempx, tempy);
                para += glm::distance(glm::vec2(tempx, tempy), glm::vec2(lastX, lastY));
                lastX = tempx;
                lastY = tempy;
                vertexs.emplace_back(tempx);
                vertexs.emplace_back(tempy);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(para);
                vertexs.emplace_back(total_length);
                if (i == segment_count - 1)
                {
                    // 针对GL_LINE_STRIP_ADJACENCY的终止坐标
                    func(param - ang_delta, tempx, tempy);
                    vertexs.emplace_back(tempx);
                    vertexs.emplace_back(tempy);
                    vertexs.emplace_back(0.0f);
                    vertexs.emplace_back(para);
                    vertexs.emplace_back(total_length);
                }
            }
            //设置总长
            for (int i = 0; i < segment_count + 3; i++)
            {
                vertexs.at(5 * i + 4) = para;
            }
        }
        //闭合
        else
        {
            vertexs.reserve((ELLIPSE_SEGMENT_COUNT + 3) * 5);
            constexpr float _2pi = M_PI * 2.0f;
            constexpr float ang_delta = _2pi / ELLIPSE_SEGMENT_COUNT;
            float radius = std::sqrt(majorx * majorx + majory * majory);
            float majorLen = std::sqrt(majorx * majorx + majory * majory);
            float b = ratio * majorLen;
            float cosa = majorx / majorLen;
            float sina = majory / majorLen;
            float param = 0.0f;
            float oldParam = param;

            // 与长度迭代的误差太大，采用长度迭代
            // 椭圆的周长。没有精确计算公式，近似采用：L=2pi*b + 4(a-b)，参考：https://baike.baidu.com/item/%E6%A4%AD%E5%9C%86%E5%91%A8%E9%95%BF/9569341?fr=ge_ala
            //float total_length = _2pi * b + 4.0f * (majorLen - b);
            float total_length = 0.0f;

            auto func = [cosa, sina, radius, ratio, centerx, centery](float ea, float& x, float& y)
            {
                float tx = radius * std::cos(ea); // parametric equation
                float ty = ratio * radius * std::sin(ea);
                x = (tx * cosa - ty * sina) + centerx; // first rotate then shift origin
                y = (tx * sina + ty * cosa) + centery;
            };

            float tempx = 0.0f, tempy = 0.0f;
            float para = 0.0f;
            float lastX = 0.0f, lastY = 0.0f;
            for (int i = 0; i < ELLIPSE_SEGMENT_COUNT; i++)
            {
                if (i == 0)
                {
                    func(-ang_delta, tempx, tempy);
                    // 针对GL_LINE_STRIP_ADJACENCY的起始坐标
                    vertexs.emplace_back(tempx);
                    vertexs.emplace_back(tempy);
                    vertexs.emplace_back(0.0f);
                    vertexs.emplace_back(0.0f);
                    vertexs.emplace_back(total_length);

                    // 起始点
                    func(0.0f, tempx, tempy);
                    vertexs.emplace_back(tempx);
                    vertexs.emplace_back(tempy);
                    vertexs.emplace_back(0.0f);
                    vertexs.emplace_back(0.0f);
                    vertexs.emplace_back(total_length);
                    lastX = tempx;
                    lastY = tempy;
                }
                oldParam = param;
                param += ang_delta;
                func(param, tempx, tempy);
                para += glm::distance(glm::vec2(tempx, tempy), glm::vec2(lastX, lastY));
                //para += std::sqrt((tempx - lastX) * (tempx - lastX) + (tempy - lastY) * (tempy - lastY));
                lastX = tempx;
                lastY = tempy;
                vertexs.emplace_back(tempx);
                vertexs.emplace_back(tempy);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(para);
                vertexs.emplace_back(total_length);

                if (i == ELLIPSE_SEGMENT_COUNT - 1)
                {
                    // 针对GL_LINE_STRIP_ADJACENCY的终止坐标
                    func(ang_delta, tempx, tempy);
                    vertexs.emplace_back(tempx);
                    vertexs.emplace_back(tempy);
                    vertexs.emplace_back(0.0f);
                    vertexs.emplace_back(para);
                    vertexs.emplace_back(total_length);
                }
            }
            //设置总长
            for (int i = 0; i < ELLIPSE_SEGMENT_COUNT + 3; i++)
            {
                vertexs.at(5 * i + 4) = para;
            }
        }

        data.setVertices(vertexs);
        isModify = false;
    }
}

void DmEllipse::getPoints(std::vector<DmVector>& pts, bool reverse /*= false*/)
{
    // 计算分段数
    double a = getEndParam() - getStartParam();
    a = Math2d::correctAngle(a);
    constexpr double kDefaultDelta = 6.0;
    double delta = kDefaultDelta;
    double count = Math2d::rad2deg(a) / delta;    //一圆周60份
    count = std::round(count);
    count = std::max(count, 2.0);
    delta = a / count;

    // 计算点
    int iCount = (int)count;
    DmVector startPt = getStartpoint();
    DmVector center = getCenter();
    DmVector major = getMajorP();
    double majorLen = major.magnitude();
    DmVector majorDir = major / majorLen;
    double ratio = getRatio();
    int startIdx = (int)pts.size();
    pts.emplace_back(startPt);
    double startPara = getStartParam();
    for (int i = 1; i <= iCount; i++)
    {
        double ang = startPara + delta * i;
        ang = getParaNormal(ang); // 翻正之后的参数
        double tx = majorLen * std::cos(ang); // parametric equation
        double ty = ratio * majorLen * std::sin(ang);
        DmVector p =  DmVector(tx, ty).rotate(majorDir) + center; // first rotate then shift origin
        pts.emplace_back(p);
    }
    if (reverse)
    {
        std::reverse(pts.begin() + startIdx, pts.end());
    }
}

void DmEllipse::update()
{
    isModify = true;
    updateVertices();
}

void DmEllipse::saveStream(OutputStream& wrt) const
{
    DmAtomicEntity::saveStream(wrt);

    auto center = getCenter();
    auto startParam = getStartParam();
    auto endParam = getEndParam();
    auto majorP = getMajorP();
    auto ratio = getRatio();
    auto normal = getNormal();
    auto closed = isClosed();

    wrt << (double)center.x << (double)center.y << (bool)closed << (double)startParam << (double)endParam << (double)majorP.x << (double)majorP.y << (double)ratio << (double)normal.x << (double)normal.y << (double)normal.z;
}

void DmEllipse::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
    int fileRev = getRevisionId("DmEllipse", revs);
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

void DmEllipse::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (rev == 0)
    {
        // legacy rev 0 format
    }
    else //big change, e.g. change supper class of DmEllipse
    {
        //step1.
        // read all legacy data one by one
    }
}

void DmEllipse::restoreStream(InputStream& rdr)
{
    DmAtomicEntity::restoreStream(rdr);

    DmVector center(true), majorP(true), normal(true);
    double startParam = 0.0, endParam = 0.0, ratio = 0.0;
    bool closed = true;
    rdr >> (double&)center.x >> (double&)center.y >> (bool&)closed >> (double&)startParam >> (double&)endParam >> (double&)majorP.x >> (double&)majorP.y >> (double&)ratio >> (double&)normal.x >> (double&)normal.y >> (double&)normal.z;

    setCenter(center);
    setStartParam(startParam);
    setEndParam(endParam);
    setClosed(closed);
    setMajorP(majorP);
    setNormal(normal);
    setRatio(ratio);
    calculateBorders();
    isModify = true;
}
