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

/// @file ConvexHull.h
/// @brief 凸包类

#ifndef CONVEXHULL_H
#define CONVEXHULL_H

#include "DmVector.h"

/// @brief 凸包类(来自AI)
class ConvexHull
{
public:
    ConvexHull() : needsUpdate(false) {}

    /// @brief 通过点列表构建
    /// @param [in] pts 点列表
    explicit ConvexHull(const std::vector<DmVector>& pts);

    /// @brief 通过包围框构建
    /// @param [in] min 包围框最小点
    /// @param [in] max 包围框最大点
    explicit ConvexHull(const DmVector& min, const DmVector& max);

    /// @brief 添加点
    /// @param [in] x 点的X坐标
    /// @param [in] y 点的Y坐标
    void addPoint(double x, double y);

    /// @brief 添加点
    /// @param [in] p 点坐标
    void addPoint(const DmVector& p);

    /// @brief 获取所有点
    /// @return 点的const引用
    const std::vector<DmVector>& getPoints() const;

    /// @brief 判断点是否在凸包内（包括边界）
    /// @param [in] p 测试点
    /// @return 在凸包内返回true
    bool containsPoint(const DmVector& p);

    /// @brief 判断两个凸包是否相交
    /// @param [in] other 另一个凸包
    /// @return 相交返回true
    bool intersects(ConvexHull& other);

    /// @brief 更新凸包
    void updateHull();

    /// @brief 获取当前凸包（如果需要更新则先更新）
    /// @return 凸包点的const引用
    const std::vector<DmVector>& getHull();

private:
    /// @brief 计算叉积
    /// @param [in] O 原点
    /// @param [in] A 终点A
    /// @param [in] B 终点B
    /// @return 叉积值
    double cross(const DmVector& O, const DmVector& A, const DmVector& B);

    /// @brief 计算两点距离的平方
    /// @param [in] p1 第一点
    /// @param [in] p2 第二点
    /// @return 距离的平方
    double distSq(const DmVector& p1, const DmVector& p2);

    /// @brief 判断两个线段是否相交
    /// @param [in] p1 线段1起点
    /// @param [in] p2 线段1终点
    /// @param [in] q1 线段2起点
    /// @param [in] q2 线段2终点
    /// @return 相交返回true
    bool doSegmentsIntersect(const DmVector& p1, const DmVector& p2,
                             const DmVector& q1, const DmVector& q2);

    /// @brief 检查点r是否在线段pq上
    /// @param [in] p 线段起点
    /// @param [in] q 线段终点
    /// @param [in] r 测试点
    /// @return 在线上返回true
    bool onSegment(const DmVector& p, const DmVector& q, const DmVector& r);

private:
    std::vector<DmVector> points;   ///< 添加的顶点
    std::vector<DmVector> hull;     ///< 凸包的有序顶点
    bool needsUpdate;               ///< 是否需要更新
};

#endif //CONVEXHULL_H
