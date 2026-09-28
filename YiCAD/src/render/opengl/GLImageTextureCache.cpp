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

/// @file GLImageTextureCache.cpp
/// @brief 光栅图像的纹理缓存实现

#include "GLImageTextureCache.h"

#include <QImage>

#include "GLFrameStats.h"

opengl::GLImageTextureCache::~GLImageTextureCache()
{
    for (const Entry& entry : m_textures)
    {
        glDeleteTextures(1, &entry.id);
    }
}

GLuint opengl::GLImageTextureCache::texture(const QString& source, const std::function<QImage()>& load)
{
    auto it = m_textures.find(source);
    if (it != m_textures.end())
    {
        it->used = true;
        return it->id;
    }

    Entry entry;
    entry.used = true;
    glGenTextures(1, &entry.id);
    glBindTexture(GL_TEXTURE_2D, entry.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // GL 的纹理坐标原点在左下角，图像的第一行在上面，所以上下翻转
    const QImage image = load().convertToFormat(QImage::Format_RGBA8888).mirrored();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 image.constBits());
    GLFrameStats::addUploadBytes(image.sizeInBytes());
    glBindTexture(GL_TEXTURE_2D, 0);

    m_textures.insert(source, entry);
    return entry.id;
}

void opengl::GLImageTextureCache::beginSweep()
{
    for (Entry& entry : m_textures)
    {
        entry.used = false;
    }
}

void opengl::GLImageTextureCache::endSweep()
{
    for (auto it = m_textures.begin(); it != m_textures.end();)
    {
        if (it->used)
        {
            ++it;
        }
        else
        {
            glDeleteTextures(1, &it->id);
            it = m_textures.erase(it);
        }
    }
}
