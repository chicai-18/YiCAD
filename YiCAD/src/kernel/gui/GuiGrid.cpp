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

/// @file GuiGrid.cpp
/// @brief 栅格背景类实现

#include "GuiGrid.h"

#include<cmath>

#include "Debug.h"


GuiGrid::GuiGrid()
    : m_baseGrid(false)
{
}

/// @brief 获取最近的栅格交点
/// @param coord 当前鼠标点击的点
/// @return 最近的栅格交点坐标
DmVector GuiGrid::snapGrid(const DmVector& coord) const
{
    if (m_cellV.x < DM_TOLERANCE || m_cellV.y < DM_TOLERANCE)
    {
        return coord;
    }

    DmVector vp(coord - m_baseGrid);

    return m_baseGrid + vp - DmVector(remainder(vp.x, m_cellV.x), remainder(vp.y, m_cellV.y));
}

/// @brief 获取栅格交点集合
std::vector<DmVector> const& GuiGrid::getPoints() const
{
    return m_pts;
}

/// @brief 设置栅格交点集合
void GuiGrid::setPoints(const std::vector<DmVector>& points)
{
    m_pts.clear();
    m_pts = points;
}

/// @brief 获取网格单元向量
DmVector const& GuiGrid::getCellVector() const
{
    return m_cellV;
}

/// @brief 设置网格单元向量
void GuiGrid::setCellVector(const DmVector& cellVec)
{
    m_cellV = cellVec;
}

/// @brief 设置基准网格点
void GuiGrid::setBaseGrid(const DmVector& baseGrid)
{
    m_baseGrid = baseGrid;
}
