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

/// @file GLVertexArray.h
/// @brief OpenGL顶点数组对象(VAO)封装

#ifndef GLVERTEXARRAY_H
#define GLVERTEXARRAY_H

#include "GLVertexBuffer.h"
#include "GLVertexBufferLayout.h"

namespace opengl
{

/// @brief 顶点数组
class GLVertexArray
{
public:
    /// @brief 顶点数组
    GLVertexArray();
    ~GLVertexArray();
    GLVertexArray(const GLVertexArray&) = delete;
    GLVertexArray& operator=(const GLVertexArray&) = delete;

    /// @brief 生成VAO对象
    void gen();

    /// @brief 绑定VAO
    void bind();

    /// @brief 取消绑定VAO
    void unbind();

    /// @brief 释放VAO对象
    void freeVAO();

    /// @brief 检查VAO是否有效
    /// @return 有效返回true
    bool isValid() const;

    /// @brief 向VAO添加顶点缓冲区及布局
    /// @param [in,out] vb 顶点缓冲区
    /// @param [in] layout 顶点布局
    void addBuffer(GLVertexBuffer& vb, GLVertexBufferLayout& layout);

    /// @brief 获取VAO的ID
    /// @return VAO的ID
    unsigned int getID() const { return m_va_id; }

private:
    unsigned int m_va_id = 0;
};

}

#endif //GLVERTEXARRAY_H
