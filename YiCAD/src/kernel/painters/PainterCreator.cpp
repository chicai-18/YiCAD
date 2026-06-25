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

/// @file PainterCreator.cpp
/// @brief 画笔工厂类实现，根据不同后端创建画笔实例

#include "PainterCreator.h"

GLPainter* PainterCreator::createOpenGLPainter(const unsigned int width, const unsigned int height)
{
    return new GLPainter();
}

GLCachePainter* PainterCreator::createOpenGLCachePainter(const unsigned int width, const unsigned int height)
{
    return new GLCachePainter();
}

Painter* PainterCreator::createVulkanPainter(const unsigned int width, const unsigned int height)
{
    // TODO: Vulkan后端尚未实现
    return nullptr;
}

Painter* PainterCreator::createDirect3DPainter(const unsigned int width, const unsigned int height)
{
    // TODO: Direct3D后端尚未实现
    return nullptr;
}
