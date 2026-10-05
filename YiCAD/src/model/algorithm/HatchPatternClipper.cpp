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

/// @file HatchPatternClipper.cpp
/// @brief HatchPatternClipper 实现

#include "HatchPatternClipper.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{

constexpr double kTwoPi = 2.0 * M_PI;
constexpr double kBulgeTolerance = 1.0e-5;   ///< 与 DmPolyline 相同：凸度小于它的段是直线
constexpr double kMinRunLength = 1.0e-9;

double dot(const DmVector& a, const DmVector& b)
{
    return a.x * b.x + a.y * b.y;
}

/// @brief 环的一条边：直线段，或凸度给出的圆弧
struct Edge
{
    DmVector a;
    DmVector b;
    bool arc = false;
    DmVector center;
    double radius = 0.0;
    double startAngle = 0.0;   ///< 圆心到 a 的角度
    double sweep = 0.0;        ///< 从 a 到 b 的有向扫角（逆时针为正）
    double low = 0.0;          ///< 边在法向上的投影范围（快速排除）
    double high = 0.0;
};

/// @brief 半开规则的符号：在线上（0）算正侧
int sideOf(double f)
{
    return f >= 0.0 ? 1 : -1;
}

/// @brief 把环拆成边，并记下边界的范围
void collectEdges(std::span<const GiLoop> loops, const DmVector& normal, std::vector<Edge>& edges, DmVector& minCorner,
                  DmVector& maxCorner)
{
    minCorner = DmVector(std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    maxCorner = DmVector(-std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
    auto extend = [&](const DmVector& p, double r) {
        minCorner.x = std::min(minCorner.x, p.x - r);
        minCorner.y = std::min(minCorner.y, p.y - r);
        maxCorner.x = std::max(maxCorner.x, p.x + r);
        maxCorner.y = std::max(maxCorner.y, p.y + r);
    };
    for (const GiLoop& loop : loops)
    {
        const std::size_t n = loop.points.size();
        if (n < 2)
        {
            continue;
        }
        for (std::size_t i = 0; i < n; ++i)
        {
            Edge e;
            e.a = loop.points[i];
            e.b = loop.points[(i + 1) % n];
            const double bulge = i < loop.bulges.size() ? loop.bulges[i] : 0.0;
            const DmVector chord = e.b - e.a;
            const double c = std::hypot(chord.x, chord.y);
            if (std::fabs(bulge) >= kBulgeTolerance && c > 0.0)
            {
                // 圆心在弦中点沿左法向 (c/2)·cot(θ/2) 处，θ = 4·atan(凸度) 为有向扫角
                const DmVector left(-chord.y / c, chord.x / c);
                const DmVector middle = (e.a + e.b) * 0.5;
                e.arc = true;
                e.center = middle + left * (c * 0.5 * (1.0 - bulge * bulge) / (2.0 * bulge));
                e.radius = std::hypot(e.a.x - e.center.x, e.a.y - e.center.y);
                e.startAngle = std::atan2(e.a.y - e.center.y, e.a.x - e.center.x);
                e.sweep = 4.0 * std::atan(bulge);
                const double cn = dot(e.center, normal);
                e.low = cn - e.radius;
                e.high = cn + e.radius;
                extend(e.center, e.radius);
            }
            else
            {
                const double fa = dot(e.a, normal);
                const double fb = dot(e.b, normal);
                e.low = std::min(fa, fb);
                e.high = std::max(fa, fb);
                extend(e.a, 0.0);
            }
            edges.push_back(e);
        }
    }
}

/// @brief 圆弧边上参数 u ∈ [0, 1] 处的点
DmVector arcPoint(const Edge& e, double u)
{
    if (u <= 0.0)
    {
        return e.a;
    }
    if (u >= 1.0)
    {
        return e.b;
    }
    const double angle = e.startAngle + e.sweep * u;
    return DmVector(e.center.x + e.radius * std::cos(angle), e.center.y + e.radius * std::sin(angle));
}

/// @brief 圆弧边与一条图案线的交点（线上的位置 s）
/// @details 按符号序列计数：起点的符号、根之间各区间中点的符号、终点的符号，相邻两项不同就是一次穿过，
///          位置在它们的分界（起点、根、终点）。起止点的符号与相邻直线边用同一个半开规则，所以拼起来的环计数一致
void crossArc(const Edge& e, const DmVector& origin, const DmVector& dir, const DmVector& normal, double offset,
              std::vector<double>& crossings)
{
    auto side = [&](const DmVector& p) { return dot(p, normal) - offset; };
    // f(θ) = (C·n − c) + r·cos(θ − φ)
    const double g = dot(e.center, normal) - offset;
    const double phi = std::atan2(normal.y, normal.x);
    std::vector<double> roots;
    const double m = -g / e.radius;
    if (std::fabs(m) < 1.0)
    {
        const double alpha = std::acos(m);
        for (double theta : {phi + alpha, phi - alpha})
        {
            double delta = e.sweep >= 0.0 ? theta - e.startAngle : e.startAngle - theta;
            delta -= kTwoPi * std::floor(delta / kTwoPi);
            const double u = delta / std::fabs(e.sweep);
            if (u > 0.0 && u < 1.0)
            {
                roots.push_back(u);
            }
        }
        std::sort(roots.begin(), roots.end());
    }
    std::vector<double> breaks;
    breaks.reserve(roots.size() + 2);
    breaks.push_back(0.0);
    breaks.insert(breaks.end(), roots.begin(), roots.end());
    breaks.push_back(1.0);

    // 线与整个圆不相交或相切（|m| ≥ 1）时弧的内部同号，取圆心那一侧的符号：相切时切点可能正好落在区间中点上，在那里取值会得 0
    const bool noRoots = !(std::fabs(m) < 1.0);
    int previous = sideOf(side(e.a));
    for (std::size_t i = 0; i + 1 < breaks.size(); ++i)
    {
        const int current = noRoots ? sideOf(g) : sideOf(side(arcPoint(e, (breaks[i] + breaks[i + 1]) * 0.5)));
        if (current != previous)
        {
            crossings.push_back(dot(arcPoint(e, breaks[i]) - origin, dir));
        }
        previous = current;
    }
    if (sideOf(side(e.b)) != previous)
    {
        crossings.push_back(dot(e.b - origin, dir));
    }
}

}  // namespace

double HatchPatternClipper::period(const std::vector<double>& dashes)
{
    double p = 0.0;
    for (double d : dashes)
    {
        p += std::fabs(d);
    }
    return p;
}

void HatchPatternClipper::splitRun(const HatchPatternRun& run, const std::vector<double>& dashes,
                                   std::vector<std::pair<DmVector, DmVector>>& segments, std::vector<DmVector>& dots)
{
    const double p = period(dashes);
    const double length = run.start.distanceTo(run.end);
    if (dashes.empty() || !(p > 0.0) || !(length > 0.0))
    {
        segments.emplace_back(run.start, run.end);
        return;
    }
    const DmVector dir = (run.end - run.start) / length;
    for (double c = -run.phase; c < length; c += p)
    {
        double pos = c;
        for (double d : dashes)
        {
            if (d > 0.0)
            {
                const double s0 = std::max(pos, 0.0);
                const double s1 = std::min(pos + d, length);
                if (s1 > s0)
                {
                    segments.emplace_back(run.start + dir * s0, run.start + dir * s1);
                }
            }
            else if (d == 0.0 && pos >= 0.0 && pos <= length)
            {
                dots.push_back(run.start + dir * pos);
            }
            pos += std::fabs(d);
        }
    }
}

double HatchPatternClipper::spacing(const GiHatchPatternLine& line)
{
    const double len = std::hypot(line.direction.x, line.direction.y);
    if (!(len > 0.0))
    {
        return 0.0;
    }
    return std::fabs((line.offset.y * line.direction.x - line.offset.x * line.direction.y) / len);
}

bool HatchPatternClipper::clip(std::span<const GiLoop> loops, const GiHatchPatternLine& line,
                               std::vector<HatchPatternRun>& runs)
{
    const double len = std::hypot(line.direction.x, line.direction.y);
    if (!(len > 0.0) || !std::isfinite(len))
    {
        return false;
    }
    const DmVector dir(line.direction.x / len, line.direction.y / len);
    const DmVector normal(-dir.y, dir.x);
    // 相邻两条线在法向上的距离（有向）
    const double step = dot(line.offset, normal);
    if (!(std::fabs(step) > 1.0e-12 * std::max(1.0, std::hypot(line.offset.x, line.offset.y))))
    {
        return false;
    }

    std::vector<Edge> edges;
    DmVector minCorner;
    DmVector maxCorner;
    collectEdges(loops, normal, edges, minCorner, maxCorner);
    if (edges.empty())
    {
        return true;
    }

    // 与边界的范围相交的线：第 k 条线在法向上位于 base·n + k·step
    const double base = dot(line.base, normal);
    double tMin = std::numeric_limits<double>::max();
    double tMax = -std::numeric_limits<double>::max();
    for (const DmVector& corner : {minCorner, maxCorner, DmVector(minCorner.x, maxCorner.y), DmVector(maxCorner.x, minCorner.y)})
    {
        const double t = (dot(corner, normal) - base) / step;
        tMin = std::min(tMin, t);
        tMax = std::max(tMax, t);
    }
    const double kFirst = std::floor(tMin);
    const double kLast = std::ceil(tMax);
    if (!std::isfinite(kFirst) || !std::isfinite(kLast) || kLast - kFirst + 1.0 > kMaxLines)
    {
        return false;
    }

    const double patternPeriod = period(line.dashes);
    std::vector<double> crossings;
    for (double k = kFirst; k <= kLast; k += 1.0)
    {
        const DmVector origin = line.base + line.offset * k;
        const double offset = dot(origin, normal);
        crossings.clear();
        for (const Edge& e : edges)
        {
            if (e.high < offset || e.low > offset)
            {
                continue;
            }
            if (e.arc)
            {
                crossArc(e, origin, dir, normal, offset, crossings);
                continue;
            }
            const double fa = dot(e.a, normal) - offset;
            const double fb = dot(e.b, normal) - offset;
            if (sideOf(fa) != sideOf(fb))
            {
                const double t = fa / (fa - fb);
                const DmVector x = e.a + (e.b - e.a) * t;
                crossings.push_back(dot(x - origin, dir));
            }
        }
        if (crossings.size() < 2)
        {
            continue;
        }
        std::sort(crossings.begin(), crossings.end());
        for (std::size_t i = 0; i + 1 < crossings.size(); i += 2)
        {
            const double s0 = crossings[i];
            const double s1 = crossings[i + 1];
            if (s1 - s0 <= kMinRunLength)
            {
                continue;
            }
            HatchPatternRun run;
            run.start = origin + dir * s0;
            run.end = origin + dir * s1;
            run.phase = patternPeriod > 0.0 ? s0 - patternPeriod * std::floor(s0 / patternPeriod) : 0.0;
            runs.push_back(run);
        }
    }
    return true;
}
