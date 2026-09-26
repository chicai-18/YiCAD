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

/// @file CircleData.h
/// @brief 圆数据结构类，定义圆的几何属性（圆心、半径）

#ifndef CIRCLEDATA_H
#define CIRCLEDATA_H

#include "EntityData.h"
#include "DmVector.h"

/// @brief 圆数据结构
class CircleData : public EntityData
{
public:
    /// @brief 默认构造函数
    CircleData();

    /// @brief 带参数构造函数
    /// @param center 圆心坐标
    /// @param radius 半径
    CircleData(DmVector const& center, double radius);

    /// @brief 判断圆数据是否有效
    /// @return 若圆心有效且半径大于容差则返回true
    bool isValid() const;

    /// @brief 比较运算符
    /// @param rhs 右侧比较对象
    /// @return 若圆心和半径相等则返回true
    bool operator==(CircleData const& rhs) const;

public:
    /// @brief 获取圆心
    /// @return 圆心坐标
    DmVector getCenter() const;

    /// @brief 设置圆心
    /// @param ptCenter 圆心坐标
    void setCenter(const DmVector& ptCenter);

    /// @brief 获取半径
    /// @return 半径值
    double getRadius() const;

    /// @brief 设置半径
    /// @param dRadius 半径值
    void setRadius(const double& dRadius);

    /// @brief 获取渲染顶点数据引用
    /// @return 顶点数据常量引用
    const std::vector<float>& getVerticesRef() const;

    /// @brief 设置渲染顶点数据
    /// @param vs 顶点数据向量
    void setVertices(const std::vector<float>& vs);

private:
    DmVector            m_ptCenter;     ///< 圆心
    double              m_dRadius;      ///< 半径

    std::vector<float>  m_vertices;     ///< 用于渲染的特定结构数据（x,y,z, parameter, total_length）
};

#endif // CIRCLEDATA_H
