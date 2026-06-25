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

/// @file GLVertexManager.h
/// @brief OpenGL顶点管理器，管理当前绘制路径的顶点和跳转信息

#ifndef GLVERTEXMANAGER_H
#define GLVERTEXMANAGER_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

/// @brief 顶点管理
class GLVertexManager
{
public:
    GLVertexManager() = default;

    /// @brief 添加顶点
    /// @param [in] x 顶点X坐标
    /// @param [in] y 顶点Y坐标
    void addVertex(double x, double y);

    /// @brief 将当前顶点保存到顶点数据，保存跳转
    void jump();

    /// @brief 清空顶点和跳转数据
    void clear();

    /// @brief 获取顶点数据
    /// @return 顶点数据的常量引用
    std::vector<glm::vec4> getVertexes() const;

    /// @brief 获取跳转数据
    /// @return 跳转数据的常量引用
    std::vector<int> getJumps() const;

private:
    /// @brief 将当前顶点保存到顶点数据
    void appendCurrentVertex();

private:
    std::vector<glm::vec4>  m_vertex_data;          ///< 顶点数据
    std::vector<int>        m_jumps;                ///< 跳转（用于顶点数据的多次绘制）
    std::vector<glm::vec4>  m_current_vertices;     ///< 当前顶点
    float                   m_para = 0.0f;          ///< 参数
};

#endif //GLVERTEXMANAGER_H
