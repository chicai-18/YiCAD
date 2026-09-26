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

/// @file HatchData.h
/// @brief 填充数据结构类，定义填充的边界、图案、角度、缩放等属性

#ifndef HATCHDATA_H
#define HATCHDATA_H

#include "EntityData.h"
#include "DmVector.h"
#include "DmPattern.h"
#include "DmRegion.h"

/// @brief 填充数据结构
class HatchData : public EntityData
{
public:
    /// @brief 默认构造函数
    HatchData();

    /// @brief 带图案参数的构造函数
    /// @param solid 是否为实体填充
    /// @param scale 图案缩放
    /// @param angle 图案角度
    /// @param pattern 填充图案指针
    HatchData(bool solid, double scale, double angle, DmPattern* pattern);

    /// @brief 带图案名称的构造函数
    /// @param solid 是否为实体填充
    /// @param scale 图案缩放
    /// @param angle 图案角度
    /// @param pattern 填充图案名称（宽字符串）
    HatchData(bool solid, double scale, double angle, const std::wstring& pattern);

    /// @brief 带实心标志和图案名称的构造函数
    /// @param solid 是否为实体填充
    /// @param pattern 填充图案名称（宽字符串）
    HatchData(bool solid, const std::wstring& pattern);

public:
    /// @brief 获取填充边界
    /// @return 区域指针
    DmRegionPtr getBoundary() const;

    /// @brief 设置填充边界
    /// @param b 区域指针
    void setBoundary(DmRegionPtr b);

    /// @brief 获取图案名称
    /// @return 图案名称（宽字符串）
    std::wstring getPatternName() const;

    /// @brief 设置图案名称
    /// @param patternName 图案名称（宽字符串）
    void setPatternName(const std::wstring& patternName);

    /// @brief 获取图案角度
    /// @return 图案角度（弧度）
    double getPatternAngle() const;

    /// @brief 设置图案角度
    /// @param angle 图案角度（弧度）
    void setPatternAngle(const double& angle);

    /// @brief 获取图案缩放
    /// @return 图案缩放值
    double getPatternScale() const;

    /// @brief 设置图案缩放
    /// @param scale 图案缩放值（必须大于0）
    void setPatternScale(const double& scale);

    /// @brief 判断是否为实体填充
    /// @return 若为实体填充则返回true
    bool isSolid() const;

    /// @brief 设置是否为实体填充
    /// @param solid 是否为实体填充
    void setIsSolid(const bool& solid);

    /// @brief 获取填充环数量
    /// @return 填充环数量
    int getLoops();

    /// @brief 获取填充图案
    /// @return 填充图案对象
    DmPattern getPattern() const;

    /// @brief 设置填充图案
    /// @param pattern 填充图案对象
    void setPattern(const DmPattern& pattern);

private:
    bool            m_solid;        ///< 是否为实体填充
    DmRegionPtr     m_boundary;     ///< 边界（含孔洞）

    // pattern data
    double          m_dAngle;       ///< 填充角度
    double          m_dScale;       ///< 填充缩放（必须大于0）

    // pattern
    DmPatternPtr    m_pPattern;     ///< 填充图案
};

#endif // HATCHDATA_H
