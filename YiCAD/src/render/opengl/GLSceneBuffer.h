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

/// @file GLSceneBuffer.h
/// @brief 场景底图的离屏帧缓存（RENDER_PLAN.md 1.3 步）

#ifndef GLSCENEBUFFER_H
#define GLSCENEBUFFER_H

#include <GL/glew.h>

namespace opengl
{

/// @brief 场景底图：与目标帧缓存（QOpenGLWidget 的帧缓存）同尺寸、同采样数、同颜色格式的离屏帧缓存
/// @details 画布把背景与文档画进来，之后每帧用 blitTo() 拷到目标上再画叠加层。多重采样的缓冲之间直接拷贝，
///          不先解析，拷过去的每个采样与直接画在目标上的相同，最后由 Qt 统一解析，所以结果与直接画一样。
///          GL 要求这样拷贝的两边采样数相同（颜色格式也按相同处理），因此按目标的实际参数建。
///          对象属于建它时的 GL 上下文，除 forget() 外的调用都要求该上下文是当前的。
class GLSceneBuffer
{
public:
    GLSceneBuffer() = default;
    ~GLSceneBuffer();
    GLSceneBuffer(const GLSceneBuffer&) = delete;
    GLSceneBuffer& operator=(const GLSceneBuffer&) = delete;

    /// @brief 按目标帧缓存调整自身：还没建或尺寸变了时，按目标的采样数与颜色格式重建
    /// @param [in] target 目标帧缓存，调用时须已绑定为当前帧缓存；调用后仍绑定它
    /// @param [in] width 宽（像素）
    /// @param [in] height 高（像素）
    /// @param [out] recreated 重建了（原有内容作废）时置为 true，否则置为 false
    /// @return 可用时返回 true；尺寸为 0、帧缓存不完整或采样数与目标对不上时返回 false，调用方改为直接画在目标上
    bool resize(GLuint target, int width, int height, bool* recreated);

    /// @brief 绑定为当前帧缓存，之后的绘制画进底图
    void bind();

    /// @brief 把全部颜色（多重采样时是全部采样）拷到目标帧缓存，然后重新绑定目标
    /// @param [in] target 目标帧缓存
    void blitTo(GLuint target);

    /// @brief 释放帧缓存与渲染缓冲
    void destroy();

    /// @brief 丢下对象名而不释放：建它的上下文已销毁时用，对象已随上下文一起释放
    void forget();

    /// @brief 是否建过（不论是否可用）
    bool isValid() const;

private:
    GLuint m_fbo = 0;           ///< 帧缓存
    GLuint m_color = 0;         ///< 颜色渲染缓冲
    GLuint m_depthStencil = 0;  ///< 深度模板渲染缓冲
    int m_width = 0;            ///< 宽（像素）
    int m_height = 0;           ///< 高（像素）
    bool m_usable = false;      ///< 帧缓存完整且采样数与目标相同
};

}

#endif  // GLSCENEBUFFER_H
