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

/// @file PainterCreator.h
/// @brief 画笔工厂类，根据渲染后端创建对应的画笔实例

#ifndef PAINTERCREATOR_H
#define PAINTERCREATOR_H

#include "GLPainter.h"
#include "GLCachePainter.h"

using namespace opengl;

/// @brief 画笔工厂类，负责创建不同后端的画笔实例
class PainterCreator
{
public:
    /// @brief 创建OpenGL画笔
    /// @param [in] width 设备宽度（像素）
    /// @param [in] height 设备高度（像素）
    /// @return OpenGL画笔实例
    static GLPainter* createOpenGLPainter(const unsigned int width, const unsigned int height);

    /// @brief 创建带缓存的OpenGL画笔
    /// @param [in] width 设备宽度（像素）
    /// @param [in] height 设备高度（像素）
    /// @return 带缓存的OpenGL画笔实例
    static GLCachePainter* createOpenGLCachePainter(const unsigned int width, const unsigned int height);

    /// @brief 创建Vulkan画笔（预留）
    /// @param [in] width 设备宽度（像素）
    /// @param [in] height 设备高度（像素）
    /// @return Vulkan画笔实例（当前返回nullptr）
    static Painter* createVulkanPainter(const unsigned int width, const unsigned int height);

    /// @brief 创建Direct3D画笔（预留）
    /// @param [in] width 设备宽度（像素）
    /// @param [in] height 设备高度（像素）
    /// @return Direct3D画笔实例（当前返回nullptr）
    static Painter* createDirect3DPainter(const unsigned int width, const unsigned int height);
};

#endif //PAINTERCREATOR_H
