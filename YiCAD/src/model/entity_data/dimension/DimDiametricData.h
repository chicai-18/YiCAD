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

/// @file DimDiametricData.h
/// @brief 直径标注数据类

#ifndef DIMDIAMETRICDATA_H
#define DIMDIAMETRICDATA_H

#include "DimensionData.h"
#include "DmVector.h"

/// @brief 直径标注
class DimDiametricData : public DimensionData
{
public:
    DimDiametricData();

    /// @brief 获取直径第一点
    /// @return 直径第一点坐标
    DmVector getDiameter1Point() const;

    /// @brief 设置直径第一点
    /// @param [in] pt 坐标点
    void setDiameter1Point(const DmVector& pt);

    /// @brief 获取直径第二点
    /// @return 直径第二点坐标
    DmVector getDiameter2Point() const;

    /// @brief 设置直径第二点
    /// @param [in] pt 坐标点
    void setDiameter2Point(const DmVector& pt);

    /// @brief 获取引线长度
    /// @return 引线长度
    double getLeaderLength() const;

    /// @brief 设置引线长度
    /// @param [in] dLeaderLength 引线长度
    void setLeaderLength(const double dLeaderLength);

private:
    DmVector m_ptDiameter1Point; ///< 直径第一点
    DmVector m_ptDiameter2Point; ///< 直径第二点
    double   m_dLeaderLength;    ///< 引线长度
};

#endif // DIMDIAMETRICDATA_H
