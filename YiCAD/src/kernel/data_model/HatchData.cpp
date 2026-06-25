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

/// @file HatchData.cpp
/// @brief 填充数据结构类实现

#include "HatchData.h"

/// @brief 默认构造函数
HatchData::HatchData()
    : EntityData()
    , m_solid(false)
    , m_dScale(1.0)
    , m_dAngle(0.0)
    , m_pPattern(new DmPattern())
    , m_boundary(new DmRegion())
{
    setEntityType(EEntityType::eHatch);
}

/// @brief 带图案参数的构造函数
/// @param solid 是否为实体填充
/// @param scale 图案缩放
/// @param angle 图案角度
/// @param pattern 填充图案指针
HatchData::HatchData(bool solid, double scale, double angle, DmPattern* pattern)
    : EntityData()
    , m_solid(solid)
    , m_dScale(scale)
    , m_dAngle(angle)
    , m_boundary(new DmRegion())
{
    if (pattern)
    {
        m_pPattern = std::make_shared<DmPattern>(*pattern);
    }
    else
    {
        m_pPattern = std::make_shared<DmPattern>();
    }
    setEntityType(EEntityType::eHatch);
}

/// @brief 带图案名称的构造函数
/// @param solid 是否为实体填充
/// @param scale 图案缩放
/// @param angle 图案角度
/// @param pattern 填充图案名称
HatchData::HatchData(bool solid, double scale, double angle, const std::wstring& pattern)
    : EntityData()
    , m_solid(solid)
    , m_dScale(scale)
    , m_dAngle(angle)
    , m_pPattern(new DmPattern(pattern))
    , m_boundary(new DmRegion())
{
    setEntityType(EEntityType::eHatch);
}

/// @brief 带实心标志和图案名称的构造函数
/// @param solid 是否为实体填充
/// @param pattern 填充图案名称
HatchData::HatchData(bool solid, const std::wstring& pattern)
    : EntityData()
    , m_solid(solid)
    , m_dScale(1.0)
    , m_dAngle(0.0)
    , m_pPattern(new DmPattern(pattern))
    , m_boundary(new DmRegion())
{
    setEntityType(EEntityType::eHatch);
}

DmRegionPtr HatchData::getBoundary() const
{
    return m_boundary;
}

void HatchData::setBoundary(DmRegionPtr b)
{
    m_boundary = b;
}

std::wstring HatchData::getPatternName() const
{
    return m_pPattern->getPatternName();
}

void HatchData::setPatternName(const std::wstring& patternName)
{
    m_pPattern->setPatternName(patternName);
    if (patternName == L"SOLID")
    {
        m_solid = true;
    }
}

double HatchData::getPatternAngle() const
{
    return m_dAngle;
}

void HatchData::setPatternAngle(const double& angle)
{
    m_dAngle = angle;
}

double HatchData::getPatternScale() const
{
    return m_dScale;
}

void HatchData::setPatternScale(const double& scale)
{
    m_dScale = scale;
}

DmPattern HatchData::getPattern() const
{
    return *m_pPattern;
}

void HatchData::setPattern(const DmPattern& pattern)
{
    m_pPattern = std::make_shared<DmPattern>(pattern);
}

bool HatchData::isSolid() const
{
    return m_solid;
}

void HatchData::setIsSolid(const bool& solid)
{
    m_solid = solid;
}

int HatchData::getLoops()
{
    return (int)m_boundary->size();
}
