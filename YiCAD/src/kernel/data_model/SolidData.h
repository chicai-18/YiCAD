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

/// @file SolidData.h
/// @brief 二维填充实体数据类

#ifndef SOLIDDATA_H
#define SOLIDDATA_H

#include <array>

#include "EntityData.h"
#include "DmVector.h"

class SolidData : public EntityData
{
public:
    SolidData();

    /// @brief 使用顶点列表构造填充实体
    /// @param [in] corners 顶点列表
    SolidData(const std::vector<DmVector>& corners);

    /// @brief 获取填充实体顶点列表
    /// @return 顶点向量
    std::vector<DmVector> getCorners() const;

    /// @brief 设置填充实体顶点列表
    /// @param [in] corners 顶点向量
    void setCorners(const std::vector<DmVector>& corners);

    /// @brief 获取指定索引位置的顶点
    /// @param [in] index 顶点索引
    /// @return 顶点坐标
    DmVector getCornerAt(const int& index) const;

    /// @brief 设置指定索引位置的顶点
    /// @param [in] index 顶点索引
    /// @param [in] corner 顶点坐标
    void setCornerAt(const int& index, const DmVector& corner);

    /// @brief 获取顶点数量
    /// @return 顶点数量
    int getCornerSize() const;

private:
    std::vector<DmVector> m_corners; ///< 构成实体的顶点，至少3个顶点，构成方式为GL_TRIANGLE_FAN
};

#endif