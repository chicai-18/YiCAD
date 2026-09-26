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

/// @file PointData.h
/// @brief 点数据结构类，定义点的位置坐标

#ifndef POINTDATA_H
#define POINTDATA_H

#include "EntityData.h"
#include "DmVector.h"

/// @brief 点数据结构
class PointData : public EntityData
{
public:
    /// @brief 默认构造函数
    PointData();

    /// @brief 带位置参数的构造函数
    /// @param pos 点坐标
    PointData(const DmVector& pos);

public:
    /// @brief 获取位置
    /// @return 点坐标
    DmVector getPosition() const;

    /// @brief 设置位置
    /// @param pt 点坐标
    void setPosition(const DmVector& pt);

private:
    DmVector    m_pPosition;    ///< 定位点坐标
};

#endif // POINTDATA_H
