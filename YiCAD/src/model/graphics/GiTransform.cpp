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

/// @file GiTransform.cpp
/// @brief GiTransform 实现

#include "GiTransform.h"

#include <algorithm>
#include <cmath>

GiTransform::GiTransform(double a, double b, double c, double d, double tx, double ty)
    : m_a(a)
    , m_b(b)
    , m_c(c)
    , m_d(d)
    , m_tx(tx)
    , m_ty(ty)
{
}

GiTransform GiTransform::translation(const DmVector& offset)
{
    return GiTransform(1.0, 0.0, 0.0, 1.0, offset.x, offset.y);
}

GiTransform GiTransform::rotation(double angle, const DmVector& center)
{
    const double cosA = std::cos(angle);
    const double sinA = std::sin(angle);
    // p' = c + R(p - c)
    return GiTransform(cosA, sinA, -sinA, cosA,
                       center.x - (cosA * center.x - sinA * center.y),
                       center.y - (sinA * center.x + cosA * center.y));
}

GiTransform GiTransform::scaling(const DmVector& factor, const DmVector& center)
{
    return GiTransform(factor.x, 0.0, 0.0, factor.y,
                       center.x - factor.x * center.x,
                       center.y - factor.y * center.y);
}

GiTransform GiTransform::mirroring(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    const double dx = axisPoint2.x - axisPoint1.x;
    const double dy = axisPoint2.y - axisPoint1.y;
    const double len2 = dx * dx + dy * dy;
    if (len2 <= 0.0)
    {
        return GiTransform();
    }
    // 反射矩阵 [[cos2θ, sin2θ], [sin2θ, -cos2θ]]，θ 为轴的方向角
    const double cos2 = (dx * dx - dy * dy) / len2;
    const double sin2 = 2.0 * dx * dy / len2;
    const DmVector& p = axisPoint1;
    return GiTransform(cos2, sin2, sin2, -cos2,
                       p.x - (cos2 * p.x + sin2 * p.y),
                       p.y - (sin2 * p.x - cos2 * p.y));
}

DmVector GiTransform::apply(const DmVector& point) const
{
    return DmVector(m_a * point.x + m_c * point.y + m_tx,
                    m_b * point.x + m_d * point.y + m_ty);
}

DmVector GiTransform::applyVector(const DmVector& vector) const
{
    return DmVector(m_a * vector.x + m_c * vector.y,
                    m_b * vector.x + m_d * vector.y);
}

GiTransform GiTransform::operator*(const GiTransform& rhs) const
{
    return GiTransform(m_a * rhs.m_a + m_c * rhs.m_b,
                       m_b * rhs.m_a + m_d * rhs.m_b,
                       m_a * rhs.m_c + m_c * rhs.m_d,
                       m_b * rhs.m_c + m_d * rhs.m_d,
                       m_a * rhs.m_tx + m_c * rhs.m_ty + m_tx,
                       m_b * rhs.m_tx + m_d * rhs.m_ty + m_ty);
}

GiTransform GiTransform::inverse() const
{
    const double det = determinant();
    if (det == 0.0)
    {
        return GiTransform();
    }
    const double ia = m_d / det;
    const double ib = -m_b / det;
    const double ic = -m_c / det;
    const double id = m_a / det;
    return GiTransform(ia, ib, ic, id,
                       -(ia * m_tx + ic * m_ty),
                       -(ib * m_tx + id * m_ty));
}

double GiTransform::determinant() const
{
    return m_a * m_d - m_b * m_c;
}

bool GiTransform::isIdentity(double tolerance) const
{
    return std::abs(m_a - 1.0) <= tolerance && std::abs(m_b) <= tolerance
        && std::abs(m_c) <= tolerance && std::abs(m_d - 1.0) <= tolerance
        && std::abs(m_tx) <= tolerance && std::abs(m_ty) <= tolerance;
}

bool GiTransform::isSimilarity(double* scale, double tolerance) const
{
    // 两列正交且等长
    const double lenX = std::hypot(m_a, m_b);
    const double lenY = std::hypot(m_c, m_d);
    const double ref = std::max(lenX, lenY);
    if (ref <= 0.0)
    {
        return false;
    }
    const double dot = m_a * m_c + m_b * m_d;
    const bool similar = std::abs(lenX - lenY) <= tolerance * ref
        && std::abs(dot) <= tolerance * ref * ref;
    if (similar && scale)
    {
        *scale = lenX;
    }
    return similar;
}

bool GiTransform::operator==(const GiTransform& other) const
{
    return m_a == other.m_a && m_b == other.m_b && m_c == other.m_c && m_d == other.m_d
        && m_tx == other.m_tx && m_ty == other.m_ty;
}
