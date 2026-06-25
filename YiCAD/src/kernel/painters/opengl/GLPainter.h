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

/// @file GLPainter.h
/// @brief 不带缓存的OpenGL画笔，直接逐次提交绘制

#ifndef GLPAINTER_H
#define GLPAINTER_H

#include <stack>
#include <glm/glm.hpp>
#include "Painter.h"
#include "GLVertexManager.h"
#include "GLShader.h"
#include "GLVertexArray.h"
#include "GLVertexBuffer.h"
#include "GLPainterCommon.h"
#include "GLRenderBuffer.h"
#include "GLPenData.h"

namespace opengl
{

/// @brief 不带缓存的画笔 
class GLPainter : public Painter
{
    GL_PAINTER_COMMON()
public:
    GLPainter();

    virtual void move_to(double x, double y);
    virtual void line_to(double x, double y);
    virtual void point(double x, double y, double pixelSize);
    virtual void triangle(double x1, double y1, double x2, double y2, double x3, double y3);
    virtual void rectangle(double x1, double y1, double w, double h);

    virtual void lineWidth(double lineWidth);
    virtual void setDash(const double* dashes, const int num_dashes);
    virtual void resetDash();
    virtual void source_rgb(double r, double g, double b);
    virtual void source_rgba(double r, double g, double b, double a);

    /// @brief 一次绘制提交，一种类型绘制提交一次
    virtual void stroke() override;

    /// @brief 设置是否填充模式
    /// @param [in] fill 是否填充
    virtual void setFill(bool fill);

protected:
    /// @brief 载入顶点数据到OpenGL缓冲区
    /// @param [in] vertices 顶点数据指针
    /// @param [in] size 数据大小（字节）
    void loadVertexData(float* vertices, int size);

    /// @brief 执行绘制调用
    void draw();

    /// @brief 绑定OpenGL缓冲区
    void bind();

    /// @brief 取消绑定OpenGL缓冲区
    void unbind();

private:
    GLVertexManager* m_manager = nullptr;

    bool            m_fill = false;     ///< 是否填充
    GLPenData       m_penData;           ///< 线型颜色

    // OpenGL对象
    GLVertexArray   m_vao;
    GLVertexBuffer  m_vbo;
};

}

#endif //GLPAINTER_H
