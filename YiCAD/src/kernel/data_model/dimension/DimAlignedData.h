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

/// @file DimAlignedData.h
/// @brief 对齐标注数据类

#ifndef DIMALIGNEDDATA_H
#define DIMALIGNEDDATA_H

#include "DimensionData.h"
#include "DmVector.h"

/// @brief 对齐标注
class DimAlignedData : public DimensionData
{
public:
    DimAlignedData();

    /// @brief 获取旋转角度
    /// @return 旋转角度
    double getAngle() const;

    /// @brief 设置旋转角度
    /// @param [in] dAngle 旋转角度
    void setAngle(const double& dAngle);

    /// @brief 获取第一条尺寸界线起点
    /// @return 尺寸界线第一点坐标
    DmVector getXLine1Point() const;

    /// @brief 设置第一条尺寸界线起点
    /// @param [in] pt 坐标点
    void setXLine1Point(const DmVector& pt);

    /// @brief 获取第二条尺寸界线起点
    /// @return 尺寸界线第二点坐标
    DmVector getXLine2Point() const;

    /// @brief 设置第二条尺寸界线起点
    /// @param [in] pt 坐标点
    void setXLine2Point(const DmVector& pt);

    /// @brief 获取中心线点
    /// @return 中心线点坐标
    DmVector getMidLinePoint() const;

    /// @brief 设置中心线点
    /// @param [in] pt 坐标点
    void setMidLinePoint(const DmVector& pt);

    /// @brief 获取倾斜角度
    /// @return 倾斜角度
    double getOblique() const;

    /// @brief 设置倾斜角度
    /// @param [in] dOblique 倾斜角度
    void setOblique(const double& dOblique);

private:
    double   m_dAngle;         ///< 旋转角度
    DmVector m_ptXLine1Point;  ///< 第一点坐标
    DmVector m_ptXLine2Point;  ///< 第二点坐标
    DmVector m_ptMidLinePoint; ///< 中心线坐标
    double   m_dOblique;       ///< 标注图元的倾斜角度
};

#endif // DIMALIGNEDDATA_H
