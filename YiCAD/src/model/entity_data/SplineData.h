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

/// @file SplineData.h
/// @brief Nurbs样条曲线数据类

#ifndef SPLINEDATA_H
#define SPLINEDATA_H

#include "EntityData.h"
#include "DmVector.h"

/// @brief Nurbs样条曲线
class SplineData : public EntityData
{
public:
    SplineData();

    /// @brief 带参数的样条曲线构造函数
    /// @param [in] degree 阶数
    /// @param [in] closed 是否闭合
    /// @param [in] splineType 样条曲线类型, 默认为控制点样条
    SplineData(int degree, bool closed, ESplineType splineType = ESplineType::eControlPoints);

    /// @brief 获取样条曲线类型
    /// @return 样条曲线类型
    ESplineType getSplineType() const;

    /// @brief 设置样条曲线类型
    /// @param [in] eSplineType 样条曲线类型
    void setSplineType(const ESplineType& eSplineType);

    /// @brief 获取阶数
    /// @return 阶数
    int getDegree() const;

    /// @brief 设置阶数
    /// @param [in] iDegree 阶数
    void setDegree(const int& iDegree);

    /// @brief 获取是否闭合
    /// @return 是否闭合
    bool getIsClosed() const;

    /// @brief 设置是否闭合
    /// @param [in] isClosed 是否闭合
    void setIsClosed(const bool& isClosed);

    /// @brief 获取控制点集
    /// @return 控制点向量
    std::vector<DmVector> getControlPoints() const;

    /// @brief 设置控制点集
    /// @param [in] controlPoints 控制点向量
    void setControlPoints(const std::vector<DmVector>& controlPoints);

    /// @brief 获取指定索引的控制点
    /// @param [in] i 控制点索引
    /// @return 控制点坐标
    DmVector getControlPointAt(int i) const;

    /// @brief 获取控制点数量
    /// @return 控制点数量
    int getControlPointsSize() const;

    /// @brief 移除指定索引的控制点
    /// @param [in] i 控制点索引
    void removeControlPointAt(int i);

    /// @brief 获取节点数据
    /// @return 节点数据向量
    std::vector<double> getKnots() const;

    /// @brief 获取指定索引的节点
    /// @param [in] i 节点索引
    /// @return 节点值
    double getKnotAt(int i) const;

    /// @brief 设置节点数据
    /// @param [in] knots 节点数据向量
    void setKnots(const std::vector<double>& knots);

    /// @brief 获取节点数量
    /// @return 节点数量
    int getKnotsSize() const;

    /// @brief 移除指定索引的节点
    /// @param [in] i 节点索引
    void removeKnotAt(int i);

    /// @brief 获取拟合点集
    /// @return 拟合点向量
    std::vector<DmVector> getFitPoints() const;

    /// @brief 设置拟合点集
    /// @param [in] fitPoints 拟合点向量
    void setFitPoints(const std::vector<DmVector>& fitPoints);

    /// @brief 获取指定索引的拟合点
    /// @param [in] i 拟合点索引
    /// @return 拟合点坐标
    DmVector getFitPointAt(int i) const;

    /// @brief 获取拟合点数量
    /// @return 拟合点数量
    int getFitPointsSize() const;

    /// @brief 移除指定索引的拟合点
    /// @param [in] i 拟合点索引
    void removeFitPointAt(int i);

private:
    ESplineType              m_eType;            ///< 样条线的类型
    int                      m_iDegree;          ///< 阶数
    bool                     m_bClosed;          ///< 是否闭合
    std::vector<DmVector>    m_vecControlPoints; ///< 控制点集
    std::vector<double>      m_vecKnots;         ///< 节点数据
    std::vector<DmVector>    m_vecFitPoints;     ///< 拟合点（仅限拟合点样条）
};

#endif // SPLINEDATA_H
