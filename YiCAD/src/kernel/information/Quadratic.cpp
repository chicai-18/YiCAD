/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file Quadratic.cpp
/// @brief 一元或二次曲线方程类实现

#include "Quadratic.h"

#include <cfloat>
#include <QDebug>

#include "Math2d.h"
#include "Information.h"

#include "DmArc.h"
#include "DmCircle.h"
#include "DmEllipse.h"
#include "DmLine.h"
#include "Debug.h"

constexpr int QUADRATIC_MATRIX_SIZE = 2;    ///< 二次矩阵维度
constexpr int LINEAR_EQ_COEFF_COUNT = 3;    ///< 线性方程系数个数
constexpr int QUADRATIC_EQ_COEFF_COUNT = 6; ///< 二次方程系数个数

Quadratic::Quadratic()
    : m_mQuad(QUADRATIC_MATRIX_SIZE, QUADRATIC_MATRIX_SIZE)
    , m_vLinear(QUADRATIC_MATRIX_SIZE)
    , m_dConst(0.0)
    , m_bIsQuadratic(false)
    , m_bValid(false)
{}

Quadratic::Quadratic(const Quadratic& lc0)
    : m_dConst(0.0)
    , m_bIsQuadratic(lc0.isQuadratic())
    , m_bValid(lc0)
{
    if (m_bValid == false)
    {
        return;
    }
    if (m_bIsQuadratic)
    {
        m_mQuad = lc0.getQuad();
    }
    m_vLinear = lc0.getLinear();
    m_dConst = lc0.m_dConst;
}

Quadratic& Quadratic::operator=(const Quadratic& lc0)
{
    if (lc0.isQuadratic())
    {
        m_mQuad.resize(QUADRATIC_MATRIX_SIZE, QUADRATIC_MATRIX_SIZE, false);
        m_mQuad = lc0.getQuad();
    }
    m_vLinear.resize(QUADRATIC_MATRIX_SIZE);
    m_vLinear = lc0.getLinear();
    m_dConst = lc0.m_dConst;
    m_bIsQuadratic = lc0.isQuadratic();
    m_bValid = lc0.m_bValid;
    return *this;
}

Quadratic::Quadratic(std::vector<double> ce)
    : m_mQuad(QUADRATIC_MATRIX_SIZE, QUADRATIC_MATRIX_SIZE)
    , m_vLinear(QUADRATIC_MATRIX_SIZE)
    , m_dConst(0.0)
    , m_bIsQuadratic(false)
    , m_bValid(false)
{
    if (ce.size() == QUADRATIC_EQ_COEFF_COUNT)
    {
        // 二次曲线
        m_mQuad(0, 0) = ce[0];
        m_mQuad(0, 1) = 0.5 * ce[1];
        m_mQuad(1, 0) = m_mQuad(0, 1);
        m_mQuad(1, 1) = ce[2];
        m_vLinear(0) = ce[3];
        m_vLinear(1) = ce[4];
        m_dConst = ce[5];
        m_bIsQuadratic = true;
        m_bValid = true;
        return;
    }
    if (ce.size() == LINEAR_EQ_COEFF_COUNT)
    {
        // 一次曲线
        m_vLinear(0) = ce[0];
        m_vLinear(1) = ce[1];
        m_dConst = ce[2];
        m_bIsQuadratic = false;
        m_bValid = true;
        return;
    }
    m_bValid = false;
}

Quadratic::Quadratic(const DmAtomicEntity* circle, const DmVector& point)
    : m_mQuad(QUADRATIC_MATRIX_SIZE, QUADRATIC_MATRIX_SIZE)
    , m_vLinear(QUADRATIC_MATRIX_SIZE)
    , m_dConst(0.0)
    , m_bIsQuadratic(true)
    , m_bValid(true)
{
    if (circle == nullptr)
    {
        m_bValid = false;
        return;
    }
    switch (circle->getEntityType())
    {
    case DM::EntityArc:
    case DM::EntityCircle:
    {
        DmVector center;
        double r;

        center = circle->getCenter();
        r = circle->getRadius();
        if (center == false)
        {
            m_bValid = false;
            return;
        }
        double c = 0.5 * (center.distanceTo(point));
        double d = 0.5 * r;
        if (fabs(c) < DM_TOLERANCE || fabs(d) < DM_TOLERANCE || fabs(c - d) < DM_TOLERANCE)
        {
            m_bValid = false;
            return;
        }
        m_mQuad(0, 0) = 1. / (d * d);
        m_mQuad(0, 1) = 0.;
        m_mQuad(1, 0) = 0.;
        m_mQuad(1, 1) = 1. / (d * d - c * c);
        m_vLinear(0) = 0.;
        m_vLinear(1) = 0.;
        m_dConst = -1.;
        center = (center + point) * 0.5;
        rotate(center.angleTo(point));
        move(center);
        return;
    }
    case DM::EntityLine:
    {
        const DmLine* line = static_cast<const DmLine*>(circle);

        DmVector direction = line->getEndpoint() - line->getStartpoint();
        double l2 = direction.squared();
        if (l2 < DM_TOLERANCE2)
        {
            m_bValid = false;
            return;
        }
        DmVector projection = line->getNearestPointOnEntity(point, false);
        double p2 = (projection - point).squared();
        if (p2 < DM_TOLERANCE2)
        {
            // point on line, return a straight line
            m_bIsQuadratic = false;
            m_vLinear(0) = direction.y;
            m_vLinear(1) = -direction.x;
            m_dConst = direction.x * point.y - direction.y * point.x;
            return;
        }
        DmVector center = (projection + point) * 0.5;
        double p = sqrt(p2);
        m_bIsQuadratic = true;
        m_bValid = true;
        m_mQuad(0, 0) = 0.;
        m_mQuad(0, 1) = 0.;
        m_mQuad(1, 0) = 0.;
        m_mQuad(1, 1) = 1.;
        m_vLinear(0) = -2. * p;
        m_vLinear(1) = 0.;
        m_dConst = 0.;
        rotate(center.angleTo(point));
        move(center);
        return;
    }
    default:
        m_bValid = false;
        return;
    }
}

bool Quadratic::isQuadratic() const
{
    return m_bIsQuadratic;
}

Quadratic::operator bool() const
{
    return m_bValid;
}

bool Quadratic::isValid() const
{
    return m_bValid;
}

bool Quadratic::operator==(bool valid) const
{
    return m_bValid == valid;
}

bool Quadratic::operator!=(bool valid) const
{
    return m_bValid != valid;
}

boost::numeric::ublas::vector<double>& Quadratic::getLinear()
{
    return m_vLinear;
}

const boost::numeric::ublas::vector<double>& Quadratic::getLinear() const
{
    return m_vLinear;
}

boost::numeric::ublas::matrix<double>& Quadratic::getQuad()
{
    return m_mQuad;
}

const boost::numeric::ublas::matrix<double>& Quadratic::getQuad() const
{
    return m_mQuad;
}

double const& Quadratic::constTerm() const
{
    return m_dConst;
}

double& Quadratic::constTerm()
{
    return m_dConst;
}

Quadratic::Quadratic(const DmEntity* circle0, const DmEntity* circle1, bool mirror)
    : m_mQuad(QUADRATIC_MATRIX_SIZE, QUADRATIC_MATRIX_SIZE)
    , m_vLinear(QUADRATIC_MATRIX_SIZE)
    , m_dConst(0.0)
    , m_bIsQuadratic(false)
    , m_bValid(false)
{
    if (!(isEntityArcCircleLine(circle0) && isEntityArcCircleLine(circle1)))
    {
        return;
    }

    if (circle1->getEntityType() != DM::EntityLine)
    {
        std::swap(circle0, circle1);
    }
    if (circle0->getEntityType() == DM::EntityLine)
    {
        // two lines
        DmLine* line0 = (DmLine*)circle0;
        DmLine* line1 = (DmLine*)circle1;

        auto centers = Information::getIntersection(line0, line1);

        if (centers.size() != 1)
        {
            return;
        }
        double angle = 0.5 * (line0->getStartAngle() + line1->getStartAngle());
        m_bValid = true;
        m_bIsQuadratic = true;
        m_mQuad(0, 0) = 0.;
        m_mQuad(0, 1) = 0.5;
        m_mQuad(1, 0) = 0.5;
        m_mQuad(1, 1) = 0.;
        m_vLinear(0) = 0.;
        m_vLinear(1) = 0.;
        m_dConst = 0.;
        rotate(angle);
        move(centers.get(0));
        return;
    }
    if (circle1->getEntityType() == DM::EntityLine)
    {
        const DmLine* line1 = static_cast<const DmLine*>(circle1);
        DmVector normal = line1->getNormalVector() * circle0->getRadius();
        DmVector disp = line1->getNearestPointOnEntity(circle0->getCenter(), false) - circle0->getCenter();
        if (normal.dotP(disp) > 0.)
        {
            normal *= -1.;
        }
        if (mirror)
        {
            normal *= -1.;
        }

        DmLine directrix(line1->getStartpoint() + normal, line1->getEndpoint() + normal);
        Quadratic lc0(&directrix, circle0->getCenter());
        *this = lc0;
        return;

        m_mQuad = lc0.getQuad();
        m_vLinear = lc0.getLinear();
        m_bIsQuadratic = true;
        m_bValid = true;
        m_dConst = lc0.m_dConst;

        return;
    }
    // two circles
    double const f = (circle0->getCenter() - circle1->getCenter()).magnitude() * 0.5;
    double const a = fabs(circle0->getRadius() + circle1->getRadius()) * 0.5;
    double const c = fabs(circle0->getRadius() - circle1->getRadius()) * 0.5;
    if (a < DM_TOLERANCE)
    {
        return;
    }
    DmVector center = (circle0->getCenter() + circle1->getCenter()) * 0.5;
    double angle = center.angleTo(circle0->getCenter());
    if (f < a)
    {
        // ellipse
        double const ratio = sqrt(a * a - f * f) / a;
        DmVector const& majorP = DmVector{ angle } * a;
        DmEllipse const ellipse(nullptr, {center, majorP, DmVector(0.0, 0.0, 1.0), ratio, true, 0., 0.});
        auto const& lc0 = ellipse.getQuadratic();

        m_mQuad = lc0.getQuad();
        m_vLinear = lc0.getLinear();
        m_bIsQuadratic = lc0.isQuadratic();
        m_bValid = lc0.isValid();
        m_dConst = lc0.m_dConst;
        return;
    }

    if (c < DM_TOLERANCE)
    {
        m_bValid = true;
        m_bIsQuadratic = true;
        m_mQuad(0, 0) = 0.;
        m_mQuad(0, 1) = 0.5;
        m_mQuad(1, 0) = 0.5;
        m_mQuad(1, 1) = 0.;
        m_vLinear(0) = 0.;
        m_vLinear(1) = 0.;
        m_dConst = 0.;
        rotate(angle);
        move(center);
        return;
    }
    double b2 = f * f - c * c;
    m_bValid = true;
    m_bIsQuadratic = true;
    m_mQuad(0, 0) = 1. / (c * c);
    m_mQuad(0, 1) = 0.;
    m_mQuad(1, 0) = 0.;
    m_mQuad(1, 1) = -1. / b2;
    m_vLinear(0) = 0.;
    m_vLinear(1) = 0.;
    m_dConst = -1.;
    rotate(angle);
    move(center);
    return;
}

Quadratic::Quadratic(const DmVector& point0, const DmVector& point1)
    : m_mQuad(QUADRATIC_MATRIX_SIZE, QUADRATIC_MATRIX_SIZE)
    , m_vLinear(QUADRATIC_MATRIX_SIZE)
    , m_dConst(0.0)
    , m_bIsQuadratic(false)
    , m_bValid(false)
{
    DmVector vStart = (point0 + point1) * 0.5;
    DmVector vEnd = vStart + (point0 - vStart).rotate(0.5 * M_PI);
    *this = DmLine(vStart, vEnd).getQuadratic();
}

std::vector<double> Quadratic::getCoefficients() const
{
    std::vector<double> ret(0, 0.);
    if (isValid() == false)
    {
        return ret;
    }
    if (m_bIsQuadratic)
    {
        ret.push_back(m_mQuad(0, 0));
        ret.push_back(m_mQuad(0, 1) + m_mQuad(1, 0));
        ret.push_back(m_mQuad(1, 1));
    }
    ret.push_back(m_vLinear(0));
    ret.push_back(m_vLinear(1));
    ret.push_back(m_dConst);
    return ret;
}

Quadratic Quadratic::move(const DmVector& v)
{
    // 关于移动旋转的变换，参考：https://chat.deepseek.com/share/hh1utdlwjoke5pz3bp
    if (m_bValid == false || v.valid == false)
    {
        return *this;
    }

    m_dConst -= m_vLinear(0) * v.x + m_vLinear(1) * v.y;

    if (m_bIsQuadratic)
    {
        m_vLinear(0) -= 2. * m_mQuad(0, 0) * v.x + (m_mQuad(0, 1) + m_mQuad(1, 0)) * v.y;
        m_vLinear(1) -= 2. * m_mQuad(1, 1) * v.y + (m_mQuad(0, 1) + m_mQuad(1, 0)) * v.x;
        m_dConst += m_mQuad(0, 0) * v.x * v.x + (m_mQuad(0, 1) + m_mQuad(1, 0)) * v.x * v.y + m_mQuad(1, 1) * v.y * v.y;
    }
    return *this;
}

Quadratic Quadratic::rotate(const double& angle)
{
    // 关于移动旋转的变换，参考：https://chat.deepseek.com/share/hh1utdlwjoke5pz3bp
    using namespace boost::numeric::ublas;
    auto Rt = rotationMatrix(angle); // 实际是旋转矩阵的转置
    auto R = trans(Rt);
    m_vLinear = prod(m_vLinear, Rt);
    if (m_bIsQuadratic)
    {
        m_mQuad = prod(m_mQuad, Rt);
        m_mQuad = prod(R, m_mQuad);
    }
    return *this;
}

Quadratic Quadratic::rotate(const DmVector& center, const double& angle)
{
    move(-center);
    rotate(angle);
    move(center);
    return *this;
}

Quadratic Quadratic::flipXY(void) const
{
    Quadratic qf(*this);
    if (isQuadratic())
    {
        std::swap(qf.m_mQuad(0, 0), qf.m_mQuad(1, 1));
        std::swap(qf.m_mQuad(0, 1), qf.m_mQuad(1, 0));
    }
    std::swap(qf.m_vLinear(0), qf.m_vLinear(1));
    return qf;
}

DmVectorSolutions Quadratic::getIntersection(const Quadratic& l1, const Quadratic& l2)
{
    DmVectorSolutions ret;
    if (l1 == false || l2 == false)
    {
        return ret;
    }
    auto p1 = &l1;
    auto p2 = &l2;
    if (p1->isQuadratic() == false) // 二次曲线放前面
    {
        std::swap(p1, p2);
    }

    // p1,p2均为线性
    if (p1->isQuadratic() == false)
    {
        std::vector<std::vector<double>> ce(2, std::vector<double>(3, 0.));
        ce[0][0] = p1->m_vLinear(0);
        ce[0][1] = p1->m_vLinear(1);
        ce[0][2] = -p1->m_dConst;
        ce[1][0] = p2->m_vLinear(0);
        ce[1][1] = p2->m_vLinear(1);
        ce[1][2] = -p2->m_dConst;
        std::vector<double> sn(2, 0.);
        if (Math2d::linearSolver(ce, sn))
        {
            ret.push_back(DmVector(sn[0], sn[1]));
        }
        return ret;
    }

    // p1为二次式，p2为线性
    if (p2->isQuadratic() == false)
    {
        if (fabs(p2->m_vLinear(0)) + DBL_EPSILON < fabs(p2->m_vLinear(1)))
        {
            ret = getIntersection(p1->flipXY(), p2->flipXY()).flipXY();
            return ret;
        }
        std::vector<std::vector<double>> ce(0);
        if (fabs(p2->m_vLinear(1)) < DM_TOLERANCE)
        {
            const double angle = 0.25 * M_PI;
            Quadratic p11(*p1);
            Quadratic p22(*p2);
            ce.push_back(p11.rotate(angle).getCoefficients());
            ce.push_back(p22.rotate(angle).getCoefficients());
            ret = Math2d::simultaneousQuadraticSolverMixed(ce);
            ret.rotate(-angle);
            return ret;
        }
        ce.push_back(p1->getCoefficients());
        ce.push_back(p2->getCoefficients());
        ret = Math2d::simultaneousQuadraticSolverMixed(ce);
        return ret;
    }

    // 其他
    if (fabs(p1->m_mQuad(0, 0)) < DM_TOLERANCE && fabs(p1->m_mQuad(0, 1)) < DM_TOLERANCE
        && fabs(p2->m_mQuad(0, 0)) < DM_TOLERANCE && fabs(p2->m_mQuad(0, 1)) < DM_TOLERANCE)
    {
        if (fabs(p1->m_mQuad(1, 1)) < DM_TOLERANCE && fabs(p2->m_mQuad(1, 1)) < DM_TOLERANCE)
        {
            // linear
            std::vector<double> ce(0);
            ce.push_back(p1->m_vLinear(0));
            ce.push_back(p1->m_vLinear(1));
            ce.push_back(p1->m_dConst);
            Quadratic lc10(ce);
            ce.clear();
            ce.push_back(p2->m_vLinear(0));
            ce.push_back(p2->m_vLinear(1));
            ce.push_back(p2->m_dConst);
            Quadratic lc11(ce);
            return getIntersection(lc10, lc11);
        }
        return getIntersection(p1->flipXY(), p2->flipXY()).flipXY();
    }
    std::vector<std::vector<double>> ce(0);
    ce.push_back(p1->getCoefficients());
    ce.push_back(p2->getCoefficients());
    if (DEBUG->getLevel() >= Debug::D_INFORMATIONAL)
    {
        DEBUG_HEADER
            std::cout << *p1 << std::endl;
        std::cout << *p2 << std::endl;
    }
    auto sol = Math2d::simultaneousQuadraticSolverFull(ce);
    bool valid = sol.size() > 0;
    for (auto& v : sol)
    {
        if (v.magnitude() >= DM_MAXDOUBLE)
        {
            valid = false;
            break;
        }
    }
    if (valid)
    {
        return sol;
    }
    ce.clear();
    ce.push_back(p1->getCoefficients());
    ce.push_back(p2->getCoefficients());
    sol = Math2d::simultaneousQuadraticSolverFull(ce);
    ret.clear();
    for (auto const& v : sol)
    {
        if (v.magnitude() <= DM_MAXDOUBLE)
        {
            ret.push_back(v);
            if (DEBUG->getLevel() >= Debug::D_INFORMATIONAL)
            {
                DEBUG_HEADER
                    std::cout << v << std::endl;
            }
        }
    }
    return ret;
}

bool Quadratic::isEntityArcCircleLine(const DmEntity* e) const
{
    switch (e->getEntityType())
    {
    case DM::EntityArc:
    case DM::EntityCircle:
    case DM::EntityLine:
    case DM::EntityPoint:
        return true;
    default:
        return false;
    }
}

boost::numeric::ublas::matrix<double> Quadratic::rotationMatrix(const double& angle)
{
    boost::numeric::ublas::matrix<double> ret(QUADRATIC_MATRIX_SIZE, QUADRATIC_MATRIX_SIZE);
    ret(0, 0) = cos(angle);
    ret(0, 1) = sin(angle);
    ret(1, 0) = -ret(0, 1);
    ret(1, 1) = ret(0, 0);
    return ret;
}

// Dumps the point's data to stdout.
std::ostream& operator<<(std::ostream& os, const Quadratic& q)
{
    os << " quadratic form: ";
    if (q == false)
    {
        os << " invalid quadratic form" << std::endl;
        return os;
    }
    os << std::endl;
    auto ce = q.getCoefficients();
    unsigned short i = 0;
    if (ce.size() == QUADRATIC_EQ_COEFF_COUNT)
    {
        os << ce[0] << "*x^2 " << ((ce[1] >= 0.) ? "+" : " ") << ce[1] << "*x*y  " << ((ce[2] >= 0.) ? "+" : " ") << ce[2] << "*y^2 ";
        i = 3;
    }
    if (q.isQuadratic() && ce[i] >= 0.)
    {
        os << "+";
    }
    os << ce[i] << "*x " << ((ce[i + 1] >= 0.) ? "+" : " ") << ce[i + 1] << "*y " << ((ce[i + 2] >= 0.) ? "+" : " ") << ce[i + 2] << " == 0" << std::endl;
    return os;
}
