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

/// @file GLRhiSurface.h
/// @brief RHI 呈现表面的 OpenGL 实现

#ifndef GLRHISURFACE_H
#define GLRHISURFACE_H

#include <GL/glew.h>

#include "RhiResources.h"

class QOpenGLContext;
class QOpenGLWidget;

/// @brief GL 的呈现表面：一个上下文加它的帧缓冲
class GLRhiSurface : public RhiSurface
{
public:
    virtual QOpenGLContext* context() const = 0;
    /// @brief 让本表面的上下文成为当前
    virtual void makeCurrent() = 0;
    /// @brief 本帧的"交换链图像"
    virtual GLuint framebuffer() const = 0;
};

/// @brief QOpenGLWidget 的帧缓冲（第 4.7.3 节："交换链图像"就是 defaultFramebufferObject()）
/// @details 在 paintGL 里开始帧；QOpenGLWidget 自己呈现。尺寸是设备像素
class GLRhiWidgetSurface final : public GLRhiSurface
{
public:
    explicit GLRhiWidgetSurface(QOpenGLWidget& widget);

    std::uint32_t width() const override;
    std::uint32_t height() const override;
    RhiFormat colorFormat() const override { return RhiFormat::RGBA8Unorm; }
    RhiFormat depthStencilFormat() const override { return RhiFormat::Depth24Stencil8; }
    std::uint32_t sampleCount() const override;

    QOpenGLContext* context() const override;
    void makeCurrent() override;
    GLuint framebuffer() const override;

private:
    QOpenGLWidget& m_widget;
};

#endif // GLRHISURFACE_H
