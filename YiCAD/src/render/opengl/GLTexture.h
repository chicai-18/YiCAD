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

/// @file GLTexture.h
/// @brief OpenGL纹理封装（暂时不用）

#ifndef GLTEXTURE_H
#define GLTEXTURE_H

#include <GL/glew.h>

/// @brief 对纹理的封装（暂时不用）
class GLTexture
{
public:
    GLTexture();
    ~GLTexture();
    GLTexture(const GLTexture&) = delete;
    GLTexture& operator=(const GLTexture&) = delete;

    /// @brief 创建纹理对象
    /// @param [in] width 宽度（像素）
    /// @param [in] height 高度（像素）
    /// @param [in] useMultiSample 是否使用多重采样
    /// @param [in] useAlpha 是否使用Alpha通道
    void gen(int width, int height, bool useMultiSample = true, bool useAlpha = true);

    /// @brief 绑定纹理
    void bind();

    /// @brief 取消绑定纹理
    void unbind();

    /// @brief 释放纹理对象
    void freeTexture();

    /// @brief 获取纹理ID
    /// @return 纹理ID
    unsigned int getID() const { return id; }

    /// @brief 检查纹理是否有效
    /// @return 有效返回true
    bool isValid() const;

private:
    GLuint id = 0;
    bool useMultiSample = true;
    bool useAlpha = true;
};

#endif //GLTEXTURE_H
