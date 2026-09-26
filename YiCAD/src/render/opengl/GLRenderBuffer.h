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

/// @file GLRenderBuffer.h
/// @brief OpenGL渲染缓存封装（暂时不用）

#ifndef GLRENDERBUFFER_H
#define GLRENDERBUFFER_H

#include <GL/glew.h>

/// @brief 对RenderBuffer的封装（暂时不用）
class GLRenderBuffer
{
public:
    GLRenderBuffer();
    ~GLRenderBuffer();
    GLRenderBuffer(const GLRenderBuffer&) = delete;
    GLRenderBuffer& operator=(const GLRenderBuffer&) = delete;

    /// @brief 创建渲染缓存对象
    /// @param [in] width 宽度（像素）
    /// @param [in] height 高度（像素）
    /// @param [in] useMultiSample 是否使用多重采样
    void gen(int width, int height, bool useMultiSample = true);

    /// @brief 绑定渲染缓存
    void bind();

    /// @brief 取消绑定渲染缓存
    void unbind();

    /// @brief 释放渲染缓存对象
    void freeRBO();

    /// @brief 检查渲染缓存是否有效
    /// @return 有效返回true
    bool isValid() const;

private:
    GLuint id = 0;
    bool useMultiSample = false;
};

#endif //GLRENDERBUFFER_H
