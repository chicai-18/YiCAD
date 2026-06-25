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

/// @file LineData.h
/// @brief 直线数据结构类，定义直线的起点和终点

#ifndef LINEDATA_H
#define LINEDATA_H

#include "EntityData.h"
#include "DmVector.h"

/// @brief 直线数据结构
class LineData : public EntityData
{
public:
    /// @brief 默认构造函数
    LineData();

    /// @brief 带起始终止点的构造函数
    /// @param startPt 起点坐标
    /// @param endPt 终点坐标
    LineData(const DmVector& startPt, const DmVector& endPt);

public:
    /// @brief 获取起点
    /// @return 起点坐标
    DmVector getStartPoint() const;

    /// @brief 设置起点
    /// @param pt 起点坐标
    void setStartPoint(const DmVector& pt);

    /// @brief 获取终点
    /// @return 终点坐标
    DmVector getEndPoint() const;

    /// @brief 设置终点
    /// @param pt 终点坐标
    void setEndPoint(const DmVector& pt);

    /// @brief 获取渲染顶点数据引用
    /// @return 顶点数据常量引用
    const std::vector<float>& getVerticesRef() const;

    /// @brief 设置渲染顶点数据
    /// @param vs 顶点数据向量
    void setVertices(const std::vector<float>& vs);

private:
    DmVector            m_startPoint;   ///< 起点
    DmVector            m_endPoint;     ///< 终点

    std::vector<float>  m_vertices;     ///< 用于渲染的特定结构数据（x,y,z, parameter, total_length）
};

#endif // LINEDATA_H
