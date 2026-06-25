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

/// @file QuarticEquation.cpp
/// @brief 一元四次方程求解类实现

#include "QuarticEquation.h"
#include <cmath>
#include <cfloat>

constexpr double IMAG_TOLERANCE = 1e-10;          ///< 虚部容差，用于判实数解
constexpr double CUBE_ROOT_MULTIPLIER = 3.0;       ///< 三次方根乘数
constexpr double OMEGA_REAL = -0.5;                 ///< 复数单位根的实部
constexpr double OMEGA_IMAG = 0.86602540378443864676372317075294; ///< 复数单位根的虚部

QuarticEquation::QuarticEquation(double a, double b, double c, double d, double e)
    : m_a(a)
    , m_b(b)
    , m_c(c)
    , m_d(d)
    , m_e(e)
{
}

std::vector<double> QuarticEquation::getResults()
{
    std::complex<double> x[4];
    std::complex<double> a(m_a);
    std::complex<double> b(m_b);
    std::complex<double> c(m_c);
    std::complex<double> d(m_d);
    std::complex<double> e(m_e);
    Ferrari(x, a, b, c, d, e);

    std::vector<double> res;
    for (int i = 0; i < 4; i++)
    {
        double imag = x[i].imag();
        if (std::abs(imag) < IMAG_TOLERANCE)
        {
            res.emplace_back(x[i].real());
        }
    }

    return res;
}

/***************************************
对一个复数 x 开 n 次方
***************************************/
std::complex<double> QuarticEquation::sqrtn(const std::complex<double>& x, double n)
{
    double r = _hypot(x.real(), x.imag()); // 模
    if (r > 0.0)
    {
        double a = atan2(x.imag(), x.real()); // 辐角
        n = 1.0 / n;
        r = pow(r, n);
        a *= n;
        return std::complex<double>(r * cos(a), r * sin(a));
    }
    else
    {
        return std::complex<double>();
    }
}

/***************************************
使用费拉里法求解一元四次方程 a*x^4 + b*x^3 + c*x^2 + d*x + e = 0
***************************************/
void QuarticEquation::Ferrari(std::complex<double> x[4]
    , std::complex<double> a
    , std::complex<double> b
    , std::complex<double> c
    , std::complex<double> d
    , std::complex<double> e)
{
    a = 1.0 / a;
    b *= a;
    c *= a;
    d *= a;
    e *= a;
    std::complex<double> P = (c * c + 12.0 * e - 3.0 * b * d) / 9.0;
    std::complex<double> Q = (27.0 * d * d + 2.0 * c * c * c + 27.0 * b * b * e - 72.0 * c * e - 9.0 * b * c * d) / 54.0;
    std::complex<double> D = sqrtn(Q * Q - P * P * P, 2.0);
    std::complex<double> u = Q + D;
    std::complex<double> v = Q - D;
    if (v.real() * v.real() + v.imag() * v.imag() > u.real() * u.real() + u.imag() * u.imag())
    {
        u = sqrtn(v, CUBE_ROOT_MULTIPLIER);
    }
    else
    {
        u = sqrtn(u, CUBE_ROOT_MULTIPLIER);
    }
    std::complex<double> y;
    if (u.real() * u.real() + u.imag() * u.imag() > 0.0)
    {
        v = P / u;
        std::complex<double> o1(OMEGA_REAL, +OMEGA_IMAG);
        std::complex<double> o2(OMEGA_REAL, -OMEGA_IMAG);
        std::complex<double>& yMax = x[0];
        double m2 = 0.0;
        double m2Max = 0.0;
        int iMax = -1;
        for (int i = 0; i < 3; ++i)
        {
            y = u + v + c / CUBE_ROOT_MULTIPLIER;
            u *= o1;
            v *= o2;
            a = b * b + 4.0 * (y - c);
            m2 = a.real() * a.real() + a.imag() * a.imag();
            if (0 == i || m2Max < m2)
            {
                m2Max = m2;
                yMax = y;
                iMax = i;
            }
        }
        y = yMax;
    }
    else
    {
        // 一元三次方程，三重根
        y = c / CUBE_ROOT_MULTIPLIER;
    }
    std::complex<double> m = sqrtn(b * b + 4.0 * (y - c), 2.0);
    if (m.real() * m.real() + m.imag() * m.imag() >= DBL_MIN)
    {
        std::complex<double> n = (b * y - 2.0 * d) / m;
        a = sqrtn((b + m) * (b + m) - 8.0 * (y + n), 2.0);
        x[0] = (-(b + m) + a) / 4.0;
        x[1] = (-(b + m) - a) / 4.0;
        a = sqrtn((b - m) * (b - m) - 8.0 * (y - n), 2.0);
        x[2] = (-(b - m) + a) / 4.0;
        x[3] = (-(b - m) - a) / 4.0;
    }
    else
    {
        a = sqrtn(b * b - 8.0 * y, 2.0);
        x[0] =
            x[1] = (-b + a) / 4.0;
        x[2] =
            x[3] = (-b - a) / 4.0;
    }
}
