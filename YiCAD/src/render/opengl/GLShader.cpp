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

/// @file GLShader.cpp
/// @brief OpenGL着色器封装实现，支持shader文件解析、编译和链接

#include "GLShader.h"
#include "Debug.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

using namespace opengl;

namespace
{
    constexpr int SHADER_STAGE_COUNT = 3;   ///< 着色器阶段数(vertex/geometry/fragment)
}

GLShader::GLShader()
    : m_file_path("")
    , m_shader_id(0)
    , m_has_geometry(false)
{
}

GLShader::~GLShader()
{
    glDeleteProgram(m_shader_id);
}

void GLShader::gen(const std::string& filepath, std::function<void(GLuint)> variableBinder)
{
    m_shader_id = 0;
    m_file_path = filepath;

    GLShaderProgramSource source = parseShader(m_file_path);
    unsigned int vertexshaderID = 0;
    unsigned int geometryshaderID = 0;
    unsigned int fragmentshaderID = 0;
    vertexshaderID = compileShaders(source.vertexSource, GL_VERTEX_SHADER);
    if (m_has_geometry)
    {
        geometryshaderID = compileShaders(source.geometrySource, GL_GEOMETRY_SHADER);
    }

    fragmentshaderID = compileShaders(source.fragmentSource, GL_FRAGMENT_SHADER);

    m_shader_id = linkProgram(vertexshaderID, geometryshaderID, fragmentshaderID, std::move(variableBinder));

    bind();
}

unsigned int GLShader::getID() const
{
    return m_shader_id;
}

GLShaderProgramSource GLShader::parseShader(const std::string& filepath)
{
    std::ifstream stream(filepath);

    enum ShaderType
    {
        NONE = -1,
        VERTEX = 0,
        GEOMETRY = 1,
        FRAGMENT = 2
    };

    std::string line;
    std::stringstream ss[SHADER_STAGE_COUNT];
    ShaderType type = ShaderType::NONE;

    while (getline(stream, line))
    {
        if (line.find("#shader") != std::string::npos)
        {
            if (line.find("vertex") != std::string::npos)
            {
                type = ShaderType::VERTEX;
            }
            else if (line.find("geometry") != std::string::npos)
            {
                type = ShaderType::GEOMETRY;
                m_has_geometry = true;
            }
            else if (line.find("fragment") != std::string::npos)
            {
                type = ShaderType::FRAGMENT;
            }
            else if (line.find("include") != std::string::npos)
            {
                std::string name = line.substr(line.find("include") + strlen("include"));
                name = name.substr(name.find("\"") + 1);
                name = name.substr(0, name.find("\""));
                std::string dir = filepath.substr(0, filepath.find_last_of('/'));
                std::string fullname = dir + "/" + name;
                std::ifstream newstream(fullname);
                std::string newline;
                while (getline(newstream, newline))
                {
                    ss[static_cast<int>(type)] << newline << '\n';
                }
            }
        }
        else
        {
            ss[static_cast<int>(type)] << line << '\n';
        }
    }

    return GLShaderProgramSource{ ss[0].str(), ss[1].str(), ss[2].str() };
}

unsigned int GLShader::compileShaders(std::string shader, GLenum type)
{
    const char* shaderCode = shader.c_str();
    GLuint shaderID = glCreateShader(type);

    if (shaderID == 0)
    {
        DEBUG->print("GLShader not created");
        return 0;
    }

    glShaderSource(shaderID, 1, &shaderCode, NULL);
    glCompileShader(shaderID);

    GLint compileStatus = 0;

    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileStatus);

    if (!compileStatus)
    {
        int length = 0;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &length);
        char* error_message = new char[length];

        glGetShaderInfoLog(shaderID, length, &length, error_message);
        DEBUG->print("Cannot compile GLShader:%s\n%s", shader.c_str(), error_message);

        delete[] error_message;
        glDeleteShader(shaderID);
        return 0;
    }

    return shaderID;
}

unsigned int GLShader::linkProgram(unsigned int vertexShaderID, unsigned int geometryShaderID, unsigned int fragmentShaderID,
    std::function<void(GLuint)> variableBinder)
{
    GLuint programID = glCreateProgram();

    if (programID == 0)
    {
        DEBUG->print("Program not created");
        return 0;
    }

    if (vertexShaderID != 0)
    {
        glAttachShader(programID, vertexShaderID);
    }

    if (geometryShaderID != 0 && m_has_geometry)
    {
        glAttachShader(programID, geometryShaderID);
    }

    if (fragmentShaderID != 0)
    {
        glAttachShader(programID, fragmentShaderID);
    }

    variableBinder(programID);

    glLinkProgram(programID);

    GLint linkstatus = 0;

    glGetProgramiv(programID, GL_LINK_STATUS, &linkstatus);

    if (!linkstatus)
    {
        DEBUG->print("Error linking program");
        int length = 0;
        glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &length);
        char* error_message = new char[length];
        glGetProgramInfoLog(programID, length, &length, error_message);
        DEBUG->print("Error linking program:\n%s", error_message);
        delete[] error_message;

        glDetachShader(programID, vertexShaderID);
        glDetachShader(programID, geometryShaderID);
        glDetachShader(programID, fragmentShaderID);
        glDeleteProgram(programID);

        return 0;
    }
    return programID;
}

void GLShader::bind() const
{
    glUseProgram(m_shader_id);
}

void GLShader::unbind() const
{
    glUseProgram(0);
}

void GLShader::setUniform4f(const std::string& name, float v0, float v1, float v2, float v3)
{
    glUniform4f(getUniformLocation(name), v0, v1, v2, v3);
}

void GLShader::setUniform2f(const std::string& name, float v0, float v1)
{
    glUniform2f(getUniformLocation(name), v0, v1);
}

void GLShader::setUniform1i(const std::string& name, int value)
{
    glUniform1i(getUniformLocation(name), value);
}

void opengl::GLShader::setUniformiv(const std::string& name, int count, const int* value)
{
    glUniform1iv(getUniformLocation(name), count, value);
}

void GLShader::setUniformMat4f(const std::string& name, const glm::mat4& matrix)
{
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &matrix[0][0]);
}

void GLShader::setUniform1f(const std::string& name, float value)
{
    glUniform1f(getUniformLocation(name), value);
}

void GLShader::setUniform1fv(const std::string& name, int count, const float* value)
{
    glUniform1fv(getUniformLocation(name), count, value);
}

int GLShader::getUniformLocation(const std::string& name)
{
    int location = glGetUniformLocation(m_shader_id, name.c_str());
    if (location == -1)
    {
        return -1;
    }
    return location;
}

GLShaders_book::~GLShaders_book()
{
    if (basic_shader)
    {
        delete basic_shader;
        basic_shader = nullptr;
    }
    if (dash_shader)
    {
        delete dash_shader;
        dash_shader = nullptr;
    }
    if (line_shader)
    {
        delete line_shader;
        line_shader = nullptr;
    }
    if (line_strip_shader)
    {
        delete line_strip_shader;
        line_strip_shader = nullptr;
    }
    if (line_strip_closed_shader)
    {
        delete line_strip_closed_shader;
        line_strip_closed_shader = nullptr;
    }
    if (ray_shader)
    {
        delete ray_shader;
        ray_shader = nullptr;
    }
    if (xline_shader)
    {
        delete xline_shader;
        xline_shader = nullptr;
    }
    if (solid_shader)
    {
        delete solid_shader;
        solid_shader = nullptr;
    }
    if (triangle_shader)
    {
        delete triangle_shader;
        triangle_shader = nullptr;
    }
    if (line_no_width_shader)
    {
        delete line_no_width_shader;
        line_no_width_shader = nullptr;
    }
    if (line_strip_no_width_shader)
    {
        delete line_strip_no_width_shader;
        line_strip_no_width_shader = nullptr;
    }
    if (line_strip_closed_no_width_shader)
    {
        delete line_strip_closed_no_width_shader;
        line_strip_closed_no_width_shader = nullptr;
    }
    if (image_shader)
    {
        delete image_shader;
        image_shader = nullptr;
    }
}
