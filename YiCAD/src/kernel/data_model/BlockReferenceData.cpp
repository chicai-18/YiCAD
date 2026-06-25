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

/// @file BlockReferenceData.cpp
/// @brief 块参照数据结构类实现

#include "BlockReferenceData.h"


/// @brief 默认构造函数
BlockReferenceData::BlockReferenceData()
    : EntityData()
    , m_BlockTableRecordHandle(-1)
    , m_strBlockName(L"*U")
    , m_normal(0.0, 0.0, 1.0)
    , m_position(0.0, 0.0, 0.0)
    , m_dRotationAngle(0.0)
    , m_scale(1.0, 1.0, 1.0)
{
    setEntityType(EEntityType::eBlock);
}

int BlockReferenceData::getBlockTableRecHandle() const
{
    return m_BlockTableRecordHandle;
}

void BlockReferenceData::setBlockTableRecHHandle(const int handle)
{
    m_BlockTableRecordHandle = handle;
}

std::wstring BlockReferenceData::getBlockName() const
{
    return m_strBlockName;
}

void BlockReferenceData::setBlockName(const std::wstring name)
{
    m_strBlockName = name;
}

DmVector BlockReferenceData::getNormal() const
{
    return m_normal;
}

void BlockReferenceData::setNormal(const DmVector& normal)
{
    m_normal = normal;
}

DmVector BlockReferenceData::getPosition() const
{
    return m_position;
}

void BlockReferenceData::setPosition(const DmVector& position)
{
    m_position = position;
}

double BlockReferenceData::getRotationAngle() const
{
    return m_dRotationAngle;
}

void BlockReferenceData::setRotationAngle(const double angle)
{
    m_dRotationAngle = angle;
}

DmVector BlockReferenceData::getScale() const
{
    return m_scale;
}

void BlockReferenceData::setScale(const DmVector& scale)
{
    m_scale = scale;
}
