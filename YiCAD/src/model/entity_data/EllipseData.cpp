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

/// @file EllipseData.cpp
/// @brief 椭圆/椭圆弧数据结构类实现

#include "EllipseData.h"

/// @brief 默认构造函数
EllipseData::EllipseData()
    : EntityData()
    , m_ptCenter(DmVector(0, 0, 0))
    , m_vecMajorP(DmVector(0, 0, 0))
    , m_dRatio(0)
    , m_bIsClosed(false)
    , m_dStartParam(0)
    , m_dEndParam(0)
{
    setEntityType(EEntityType::eEllipse);
}

/// @brief 带参数的构造函数
/// @param center 椭圆中心
/// @param major 长轴端点向量
/// @param normal 法向量
/// @param ratio 短轴长轴比
/// @param isClosed 是否闭合
/// @param startParam 起始参数
/// @param endParam 终止参数
EllipseData::EllipseData(const DmVector& center, const DmVector& major, const DmVector& normal, const double& ratio, const bool& isClosed, const double& startParam, const double& endParam)
    : EntityData()
    , m_ptCenter(center)
    , m_vecMajorP(major)
    , m_dRatio(ratio)
    , m_bIsClosed(isClosed)
    , m_normal(0.0, 0.0, 1.0)      // TODO:  - 此处硬编码为Z轴法向量，未使用传入的normal参数，需确认是否为有意设计
    , m_dStartParam(startParam)
    , m_dEndParam(endParam)
{
    setEntityType(EEntityType::eEllipse);
}

double EllipseData::getStartParam() const
{
    return m_dStartParam;
}

void EllipseData::setStartParam(const double& dStartParam)
{
    m_dStartParam = dStartParam;
}

double EllipseData::getEndParam() const
{
    return m_dEndParam;
}

void EllipseData::setEndParam(const double& dEndParam)
{
    m_dEndParam = dEndParam;
}

DmVector EllipseData::getCenter() const
{
    return m_ptCenter;
}

void EllipseData::setCenter(const DmVector& pt)
{
    m_ptCenter = pt;
}

DmVector EllipseData::getMajorP() const
{
    return m_vecMajorP;
}

void EllipseData::setMajorP(const DmVector& vec)
{
    m_vecMajorP = vec;
}

double EllipseData::getRatio() const
{
    return m_dRatio;
}

void EllipseData::setRatio(const double& dRation)
{
    m_dRatio = dRation;
}

bool EllipseData::getIsClosed() const
{
    return m_bIsClosed;
}

void EllipseData::setIsClosed(const bool& isClosed)
{
    m_bIsClosed = isClosed;
}

DmVector EllipseData::getNormal() const
{
    return m_normal;
}

void EllipseData::setNormal(const DmVector& normal)
{
    m_normal = normal;
}

const std::vector<float>& EllipseData::getVerticesRef() const
{
    return m_vertices;
}

void EllipseData::setVertices(const std::vector<float>& vs)
{
    m_vertices = vs;
}
