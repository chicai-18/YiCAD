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

/// @file DimRadialData.h
/// @brief 半径标注数据类

#ifndef DIMRADIALDATA_H
#define DIMRADIALDATA_H

#include "DimensionData.h"
#include "DmVector.h"

/// @brief 半径标注
class DimRadialData : public DimensionData
{
public:
    DimRadialData();

    /// @brief 获取圆心点
    /// @return 圆心点坐标
    DmVector getCenterPoint() const;

    /// @brief 设置圆心点
    /// @param [in] pt 坐标点
    void setCenterPoint(const DmVector& pt);

    /// @brief 获取自定义位置点
    /// @return 自定义位置点坐标
    DmVector getDiameterPoint() const;

    /// @brief 设置自定义位置点
    /// @param [in] pt 坐标点
    void setDiameterPoint(const DmVector& pt);

    /// @brief 获取引线长度
    /// @return 引线长度
    double getLeaderLength() const;

    /// @brief 设置引线长度
    /// @param [in] length 引线长度
    void setLeaderLength(const double length);

private:
    DmVector m_ptCenterPoint;   ///< 中点
    DmVector m_ptDiameterPoint; ///< 自定义位置点
    double   m_dLeaderLength;   ///< 引线长度
};

#endif // DIMRADIALDATA_H
