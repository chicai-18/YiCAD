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

/// @file GLCacheUnit.h
/// @brief 缓存单元数据结构，存储同类型实体的顶点、索引和OpenGL对象

#ifndef GLCACHEUNIT_H
#define GLCACHEUNIT_H

#include <unordered_map>
#include <vector>
#include <GL/glew.h>
#include "GLVertexArray.h"
#include "GLVertexBuffer.h"

constexpr int GELEM_SIZE_FLOAT = 3; ///< 顶点三元组(x,y,z)的元素数

namespace opengl
{

/// @brief 缓冲单元。同一线型、颜色、线宽的同类型实体顶点缓存在一个缓冲单元
struct GLCacheUnit
{
    std::vector<float> vertexes;        ///< 顶点
    std::vector<int> jumps;             ///< 跳转
    std::vector<GLint> startIndices;    ///< 起始索引
    GLenum drawType = GL_POINTS;
    GLVertexArray vao;
    GLVertexBuffer vbo;

    /// @brief 释放缓存单元占用的OpenGL资源
    void free()
    {
        jumps.clear();
        if (vbo.isValid())
        {
            vbo.freeVBO();
        }
        if (vao.isValid())
        {
            vao.freeVAO();
        }
        vertexes.clear();
        startIndices.clear();
    }
};

using GLCacheUnitMap = std::unordered_map<int, GLCacheUnit>; ///< key为画笔的id

}

#endif //GLCACHEUNIT_H
