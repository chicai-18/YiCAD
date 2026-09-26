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

/// @file GLVertexManager.cpp
/// @brief OpenGL顶点管理器实现，管理当前绘制路径的顶点和跳转信息

#include "GLVertexManager.h"

namespace
{
    constexpr float DEFAULT_PARA = 0.0f;            ///< 默认参数值
}

void GLVertexManager::addVertex(double x, double y)
{
    if (m_current_vertices.size() != 0)
    {
        glm::vec3 lastPt = m_current_vertices.back();
        float dist = glm::distance(lastPt, glm::vec3(x, y, 0.0));
        m_para += dist;
    }
    m_current_vertices.emplace_back(glm::vec4(x, y, 0.0, m_para));
}

void GLVertexManager::jump()
{
    if (m_current_vertices.size() > 0)
    {
        m_jumps.emplace_back(static_cast<int>(m_current_vertices.size()));
        appendCurrentVertex();
    }
    m_para = DEFAULT_PARA;
}

void GLVertexManager::clear()
{
    m_jumps.clear();
    m_vertex_data.clear();
    m_current_vertices.clear();
    m_para = DEFAULT_PARA;
}

std::vector<glm::vec4> GLVertexManager::getVertexes() const
{
    return m_vertex_data;
}

std::vector<int> GLVertexManager::getJumps() const
{
    return m_jumps;
}

void GLVertexManager::appendCurrentVertex()
{
    m_vertex_data.insert(m_vertex_data.end(), m_current_vertices.begin(), m_current_vertices.end());
    m_current_vertices.clear();
}
