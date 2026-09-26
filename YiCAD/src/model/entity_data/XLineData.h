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

/// @file XLineData.h
/// @brief 构造线(双向射线)数据类

#ifndef XLINEDATA_H
#define XLINEDATA_H

#include "EntityData.h"
#include "DmVector.h"

/// @brief 构造线(双向射线)
class XLineData : public EntityData
{
public:
    XLineData();

    /// @brief 使用基点和方向构造
    /// @param [in] startPt 基点
    /// @param [in] dir 方向向量
    XLineData(const DmVector& startPt, const DmVector& dir);

    /// @brief 获取基点
    /// @return 基点坐标
    DmVector getBasePoint() const;

    /// @brief 设置基点
    /// @param [in] pt 基点坐标
    void setBasePoint(const DmVector& pt);

    /// @brief 获取方向向量
    /// @return 方向向量
    DmVector getDirection() const;

    /// @brief 设置方向向量
    /// @param [in] pt 方向向量
    void setDirection(const DmVector& pt);

private:
    DmVector m_ptBasePoint;  ///< 起点
    DmVector m_ptDirection;  ///< 方向点
};

#endif // XLINEDATA_H
