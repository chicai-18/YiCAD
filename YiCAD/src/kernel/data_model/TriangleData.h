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

/// @file TriangleData.h
/// @brief 三角形数据类

#ifndef TRIANGLEDATA_H
#define TRIANGLEDATA_H

#include <array>
#include "DmVector.h"

class TriangleData
{
public:
    TriangleData() = default;

    /// @brief 使用三个顶点构造三角形
    /// @param [in] p0 顶点0
    /// @param [in] p1 顶点1
    /// @param [in] p2 顶点2
    TriangleData(const DmVector& p0, const DmVector& p1, const DmVector& p2);

    /// @brief 获取三角形三个顶点
    /// @return 三个顶点的数组
    std::array<DmVector, 3> getPoints() const;

    /// @brief 设置三角形三个顶点
    /// @param [in] pts 三个顶点的数组
    void setPoints(const std::array<DmVector, 3>& pts);

    /// @brief 获取指定索引的顶点
    /// @param [in] i 顶点索引 (0-2)
    /// @return 顶点坐标
    DmVector getPointAt(int i) const;

    /// @brief 设置指定索引的顶点
    /// @param [in] i 顶点索引 (0-2)
    /// @param [in] pt 顶点坐标
    void setPointAt(int i, const DmVector& pt);

protected:
    std::array<DmVector, 3> m_pts; ///< 三角形三个顶点
};

#endif // TRIANGLEDATA_H
