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

/// @file GiNurbs.cpp
/// @brief GiNurbs 实现；算法与原 DmSpline 的求值、离散一致

#include "GiNurbs.h"

#include <cmath>

#include "Math2d.h"

namespace
{
constexpr double TOL = DM_TOLERANCE;
}

bool GiNurbs::isValid() const
{
    const int k = degree;
    const int size = static_cast<int>(controlPoints.size());
    if (k < 1)
    {
        return false;
    }
    if (!closed && size < k + 1)
    {
        return false;
    }
    // 闭合的情况会追加k个控制点，这里保证闭合满足绘制条件
    if (closed && size - k < k + 1)
    {
        return false;
    }
    if (closed && size - k < 3)
    {
        return false;
    }
    // 定义域范围有效
    const int n = size - 1;
    return k < n + 1;
}

void GiNurbs::domain(double& t1, double& t2) const
{
    if (!isValid())
    {
        return;
    }
    // 定义域：[t(k), t(n+1))
    const int k = degree;
    const int n = static_cast<int>(controlPoints.size()) - 1;
    if (k < n + 1)
    {
        t1 = knots.at(k);
        t2 = knots.at(n + 1);
    }
}

int GiNurbs::segmentCount() const
{
    if (!isValid())
    {
        return 0;
    }
    const int k = degree;
    const int n = static_cast<int>(controlPoints.size()) - 1;
    return n - k + 1;
}

int GiNurbs::findSpan(double t) const
{
    // 参考《非均匀有理B样条  第2版》P49
    double t1, t2;
    domain(t1, t2);
    if (t < t1 || t > t2) //避免后面的死循环
    {
        return -1;
    }

    const int k = degree;
    const int n = static_cast<int>(controlPoints.size()) - 1;
    if (t == knots.at(n + 1)) //特殊情况
    {
        return n;
    }
    int low = k;
    int high = n + 1;
    int mid = (low + high) / 2;
    //进行二分搜索
    while (t < knots.at(mid) || t >= knots.at(mid + 1))
    {
        if (t < knots.at(mid))
        {
            high = mid;
        }
        else
        {
            low = mid;
        }
        mid = (low + high) / 2;
    }
    return mid;
}

double GiNurbs::basis(double t, int i, int k) const
{
    // 参考《计算几何算法与实现 _ Visual C++版- 孔令德 》 P155
    // 通过de Boor地推定义计算
    double v1, v2, v = 0.0;
    if (k == 0)
    {
        if (t >= knots.at(i) && t < knots.at(i + 1))
        {
            return 1.0;
        }
        else
        {
            return 0.0;
        }
    }
    if (k > 0)
    {
        if (t < knots.at(i) || t > knots.at(i + k + 1))
        {
            return 0.0;
        }
        else
        {
            double c1, c2;
            double d = knots.at(i + k) - knots.at(i);
            if (0.0 == d)        //约定0/0
            {
                c1 = 0.0;
            }
            else
            {
                c1 = (t - knots.at(i)) / d;   //Fi,k-1(t)
            }

            d = knots.at(i + k + 1) - knots.at(i + 1);
            if (0.0 == d)
            {
                c2 = 0.0;
            }
            else
            {
                c2 = (knots.at(i + k + 1) - t) / d;   //Fi+1,k-1(t)
            }
            v1 = c1 * basis(t, i, k - 1);
            v2 = c2 * basis(t, i + 1, k - 1);
            v = v1 + v2;
        }
    }
    return v;
}

double GiNurbs::basisDerivative(double t, int i, int k, int q) const
{
    // 参考《非均匀有理B样条  第2版》P42 式2.7、P44 式2.9
    double N1 = 0.0;
    double N2 = 0.0;
    if (q == 1)
    {
        N1 = basis(t, i, k - 1);
        N2 = basis(t, i + 1, k - 1);
    }
    else
    {
        N1 = basisDerivative(t, i, k - 1, q - 1);
        N2 = basisDerivative(t, i + 1, k - 1, q - 1);
    }
    const double deno1 = knots.at(i + k) - knots.at(i);
    const double deno2 = knots.at(i + k + 1) - knots.at(i + 1);
    double a1 = 0.0;
    if (deno1 != 0.0)
    {
        a1 = 1.0 / deno1;
    }
    double a2 = 0.0;
    if (deno2 != 0.0)
    {
        a2 = 1.0 / deno2;
    }
    return (a1 * N1 - a2 * N2) * (double)k;
}

DmVector GiNurbs::evaluate(double t) const
{
    double tStart, tEnd;
    domain(tStart, tEnd);
    if (t == tEnd && !closed)// 上界特殊处理
    {
        return controlPoints.at(controlPoints.size() - 1);
    }
    // 计算有作用的样条基范围found_i
    const int k = degree;   //k次B样条曲线
    DmVector res;   //无效值
    const int found_i = findSpan(t);   //t所在节点区间的索引
    if (found_i == -1)
    {
        return res;
    }

    // 根据有作用的样条基，累加各项值（至多与[P(i-k),P(i-k+1)...,P(i)]控制点有关，参考《计算机辅助几何设计与非均匀有理B样条  修订版》P222）
    DmVector pt(0.0, 0.0);
    for (int i = found_i - k; i <= found_i; i++)
    {
        const double v = basis(t, i, k);
        pt += controlPoints.at(i) * v;
    }
    return pt;
}

DmVector GiNurbs::derivative(double t, int q) const
{
    // 参考《非均匀有理B样条  第2版》P66，式3.3
    double t1, t2;
    domain(t1, t2);
    if (t < (t1 - TOL) || t > (t2 + TOL))
    {
        return DmVector(false);
    }
    if (t >= t2 - TOL) //靠近上边界
    {
        t -= TOL; //上边界避免求出为0
    }

    const int n = static_cast<int>(controlPoints.size()) - 1;
    const int k = degree;
    DmVector res(true);
    for (int i = 0; i <= n; i++)
    {
        const DmVector& pt = controlPoints.at(i);
        const double N = basisDerivative(t, i, k, q);
        res += (pt * N);
    }
    return res;
}

void GiNurbs::sample(std::vector<DmVector>& pts) const
{
    const int k = degree;   //k次B样条曲线
    // 包含n+k+1个节点区间，即t的范围为[t0,t1,...,t(m+k+1)]。节点个数(c+1)与n,k应满足:c = n+k+1
    // B样条曲线的定义域为[tk, t(n+1)]
    const int count = segmentCount();   //n-k+1段B样条曲线
    for (int i = 0; i < count; i++)
    {
        const double t1 = knots.at(k + i);
        const double t2 = knots.at(k + i + 1);
        if (std::abs(t2 - t1) < TOL)  //重复的节点
        {
            continue;
        }
        // 1次曲线在连接处不可导，直接取控制点
        if (k == 1)
        {
            pts.emplace_back(controlPoints.at(i));
        }
        // 2次及以上，递归求点（点的切线夹角变化不大于某个值）
        else
        {
            const int parts = k * 5;    //一段B样条分成多段，计算对应的点坐标，每个控制点之间的段至少分5段，防止S型变直线
            const double step = (t2 - t1) / (double)parts;
            for (int j = 0; j < parts; j++)
            {
                const double tmpT1 = t1 + step * j;
                const double tmpT2 = t1 + step * (j + 1);
                sampleRecursive(tmpT1, tmpT2, (double)parts, pts, step / 100.0);
            }
        }

        //最后一段连接上
        if (i == count - 1)
        {
            // 闭合的情况采用均匀B样条，最后一个点通过t计算正确
            if (closed)
            {
                pts.emplace_back(evaluate(t2));   //对于闭合情况上边界获得的点正确
            }
            // 非闭合的情况采用准均匀B样条，计算时应算作最后一个定义域区间，且0阶基函数计算值为1.0，但通过basis()获得0.0值，
            // 这里不修改basis，直接采用最后一个控制点
            else
            {
                pts.emplace_back(controlPoints.at(controlPoints.size() - 1));
            }
        }
    }
}

void GiNurbs::sample(std::vector<DmVector>& pts, double tolerance) const
{
    const int k = degree;
    const int count = segmentCount();
    for (int i = 0; i < count; i++)
    {
        const double t1 = knots.at(k + i);
        const double t2 = knots.at(k + i + 1);
        if (std::abs(t2 - t1) < TOL)  // 重复的节点
        {
            continue;
        }
        if (k == 1)
        {
            pts.emplace_back(controlPoints.at(i));
        }
        else
        {
            const int parts = k * 5;
            const double step = (t2 - t1) / static_cast<double>(parts);
            for (int j = 0; j < parts; j++)
            {
                const double a = t1 + step * j;
                const double b = t1 + step * (j + 1);
                sampleTolerance(a, evaluate(a), b, evaluate(b), tolerance, 0, pts);
            }
        }
        // 最后一段连接上，同 sample()
        if (i == count - 1)
        {
            if (closed)
            {
                pts.emplace_back(evaluate(t2));
            }
            else
            {
                pts.emplace_back(controlPoints.at(controlPoints.size() - 1));
            }
        }
    }
}

void GiNurbs::sampleTolerance(double t1, const DmVector& p1, double t2, const DmVector& p2, double tolerance, int depth,
                              std::vector<DmVector>& pts) const
{
    constexpr int kMaxDepth = 24;
    constexpr double kMaxTurn = M_PI / 6.0;  // 30°
    const double mid = (t1 + t2) * 0.5;
    const DmVector pm = evaluate(mid);
    // 中点到弦的距离
    const double cx = p2.x - p1.x;
    const double cy = p2.y - p1.y;
    const double chord = std::hypot(cx, cy);
    const double deviation = chord > 0.0 ? std::abs((pm.x - p1.x) * cy - (pm.y - p1.y) * cx) / chord
                                         : std::hypot(pm.x - p1.x, pm.y - p1.y);
    bool split = deviation > tolerance;
    if (!split)
    {
        const DmVector v1 = derivative(t1);
        const DmVector v2 = derivative(t2);
        split = v1.valid && v2.valid && std::abs(Math2d::correctAngle2(v1.angleToDir(v2))) > kMaxTurn;
    }
    if (split && depth < kMaxDepth)
    {
        sampleTolerance(t1, p1, mid, pm, tolerance, depth + 1, pts);
        sampleTolerance(mid, pm, t2, p2, tolerance, depth + 1, pts);
        return;
    }
    pts.emplace_back(p1);
}

void GiNurbs::sampleRecursive(double t1, double t2, double count, std::vector<DmVector>& pts, double maxStep) const
{
    if (std::abs(t2 - t1) < maxStep)
    {
        pts.emplace_back(evaluate(t1));
        return;
    }
    const DmVector v1 = derivative(t1);
    const DmVector v2 = derivative(t2);
    double angle = Math2d::correctAngle2(v1.angleToDir(v2));
    angle = std::abs(angle); // 俩向量夹角(<=PI)，外部分段保证满足此条件
    constexpr double MaxDelta = M_PI / 60.0; //3度
    if (angle > MaxDelta)
    {
        // 二分到每段转角不超过 3 度。原先一超就整段等分成 count（三次样条为 15）段再递归，
        // 转弯处一段就要 225 个点，比 3 度的要求密得多（RENDER_PLAN.md 第 10 节阶段 4：基准图纸的
        // 2 千条样条离散出 70 万个点，平移时的 GPU 开销大半在这里）
        const double mid = (t1 + t2) * 0.5;
        sampleRecursive(t1, mid, count, pts, maxStep);
        sampleRecursive(mid, t2, count, pts, maxStep);
    }
    else
    {
        pts.emplace_back(evaluate(t1));
    }
}
