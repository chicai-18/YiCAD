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

/// @file GiNurbs.h
/// @brief GI 的 B 样条曲线及其离散

#ifndef GINURBS_H
#define GINURBS_H

#include <vector>

#include "DmVector.h"

/// @brief B 样条曲线：次数、节点向量、控制点
/// @details 与 SplineData 的表示相同：闭合样条的控制点已按周期追加了 degree 个（均匀 B 样条），
///          开放样条用准均匀节点。weights 为空表示非有理样条（YiCAD 的样条都是非有理的）
struct GiNurbs
{
    int degree = 0;
    std::vector<double> knots;
    std::vector<DmVector> controlPoints;
    std::vector<double> weights;
    bool closed = false;

    /// @brief 表示是否有效：次数、控制点数与节点数满足定义域要求
    bool isValid() const;

    /// @brief 定义域 [t(k), t(n+1)]，无效时不改动输出
    void domain(double& t1, double& t2) const;

    /// @brief 参数 t 处的点
    DmVector evaluate(double t) const;

    /// @brief 参数 t 处的 q 阶导数；t 在定义域外时返回无效向量
    DmVector derivative(double t, int q = 1) const;

    /// @brief 离散成折线，追加到 pts
    /// @details 每段节点区间先等分 degree × 5 份，再递归细分到相邻切线夹角不超过 3°；
    ///          一次曲线直接取控制点。原为 DmSpline::getPoints()，样条与旧渲染器的适配器共用这一份
    void sample(std::vector<DmVector>& pts) const;

private:
    int segmentCount() const;
    int findSpan(double t) const;
    double basis(double t, int i, int k) const;
    double basisDerivative(double t, int i, int k, int q) const;
    void sampleRecursive(double t1, double t2, double count, std::vector<DmVector>& pts, double maxStep) const;
};

#endif // GINURBS_H
