/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


/// @file DmBoundingBox.h
/// @brief 包围框类，用于表示二维轴对齐包围框

#ifndef DMBOUNDINGBOX_H
#define DMBOUNDINGBOX_H

#include "DmVector.h"

class DmBoundingBox
{
public:
    DmBoundingBox() = default;

    /// @brief 通过两个点构造包围框
    /// @param pt1 第一个点
    /// @param pt2 第二个点
    DmBoundingBox(const DmVector& pt1, const DmVector& pt2);

    /// @brief 添加一个点到包围框中，扩展包围框以包含该点
    /// @param pt 要添加的点
    void add(const DmVector& pt);

    /// @brief 判断是否完全包含另一个包围框，边界重合也算包含
    /// @param box 要检查的包围框
    /// @return 如果完全包含则返回true
    bool contains(const DmBoundingBox& box) const;

    /// @brief 判断是否与另一个包围框相交，边界重合算相交
    /// @param box 要检查的包围框
    /// @return 如果相交则返回true
    bool isCross(const DmBoundingBox& box) const;

    /// @brief 判断点是否在包围框内，边界上算内部
    /// @param pt 要检查的点
    /// @return 如果点在包围框内则返回true
    bool isPointInside(const DmVector& pt) const;

    /// @brief 获取包围框的最小点坐标
    /// @return 最小点坐标
    DmVector getMin() const;

    /// @brief 获取包围框的最大点坐标
    /// @return 最大点坐标
    DmVector getMax() const;

protected:
    DmVector min{false}; ///< 包围框最小点
    DmVector max{false}; ///< 包围框最大点
};

#endif //DMBOUNDINGBOX_H
