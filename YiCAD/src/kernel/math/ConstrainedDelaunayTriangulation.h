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
    // 用CDT的Constrained Delaunay Triangulation做三角剖分
    CdtVertices vertices;
    CdtEdges edges;

    if (!outBoundary.empty())
    {
        // 有外边界：插入外边界和孔洞，由CDT自动检测嵌套层级
        std::vector<DmVector> boundary = outBoundary;
        ConstrainedDelaunayTriangulation::removeClosingDuplicate(boundary);
        ConstrainedDelaunayTriangulation::appendContourAsEdges(vertices, edges, boundary);

        for (auto hole : holes)
        {
            ConstrainedDelaunayTriangulation::removeClosingDuplicate(hole);
            ConstrainedDelaunayTriangulation::appendContourAsEdges(vertices, edges, hole);
        }
    }
    else if (!holes.empty())
    {
        // 无外边界（字体轮廓场景）：所有轮廓作为约束边插入，
        // CDT的eraseOuterTrianglesAndHoles会根据嵌套深度自动处理。
        // 深度为奇数的区域（轮廓内部）保留，深度为偶数的区域（外部和孔洞）删除。
        for (auto contour : holes)
        {
            ConstrainedDelaunayTriangulation::removeClosingDuplicate(contour);
            ConstrainedDelaunayTriangulation::appendContourAsEdges(vertices, edges, contour);
        }
    }
    else
    {
        return; // 无输入
    }

    // Build the CDT through its public API before inserting constraints.
    if (vertices.empty() || edges.empty())
    {
        return;
    }

    CDT::RemoveDuplicatesAndRemapEdges(vertices, edges);
    edges.erase(
        std::remove_if(
            edges.begin(),
            edges.end(),
            ConstrainedDelaunayTriangulation::isDegenerateEdge),
        edges.end());
    if (edges.empty())
    {
        return;
    }

    CDT::Triangulation<double> cdt(
        CDT::VertexInsertionOrder::Auto,
        CDT::IntersectingConstraintEdges::TryResolve,
        0.0);
    cdt.insertVertices(vertices);
    cdt.insertEdges(edges);

    // 删除外部三角形和孔洞（基于嵌套深度自动判断）
    cdt.eraseOuterTrianglesAndHoles();

    // 生成三角面
    for (const auto& tri : cdt.triangles)
    {
        const auto& p0 = cdt.vertices[tri.vertices[0]];
        const auto& p1 = cdt.vertices[tri.vertices[1]];
        const auto& p2 = cdt.vertices[tri.vertices[2]];
        std::array<DmVector, 3> pts;
        pts.at(0) = DmVector(p0.x, p0.y);
        pts.at(1) = DmVector(p1.x, p1.y);
        pts.at(2) = DmVector(p2.x, p2.y);
        TriangleData data;
        data.setPoints(pts);
        DmTriangle* triangle = new DmTriangle(nullptr, data);
        triangles.emplace_back(triangle);
    }
}

#endif //CONSTRAINEDDELAUNAYTRIANGUATION_H
