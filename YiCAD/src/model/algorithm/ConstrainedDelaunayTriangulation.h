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

/// @file ConstrainedDelaunayTriangulation.h
/// @brief 约束Delaunay三角剖分类（基于CDT库）

#ifndef CONSTRAINEDDELAUNAYTRIANGUATION_H
#define CONSTRAINEDDELAUNAYTRIANGUATION_H

#include <CDT.h>
#include "DmVector.h"
#include "DmTriangle.h"
#include <algorithm>
#include <array>

/// @brief 约束Delaunay三角剖分类
class ConstrainedDelaunayTriangulation
{
public:
    /// @brief 三角化。参数T可为DmTriangle或DmTrianglePtr
    /// @details 外边界或内边界可以为空，保证有一个非空即可，函数内部判断三角形是内部还是外部，对内外边界采用相同的处理。
    /// @param [in] outBoundary 外边界点
    /// @param [in] holes 孔洞边界点列表
    /// @param [out] triangles 生成的三角形列表
    template<typename T>
    static void trianglulate(const std::vector<DmVector>& outBoundary, const std::vector<std::vector<DmVector>>& holes, std::vector<T>& triangles);

    /// @brief 三角化，输出每个三角形的三个顶点；规则同 trianglulate
    /// @details 旧渲染器的 GI 适配器剖分填充区域时用它，不必为每个三角形创建实体
    static void triangulatePoints(const std::vector<DmVector>& outBoundary, const std::vector<std::vector<DmVector>>& holes,
                                  std::vector<std::array<DmVector, 3>>& triangles);

private:
    typedef CDT::Triangulation<double>::V2dVec CdtVertices;
    typedef CDT::EdgeVec CdtEdges;

    /// Remove duplicated closing point from a contour.
    static void removeClosingDuplicate(std::vector<DmVector>& pts);
    static bool isDegenerateEdge(const CDT::Edge& edge);

    /// Append one closed contour to CDT input vertices and constraint edges.
    static void appendContourAsEdges(
        CdtVertices& vertices,
        CdtEdges& edges,
        const std::vector<DmVector>& pts);
};

template<typename T>
void ConstrainedDelaunayTriangulation::trianglulate(const std::vector<DmVector>& outBoundary, const std::vector<std::vector<DmVector>>& holes, std::vector<T>& triangles)
{
    std::vector<std::array<DmVector, 3>> points;
    triangulatePoints(outBoundary, holes, points);
    for (const auto& pts : points)
    {
        TriangleData data;
        data.setPoints(pts);
        DmTriangle* triangle = new DmTriangle(nullptr, data);
        triangles.emplace_back(triangle);
    }
}

#endif //CONSTRAINEDDELAUNAYTRIANGUATION_H
