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

/// @file GLTexture.cpp
/// @brief OpenGL纹理封装实现

#include "GLTexture.h"

GLTexture::GLTexture()
    : id(0)
    , useMultiSample(true)
    , useAlpha(true)
{
}

GLTexture::~GLTexture()
{
    freeTexture();
}

void GLTexture::gen(int width, int height, bool useMultiSample, bool useAlpha)
{
    freeTexture();
    glGenTextures(1, &id);
    bind();
    this->useAlpha = useAlpha;
    this->useMultiSample = useMultiSample;
    if (useMultiSample)
    {
        if (useAlpha)
        {
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA, width, height, GL_TRUE);
        }
        else
        {
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, width, height, GL_TRUE);
        }
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, id, 0);
    }
    else
    {
        if (useAlpha)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }
        else
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id, 0);
    }
}

void GLTexture::bind()
{
    if (useMultiSample)
    {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, id);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, id);
    }
}

void GLTexture::unbind()
{
    if (useMultiSample)
    {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void GLTexture::freeTexture()
{
    if (id != 0)
    {
        unbind();
        glDeleteTextures(1, &id);
        id = 0;
    }
}

bool GLTexture::isValid() const
{
    return id > 0;
}
