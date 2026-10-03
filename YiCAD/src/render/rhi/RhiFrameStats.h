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

/// @file RhiFrameStats.h
/// @brief 一帧内的上传字节数与绘制调用数，帧结束时记入 render.uploadBytes、render.drawCalls

#ifndef RHIFRAMESTATS_H
#define RHIFRAMESTATS_H

#include "ScopedTimer.h"

/// @brief 按帧累计上传与绘制调用（RENDER_PLAN.md 0.1 步）
/// @details RHI 后端在上传数据与发出绘制的地方各记一笔（阶段 0 起旧渲染器的 GL 调用处也记，原名 GLFrameStats，
///          第 4 阶段移到 RHI），画布在一帧开始时 beginFrame()、结束时 endFrame() 把累计值作为一次采样记入计数器。
///          埋点关闭时每次记账只是一次原子读。只在渲染线程（UI 线程）上使用。
class RhiFrameStats
{
public:
    /// @brief 记一次上传
    /// @param [in] bytes 上传的字节数
    static void addUploadBytes(long long bytes)
    {
        if (yicad::Profiler::isEnabled())
        {
            s_uploadBytes += bytes;
        }
    }

    /// @brief 记绘制调用
    /// @param [in] calls 调用次数；一次 glMultiDrawArrays 算一次
    static void addDrawCalls(long long calls = 1)
    {
        if (yicad::Profiler::isEnabled())
        {
            s_drawCalls += calls;
        }
    }

    /// @brief 一帧开始：丢弃这一帧之前别处（如块预览窗）累计的值
    static void beginFrame();

    /// @brief 一帧结束：埋点开启时把累计值记入计数器，然后清零
    static void endFrame();

private:
    static long long s_uploadBytes;
    static long long s_drawCalls;
};


#endif  // RHIFRAMESTATS_H
