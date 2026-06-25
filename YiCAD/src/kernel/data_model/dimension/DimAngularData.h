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

/// @file DimAngularData.h
/// @brief 角度标注(两线)数据类

#ifndef DIMANGULARDATA_H
#define DIMANGULARDATA_H

#include "DimensionData.h"
#include "DmVector.h"

/// @brief 角度标注(两线)
class DimAngularData : public DimensionData
{
public:
    DimAngularData();

    /// @brief 获取第一条尺寸界线起点
    /// @return 第一条尺寸界线起点坐标
    DmVector getXLine1Start() const;

    /// @brief 设置第一条尺寸界线起点
    /// @param [in] pt 坐标点
    void setXLine1Start(const DmVector& pt);

    /// @brief 获取第一条尺寸界线终点
    /// @return 第一条尺寸界线终点坐标
    DmVector getXLine1End() const;

    /// @brief 设置第一条尺寸界线终点
    /// @param [in] pt 坐标点
    void setXLine1End(const DmVector& pt);

    /// @brief 获取第二条尺寸界线起点
    /// @return 第二条尺寸界线起点坐标
    DmVector getXLine2Start() const;

    /// @brief 设置第二条尺寸界线起点
    /// @param [in] pt 坐标点
    void setXLine2Start(const DmVector& pt);

    /// @brief 获取第二条尺寸界线终点
    /// @return 第二条尺寸界线终点坐标
    DmVector getXLine2End() const;

    /// @brief 设置第二条尺寸界线终点
    /// @param [in] pt 坐标点
    void setXLine2End(const DmVector& pt);

    /// @brief 获取弧上点
    /// @return 弧上点坐标
    DmVector getArcPoint() const;

    /// @brief 设置弧上点
    /// @param [in] pt 坐标点
    void setArcPoint(const DmVector& pt);

private:
    DmVector m_ptxLine1Start; ///< 第一条尺寸界线起点
    DmVector m_ptxLine1End;   ///< 第一条尺寸界线终点
    DmVector m_ptxLine2Start; ///< 第二条尺寸界线起点
    DmVector m_ptxLine2End;   ///< 第二条尺寸界线终点
    DmVector m_ptArcPoint;    ///< 弧上点
};

#endif // DIMANGULARDATA_H
