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

/// @file GLFrameStats.cpp
/// @brief 一帧内的上传字节数与绘制调用数

#include "GLFrameStats.h"

namespace opengl
{

long long GLFrameStats::s_uploadBytes = 0;
long long GLFrameStats::s_drawCalls = 0;

void GLFrameStats::beginFrame()
{
    s_uploadBytes = 0;
    s_drawCalls = 0;
}

void GLFrameStats::endFrame()
{
    if (yicad::Profiler::isEnabled())
    {
        yicad::counters::uploadBytes().addSample(s_uploadBytes);
        yicad::counters::drawCalls().addSample(s_drawCalls);
    }
    s_uploadBytes = 0;
    s_drawCalls = 0;
}

}  // namespace opengl
