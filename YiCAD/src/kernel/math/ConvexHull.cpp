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

/// @file ConvexHull.cpp
/// @brief 凸包类实现

#include "ConvexHull.h"
#include <algorithm>
#include <stack>

constexpr double CONVEX_TOLERANCE = 1e-9; ///< 共线/重叠容差

ConvexHull::ConvexHull(const std::vector<DmVector>& pts)
{
    points = pts;
    needsUpdate = true;
}

ConvexHull::ConvexHull(const DmVector& min, const DmVector& max)
{
    // 逆时针构建
    addPoint(min);
    addPoint(max.x, min.y);
    addPoint(max);
    addPoint(min.x, max.y);
    needsUpdate = true;
}

void ConvexHull::addPoint(double x, double y)
{
    points.emplace_back(x, y);
    needsUpdate = true;
}

void ConvexHull::addPoint(const DmVector& p)
{
    points.push_back(p);
    needsUpdate = true;
}

const std::vector<DmVector>& ConvexHull::getPoints() const
{
    return points;
}

bool ConvexHull::containsPoint(const DmVector& p)
{
    const auto& hull = getHull();
    int n = static_cast<int>(hull.size());

    if (n < 3)
    {
        return false;
    }

    // 检查点是否在多边形内
    for (int i = 0; i < n; i++)
    {
        DmVector p1 = hull[i];
        DmVector p2 = hull[(i + 1) % n];

        // 检查点是否在边上
        if (std::abs(cross(p1, p2, p)) < CONVEX_TOLERANCE)
        {
            double minX = std::min(p1.x, p2.x);
            double maxX = std::max(p1.x, p2.x);
            double minY = std::min(p1.y, p2.y);
            double maxY = std::max(p1.y, p2.y);

            if (p.x >= minX - CONVEX_TOLERANCE && p.x <= maxX + CONVEX_TOLERANCE &&
                p.y >= minY - CONVEX_TOLERANCE && p.y <= maxY + CONVEX_TOLERANCE)
            {
                return true;
            }
        }

        // 如果叉积为负，说明点在当前边的右侧（对于逆时针凸包）
        if (cross(p1, p2, p) < -CONVEX_TOLERANCE)
        {
            return false;
        }
    }
    return true;
}

bool ConvexHull::intersects(ConvexHull& other)
{
    const auto& hull1 = getHull();
    const auto& hull2 = other.getHull();

    // 检查一个凸包的点是否在另一个凸包内
    for (const auto& p : hull1)
    {
        if (other.containsPoint(p))
        {
            return true;
        }
    }
    for (const auto& p : hull2)
    {
        if (containsPoint(p))
        {
            return true;
        }
    }

    // 检查边是否相交
    int n1 = static_cast<int>(hull1.size());
    int n2 = static_cast<int>(hull2.size());

    for (int i = 0; i < n1; i++)
    {
        DmVector a1 = hull1[i];
        DmVector a2 = hull1[(i + 1) % n1];

        for (int j = 0; j < n2; j++)
        {
            DmVector b1 = hull2[j];
            DmVector b2 = hull2[(j + 1) % n2];

            if (doSegmentsIntersect(a1, a2, b1, b2))
            {
                return true;
            }
        }
    }

    return false;
}

double ConvexHull::cross(const DmVector& O, const DmVector& A, const DmVector& B)
{
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

double ConvexHull::distSq(const DmVector& p1, const DmVector& p2)
{
    return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
}

void ConvexHull::updateHull()
{
    const int MIN_POINTS_FOR_HULL = 3;

    if (points.size() < MIN_POINTS_FOR_HULL)
    {
        hull = points;
        return;
    }

    // 找到最左下角的点
    int minIdx = 0;
    for (size_t i = 1; i < points.size(); i++)
    {
        if (points[i].y < points[minIdx].y ||
            (std::abs(points[i].y - points[minIdx].y) < CONVEX_TOLERANCE &&
             points[i].x < points[minIdx].x))
        {
            minIdx = static_cast<int>(i);
        }
    }
    std::swap(points[0], points[minIdx]);

    // 按极角排序
    DmVector pivot = points[0];
    std::sort(points.begin() + 1, points.end(),
              [&pivot, this](const DmVector& a, const DmVector& b)
              {
                  double orientation = cross(pivot, a, b);
                  if (std::abs(orientation) < CONVEX_TOLERANCE)
                  {
                      return distSq(pivot, a) < distSq(pivot, b);
                  }
                  else
                  {
                      return orientation > 0;
                  }
              });

    // Graham Scan 算法
    std::stack<DmVector> st;
    st.push(points[0]);
    st.push(points[1]);
    st.push(points[2]);

    for (size_t i = 3; i < points.size(); i++)
    {
        while (st.size() >= 2)
        {
            DmVector top = st.top();
            st.pop();
            DmVector nextTop = st.top();

            if (cross(nextTop, top, points[i]) > CONVEX_TOLERANCE)
            {
                st.push(top);
                break;
            }
        }
        st.push(points[i]);
    }

    // 将栈中的点转换为向量
    hull.clear();
    while (!st.empty())
    {
        hull.push_back(st.top());
        st.pop();
    }
    std::reverse(hull.begin(), hull.end());

    needsUpdate = false;
}

const std::vector<DmVector>& ConvexHull::getHull()
{
    if (needsUpdate)
    {
        updateHull();
    }
    return hull;
}

bool ConvexHull::doSegmentsIntersect(const DmVector& p1, const DmVector& p2,
                                     const DmVector& q1, const DmVector& q2)
{
    double cross1 = cross(p1, p2, q1);
    double cross2 = cross(p1, p2, q2);
    double cross3 = cross(q1, q2, p1);
    double cross4 = cross(q1, q2, p2);

    // 检查一般情况下的相交
    if (cross1 * cross2 < -CONVEX_TOLERANCE && cross3 * cross4 < -CONVEX_TOLERANCE)
    {
        return true;
    }

    // 检查共线情况下的重叠
    if (std::abs(cross1) < CONVEX_TOLERANCE && onSegment(p1, p2, q1))
    {
        return true;
    }
    if (std::abs(cross2) < CONVEX_TOLERANCE && onSegment(p1, p2, q2))
    {
        return true;
    }
    if (std::abs(cross3) < CONVEX_TOLERANCE && onSegment(q1, q2, p1))
    {
        return true;
    }
    if (std::abs(cross4) < CONVEX_TOLERANCE && onSegment(q1, q2, p2))
    {
        return true;
    }

    return false;
}

bool ConvexHull::onSegment(const DmVector& p, const DmVector& q, const DmVector& r)
{
    if (std::abs(cross(p, q, r)) > CONVEX_TOLERANCE)
    {
        return false;
    }

    return (r.x <= std::max(p.x, q.x) + CONVEX_TOLERANCE &&
            r.x >= std::min(p.x, q.x) - CONVEX_TOLERANCE &&
            r.y <= std::max(p.y, q.y) + CONVEX_TOLERANCE &&
            r.y >= std::min(p.y, q.y) - CONVEX_TOLERANCE);
}
