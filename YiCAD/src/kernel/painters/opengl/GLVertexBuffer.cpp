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

/// @file GLVertexBuffer.cpp
/// @brief OpenGL顶点缓冲区对象(VBO)封装实现

#include "GLVertexBuffer.h"
#include <GL/glew.h>

using namespace opengl;

GLVertexBuffer::GLVertexBuffer()
    : m_vb_id(0)
{
}

GLVertexBuffer::~GLVertexBuffer()
{
    freeVBO();
}

void GLVertexBuffer::gen(const void* data, unsigned int size)
{
    freeVBO();
    glGenBuffers(1, &m_vb_id);
    glBindBuffer(GL_ARRAY_BUFFER, m_vb_id);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

// 绑定命名缓冲区对象
void GLVertexBuffer::bind()
{
    glBindBuffer(GL_ARRAY_BUFFER, m_vb_id);
}

void GLVertexBuffer::unbind()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLVertexBuffer::freeVBO()
{
    if (m_vb_id != 0)
    {
        unbind();
        glDeleteBuffers(1, &m_vb_id);
        m_vb_id = 0;
    }
}

bool opengl::GLVertexBuffer::isValid() const
{
    return m_vb_id > 0;
}
