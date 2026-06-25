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

/// @file Math2d.cpp
/// @brief 二维数学工具类实现

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/math/special_functions/ellint_2.hpp>
#include <boost/math/tools/polynomial.hpp>
#include <boost/math/tools/roots.hpp>
#include <boost/math/tools/quartic_roots.hpp>

#include <cmath>
#include <muParser.h>
#include <QString>
#include <QDebug>

#include "Math2d.h"
#include "DmVector.h"
#include "Debug.h"

namespace
{
    constexpr double M_PI_X2 = M_PI * 2;             ///< 2*PI
    constexpr double ANGLE_READABLE_TOLERANCE = 0.001; ///< 角度可读性容差
    constexpr double QUADRATIC_TOLERANCE = 1e-24;      ///< 二次方程求解容差
    constexpr double CUBIC_TOLERANCE_P = 1.0e-75;      ///< 三次方程P容差
    constexpr double QUARTIC_TOLERANCE_R = 1.0e-75;    ///< 四次方程R容差
    constexpr int NEWTON_ITERATIONS = 20;              ///< 牛顿法迭代次数
}

// Rounds the given double to the closest int.
int Math2d::round(double v)
{
    return static_cast<int>(lrint(v));
}

double Math2d::round(double v, double precision)
{
    if (precision == 0.0)
    {
        return v;
    }
    double nd = v / precision;
    double n = round(nd);
    return n * precision;
}

// Save pow function
double Math2d::pow(double x, double y)
{
    errno = 0;
    double ret = ::pow(x, y);
    if (errno == EDOM)
    {
        ret = 0.0;
    }
    else if (errno == ERANGE)
    {
        ret = 0.0;
    }
    return ret;
}

// pow of vector components
DmVector Math2d::pow(DmVector vp, double y)
{
    return DmVector(pow(vp.x, y), pow(vp.y, y));
}

// Save equal function for real types
bool Math2d::equal(const double d1, const double d2)
{
    return fabs(d1 - d2) < DM_TOLERANCE;
}

// Converts radians to degrees.
double Math2d::rad2deg(double a)
{
    return 180. / M_PI * a;
}

// Converts degrees to radians.
double Math2d::deg2rad(double a)
{
    return M_PI / 180.0 * a;
}

// Converts radians to gradians.
double Math2d::rad2gra(double a)
{
    return 200. / M_PI * a;
}

double Math2d::gra2rad(double a)
{
    return M_PI / 200. * a;
}

// Finds greatest common divider using Euclid's algorithm.
unsigned Math2d::findGCD(unsigned a, unsigned b)
{
    while (b)
    {
        unsigned rem = a % b;
        a = b;
        b = rem;
    }
    return a;
}

/// Tests if angle a is between a1 and a2. a, a1 and a2 must be in the
/// range between 0 and 2*PI.
/// All angles in rad.
/// @return true if the angle a is between a1 and a2.
bool Math2d::isAngleBetween(double a, double a1, double a2)
{
    if (getAngleDifferenceU(a2, a1) < DM_TOLERANCE_ANGLE)
    {
        return true;
    }
    const double tol = 0.5 * DM_TOLERANCE_ANGLE;
    const double diff0 = correctAngle(a2 - a1) + tol;

    return diff0 >= correctAngle(a - a1) || diff0 >= correctAngle(a2 - a);
}

// Corrects the given angle to the range of 0 to +PI*2.0.
double Math2d::correctAngle(double a)
{
    return fmod(M_PI + remainder(a - M_PI, M_PI_X2), M_PI_X2);
}

// Corrects the given angle to the range of -PI to +PI.
double Math2d::correctAngle2(double a)
{
    return remainder(a, M_PI_X2);
}

// Returns the given angle as an Unsigned Angle in the range of 0 to +PI.
double Math2d::correctAngleU(double a)
{
    return fabs(remainder(a, M_PI_X2));
}

/// @return The angle that needs to be added to a1 to reach a2. Always positive and less than 2*pi.
double Math2d::getAngleDifference(double a1, double a2, bool reversed)
{
    if (reversed)
    {
        std::swap(a1, a2);
    }
    return correctAngle(a2 - a1);
}

double Math2d::getAngleDifferenceU(double a1, double a2)
{
    return correctAngleU(a1 - a2);
}

/// Makes a text constructed with the given angle readable. Used for dimension texts and for mirroring texts.
/// @param readable true: make angle readable, false: unreadable
/// @param corrected Will point to true if the given angle was corrected, false otherwise.
/// @return The given angle or the given angle+PI, depending which on is readable from the bottom or right.
double Math2d::makeAngleReadable(double angle, bool readable, bool* corrected)
{
    double ret = correctAngle(angle);

    bool cor = isAngleReadable(ret) ^ readable;

    // quadrant 1 & 4
    if (cor)
    {
        ret = correctAngle(angle + M_PI);
    }

    if (corrected)
    {
        *corrected = cor;
    }

    return ret;
}

/// @return true: if the given angle is in a range that is readable for texts created with that angle.
bool Math2d::isAngleReadable(double angle)
{
    if (angle > M_PI_2)
    {
        return fabs(remainder(angle, M_PI_X2)) < (M_PI_2 - ANGLE_READABLE_TOLERANCE);
    }
    else
    {
        return fabs(remainder(angle, M_PI_X2)) < (M_PI_2 + ANGLE_READABLE_TOLERANCE);
    }
}

/// @param tol Tolerance in rad.
/// @retval true The two angles point in the same direction.
bool Math2d::isSameDirection(double dir1, double dir2, double tol)
{
    return getAngleDifferenceU(dir1, dir2) < tol;
}

// Evaluates a mathematical expression and returns the result.
// If an error occurred, the given default value 'def' will be returned.
double Math2d::eval(const QString& expr, double def)
{
    bool ok;
    double res = Math2d::eval(expr, &ok);

    if (!ok)
    {
        return def;
    }

    return res;
}

// Evaluates a mathematical expression and returns the result.
// If an error occurred, ok will be set to false (if ok isn't NULL).
double Math2d::eval(const QString& expr, bool* ok)
{
    bool okTmp(false);
    if (!ok)
    {
        ok = &okTmp;
    }
    if (expr.isEmpty())
    {
        *ok = false;
        return 0.0;
    }
    double ret(0.);
    try
    {
        mu::Parser p;
        p.DefineConst(_T("pi"), M_PI);
#ifdef _UNICODE
        p.SetExpr(expr.toStdWString());
#else
        p.SetExpr(expr.toStdString());
#endif
        ret = p.Eval();
        *ok = true;
    }
    catch (mu::Parser::exception_type& e)
    {
        mu::console() << e.GetMsg() << std::endl;
        *ok = false;
    }
    return ret;
}

/// Converts a double into a string which is as short as possible
/// @param value The double value
/// @param prec Precision e.g. a precision of 1 would mean that a value of 2.12030 will be converted to "2.1". 2.000 is always just "2").
QString Math2d::doubleToString(double value, double prec)
{
    if (prec < DM_TOLERANCE)
    {
        return QString().setNum(value, prec);
    }

    double const num = Math2d::round(value / prec) * prec;

    QString exaStr = Math2d::doubleToString(1. / prec, 10);
    int const dotPos = exaStr.indexOf('.');

    if (dotPos == -1)
    {
        // big numbers for the precision
        return QString().setNum(Math2d::round(num));
    }
    else
    {
        // number of digits after the point
        int digits = dotPos - 1;
        return Math2d::doubleToString(num, digits);
    }
}

/// Converts a double into a string which is as short as possible.
/// @param value The double value
/// @param prec Precision
QString Math2d::doubleToString(double value, int prec)
{
    QString valStr;

    valStr.setNum(value, 'f', prec);

    if (valStr.contains('.'))
    {
        if (valStr.at(valStr.length() - 1) == '.')
        {
            valStr.truncate(valStr.length() - 1);
        }
    }

    return valStr;
}

std::vector<double> Math2d::quadraticSolver(const std::vector<double>& ce)
{
    std::vector<double> ans(0, 0.);
    if (ce.size() != 2)
    {
        return ans;
    }
    using LDouble = long double;
    LDouble const b = -0.5L * ce[0];
    LDouble const c = ce[1];
    LDouble const b2 = b * b;
    LDouble const discriminant = b2 - c;
    LDouble const fc = std::abs(c);
    LDouble const TOL = QUADRATIC_TOLERANCE;

    if (discriminant < 0.L)
    {
        return ans;
    }

    LDouble r;

    if (b2 >= fc)
    {
        r = std::abs(b) * std::sqrt(1.L - c / b2);
    }
    else
    {
        r = std::sqrt(fc) * std::sqrt(1.L + b2 / fc);
    }

    if (r >= TOL * std::abs(b))
    {
        if (b >= 0.L)
        {
            ans.push_back(b + r);
        }
        else
        {
            ans.push_back(b - r);
        }

        ans.push_back(c / ans.front());
    }
    else
    {
        ans.push_back(b);
    }
    return ans;
}

std::vector<double> Math2d::cubicSolver(const std::vector<double>& ce)
{
    std::vector<double> ans(0, 0.);
    if (ce.size() != 3)
    {
        return ans;
    }
    double shift = (1. / 3) * ce[0];
    double p = ce[1] - shift * ce[0];
    double q = ce[0] * ((2. / 27) * ce[0] * ce[0] - (1. / 3) * ce[1]) + ce[2];

    double discriminant = (1. / 27) * p * p * p + (1. / 4) * q * q;
    if (fabs(p) < CUBIC_TOLERANCE_P)
    {
        ans.push_back((q > 0) ? -pow(q, (1. / 3)) : pow(-q, (1. / 3)));
        ans[0] -= shift;
        return ans;
    }
    if (discriminant > 0)
    {
        std::vector<double> ce2(2, 0.);
        ce2[0] = q;
        ce2[1] = -1. / 27 * p * p * p;
        auto r = quadraticSolver(ce2);
        if (r.size() == 0)
        {
            std::cerr << __FILE__ << " : " << __func__ << " : line" << __LINE__ << " :cubicSolver()::Error cubicSolver(" << ce[0] << ' ' << ce[1] << ' ' << ce[2] << ")\n";
        }
        double u = 0.0;
        double v = 0.0;
        u = (q <= 0) ? pow(r[0], 1. / 3) : -pow(-r[1], 1. / 3);
        v = (-1. / 3) * p / u;
        ans.push_back(u + v - shift);
    }
    else
    {
        std::complex<double> u(q, 0);
        std::complex<double> rt[3];
        u = std::pow(-0.5 * u - sqrt(0.25 * u * u + p * p * p / 27), 1. / 3);
        rt[0] = u - p / (3. * u) - shift;
        std::complex<double> w(-0.5, sqrt(3.) / 2);
        rt[1] = u * w - p / (3. * u * w) - shift;
        rt[2] = u / w - p * w / (3. * u) - shift;
        ans.push_back(rt[0].real());
        ans.push_back(rt[1].real());
        ans.push_back(rt[2].real());
    }
    for (double& x0 : ans)
    {
        double dx = 0.;
        for (size_t i = 0; i < NEWTON_ITERATIONS; ++i)
        {
            double f = ((x0 + ce[0]) * x0 + ce[1]) * x0 + ce[2];
            double df = (3. * x0 + 2. * ce[0]) * x0 + ce[1];
            if (fabs(df) > fabs(f) + DM_TOLERANCE)
            {
                dx = f / df;
                x0 -= dx;
            }
            else
            {
                break;
            }
        }
    }

    return ans;
}

std::vector<double> Math2d::quarticSolver(const std::vector<double>& ce)
{
    std::vector<double> ans(0, 0.);
    if (ce.size() != 4)
    {
        return ans;
    }
    double shift = 0.25 * ce[0];
    double shift2 = shift * shift;
    double a2 = ce[0] * ce[0];
    double p = ce[1] - (3. / 8) * a2;
    double q = ce[2] + ce[0] * ((1. / 8) * a2 - 0.5 * ce[1]);
    double r = ce[3] - shift * ce[2] + (ce[1] - 3. * shift2) * shift2;
    if (q * q <= 1.e-4 * DM_TOLERANCE * fabs(p * r))
    {
        double discriminant = 0.25 * p * p - r;
        if (discriminant < -1.e3 * DM_TOLERANCE)
        {
            return ans;
        }
        double t2[2];
        t2[0] = -0.5 * p - sqrt(fabs(discriminant));
        t2[1] = -p - t2[0];
        if (t2[1] >= 0.)
        {
            ans.push_back(sqrt(t2[1]) - shift);
            ans.push_back(-sqrt(t2[1]) - shift);
        }
        if (t2[0] >= 0.)
        {
            ans.push_back(sqrt(t2[0]) - shift);
            ans.push_back(-sqrt(t2[0]) - shift);
        }
        return ans;
    }
    if (fabs(r) < QUARTIC_TOLERANCE_R)
    {
        std::vector<double> cubic(3, 0.);
        cubic[1] = p;
        cubic[2] = q;
        ans.push_back(0.);
        auto r = cubicSolver(cubic);
        std::copy(r.begin(), r.end(), std::back_inserter(ans));
        for (size_t i = 0; i < ans.size(); i++)
        {
            ans[i] -= shift;
        }
        return ans;
    }

    std::vector<double> cubic(3, 0.);
    cubic[0] = 2. * p;
    cubic[1] = p * p - 4. * r;
    cubic[2] = -q * q;
    auto r3 = cubicSolver(cubic);
    if (r3.size() == 1)
    {
        if (r3[0] < 0.)
        {
            DEBUG_HEADER
                qDebug() << "Quartic Error:: Found one real root for cubic, but negative\n";
            return ans;
        }
        double sqrtz0 = sqrt(r3[0]);
        std::vector<double> ce2(2, 0.);
        ce2[0] = -sqrtz0;
        ce2[1] = 0.5 * (p + r3[0]) + 0.5 * q / sqrtz0;
        auto r1 = quadraticSolver(ce2);
        if (r1.size() == 0)
        {
            ce2[0] = sqrtz0;
            ce2[1] = 0.5 * (p + r3[0]) - 0.5 * q / sqrtz0;
            r1 = quadraticSolver(ce2);
        }
        for (auto& x : r1)
        {
            x -= shift;
        }
        return r1;
    }
    if (r3[0] > 0. && r3[1] > 0.)
    {
        double sqrtz0 = sqrt(r3[0]);
        std::vector<double> ce2(2, 0.);
        ce2[0] = -sqrtz0;
        ce2[1] = 0.5 * (p + r3[0]) + 0.5 * q / sqrtz0;
        ans = quadraticSolver(ce2);
        ce2[0] = sqrtz0;
        ce2[1] = 0.5 * (p + r3[0]) - 0.5 * q / sqrtz0;
        auto r1 = quadraticSolver(ce2);
        std::copy(r1.begin(), r1.end(), std::back_inserter(ans));
        for (auto& x : ans)
        {
            x -= shift;
        }
    }
    for (double& x0 : ans)
    {
        double dx = 0.;
        for (size_t i = 0; i < NEWTON_ITERATIONS; ++i)
        {
            double f = (((x0 + ce[0]) * x0 + ce[1]) * x0 + ce[2]) * x0 + ce[3];
            double df = ((4. * x0 + 3. * ce[0]) * x0 + 2. * ce[1]) * x0 + ce[2];
            if (fabs(df) > DM_TOLERANCE2)
            {
                dx = f / df;
                x0 -= dx;
            }
            else
            {
                break;
            }
        }
    }

    return ans;
}

bool Math2d::linearSolver(const std::vector<std::vector<double>>& mt, std::vector<double>& sn)
{
    size_t mSize(mt.size()); // rows
    size_t aSize(mSize + 1); // columns of augmented matrix
    if (std::any_of(mt.begin(), mt.end(), [&aSize](const std::vector<double>& v) -> bool { return v.size() != aSize; }))
    {
        return false;
    }
    sn.resize(mSize); // to hold the solution

    // solve the linear equation by Gauss-Jordan elimination
    std::vector<std::vector<double>> mt0(mt); // copy the matrix;
    for (size_t i = 0; i < mSize; ++i)
    {
        size_t imax(i);
        double cmax(fabs(mt0[i][i]));
        for (size_t j = i + 1; j < mSize; ++j)
        {
            if (fabs(mt0[j][i]) > cmax)
            {
                imax = j;
                cmax = fabs(mt0[j][i]);
            }
        }
        if (cmax < DM_TOLERANCE2)
        {
            return false;
        }
        if (imax != i)
        {
            std::swap(mt0[i], mt0[imax]);
        }
        for (size_t k = i + 1; k <= mSize; ++k)
        {
            mt0[i][k] /= mt0[i][i];
        }
        mt0[i][i] = 1.;
        for (size_t j = 0; j < mSize; ++j)
        {
            if (j != i)
            {
                double& a = mt0[j][i];
                for (size_t k = i + 1; k <= mSize; ++k)
                {
                    mt0[j][k] -= mt0[i][k] * a;
                }
                a = 0.;
            }
        }
    }
    for (size_t i = 0; i < mSize; ++i)
    {
        sn[i] = mt0[i][mSize];
    }

    return true;
}

/// wrapper of elliptic integral of the second type, Legendre form
/// @param k the elliptic modulus or eccentricity
/// @param phi elliptic angle, must be within range of [0, M_PI]
double Math2d::ellipticIntegral_2(const double& k, const double& phi)
{
    double a = remainder(phi - M_PI_2, M_PI);
    if (a > 0.)
    {
        return boost::math::ellint_2<double, double>(k, a);
    }
    else
    {
        return -boost::math::ellint_2<double, double>(k, fabs(a));
    }
}

DmVectorSolutions Math2d::simultaneousQuadraticSolver(const std::vector<double>& m)
{
    DmVectorSolutions ret(0);
    if (m.size() != 8)
    {
        return ret;
    }
    std::vector<double> c1(0, 0.);
    std::vector<std::vector<double>> m1(0, c1);
    c1.resize(6);
    c1[0] = m[0];
    c1[1] = 0.;
    c1[2] = m[1];
    c1[3] = 0.;
    c1[4] = 0.;
    c1[5] = -1.;
    m1.push_back(c1);
    c1[0] = m[2];
    c1[1] = 2. * m[3];
    c1[2] = m[4];
    c1[3] = m[5];
    c1[4] = m[6];
    c1[5] = m[7];
    m1.push_back(c1);

    return simultaneousQuadraticSolverFull(m1);
}

DmVectorSolutions Math2d::simultaneousQuadraticSolverFull(const std::vector<std::vector<double>>& m)
{
    using boost::math::tools::polynomial;
    using namespace boost::math::tools;
    DmVectorSolutions ret;
    if (m.size() != 2)
    {
        return ret;
    }
    if (m[0].size() == 3 || m[1].size() == 3)
    {
        return simultaneousQuadraticSolverMixed(m);
    }
    if (m[0].size() != 6 || m[1].size() != 6)
    {
        return ret;
    }

    auto& a = m[0][0];
    auto& b = m[0][1];
    auto& c = m[0][2];
    auto& d = m[0][3];
    auto& e = m[0][4];
    auto& f = m[0][5];

    auto& g = m[1][0];
    auto& h = m[1][1];
    auto& i = m[1][2];
    auto& j = m[1][3];
    auto& k = m[1][4];
    auto& l = m[1][5];

    double a2 = a * a;
    double b2 = b * b;
    double c2 = c * c;
    double d2 = d * d;
    double e2 = e * e;
    double f2 = f * f;

    double g2 = g * g;
    double h2 = h * h;
    double i2 = i * i;
    double j2 = j * j;
    double k2 = k * k;
    double l2 = l * l;
    std::vector<double> qy(5, 0.);
    // y^4
    qy[4] = -c2 * g2 + b * c * g * h - a * c * h2 - b2 * g * i + 2. * a * c * g * i + a * b * h * i - a2 * i2;
    // y^3
    qy[3] = -2. * c * e * g2 + c * d * g * h + b * e * g * h - a * e * h2 - 2. * b * d * g * i + 2. * a * e * g * i + a * d * h * i +
        b * c * g * j - 2. * a * c * h * j + a * b * i * j - b2 * g * k + 2. * a * c * g * k + a * b * h * k - 2. * a2 * i * k;
    // y^2
    qy[2] = (-e2 * g2 + d * e * g * h - d2 * g * i + c * d * g * j + b * e * g * j - 2. * a * e * h * j + a * d * i * j - a * c * j2 -
        2. * b * d * g * k + 2. * a * e * g * k + a * d * h * k + a * b * j * k - a2 * k2 - b2 * g * l + 2. * a * c * g * l + a * b * h * l - 2. * a2 * i * l)
        - (2. * c * f * g2 - b * f * g * h + a * f * h2 - 2. * a * f * g * i);
    // y
    qy[1] = (d * e * g * j - a * e * j2 - d2 * g * k + a * d * j * k - 2. * b * d * g * l + 2. * a * e * g * l + a * d * h * l + a * b * j * l - 2. * a2 * k * l)
        - (2. * e * f * g2 - d * f * g * h - b * f * g * j + 2. * a * f * h * j - 2. * a * f * g * k);
    // y^0
    qy[0] = -d2 * g * l + a * d * j * l - a2 * l2
        - (f2 * g2 - d * f * g * j + a * f * j2 - 2. * a * f * g * l);
    if (DEBUG->getLevel() >= Debug::D_INFORMATIONAL)
    {
        DEBUG_HEADER
            std::cout << qy[4] << "*y^4 +(" << qy[3] << ")*y^3+(" << qy[2] << ")*y^2+(" << qy[1] << ")*y+(" << qy[0] << ")==0" << std::endl;
    }

    // 一元四次方程求解
    std::array<double, 4> rootsArr = boost::math::tools::quartic_roots(qy[4], qy[3], qy[2], qy[1], qy[0]);
    std::vector<double> roots;  // 有效的值（实根）
    for (double root : rootsArr)
    {
        if (!std::isnan(root)) // quartic_roots 会用NaN填充非实根位置[citation:5]
        {
            roots.emplace_back(root);
        }
    }

    if (DEBUG->getLevel() >= Debug::D_INFORMATIONAL)
    {
        std::cout << "roots.size()= " << roots.size() << std::endl;
    }

    if (roots.size() == 0)
    {
        return ret;
    }
    std::vector<double> ce(0, 0.);

    for (size_t i0 = 0; i0 < roots.size(); i0++)
    {
        if (DEBUG->getLevel() >= Debug::D_INFORMATIONAL)
        {
            DEBUG_HEADER
                std::cout << "y=" << roots[i0] << std::endl;
        }
        ce.resize(3);
        ce[0] = a;
        ce[1] = b * roots[i0] + d;
        ce[2] = c * roots[i0] * roots[i0] + e * roots[i0] + f;
        if (fabs(ce[0]) < 1e-75 && fabs(ce[1]) < 1e-75)
        {
            ce[0] = g;
            ce[1] = h * roots[i0] + j;
            ce[2] = i * roots[i0] * roots[i0] + k * roots[i0] + f;
        }
        if (fabs(ce[0]) < 1e-75 && fabs(ce[1]) < 1e-75)
        {
            continue;
        }

        if (fabs(a) > 1e-75)
        {
            std::vector<double> ce2(2, 0.);
            ce2[0] = ce[1] / ce[0];
            ce2[1] = ce[2] / ce[0];
            auto xRoots = quadraticSolver(ce2);
            for (size_t j0 = 0; j0 < xRoots.size(); j0++)
            {
                DmVector vp(xRoots[j0], roots[i0]);
                if (simultaneousQuadraticVerify(m, vp))
                {
                    ret.push_back(vp);
                }
            }
            continue;
        }
        DmVector vp(-ce[2] / ce[1], roots[i0]);
        if (simultaneousQuadraticVerify(m, vp))
        {
            ret.push_back(vp);
        }
    }
    if (DEBUG->getLevel() >= Debug::D_INFORMATIONAL)
    {
        DEBUG_HEADER
            std::cout << "ret=" << ret << std::endl;
    }
    return ret;
}

DmVectorSolutions Math2d::simultaneousQuadraticSolverMixed(const std::vector<std::vector<double>>& m)
{
    DmVectorSolutions ret;
    auto p0 = &(m[0]);
    auto p1 = &(m[1]);
    if (p1->size() == 3)
    {
        std::swap(p0, p1);
    }
    if (p1->size() == 3)
    {
        std::vector<double> sn(2, 0.);
        std::vector<std::vector<double>> ce;
        ce.push_back(m[0]);
        ce.push_back(m[1]);
        ce[0][2] = -ce[0][2];
        ce[1][2] = -ce[1][2];
        if (Math2d::linearSolver(ce, sn))
        {
            ret.push_back(DmVector(sn[0], sn[1]));
        }
        return ret;
    }
    const double& a = p0->at(0);
    const double& b = p0->at(1);
    const double& c = p0->at(2);
    const double& d = p1->at(0);
    const double& e = p1->at(1);
    const double& f = p1->at(2);
    const double& g = p1->at(3);
    const double& h = p1->at(4);
    const double& i = p1->at(5);
    std::vector<double> ce(3, 0.);
    const double& a2 = a * a;
    const double& b2 = b * b;
    const double& c2 = c * c;
    ce[0] = -f * a2 + a * b * e - b2 * d;
    ce[1] = a * b * g - a2 * h - (2 * b * c * d - a * c * e);
    ce[2] = a * c * g - c2 * d - a2 * i;
    std::vector<double> roots(0, 0.);
    if (fabs(ce[1]) > DM_TOLERANCE15 && fabs(ce[0] / ce[1]) < DM_TOLERANCE15)
    {
        roots.push_back(-ce[2] / ce[1]);
    }
    else
    {
        std::vector<double> ce2(2, 0.);
        ce2[0] = ce[1] / ce[0];
        ce2[1] = ce[2] / ce[0];
        roots = quadraticSolver(ce2);
    }

    if (roots.size() == 0)
    {
        return DmVectorSolutions();
    }
    for (size_t i0 = 0; i0 < roots.size(); i0++)
    {
        ret.push_back(DmVector(-(b * roots.at(i0) + c) / a, roots.at(i0)));
    }

    return ret;
}

bool Math2d::simultaneousQuadraticVerify(const std::vector<std::vector<double>>& m, DmVector& v)
{
    DmVector v0 = v;
    auto& a = m[0][0];
    auto& b = m[0][1];
    auto& c = m[0][2];
    auto& d = m[0][3];
    auto& e = m[0][4];
    auto& f = m[0][5];

    auto& g = m[1][0];
    auto& h = m[1][1];
    auto& i = m[1][2];
    auto& j = m[1][3];
    auto& k = m[1][4];
    auto& l = m[1][5];

    double sum0 = 0., sum1 = 0.;
    double f00 = 0., f01 = 0.;
    double amax0 = 0.0;
    double amax1 = 0.0;
    for (size_t nIter = 0; nIter < NEWTON_ITERATIONS; ++nIter)
    {
        double& x = v.x;
        double& y = v.y;
        double x2 = x * x;
        double y2 = y * y;
        double const terms0[12] = { a * x2, b * x * y, c * y2, d * x, e * y, f, g * x2, h * x * y, i * y2, j * x, k * y, l };
        amax0 = fabs(terms0[0]);
        amax1 = fabs(terms0[6]);
        double px = 2. * a * x + b * y + d;
        double py = b * x + 2. * c * y + e;
        sum0 = 0.;
        for (int i0 = 0; i0 < 6; i0++)
        {
            if (amax0 < fabs(terms0[i0]))
            {
                amax0 = fabs(terms0[i0]);
            }
            sum0 += terms0[i0];
        }
        std::vector<std::vector<double>> nrCe;
        nrCe.push_back(std::vector<double>{px, py, sum0});
        px = 2. * g * x + h * y + j;
        py = h * x + 2. * i * y + k;
        sum1 = 0.;
        for (int i0 = 6; i0 < 12; i0++)
        {
            if (amax1 < fabs(terms0[i0]))
            {
                amax1 = fabs(terms0[i0]);
            }
            sum1 += terms0[i0];
        }
        nrCe.push_back(std::vector<double>{px, py, sum1});
        std::vector<double> dn;
        bool ret = linearSolver(nrCe, dn);
        if (!nIter)
        {
            f00 = sum0;
            f01 = sum1;
        }
        if (!ret)
        {
            break;
        }
        v -= DmVector(dn[0], dn[1]);
    }
    if (fabs(sum0) > fabs(f00) && fabs(sum1) > fabs(f01))
    {
        v = v0;
        sum0 = f00;
        sum1 = f01;
    }

    const double tols = 2. * sqrt(6.) * sqrt(DBL_EPSILON);

    return (amax0 <= tols || fabs(sum0) / amax0 < tols) && (amax1 <= tols || fabs(sum1) / amax1 < tols);
}
