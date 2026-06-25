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

/// @file GLFrameBuffer.cpp
/// @brief OpenGL帧缓存封装实现

#include "GLFrameBuffer.h"

GLFrameBuffer::GLFrameBuffer()
    : id(0)
    , default_id(0)
{
}

GLFrameBuffer::~GLFrameBuffer()
{
    freeFBO();
}

void GLFrameBuffer::gen(unsigned int default_id)
{
    freeFBO();
    this->default_id = default_id;
    glGenFramebuffers(1, &id);
    bind();
}

void GLFrameBuffer::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, id);
}

void GLFrameBuffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, default_id);
}

void GLFrameBuffer::freeFBO()
{
    if (id != 0)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, default_id);
        glDeleteFramebuffers(1, &id);
        id = 0;
    }
}

bool GLFrameBuffer::isValid() const
{
    return id > 0;
}
