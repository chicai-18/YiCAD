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

/// @file GLImageTextureCache.h
/// @brief 光栅图像的纹理缓存（RENDER_PLAN.md 1.2 步）

#ifndef GLIMAGETEXTURECACHE_H
#define GLIMAGETEXTURECACHE_H

#include <functional>

#include <GL/glew.h>
#include <QHash>
#include <QString>

class QImage;

namespace opengl
{

/// @brief 光栅图像的纹理缓存：按图片来源保存纹理，同一来源只解码、上传一次
/// @details 来源由调用方给出（文件的路径与修改时间，或内嵌像素的内容哈希），缓存不认识图像实体。
///          整图重建时先 beginSweep()，重建中取过的纹理记为用到，endSweep() 释放这一轮没用到的；
///          两次整图重建之间取的纹理（局部重建的选中组、高亮组）不释放，下一轮整图重建再判断。
///          纹理属于当前 GL 上下文，除构造外的调用都要求它是当前的。
class GLImageTextureCache
{
public:
    GLImageTextureCache() = default;
    ~GLImageTextureCache();
    GLImageTextureCache(const GLImageTextureCache&) = delete;
    GLImageTextureCache& operator=(const GLImageTextureCache&) = delete;

    /// @brief 取来源对应的纹理；缓存里没有时调用 load 解码并上传
    /// @param [in] source 图片来源，同一来源的图片内容相同
    /// @param [in] load 解码图片；返回空图像时得到一张空纹理，采样结果为黑色（与原先的行为相同）
    /// @return 纹理对象名
    GLuint texture(const QString& source, const std::function<QImage()>& load);

    /// @brief 整图重建开始：全部纹理记为没用到
    void beginSweep();

    /// @brief 整图重建结束：释放 beginSweep() 之后没取过的纹理
    void endSweep();

private:
    /// @brief 一张纹理
    struct Entry
    {
        GLuint id = 0;      ///< 纹理对象名
        bool used = false;  ///< 这一轮整图重建用到了
    };

    QHash<QString, Entry> m_textures;  ///< 按图片来源
};

}

#endif  // GLIMAGETEXTURECACHE_H
