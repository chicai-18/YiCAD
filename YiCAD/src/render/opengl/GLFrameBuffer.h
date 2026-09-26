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

/// @file GLFrameBuffer.h
/// @brief OpenGL帧缓存封装（暂时不用）

#ifndef GLFRAMEBUFFER_H
#define GLFRAMEBUFFER_H

#include <GL/glew.h>

/// @brief 对帧缓存的封装（暂时不用）
class GLFrameBuffer
{
public:
    GLFrameBuffer();
    ~GLFrameBuffer();
    GLFrameBuffer(const GLFrameBuffer&) = delete;
    GLFrameBuffer& operator=(const GLFrameBuffer&) = delete;

    /// @brief 创建帧缓存对象
    /// @param [in] default_id 默认帧缓存ID
    void gen(unsigned int default_id);

    /// @brief 绑定帧缓存
    void bind();

    /// @brief 取消绑定帧缓存
    void unbind();

    /// @brief 释放帧缓存对象
    void freeFBO();

    /// @brief 获取帧缓存ID
    /// @return 帧缓存ID
    unsigned int getID() const { return id; }

    /// @brief 检查帧缓存是否有效
    /// @return 有效返回true
    bool isValid() const;

private:
    GLuint id = 0;
    GLuint default_id = 0;
};

#endif //GLFRAMEBUFFER_H
