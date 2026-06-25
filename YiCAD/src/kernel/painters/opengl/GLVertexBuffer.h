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

/// @file GLVertexBuffer.h
/// @brief OpenGL顶点缓冲区对象(VBO)封装

#ifndef GLVERTEXBUFFER_H
#define GLVERTEXBUFFER_H

namespace opengl
{

/// @brief 顶点缓冲区
class GLVertexBuffer
{
public:
    /// @brief 顶点缓冲区
    GLVertexBuffer();
    ~GLVertexBuffer();
    GLVertexBuffer(const GLVertexBuffer&) = delete;
    GLVertexBuffer& operator=(const GLVertexBuffer&) = delete;

    /// @brief 生成VBO并上传数据
    /// @param [in] data 顶点数据指针
    /// @param [in] size 数据大小（字节）
    void gen(const void* data, unsigned int size);

    /// @brief 绑定命名缓冲区对象
    void bind();

    /// @brief 取消绑定命名缓冲区对象
    void unbind();

    /// @brief 释放VBO对象
    void freeVBO();

    /// @brief 获取VBO的ID
    /// @return VBO的ID
    unsigned int getID() const { return m_vb_id; }

    /// @brief 检查VBO是否有效
    /// @return 有效返回true
    bool isValid() const;

private:
    unsigned int m_vb_id = 0;
};

}

#endif //GLVERTEXBUFFER_H
