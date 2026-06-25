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

/// @file ArcData.h
/// @brief 圆弧数据结构类，定义圆弧的几何属性（圆心、半径、角度、法向量）

#ifndef ARCDATA_H
#define ARCDATA_H

#include "EntityData.h"
#include "DmVector.h"

/// @brief 圆弧数据结构
class ArcData : public EntityData
{
public:
    /// @brief 默认构造函数
    ArcData();

    /// @brief 带参数构造函数
    /// @param center 圆心坐标
    /// @param normal 圆弧平面法向量
    /// @param radius 半径
    /// @param startAngle 起始角度（弧度）
    /// @param endAngle 终止角度（弧度）
    ArcData(const DmVector& center, const DmVector& normal, const double& radius, const double& startAngle, const double& endAngle);

public:
    /// @brief 获取圆心
    /// @return 圆心坐标
    DmVector getCenter() const;

    /// @brief 设置圆心
    /// @param center 圆心坐标
    void setCenter(const DmVector& center);

    /// @brief 获取半径
    /// @return 半径值
    double getRadius() const;

    /// @brief 设置半径
    /// @param dRadius 半径值
    void setRadius(const double& dRadius);

    /// @brief 获取起始角度
    /// @return 起始角度（弧度）
    double getStartAngle() const;

    /// @brief 设置起始角度
    /// @param dStartAngle 起始角度（弧度）
    void setStartAngle(const double& dStartAngle);

    /// @brief 获取终止角度
    /// @return 终止角度（弧度）
    double getEndAngle() const;

    /// @brief 设置终止角度
    /// @param dEndAngle 终止角度（弧度）
    void setEndAngle(const double& dEndAngle);

    /// @brief 获取法向量
    /// @return 法向量
    DmVector getNormal() const;

    /// @brief 设置法向量
    /// @param normal 法向量
    void setNormal(const DmVector& normal);

    /// @brief 获取渲染顶点数据引用
    /// @return 顶点数据常量引用
    const std::vector<float>& getVerticesRef() const;

    /// @brief 设置渲染顶点数据
    /// @param vs 顶点数据向量
    void setVertices(const std::vector<float>& vs);

    /// @brief 判断圆弧数据是否有效
    /// @return 若圆心有效且半径大于容差且角度差大于容差则返回true
    bool isValid() const;

private:
    DmVector            m_ptCenter;     ///< 圆心
    double              m_dRadius;      ///< 半径
    double              m_dStartAngle;  ///< 起始角度
    double              m_dEndAngle;    ///< 终止角度
    DmVector            m_normal;       ///< 圆弧所在平面法向量

    std::vector<float>  m_vertices;     ///< 用于渲染的特定结构数据（x,y,z, parameter, total_length）
};

#endif // ARCDATA_H
