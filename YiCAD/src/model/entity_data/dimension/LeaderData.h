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

/// @file LeaderData.h
/// @brief 引线数据类

#ifndef LEADERDATA_H
#define LEADERDATA_H

#include "EntityData.h"
#include "DmVector.h"

/// @brief 引线
class LeaderData : public EntityData
{
public:
    LeaderData();

    /// @brief 获取引线顶点列表
    /// @return 顶点向量
    std::vector<DmVector> getVertexs() const;

    /// @brief 设置引线顶点列表
    /// @param [in] vertexs 顶点向量
    void setVertexs(const std::vector<DmVector>& vertexs);

    /// @brief 获取是否有箭头
    /// @return 是否有箭头
    bool getIsArrow() const;

    /// @brief 设置是否有箭头
    /// @param [in] isArrow 是否有箭头
    void setIsArrow(const bool& isArrow);

private:
    std::vector<DmVector> m_vecVertexs; ///< 引线顶点列表
    bool                   m_isArrow;   ///< 是否有箭头
};

#endif // LEADERDATA_H
