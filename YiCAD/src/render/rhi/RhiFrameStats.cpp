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

/// @file RhiFrameStats.cpp
/// @brief 一帧内的上传字节数与绘制调用数

#include "RhiFrameStats.h"

long long RhiFrameStats::s_uploadBytes = 0;
long long RhiFrameStats::s_drawCalls = 0;

void RhiFrameStats::beginFrame()
{
    s_uploadBytes = 0;
    s_drawCalls = 0;
}

void RhiFrameStats::endFrame()
{
    if (yicad::Profiler::isEnabled())
    {
        yicad::counters::uploadBytes().addSample(s_uploadBytes);
        yicad::counters::drawCalls().addSample(s_drawCalls);
    }
    s_uploadBytes = 0;
    s_drawCalls = 0;
}

