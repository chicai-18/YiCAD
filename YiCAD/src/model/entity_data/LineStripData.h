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

/// @file LineStripData.h
/// @brief 线段串数据结构类，定义由多个点组成的线段序列及其闭合属性

#ifndef LINESTRIPDATA_H
#define LINESTRIPDATA_H

#include <vector>
#include "EntityData.h"
#include "DmVector.h"

/// @brief 线段串数据结构
class LineStripData : public EntityData
{
public:
    /// @brief 默认构造函数
    LineStripData();

    /// @brief 带顶点集合的构造函数
    /// @param pts 顶点坐标列表
    /// @param isClosed 是否闭合
    LineStripData(const std::vector<DmVector>& pts, bool isClosed);

    /// @brief 获取所有顶点
    /// @return 顶点坐标列表
    std::vector<DmVector> getPoints() const;

    /// @brief 设置所有顶点
    /// @param pts 顶点坐标列表
    void setPoints(const std::vector<DmVector>& pts);

    /// @brief 追加一个顶点
    /// @param pt 顶点坐标
    void appendPoint(const DmVector& pt);

    /// @brief 获取顶点数量
    /// @return 顶点数量
    int getPointCount() const;

    /// @brief 获取指定索引的顶点
    /// @param i 顶点索引
    /// @return 顶点坐标
    DmVector getPointAt(int i) const;

    /// @brief 设置指定索引的顶点
    /// @param i 顶点索引
    /// @param pt 顶点坐标
    void setPointAt(int i, const DmVector& pt);

    /// @brief 清空所有顶点
    void clear();

    /// @brief 判断是否闭合
    /// @return 若闭合则返回true
    bool isClosed() const;

    /// @brief 设置是否闭合
    /// @param isClosed 是否闭合
    void setIsClosed(bool isClosed);

    /// @brief 获取渲染顶点数据引用
    /// @return 顶点数据常量引用
    const std::vector<float>& getVerticesRef() const;

    /// @brief 设置渲染顶点数据
    /// @param vs 顶点数据向量
    void setVertices(const std::vector<float>& vs);

private:
    std::vector<DmVector>   m_points;       ///< 顶点列表
    bool                    m_isClosed;     ///< 是否闭合

    std::vector<float>      m_vertices;     ///< 用于渲染的特定结构数据（x,y,z, parameter, total_length）
};

#endif // LINESTRIPDATA_H
