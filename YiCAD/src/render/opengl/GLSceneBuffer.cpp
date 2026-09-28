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

/// @file GLSceneBuffer.cpp
/// @brief 场景底图的离屏帧缓存实现

#include "GLSceneBuffer.h"

#include "YiCadLog.h"

namespace
{

/// @brief 当前绑定的帧缓存的颜色格式（第 0 个颜色附件的内部格式）；取不到时按 GL_RGBA8
GLint boundColorFormat(GLuint target)
{
    GLint format = GL_RGBA8;
    if (target == 0)
    {
        return format;
    }
    GLint type = GL_NONE;
    GLint name = 0;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);
    if (type == GL_RENDERBUFFER)
    {
        // Qt 的多重采样帧缓存用渲染缓冲
        GLint previous = 0;
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &previous);
        glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(name));
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &format);
        glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(previous));
    }
    else if (type == GL_TEXTURE)
    {
        // 没有多重采样时 Qt 的帧缓存用纹理
        GLint previous = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(name));
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previous));
    }
    return format;
}

/// @brief 建一个渲染缓冲并挂到当前帧缓存上；samples 为 0 时不用多重采样
/// @return 实际分配的采样数（驱动可以多给）
GLint attachRenderbuffer(GLuint* id, GLint samples, GLenum format, int width, int height, GLenum attachment)
{
    glGenRenderbuffers(1, id);
    glBindRenderbuffer(GL_RENDERBUFFER, *id);
    if (samples > 0)
    {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, format, width, height);
    }
    else
    {
        glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
    }
    GLint actual = 0;
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &actual);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, *id);
    return actual;
}

}  // namespace

opengl::GLSceneBuffer::~GLSceneBuffer()
{
    destroy();
}

bool opengl::GLSceneBuffer::resize(GLuint target, int width, int height, bool* recreated)
{
    *recreated = false;
    if (m_fbo != 0 && width == m_width && height == m_height)
    {
        return m_usable;
    }
    destroy();
    if (width <= 0 || height <= 0)
    {
        return false;
    }

    // 目标的采样数与颜色格式，要在绑定自己的帧缓存之前取
    GLint samples = 0;
    glGetIntegerv(GL_SAMPLES, &samples);
    const GLint format = boundColorFormat(target);

    GLint previousRenderbuffer = 0;
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &previousRenderbuffer);
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    const GLint colorSamples =
        attachRenderbuffer(&m_color, samples, static_cast<GLenum>(format), width, height, GL_COLOR_ATTACHMENT0);
    const GLint depthSamples =
        attachRenderbuffer(&m_depthStencil, samples, GL_DEPTH24_STENCIL8, width, height, GL_DEPTH_STENCIL_ATTACHMENT);
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(previousRenderbuffer));
    glBindFramebuffer(GL_FRAMEBUFFER, target);

    m_width = width;
    m_height = height;
    m_usable = status == GL_FRAMEBUFFER_COMPLETE && colorSamples == samples && depthSamples == samples;
    if (!m_usable)
    {
        YICAD_LOG(yicad::log::render(), yicad::LogLevel::Warning)
            << "场景底图不可用，改为每帧直接绘制：帧缓存状态 " << status << "，目标采样数 "
            << samples << "，实际 " << colorSamples << "/" << depthSamples;
    }
    *recreated = true;
    return m_usable;
}

void opengl::GLSceneBuffer::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void opengl::GLSceneBuffer::blitTo(GLuint target)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target);
    glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, target);
}

void opengl::GLSceneBuffer::destroy()
{
    if (m_fbo != 0)
    {
        glDeleteFramebuffers(1, &m_fbo);
    }
    if (m_color != 0)
    {
        glDeleteRenderbuffers(1, &m_color);
    }
    if (m_depthStencil != 0)
    {
        glDeleteRenderbuffers(1, &m_depthStencil);
    }
    forget();
}

void opengl::GLSceneBuffer::forget()
{
    m_fbo = 0;
    m_color = 0;
    m_depthStencil = 0;
    m_width = 0;
    m_height = 0;
    m_usable = false;
}

bool opengl::GLSceneBuffer::isValid() const
{
    return m_fbo != 0;
}
