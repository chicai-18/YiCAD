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

/// @file MLeaderData.h
/// @brief 多重引线数据类

#ifndef MLEADERDATA_H
#define MLEADERDATA_H

#include "EntityData.h"
#include "DmVector.h"

/// @brief 多重引线
class MLeaderData : public EntityData
{
public:
    MLeaderData();

    /// @brief 获取多重引线顶点列表
    /// @return 每段线段的点集
    std::vector<std::vector<DmVector>> getVertexs() const;

    /// @brief 设置多重引线顶点列表
    /// @param [in] vertexs 每段线段的点集
    void setVertexs(const std::vector<std::vector<DmVector>>& vertexs);

    /// @brief 获取是否有箭头
    /// @return 是否有箭头
    bool getIsArrow() const;

    /// @brief 设置是否有箭头
    /// @param [in] isArrow 是否有箭头
    void setIsArrow(const bool& isArrow);

private:
    std::vector<std::vector<DmVector>> m_vecVertexs; ///< 多重引线跟转折点相连的末端的每条线段的点集
    bool                               m_isArrow;    ///< 是否有箭头
};

#endif // MLEADERDATA_H
