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

/// @file ConstrainedDelaunayTriangulation.cpp
/// @brief Constrained Delaunay Triangulation implementation (based on CDT library)

#include "ConstrainedDelaunayTriangulation.h"
#include <cmath>

void ConstrainedDelaunayTriangulation::removeClosingDuplicate(std::vector<DmVector>& pts)
{
    if (pts.size() >= 2)
    {
        const auto& first = pts.front();
        const auto& last = pts.back();
        double dx = std::fabs(first.x - last.x);
        double dy = std::fabs(first.y - last.y);
        if (dx < 1e-12 && dy < 1e-12)
        {
            pts.pop_back();
        }
    }
}

bool ConstrainedDelaunayTriangulation::isDegenerateEdge(const CDT::Edge& edge)
{
    return edge.v1() == edge.v2();
}

void ConstrainedDelaunayTriangulation::appendContourAsEdges(
    CDT::Triangulation<double>::V2dVec& vertices,
    CDT::EdgeVec& edges,
    const std::vector<DmVector>& pts)
{
    if (pts.size() < 3)
    {
        return;
    }

    CDT::VertInd baseIdx = static_cast<CDT::VertInd>(vertices.size());
    for (const auto& pt : pts)
    {
        vertices.push_back(CDT::V2d<double>(pt.x, pt.y));
    }

    // Generate closed constraint edges
    for (size_t i = 0; i < pts.size(); ++i)
    {
        size_t j = (i + 1) % pts.size();
        CDT::VertInd v1 = baseIdx + static_cast<CDT::VertInd>(i);
        CDT::VertInd v2 = baseIdx + static_cast<CDT::VertInd>(j);
        edges.push_back(CDT::Edge(v1, v2));
    }
}

void ConstrainedDelaunayTriangulation::triangulatePoints(const std::vector<DmVector>& outBoundary,
    const std::vector<std::vector<DmVector>>& holes, std::vector<std::array<DmVector, 3>>& triangles)
{
    // 用CDT的Constrained Delaunay Triangulation做三角剖分
    CdtVertices vertices;
    CdtEdges edges;

    if (!outBoundary.empty())
    {
        // 有外边界：插入外边界和孔洞，由CDT自动检测嵌套层级
        std::vector<DmVector> boundary = outBoundary;
        removeClosingDuplicate(boundary);
        appendContourAsEdges(vertices, edges, boundary);

        for (auto hole : holes)
        {
            removeClosingDuplicate(hole);
            appendContourAsEdges(vertices, edges, hole);
        }
    }
    else if (!holes.empty())
    {
        // 无外边界（字体轮廓场景）：所有轮廓作为约束边插入，
        // CDT的eraseOuterTrianglesAndHoles会根据嵌套深度自动处理。
        // 深度为奇数的区域（轮廓内部）保留，深度为偶数的区域（外部和孔洞）删除。
        for (auto contour : holes)
        {
            removeClosingDuplicate(contour);
            appendContourAsEdges(vertices, edges, contour);
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
            isDegenerateEdge),
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
    triangles.reserve(triangles.size() + cdt.triangles.size());
    for (const auto& tri : cdt.triangles)
    {
        const auto& p0 = cdt.vertices[tri.vertices[0]];
        const auto& p1 = cdt.vertices[tri.vertices[1]];
        const auto& p2 = cdt.vertices[tri.vertices[2]];
        triangles.push_back({ DmVector(p0.x, p0.y), DmVector(p1.x, p1.y), DmVector(p2.x, p2.y) });
    }
}
