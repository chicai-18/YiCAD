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
