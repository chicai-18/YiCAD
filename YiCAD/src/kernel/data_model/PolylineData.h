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

/// @file PolylineData.h
/// @brief 多段线数据结构类，定义2D/3D多段线的顶点、凸度、线宽等属性

#ifndef POLYLINEDATA_H
#define POLYLINEDATA_H

#include <vector>

#include "EntityData.h"
#include "DmVector.h"

/// @brief 多段线数据结构（2D/3D多段线）
class PolylineData : public EntityData
{
public:
    /// @brief 默认构造函数
    PolylineData();

    /// @brief 带全部参数的构造函数
    /// @param vertexs 顶点坐标列表
    /// @param bulges 凸度列表（凸度为0表示直线段，1表示半圆，负值表示顺时针）
    /// @param lineWeights 线宽列表
    /// @param isClosed 是否闭合
    PolylineData(const std::vector<DmVector>& vertexs, const std::vector<double>& bulges, std::vector<double>& lineWeights, bool isClosed);

public:
    /// @brief 获取所有顶点
    /// @return 顶点坐标列表
    std::vector<DmVector> getVertexs() const;

    /// @brief 设置所有顶点
    /// @param vecVertexs 顶点坐标列表
    void setVertexs(const std::vector<DmVector>& vecVertexs);

    /// @brief 获取指定索引的顶点
    /// @param i 顶点索引
    /// @return 顶点坐标
    DmVector getVertexAt(int i) const;

    /// @brief 设置指定索引的顶点（不新增顶点）
    /// @param i 顶点索引
    /// @param vertex 顶点坐标
    void setVertexAt(int i, const DmVector& vertex);

    /// @brief 在指定位置插入顶点
    /// @param i 插入位置索引
    /// @param vertex 顶点坐标
    void insertVertex(int i, const DmVector& vertex);

    /// @brief 追加一个顶点
    /// @param vertex 顶点坐标
    void appendVertex(const DmVector& vertex);

    /// @brief 删除指定索引的顶点
    /// @param i 顶点索引
    void removeVertex(int i);

    /// @brief 获取顶点数量
    /// @return 顶点数量
    int getVertexCount() const;

    /// @brief 获取所有线宽
    /// @return 线宽列表
    std::vector<double> getLineWeights() const;

    /// @brief 设置所有线宽
    /// @param vecLineWeights 线宽列表
    void setLineWeights(const std::vector<double>& vecLineWeights);

    /// @brief 获取指定索引段的起始终止线宽
    /// @param i 段索引
    /// @param startWeight 输出：起始线宽
    /// @param endWeight 输出：终止线宽
    void getLineWeightsAt(int i, double& startWeight, double& endWeight) const;

    /// @brief 在指定位置插入线宽
    /// @param i 插入位置索引
    /// @param startWeight 起始线宽
    /// @param endWeight 终止线宽
    void insertLineWeight(int i, double startWeight, double endWeight);

    /// @brief 追加起始终止线宽
    /// @param startWeight 起始线宽
    /// @param endWeight 终止线宽
    void appendLineWeight(double startWeight, double endWeight);

    /// @brief 追加单个线宽值
    /// @param weight 线宽值
    void appendLineWeight(double weight);

    /// @brief 删除指定索引的线宽
    /// @param i 线宽索引
    void removeLineWeight(int i);

    /// @brief 获取线宽数量
    /// @return 线宽数量
    int getLineWeightCount();

    /// @brief 获取所有凸度
    /// @return 凸度列表
    std::vector<double> getBulges() const;

    /// @brief 设置所有凸度
    /// @param vecBulges 凸度列表
    void setBulges(const std::vector<double>& vecBulges);

    /// @brief 获取指定索引的凸度
    /// @param i 凸度索引
    /// @return 凸度值
    double getBulgeAt(int i) const;

    /// @brief 在指定位置插入凸度
    /// @param i 插入位置索引
    /// @param bulge 凸度值
    void insertBulge(int i, double bulge);

    /// @brief 追加凸度
    /// @param bulge 凸度值
    void appendBulge(const double& bulge);

    /// @brief 删除指定索引的凸度
    /// @param i 凸度索引
    void removeBulge(int i);

    /// @brief 获取凸度数量
    /// @return 凸度数量
    int getBulgesCount() const;

    /// @brief 获取是否闭合
    /// @return 若闭合则返回true
    bool getIsClosed() const;

    /// @brief 设置是否闭合
    /// @param isClosed 是否闭合
    void setIsClosed(const bool& isClosed);

private:
    std::vector<DmVector>   m_vecVertexs;       ///< 顶点集合
    std::vector<double>     m_vecLineWeights;   ///< 线宽集合
    /// @brief 凸度集合
    /// @note 凸度是圆弧段四分之一夹角的正切值，如果圆弧从起点到终点顺时针移动，则凸度为负值。
    /// 凸度为0表示直线段，凸度为1表示半圆。
    std::vector<double>     m_vecBulges;
    bool                    m_isClosed;         ///< 是否闭合
};

#endif // POLYLINEDATA_H
