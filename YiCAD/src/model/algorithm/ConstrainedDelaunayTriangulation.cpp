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
