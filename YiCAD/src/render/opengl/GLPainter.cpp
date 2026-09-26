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

/// @file GLPainter.cpp
/// @brief 不带缓存的OpenGL画笔实现，直接逐次提交绘制

#include "GLPainter.h"
#include "GLVertexBufferLayout.h"
#include "Debug.h"
#include <glm/gtc/type_ptr.hpp>

using namespace opengl;

namespace
{
    constexpr int VERTEX_FLOAT_COUNT = 4;       ///< 每顶点浮点数(x,y,z,para)
    constexpr int VERTEX_SIZE_MULT = 4;          ///< sizeof(float)为4字节
    constexpr float ZERO_COORD = 0.0f;           ///< Z坐标零值
    constexpr float FULL_ALPHA = 1.0f;           ///< 完全不透明分量
}

GLPainter::GLPainter()
    : m_fill(false)
{
    m_manager = new GLVertexManager();
}

void GLPainter::move_to(double x, double y)
{
    m_manager->jump();
    m_manager->addVertex(x, y);
}

void GLPainter::line_to(double x, double y)
{
    m_manager->addVertex(x, y);
    m_manager->jump();
}

void opengl::GLPainter::point(double x, double y, double pixelSize)
{
    // TODO: 点绘制待实现
}

void opengl::GLPainter::triangle(double x1, double y1, double x2, double y2, double x3, double y3)
{
    m_manager->jump();
    m_manager->addVertex(x1, y1);
    m_manager->addVertex(x2, y2);
    m_manager->addVertex(x3, y3);
    m_manager->jump();
}

void opengl::GLPainter::rectangle(double x1, double y1, double w, double h)
{
    m_manager->jump();
    m_manager->addVertex(x1, y1);// 左上
    m_manager->addVertex(x1, y1 - h);// 左下
    m_manager->addVertex(x1 + w, y1 - h);// 右下

    //m_manager->addVertex(x1, y1);// 左上
    //m_manager->addVertex(x1 + w, y1 - h);// 右下
    m_manager->addVertex(x1 + w, y1);// 右上
    m_manager->jump();
}

void GLPainter::lineWidth(double lineWidth)
{
    m_penData.lineWidth = lineWidth;
}

void GLPainter::setDash(const double* dashes, const int num_dashes)
{
    m_penData.setDash(dashes, num_dashes);
}

void GLPainter::resetDash()
{
    setDash(nullptr, 0);
}

void GLPainter::source_rgb(double r, double g, double b)
{
    m_painterCommon.selectColor(r, g, b, FULL_ALPHA);
}

void GLPainter::source_rgba(double r, double g, double b, double a)
{
    m_painterCommon.selectColor(r, g, b, a);
}

// TODO: 以下为模型变换注释代码，待确认是否需要恢复
//void opengl::GLPainter::translate(double x, double y)
//{
//	m_model = glm::translate(m_model, glm::vec3((float)x, (float)y, 0.0f));
//	m_matChanged = true;
//}
//
//void opengl::GLPainter::scale(double s)
//{
//	m_model = glm::scale(m_model, glm::vec3((float)s, (float)s, (float)s));
//	m_matChanged = true;
//}
//
//void opengl::GLPainter::rotate(double r)
//{
//	m_model = glm::rotate(m_model, (float)r, glm::vec3(0.0f, 0.0f, 1.0f));
//	m_matChanged = true;
//}
//
//void GLPainter::reset_transformations()
//{
//	m_model = glm::mat4(1.0f);
//	m_matChanged = true;
//}

void GLPainter::stroke()
{
    m_manager->jump();

    auto vertexes = m_manager->getVertexes();
    if (vertexes.size() != 0)
    {
        loadVertexData(&vertexes.at(0).x, static_cast<int>(vertexes.size() * (VERTEX_SIZE_MULT * sizeof(float))));
        draw();
    }
    m_manager->clear();
}

void GLPainter::setFill(bool fill)
{
    m_fill = fill;
}

void GLPainter::loadVertexData(float* vertices, int size)
{
    m_vao.gen();
    m_vbo.gen(vertices, size);

    GLVertexBufferLayout layout;
    layout.push<float>(3);      // (x,y,z)
    layout.push<float>(1);      // (para)
    m_vao.addBuffer(m_vbo, layout);
}

void GLPainter::draw()
{
    GLenum fillMode = m_fill ? GL_FILL : GL_LINE;
    glPolygonMode(GL_FRONT_AND_BACK, fillMode);
    //为了保证多个stroke()都能绘制出来，不能clear
    //glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_painterCommon.useShader(ShaderType::DASH);
    //m_painterCommon.sendMVP();
    m_painterCommon.sendUniform(false, false, true, m_penData);
    bind();
    GLenum renderMode = GL_LINES;
    if (m_fill)
    {
        renderMode = GL_TRIANGLE_FAN;
        //renderMode = GL_TRIANGLES;
    }
    else
    {
        glLineWidth(m_penData.lineWidth);
    }
    auto jumps = m_manager->getJumps();
    std::vector<int>::iterator it;
    int l = 0;
    for (it = jumps.begin(); it != jumps.end(); ++it)
    {
        glDrawArrays(renderMode, l, *(it));
        l += *(it);
    }
}

void GLPainter::bind()
{
    m_vbo.bind();
    m_vao.bind();
}

void GLPainter::unbind()
{
    m_vbo.unbind();
    m_vao.unbind();
}
