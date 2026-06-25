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

/// @file RayData.h
/// @brief 射线数据结构类，定义射线的起点和方向向量

#ifndef RAYDATA_H
#define RAYDATA_H

#include "EntityData.h"
#include "DmVector.h"

/// @brief 射线数据结构
class RayData : public EntityData
{
public:
    /// @brief 默认构造函数
    RayData();

    /// @brief 带起始终止点的构造函数
    /// @param startPt 起点坐标
    /// @param dir 方向向量（起点到方向点的向量）
    RayData(const DmVector& startPt, const DmVector& dir);

public:
    /// @brief 获取起点
    /// @return 起点坐标
    DmVector getBasePoint() const;

    /// @brief 设置起点
    /// @param pt 起点坐标
    void setBasePoint(const DmVector& pt);

    /// @brief 获取方向向量
    /// @return 方向向量
    DmVector getDirection() const;

    /// @brief 设置方向向量
    /// @param pt 方向向量
    void setDirection(const DmVector& pt);

private:
    DmVector    m_ptBasePoint;      ///< 起点
    DmVector    m_ptDirection;      ///< 起点到方向点的向量
};

#endif // RAYDATA_H
