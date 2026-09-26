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

/// @file PolylineData.cpp
/// @brief 多段线数据结构类实现

#include "PolylineData.h"

/// @brief 默认构造函数
PolylineData::PolylineData()
    : EntityData()
    , m_vecVertexs(std::vector<DmVector>())
    , m_vecLineWeights(std::vector<double>())
    , m_vecBulges(std::vector<double>())
    , m_isClosed(false)
{
    setEntityType(EEntityType::ePolyline);
}

/// @brief 带全部参数的构造函数
/// @param vertexs 顶点坐标列表
/// @param bulges 凸度列表
/// @param lineWeights 线宽列表
/// @param isClosed 是否闭合
PolylineData::PolylineData(const std::vector<DmVector>& vertexs, const std::vector<double>& bulges, std::vector<double>& lineWeights, bool isClosed)
    : EntityData()
    , m_vecVertexs(vertexs)
    , m_vecLineWeights(lineWeights)
    , m_vecBulges(bulges)
    , m_isClosed(isClosed)
{
    setEntityType(EEntityType::ePolyline);
}

std::vector<DmVector> PolylineData::getVertexs() const
{
    return m_vecVertexs;
}

void PolylineData::setVertexs(const std::vector<DmVector>& vecVertexs)
{
    m_vecVertexs = vecVertexs;
}

DmVector PolylineData::getVertexAt(int i) const
{
    if (i >= static_cast<int>(m_vecVertexs.size()))
    {
        return DmVector(false);
    }
    else
    {
        return m_vecVertexs.at(i);
    }
}

void PolylineData::setVertexAt(int i, const DmVector& vertex)
{
    // 只修改已有的顶点，不新增顶点，因为新增顶点后得同步添加凸度，麻烦
    if (i >= getVertexCount())
    {
        return;
    }
    m_vecVertexs.at(i) = vertex;
}

void PolylineData::insertVertex(int i, const DmVector& vertex)
{
    m_vecVertexs.insert(m_vecVertexs.begin() + i, vertex);
}

void PolylineData::appendVertex(const DmVector& vertex)
{
    m_vecVertexs.emplace_back(vertex);
}

void PolylineData::removeVertex(int i)
{
    m_vecVertexs.erase(m_vecVertexs.begin() + i);
}

int PolylineData::getVertexCount() const
{
    return (int)m_vecVertexs.size();
}

std::vector<double> PolylineData::getLineWeights() const
{
    return m_vecLineWeights;
}

void PolylineData::setLineWeights(const std::vector<double>& vecLineWeights)
{
    m_vecLineWeights = vecLineWeights;
}

void PolylineData::getLineWeightsAt(int i, double& startWeight, double& endWeight) const
{
    startWeight = m_vecLineWeights.at(i * 2);
    endWeight = m_vecLineWeights.at(i * 2 + 1);
}

void PolylineData::insertLineWeight(int i, double startWeight, double endWeight)
{
    m_vecLineWeights.insert(m_vecLineWeights.begin() + i * 2, startWeight);
    m_vecLineWeights.insert(m_vecLineWeights.begin() + i * 2 + 1, endWeight);
}

void PolylineData::appendLineWeight(double startWeight, double endWeight)
{
    m_vecLineWeights.emplace_back(startWeight);
    m_vecLineWeights.emplace_back(endWeight);
}

void PolylineData::appendLineWeight(double weight)
{
    m_vecLineWeights.emplace_back(weight);
}

void PolylineData::removeLineWeight(int i)
{
    m_vecLineWeights.erase(m_vecLineWeights.begin() + i * 2 + 1);
    m_vecLineWeights.erase(m_vecLineWeights.begin() + i * 2);
}

int PolylineData::getLineWeightCount()
{
    return (int)m_vecLineWeights.size();
}

std::vector<double> PolylineData::getBulges() const
{
    return m_vecBulges;
}

void PolylineData::setBulges(const std::vector<double>& vecBulges)
{
    m_vecBulges = vecBulges;
}

double PolylineData::getBulgeAt(int i) const
{
    return m_vecBulges.at(i);
}

void PolylineData::insertBulge(int i, double bulge)
{
    m_vecBulges.insert(m_vecBulges.begin() + i, bulge);
}

void PolylineData::appendBulge(const double& bulge)
{
    m_vecBulges.emplace_back(bulge);
}

void PolylineData::removeBulge(int i)
{
    m_vecBulges.erase(m_vecBulges.begin() + i);
}

int PolylineData::getBulgesCount() const
{
    return (int)m_vecBulges.size();
}

bool PolylineData::getIsClosed() const
{
    return m_isClosed;
}

void PolylineData::setIsClosed(const bool& isClosed)
{
    m_isClosed = isClosed;
}
