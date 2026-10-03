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

/// @file GLRhiSurface.cpp
/// @brief RHI 呈现表面的 OpenGL 实现

#include "GLRhiSurface.h"

#include <algorithm>
#include <cmath>

#include <QOpenGLContext>
#include <QOpenGLWidget>

GLRhiWidgetSurface::GLRhiWidgetSurface(QOpenGLWidget& widget)
    : m_widget(widget)
{
}

std::uint32_t GLRhiWidgetSurface::width() const
{
    return static_cast<std::uint32_t>(std::lround(m_widget.width() * m_widget.devicePixelRatioF()));
}

std::uint32_t GLRhiWidgetSurface::height() const
{
    return static_cast<std::uint32_t>(std::lround(m_widget.height() * m_widget.devicePixelRatioF()));
}

std::uint32_t GLRhiWidgetSurface::sampleCount() const
{
    // 按帧缓冲实际的采样数：上下文建好后 format().samples() 不一定是 QOpenGLWidget 自己的帧缓冲的采样数
    // （阶段 1 的场景底图也是查询目标后才建）。不在本表面的上下文里时退回请求的格式
    if (QOpenGLContext::currentContext() == m_widget.context() && m_widget.context())
    {
        GLint previous = 0;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_widget.defaultFramebufferObject());
        GLint samples = 0;
        glGetIntegerv(GL_SAMPLES, &samples);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(previous));
        return static_cast<std::uint32_t>(std::max(1, samples));
    }
    return static_cast<std::uint32_t>(std::max(1, m_widget.format().samples()));
}

QOpenGLContext* GLRhiWidgetSurface::context() const
{
    return m_widget.context();
}

void GLRhiWidgetSurface::makeCurrent()
{
    m_widget.makeCurrent();
}

GLuint GLRhiWidgetSurface::framebuffer() const
{
    return m_widget.defaultFramebufferObject();
}
