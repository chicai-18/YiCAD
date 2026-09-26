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

/// @file GLRenderBuffer.cpp
/// @brief OpenGL渲染缓存封装实现

#include "GLRenderBuffer.h"

GLRenderBuffer::GLRenderBuffer()
    : id(0)
{
}

GLRenderBuffer::~GLRenderBuffer()
{
    freeRBO();
}

void GLRenderBuffer::gen(int width, int height, bool useMultiSample)
{
    freeRBO();
    glGenRenderbuffers(1, &id);
    bind();
    this->useMultiSample = useMultiSample;
    if (useMultiSample)
    {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height);
    }
    else
    {
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    }
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, id);
}

void GLRenderBuffer::bind()
{
    glBindRenderbuffer(GL_RENDERBUFFER, id);
}

void GLRenderBuffer::unbind()
{
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void GLRenderBuffer::freeRBO()
{
    if (id != 0)
    {
        unbind();
        glDeleteRenderbuffers(1, &id);
        id = 0;
    }
}

bool GLRenderBuffer::isValid() const
{
    return id > 0;
}
