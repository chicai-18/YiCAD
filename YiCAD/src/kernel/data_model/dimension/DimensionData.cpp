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

/// @file DimensionData.cpp
/// @brief 标注基类数据实现

#include "DimensionData.h"

DimensionData::DimensionData()
    : EntityData()
    , m_ptTextPoint(DmVector(0, 0, 0))
    , m_strTextString(L"")
    , m_strDimStyle(L"")
    , m_dTextLineFactor(1.)
    , m_eTextAlign(EAttachmentPoint::kBaseAlign)
{
    setEntityType(EEntityType::eDimension);
}

DmVector DimensionData::getTextPoint() const
{
    return m_ptTextPoint;
}

void DimensionData::setTextPoint(const DmVector& textPoint)
{
    m_ptTextPoint = textPoint;
}

std::wstring DimensionData::getTextString() const
{
    return m_strTextString;
}

void DimensionData::setTextString(const std::wstring& textString)
{
    m_strTextString = textString;
}

std::wstring DimensionData::getDimStyle() const
{
    return m_strDimStyle;
}

void DimensionData::setDimStyle(const std::wstring& strStyle)
{
    m_strDimStyle = strStyle;
}

double DimensionData::getTextLineFactor() const
{
    return m_dTextLineFactor;
}

void DimensionData::setTextLineFactor(const double& dTextLineFactor)
{
    m_dTextLineFactor = dTextLineFactor;
}

EAttachmentPoint DimensionData::getTextAlign() const
{
    return m_eTextAlign;
}

void DimensionData::setTextAlign(const EAttachmentPoint& align)
{
    m_eTextAlign = align;
}
