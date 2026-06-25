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

/// @file GLPainterCommon.cpp
/// @brief 画笔通用功能类实现，包含着色器创建、坐标转换、视图矩阵管理等功能

#include "GLPainterCommon.h"
#include "Debug.h"
#include <glm/gtc/type_ptr.hpp>
#include <QCoreApplication>

namespace
{
    constexpr float MAT4_IDENTITY_VAL = 1.0f;      ///< 单位矩阵元素值
    constexpr float ORTHO_NEAR = -1.0f;             ///< 正交投影近平面
    constexpr float ORTHO_FAR = 1.0f;               ///< 正交投影远平面
    constexpr float HALF_DIVISOR = 2.0f;            ///< 对半除数
    constexpr float ZERO_COORD = 0.0f;              ///< 坐标零值
    constexpr float LINE_WIDTH_DEFAULT = 1.0f;      ///< 默认线宽
    constexpr float DASH_SUM_DEFAULT = 0.0f;        ///< 线型总和默认值
    constexpr int DASH_SIZE_DEFAULT = 0;            ///< 线型大小默认值
    constexpr int BOOL_TO_INT_TRUE = 1;             ///< 布尔转整数true值
    constexpr int BOOL_TO_INT_FALSE = 0;            ///< 布尔转整数false值
}

opengl::GLPainterCommon::GLPainterCommon()
    : m_model(MAT4_IDENTITY_VAL)
    , m_view(MAT4_IDENTITY_VAL)
    , m_proj(MAT4_IDENTITY_VAL)
    , m_mvp(MAT4_IDENTITY_VAL)
    , m_matChanged(true)
    , m_scale(MAT4_IDENTITY_VAL)
{
    auto resourcesPath = QCoreApplication::applicationDirPath().toStdString();
    m_shader_path = resourcesPath + "/resources/shaders/";
}

opengl::GLPainterCommon::~GLPainterCommon()
{
}

double opengl::GLPainterCommon::device_width()
{
    return static_cast<double>(m_deviceWidth);
}

double opengl::GLPainterCommon::device_height()
{
    return static_cast<double>(m_deviceHeight);
}

void opengl::GLPainterCommon::create_resources()
{
    // On Windows, GLEW context is not shared between libraries. We have to recreate it
    GLenum err = glewInit();
    // TODO : 在部分linux虚拟机中获得GLEW_ERROR_NO_GLX_DISPLAY
    if (err == GLEW_ERROR_NO_GLX_DISPLAY)
    {
        err = GLEW_OK;
    }
    if (err != GLEW_OK)
    {
        DEBUG->print("GLEW Error in Painter:", glewGetErrorString(err));
    }
    if (!GLEW_VERSION_2_1)
    {
        DEBUG->print("GLEW version 2.1 is not available");
    }

    m_shaderBook.basic_shader = new GLShader();
    m_shaderBook.basic_shader->gen(m_shader_path + "basic_shader.shader");
    m_shaderBook.basic_shader->unbind();

    m_shaderBook.dash_shader = new GLShader();
    m_shaderBook.dash_shader->gen(m_shader_path + "dash.shader");
    m_shaderBook.dash_shader->unbind();

    m_shaderBook.line_shader = new GLShader();
    m_shaderBook.line_shader->gen(m_shader_path + "line.shader");
    m_shaderBook.line_shader->unbind();

    m_shaderBook.line_strip_shader = new GLShader();
    m_shaderBook.line_strip_shader->gen(m_shader_path + "line_strip.shader");
    m_shaderBook.line_strip_shader->unbind();

    m_shaderBook.line_strip_closed_shader = new GLShader();
    m_shaderBook.line_strip_closed_shader->gen(m_shader_path + "line_strip_closed.shader");
    m_shaderBook.line_strip_closed_shader->unbind();

    m_shaderBook.ray_shader = new GLShader();
    m_shaderBook.ray_shader->gen(m_shader_path + "ray.shader");
    m_shaderBook.ray_shader->unbind();

    m_shaderBook.xline_shader = new GLShader();
    m_shaderBook.xline_shader->gen(m_shader_path + "xline.shader");
    m_shaderBook.xline_shader->unbind();

    m_shaderBook.solid_shader = new GLShader();
    m_shaderBook.solid_shader->gen(m_shader_path + "solid.shader");
    m_shaderBook.solid_shader->unbind();

    m_shaderBook.triangle_shader = new GLShader();
    m_shaderBook.triangle_shader->gen(m_shader_path + "triangle.shader");
    m_shaderBook.triangle_shader->unbind();

    m_shaderBook.line_no_width_shader = new GLShader();
    m_shaderBook.line_no_width_shader->gen(m_shader_path + "line_no_width.shader");
    m_shaderBook.line_no_width_shader->unbind();

    m_shaderBook.line_strip_no_width_shader = new GLShader();
    m_shaderBook.line_strip_no_width_shader->gen(m_shader_path + "line_strip_no_width.shader");
    m_shaderBook.line_strip_no_width_shader->unbind();

    m_shaderBook.line_strip_closed_no_width_shader = new GLShader();
    m_shaderBook.line_strip_closed_no_width_shader->gen(m_shader_path + "line_strip_closed_no_width.shader");
    m_shaderBook.line_strip_closed_no_width_shader->unbind();

    m_shaderBook.image_shader = new GLShader();
    m_shaderBook.image_shader->gen(m_shader_path + "image.shader");
    m_shaderBook.image_shader->setUniform1i("u_Texture", 0);
    m_shaderBook.image_shader->unbind();

    //glEnable(GL_BLEND);
    //glEnable(GL_POLYGON_SMOOTH); // 开启多边形反走样或反锯齿 优化solid三角形缩小时不显示的问题
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void opengl::GLPainterCommon::new_device_size(unsigned int width, unsigned int height)
{
    glViewport(0, 0, width, height);	//设置显示范围（可用来设置多个视口）
    m_deviceWidth = static_cast<float>(width);
    m_deviceHeight = static_cast<float>(height);
    updateProj();
}

void opengl::GLPainterCommon::user_to_device(double x, double y, double* deviceX, double* deviceY)
{
    calculateMVP();
    glm::vec4 temp = glm::vec4(x, y, 0, 1);
    temp = m_mv * temp;	// 此处计算出的temp是view下的坐标（逻辑坐标）
    *deviceX = m_deviceWidth / HALF_DIVISOR + temp.x / m_scale;
    *deviceY = m_deviceHeight / HALF_DIVISOR - temp.y / m_scale;
}

void opengl::GLPainterCommon::device_to_user(double x, double y, double* userX, double* userY)
{
    calculateMVP();
    double x_inView = (x - m_deviceWidth / HALF_DIVISOR) * m_scale;
    double y_inView = (m_deviceHeight / HALF_DIVISOR - y) * m_scale;
    glm::vec4 temp = glm::vec4(x_inView, y_inView, 0, 1);
    temp = glm::inverse(m_mv) * temp;
    *userX = temp.x;
    *userY = temp.y;
}

double opengl::GLPainterCommon::getScale()
{
    return m_scale;
}

void opengl::GLPainterCommon::setScale(double s)
{
    m_scale = s;
    updateProj();
}

void opengl::GLPainterCommon::setViewPosition(double posx, double posy)
{
    m_view = glm::translate(glm::mat4(MAT4_IDENTITY_VAL), glm::vec3(-posx, -posy, ZERO_COORD));
    m_matChanged = true;
}

void opengl::GLPainterCommon::scale(double s, double x_world, double y_world)
{
    // 通过移动视点的方式实现缩放
    double deviceX = 0.0;
    double deviceY = 0.0;
    user_to_device(x_world, y_world, &deviceX, &deviceY);
    double deviceDx = deviceX - m_deviceWidth / HALF_DIVISOR;
    double deviceDy = -(deviceY - m_deviceHeight / HALF_DIVISOR);//向上为正
    double newScale = m_scale * s;
    double Ox_new = x_world - newScale * deviceDx;	//视点的新坐标
    double Oy_new = y_world - newScale * deviceDy;
    m_view = glm::translate(glm::mat4(MAT4_IDENTITY_VAL), glm::vec3(-Ox_new, -Oy_new, ZERO_COORD));	//视图矩阵的平移部分的值，是相机坐标的负值
    m_scale = newScale;
    updateProj();
}

void opengl::GLPainterCommon::translateView(double x, double y)
{
    m_view = glm::translate(m_view, glm::vec3(x, y, 0.0));
    m_matChanged = true;
}

void opengl::GLPainterCommon::setModelOffset(double offsetX, double offsetY)
{
    m_model = glm::translate(glm::mat4(MAT4_IDENTITY_VAL), glm::vec3(offsetX, offsetY, 0.0));
    m_matChanged = true;
}

void opengl::GLPainterCommon::selectColor(float R, float G, float B, float A)
{
    m_shaderBook.basic_shader->bind();
    m_shaderBook.basic_shader->setUniform4f("u_Color", R, G, B, A);
    m_shaderBook.basic_shader->unbind();

    m_shaderBook.dash_shader->bind();
    m_shaderBook.dash_shader->setUniform4f("u_Color", R, G, B, A);
    m_shaderBook.dash_shader->unbind();

    m_shaderBook.line_shader->bind();
    m_shaderBook.line_shader->setUniform4f("u_Color", R, G, B, A);
    m_shaderBook.line_shader->unbind();

    m_shaderBook.line_strip_shader->bind();
    m_shaderBook.line_strip_shader->setUniform4f("u_Color", R, G, B, A);
    m_shaderBook.line_strip_shader->unbind();

    m_shaderBook.line_strip_closed_shader->bind();
    m_shaderBook.line_strip_closed_shader->setUniform4f("u_Color", R, G, B, A);
    m_shaderBook.line_strip_closed_shader->unbind();

    m_shaderBook.ray_shader->bind();
    m_shaderBook.ray_shader->setUniform4f("u_Color", R, G, B, A);
    m_shaderBook.ray_shader->unbind();

    m_shaderBook.xline_shader->bind();
    m_shaderBook.xline_shader->setUniform4f("u_Color", R, G, B, A);
    m_shaderBook.xline_shader->unbind();

    m_shaderBook.solid_shader->bind();
    m_shaderBook.solid_shader->setUniform4f("u_Color", R, G, B, A);
    m_shaderBook.solid_shader->unbind();

    m_shaderBook.triangle_shader->bind();
    m_shaderBook.triangle_shader->setUniform4f("u_Color", R, G, B, A);
    m_shaderBook.triangle_shader->unbind();

    m_shaderBook.line_no_width_shader->bind();
    m_shaderBook.line_no_width_shader->setUniform4f("u_Color", R, G, B, A);
    m_shaderBook.line_no_width_shader->unbind();

    m_shaderBook.line_strip_no_width_shader->bind();
    m_shaderBook.line_strip_no_width_shader->setUniform4f("u_Color", R, G, B, A);
    m_shaderBook.line_strip_shader->unbind();

    m_shaderBook.line_strip_closed_no_width_shader->bind();
    m_shaderBook.line_strip_closed_no_width_shader->setUniform4f("u_Color", R, G, B, A);
    m_shaderBook.line_strip_closed_no_width_shader->unbind();

    m_shaderBook.image_shader->bind();
    m_shaderBook.image_shader->setUniform4f("u_Color", R, G, B, A);
    m_shaderBook.image_shader->unbind();
}

void opengl::GLPainterCommon::selectSelectedColor(float R, float G, float B, float A)
{
    m_shaderBook.line_shader->bind();
    m_shaderBook.line_shader->setUniform4f("u_selectedColor", R, G, B, A);
    m_shaderBook.line_shader->unbind();

    m_shaderBook.line_strip_shader->bind();
    m_shaderBook.line_strip_shader->setUniform4f("u_selectedColor", R, G, B, A);
    m_shaderBook.line_strip_shader->unbind();

    m_shaderBook.line_strip_closed_shader->bind();
    m_shaderBook.line_strip_closed_shader->setUniform4f("u_selectedColor", R, G, B, A);
    m_shaderBook.line_strip_closed_shader->unbind();

    m_shaderBook.ray_shader->bind();
    m_shaderBook.ray_shader->setUniform4f("u_selectedColor", R, G, B, A);
    m_shaderBook.ray_shader->unbind();

    m_shaderBook.xline_shader->bind();
    m_shaderBook.xline_shader->setUniform4f("u_selectedColor", R, G, B, A);
    m_shaderBook.xline_shader->unbind();

    m_shaderBook.solid_shader->bind();
    m_shaderBook.solid_shader->setUniform4f("u_selectedColor", R, G, B, A);
    m_shaderBook.solid_shader->unbind();

    m_shaderBook.triangle_shader->bind();
    m_shaderBook.triangle_shader->setUniform4f("u_selectedColor", R, G, B, A);
    m_shaderBook.triangle_shader->unbind();

    m_shaderBook.line_no_width_shader->bind();
    m_shaderBook.line_no_width_shader->setUniform4f("u_selectedColor", R, G, B, A);
    m_shaderBook.line_no_width_shader->unbind();

    m_shaderBook.line_strip_no_width_shader->bind();
    m_shaderBook.line_strip_no_width_shader->setUniform4f("u_selectedColor", R, G, B, A);
    m_shaderBook.line_strip_no_width_shader->unbind();

    m_shaderBook.line_strip_closed_no_width_shader->bind();
    m_shaderBook.line_strip_closed_no_width_shader->setUniform4f("u_selectedColor", R, G, B, A);
    m_shaderBook.line_strip_closed_no_width_shader->unbind();

    m_shaderBook.image_shader->bind();
    m_shaderBook.image_shader->setUniform4f("u_selectedColor", R, G, B, A);
    m_shaderBook.image_shader->unbind();
}

void opengl::GLPainterCommon::selectHighlightColor(float R, float G, float B, float A)
{
    m_shaderBook.line_shader->bind();
    m_shaderBook.line_shader->setUniform4f("u_highlightColor", R, G, B, A);
    m_shaderBook.line_shader->unbind();

    m_shaderBook.line_strip_shader->bind();
    m_shaderBook.line_strip_shader->setUniform4f("u_highlightColor", R, G, B, A);
    m_shaderBook.line_strip_shader->unbind();

    m_shaderBook.line_strip_closed_shader->bind();
    m_shaderBook.line_strip_closed_shader->setUniform4f("u_highlightColor", R, G, B, A);
    m_shaderBook.line_strip_closed_shader->unbind();

    m_shaderBook.ray_shader->bind();
    m_shaderBook.ray_shader->setUniform4f("u_highlightColor", R, G, B, A);
    m_shaderBook.ray_shader->unbind();

    m_shaderBook.xline_shader->bind();
    m_shaderBook.xline_shader->setUniform4f("u_highlightColor", R, G, B, A);
    m_shaderBook.xline_shader->unbind();

    m_shaderBook.solid_shader->bind();
    m_shaderBook.solid_shader->setUniform4f("u_highlightColor", R, G, B, A);
    m_shaderBook.solid_shader->unbind();

    m_shaderBook.triangle_shader->bind();
    m_shaderBook.triangle_shader->setUniform4f("u_highlightColor", R, G, B, A);
    m_shaderBook.triangle_shader->unbind();

    m_shaderBook.line_no_width_shader->bind();
    m_shaderBook.line_no_width_shader->setUniform4f("u_highlightColor", R, G, B, A);
    m_shaderBook.line_no_width_shader->unbind();

    m_shaderBook.line_strip_no_width_shader->bind();
    m_shaderBook.line_strip_no_width_shader->setUniform4f("u_highlightColor", R, G, B, A);
    m_shaderBook.line_strip_no_width_shader->unbind();

    m_shaderBook.line_strip_closed_no_width_shader->bind();
    m_shaderBook.line_strip_closed_no_width_shader->setUniform4f("u_highlightColor", R, G, B, A);
    m_shaderBook.line_strip_closed_no_width_shader->unbind();

    m_shaderBook.image_shader->bind();
    m_shaderBook.image_shader->setUniform4f("u_highlightColor", R, G, B, A);
    m_shaderBook.image_shader->unbind();
}

void opengl::GLPainterCommon::useShader(ShaderType shaderType)
{
    switch (shaderType)
    {
    case opengl::ShaderType::BASIC:
        m_currentShader = m_shaderBook.basic_shader;
        break;
    case opengl::ShaderType::DASH:
        m_currentShader = m_shaderBook.dash_shader;
        break;
    case opengl::ShaderType::LINE:
        m_currentShader = m_shaderBook.line_shader;
        break;
    case opengl::ShaderType::LINESTRIP:
        m_currentShader = m_shaderBook.line_strip_shader;
        break;
    case opengl::ShaderType::LINETRIPCLOSED:
        m_currentShader = m_shaderBook.line_strip_closed_shader;
        break;
    case opengl::ShaderType::RAY:
        m_currentShader = m_shaderBook.ray_shader;
        break;
    case opengl::ShaderType::XLINE:
        m_currentShader = m_shaderBook.xline_shader;
        break;
    case opengl::ShaderType::SOLID:
        m_currentShader = m_shaderBook.solid_shader;
        break;
    case opengl::ShaderType::TRIANGLE:
        m_currentShader = m_shaderBook.triangle_shader;
        break;

    case opengl::ShaderType::LINE_NO_WIDTH:
        m_currentShader = m_shaderBook.line_no_width_shader;
        break;
    case opengl::ShaderType::LINESTRIP_NO_WIDTH:
        m_currentShader = m_shaderBook.line_strip_no_width_shader;
        break;
    case opengl::ShaderType::LINETRIPCLOSED_NO_WIDTH:
        m_currentShader = m_shaderBook.line_strip_closed_no_width_shader;
        break;
    case opengl::ShaderType::IMAGE:
        m_currentShader = m_shaderBook.image_shader;
        break;
    default:
        break;
    }
    m_currentShader->bind();
}

void opengl::GLPainterCommon::sendUniform(bool isSelected, bool isHighlight, bool isDisplayLineWidth, const opengl::GLPenData& penData)
{
    sendMVP();
    opengl::GLShader* currentShader = getCurrentShader();
    opengl::ShaderType shaderType = getShaderType();

    if (shaderType != opengl::ShaderType::BASIC && shaderType != opengl::ShaderType::IMAGE)
    {
        if (isDisplayLineWidth)
        {
            currentShader->setUniform1f("u_lineWidth", penData.lineWidth);
        }
        else
        {
            currentShader->setUniform1f("u_lineWidth", LINE_WIDTH_DEFAULT);
        }
        if (penData.m_dashes_size > 0)
        {
            currentShader->setUniform1fv("u_dashes", penData.m_dashes_size, &penData.m_dashes_data[0]);
            currentShader->setUniform1i("u_dashes_size", penData.m_dashes_size);
            currentShader->setUniform1f("u_dash_sum_length", penData.sum_length);
            if (penData.zero_num != 0)
            {
                currentShader->setUniform1fv("u_dash_zero_paras", penData.zero_num, &penData.zero_paras[0]);
            }
            else
            {
                currentShader->setUniform1fv("u_dash_zero_paras", 0, nullptr);
            }
            if (penData.pair_num != 0)
            {
                currentShader->setUniform1fv("u_dash_para_pairs", penData.pair_num * 2, &penData.para_pairs[0]);
            }
            else
            {
                currentShader->setUniform1fv("u_dash_para_pairs", 0, nullptr);
            }
            if (penData.blank_pair_num != 0)
            {
                currentShader->setUniform1fv("u_dash_blank_para_pairs", penData.blank_pair_num * 2, &penData.blank_para_pairs[0]);
            }
            else
            {
                currentShader->setUniform1fv("u_dash_blank_para_pairs", 0, nullptr);
            }
            currentShader->setUniform1i("u_dash_para_pairs_num", penData.pair_num);
            currentShader->setUniform1i("u_dash_blank_para_pairs_num", penData.blank_pair_num);
            currentShader->setUniform1i("u_dash_zero_num", penData.zero_num);
        }
        else
        {
            currentShader->setUniform1fv("u_dashes", 0, nullptr);
            currentShader->setUniform1i("u_dashes_size", DASH_SIZE_DEFAULT);
            currentShader->setUniform1f("u_dash_sum_length", DASH_SUM_DEFAULT);
            currentShader->setUniform1fv("u_dash_zero_paras", 0, nullptr);
            currentShader->setUniform1fv("u_dash_para_pairs", 0, nullptr);
            currentShader->setUniform1fv("u_dash_blank_para_pairs", 0, nullptr);
            currentShader->setUniform1i("u_dash_para_pairs_num", DASH_SIZE_DEFAULT);
            currentShader->setUniform1i("u_dash_blank_para_pairs_num", DASH_SIZE_DEFAULT);
            currentShader->setUniform1i("u_dash_zero_num", DASH_SIZE_DEFAULT);
        }
        currentShader->setUniform2f("u_viewportSize", device_width(), device_height());
        currentShader->setUniform1i("u_isSelected", isSelected ? BOOL_TO_INT_TRUE : BOOL_TO_INT_FALSE);
        currentShader->setUniform1i("u_isHighlighted", isHighlight ? BOOL_TO_INT_TRUE : BOOL_TO_INT_FALSE);
    }
    else if (shaderType == opengl::ShaderType::IMAGE)
    {
        currentShader->setUniform1i("u_isSelected", isSelected ? BOOL_TO_INT_TRUE : BOOL_TO_INT_FALSE);
        currentShader->setUniform1i("u_isHighlighted", isHighlight ? BOOL_TO_INT_TRUE : BOOL_TO_INT_FALSE);
    }
}

void opengl::GLPainterCommon::sendMVP()
{
    calculateMVP();

    ShaderType shaderType = getShaderType();
    if (shaderType != opengl::ShaderType::BASIC)
    {
        m_currentShader->setUniformMat4f("u_MVP", m_mvp);
        m_currentShader->setUniformMat4f("u_MV", m_mv);
        m_currentShader->setUniformMat4f("u_P", m_proj);
    }
    else if (shaderType == opengl::ShaderType::BASIC)
    {
        m_currentShader->setUniformMat4f("u_MVP", m_mvp);
    }
}

opengl::ShaderType opengl::GLPainterCommon::getShaderType()
{
    if (m_currentShader == m_shaderBook.basic_shader)
    {
        return opengl::ShaderType::BASIC;
    }
    else if (m_currentShader == m_shaderBook.dash_shader)
    {
        return opengl::ShaderType::DASH;
    }
    else if (m_currentShader == m_shaderBook.line_shader)
    {
        return opengl::ShaderType::LINE;
    }
    else if (m_currentShader == m_shaderBook.line_strip_shader)
    {
        return opengl::ShaderType::LINESTRIP;
    }
    else if (m_currentShader == m_shaderBook.line_strip_closed_shader)
    {
        return opengl::ShaderType::LINETRIPCLOSED;
    }
    else if (m_currentShader == m_shaderBook.ray_shader)
    {
        return opengl::ShaderType::RAY;
    }
    else if (m_currentShader == m_shaderBook.xline_shader)
    {
        return opengl::ShaderType::XLINE;
    }
    else if (m_currentShader == m_shaderBook.solid_shader)
    {
        return opengl::ShaderType::SOLID;
    }
    else if (m_currentShader == m_shaderBook.triangle_shader)
    {
        return opengl::ShaderType::TRIANGLE;
    }
    else if (m_currentShader == m_shaderBook.line_no_width_shader)
    {
        return opengl::ShaderType::LINE_NO_WIDTH;
    }
    else if (m_currentShader == m_shaderBook.line_strip_no_width_shader)
    {
        return opengl::ShaderType::LINESTRIP_NO_WIDTH;
    }
    else if (m_currentShader == m_shaderBook.line_strip_closed_no_width_shader)
    {
        return opengl::ShaderType::LINETRIPCLOSED_NO_WIDTH;
    }
    else if (m_currentShader == m_shaderBook.image_shader)
    {
        return opengl::ShaderType::IMAGE;
    }
    else
    {
        return opengl::ShaderType::BASIC;
    }
}

opengl::GLShader* opengl::GLPainterCommon::getCurrentShader() const
{
    return m_currentShader;
}

void opengl::GLPainterCommon::calculateMVP()
{
    if (m_matChanged)
    {
        m_mv = m_view * m_model;
        m_mvp = m_proj * m_mv;
        m_matChanged = false;
    }
}

void opengl::GLPainterCommon::updateProj()
{
    m_proj = glm::ortho(-m_deviceWidth / HALF_DIVISOR * m_scale, m_deviceWidth / HALF_DIVISOR * m_scale,
        -m_deviceHeight / HALF_DIVISOR * m_scale, m_deviceHeight / HALF_DIVISOR * m_scale,
        ORTHO_NEAR, ORTHO_FAR);
    m_matChanged = true;
}
