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

/// @file GLShader.h
/// @brief OpenGL着色器封装，支持shader编译、链接和uniform变量设置

#ifndef GLSHADER_H
#define GLSHADER_H

#include <string>
#include <functional>
#include <GL/glew.h>
#include <glm/glm.hpp>

namespace opengl
{

/// @brief 着色器类型枚举
enum class ShaderType
{
    BASIC,              ///< 基础着色器
    DASH,               ///< 按像素的线型着色器

    LINE,               ///< 直线着色器
    LINESTRIP,          ///< linestrip着色器，用于DmSpline等
    LINETRIPCLOSED,     ///< 闭合的linestrip，用于绘制圆，椭圆等闭合曲线
    RAY,                ///< 射线着色器
    XLINE,              ///< 构造线着色器
    TRIANGLE,           ///< 三角形着色器
    SOLID,              ///< solid着色器

    LINE_NO_WIDTH,              ///< 无线宽直线着色器，用于线宽为1的直线
    LINESTRIP_NO_WIDTH,         ///< 无线宽linestrip着色器，用于线宽为1的linestrip
    LINETRIPCLOSED_NO_WIDTH,    ///< 无线宽闭合的linestrip着色器，用于线宽为1的闭合linestrip

    IMAGE                       ///< 图片纹理着色器
};

/// @brief 着色器程序源
struct GLShaderProgramSource
{
    std::string vertexSource;   ///< 顶点shader
    std::string geometrySource; ///< 几何shader
    std::string fragmentSource; ///< 片段shader
};

/// @brief 着色器
class GLShader
{
public:
    GLShader();
    ~GLShader();

    /// @brief 从文件加载并编译着色器
    /// @param [in] filepath 着色器文件路径
    /// @param [in] variableBinder 绑定变量的回调（可选）
    void gen(const std::string& filepath, std::function<void(GLuint)> variableBinder = [](GLuint programId) {});

    /// @brief 绑定（使用）着色器程序
    void bind() const;

    /// @brief 取消绑定着色器程序
    void unbind() const;

    /// @brief 获取着色器程序ID
    /// @return 程序ID
    unsigned int getID() const;

    // 设置统一变量
    /// @brief 设置4浮点uniform变量
    /// @param [in] name 变量名
    /// @param [in] v0,v1,v2,v3 四个浮点值
    void setUniform4f(const std::string& name, float v0, float v1, float v2, float v3);

    /// @brief 设置整型uniform变量
    /// @param [in] name 变量名
    /// @param [in] value 整数值
    void setUniform1i(const std::string& name, int value);

    /// @brief 设置整型数组uniform变量
    /// @param [in] name 变量名
    /// @param [in] count 数组元素个数
    /// @param [in] value 整型数组指针
    void setUniformiv(const std::string& name, int count, const int* value);

    /// @brief 设置4x4矩阵uniform变量
    /// @param [in] name 变量名
    /// @param [in] matrix 矩阵引用
    void setUniformMat4f(const std::string& name, const glm::mat4& matrix);

    /// @brief 设置单浮点uniform变量
    /// @param [in] name 变量名
    /// @param [in] value 浮点值
    void setUniform1f(const std::string& name, float value);

    /// @brief 设置浮点数组uniform变量
    /// @param [in] name 变量名
    /// @param [in] count 数组元素个数
    /// @param [in] value 浮点数组指针
    void setUniform1fv(const std::string& name, int count, const float* value);

    /// @brief 设置双浮点uniform变量
    /// @param [in] name 变量名
    /// @param [in] v0 第一个浮点值
    /// @param [in] v1 第二个浮点值
    void setUniform2f(const std::string& name, float v0, float v1);

private:
    /// @brief 获取统一变量位置
    /// @param [in] name 变量名
    /// @return 位置索引
    int getUniformLocation(const std::string& name);

    /// @brief 编译着色器脚本
    /// @param [in] shader 着色器源码
    /// @param [in] type 着色器类型
    /// @return 着色器ID
    unsigned int compileShaders(std::string shader, GLenum type);

    /// @brief 解析着色器脚本
    /// @param [in] filepath 着色器文件路径
    /// @return 解析后的着色器源
    GLShaderProgramSource parseShader(const std::string& filepath);

    /// @brief 链接着色器程序
    /// @param [in] vertexShaderID 顶点着色器ID
    /// @param [in] geometryShaderID 几何着色器ID
    /// @param [in] fragmentShaderID 片段着色器ID
    /// @param [in] variableBinder 变量绑定回调
    /// @return 程序ID
    unsigned int linkProgram(unsigned int vertexShaderID, unsigned int geometryShaderID, unsigned int fragmentShaderID,
        std::function<void(GLuint)> variableBinder);

private:
    std::string     m_file_path;        ///< 文件路径
    unsigned int    m_shader_id = 0;    ///< id
    bool            m_has_geometry = false; ///< 是否包含几何
};

/// @brief 着色器手册
struct GLShaders_book
{
    GLShader* basic_shader = nullptr;                       ///< 基础着色器
    GLShader* dash_shader = nullptr;                        ///< 按像素的线型着色器

    GLShader* line_shader = nullptr;                        ///< 直线着色器
    GLShader* line_strip_shader = nullptr;                  ///< linestrip着色器
    GLShader* line_strip_closed_shader = nullptr;           ///< 闭合的linestrip着色器
    GLShader* ray_shader = nullptr;                         ///< 射线着色器
    GLShader* xline_shader = nullptr;                       ///< 构造线着色器
    GLShader* solid_shader = nullptr;                       ///< solid着色器
    GLShader* triangle_shader = nullptr;                    ///< 三角形着色器

    GLShader* line_no_width_shader = nullptr;               ///< 无线宽直线着色器
    GLShader* line_strip_no_width_shader = nullptr;         ///< 无线宽linestrip着色器
    GLShader* line_strip_closed_no_width_shader = nullptr;  ///< 无线宽闭合的linestrip着色器

    GLShader* image_shader = nullptr;                       ///< 图片纹理着色器

    ~GLShaders_book();
};

}

#endif //GLSHADER_H
