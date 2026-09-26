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

/// @file GLVertexArray.cpp
/// @brief OpenGL顶点数组对象(VAO)封装实现

#include "GLVertexArray.h"
#include <GL/glew.h>

using namespace opengl;

GLVertexArray::GLVertexArray()
    : m_va_id(0)
{
}

GLVertexArray::~GLVertexArray()
{
    freeVAO();
}

void GLVertexArray::gen()
{
    freeVAO();
    glGenVertexArrays(1, &m_va_id);
    glBindVertexArray(m_va_id);
}

void GLVertexArray::addBuffer(GLVertexBuffer& vb, GLVertexBufferLayout& layout)
{
    vb.bind();  //  layout of this vb (vertex buffer) (binding it for being sure its is the vb)

    const auto& elements = layout.getElements();

    unsigned int offset = 0;

    for (unsigned int i = 0; i < elements.size(); i++)
    {
        const auto& element = elements[i];

        glVertexAttribPointer(i, element.count, element.type, element.normalized, layout.getStride(), reinterpret_cast<const void*>(static_cast<uintptr_t>(offset)));
        glEnableVertexAttribArray(i);

        offset += element.count * GLVertexBufferElement::getSizeOfType(element.type);
    }
}

void GLVertexArray::bind()
{
    glBindVertexArray(m_va_id);
}

void GLVertexArray::unbind()
{
    glBindVertexArray(0);
}

void GLVertexArray::freeVAO()
{
    if (m_va_id != 0)
    {
        unbind();
        glDeleteVertexArrays(1, &m_va_id);
        m_va_id = 0;
    }
}

bool opengl::GLVertexArray::isValid() const
{
    return m_va_id > 0;
}
