#version 450
#extension GL_GOOGLE_include_directive : require

// 网格（第 4.4 节）：盖满画布的三角形，片段着色器按世界坐标画 1 像素宽的网格线。
// 视点对间距取模在 CPU 上用 double 算好（每帧常量），这里只做相对视点的小数运算，远离原点也不丢精度（R6）。
// 细线用网格色，每 5 格一条的粗线用辅网格色（与旧渲染器的 drawGridLine 相同）。不是网格线的地方输出透明，
// 由混合留下背景（着色器里不用 discard：Vulkan 1.3 的 SPIR-V 把它编成 spirv-cross 转不成 GLSL 430 的指令）

#include "gs_frame.glsl"

layout(location = 0) in vec2 vEye;
layout(location = 1) in vec2 vUv;

layout(location = 0) out vec4 outColor;

/// @brief 这个像素上有没有网格线：线落在像素的 [中心 - 0.5, 中心 + 0.5) 里就画，每条线只归一个像素
/// @details 不能用"到线的距离 < 0.5 像素"：线恰好落在两列像素正中间时（画布宽为偶数、间距为整数像素时常见）两边都不画，
///          浮点误差一侧偏正一侧偏负，结果半个画面的线都不见了
bool onLine(vec2 eye, vec2 offset, float spacing)
{
    vec2 c = eye + offset;
    vec2 r = (c - spacing * round(c / spacing)) / frame.viewport.z;  // 像素中心到最近一条线的有向距离（像素）
    return (r.x > -0.5 && r.x <= 0.5) || (r.y > -0.5 && r.y <= 0.5);
}

void main()
{
    float minor = frame.grid.x;
    float major = frame.grid.y;
    outColor = vec4(0.0);
    if (frame.grid.z == 0.0 || minor <= 0.0)
    {
        return;
    }
    if (onLine(vEye, frame.gridOffset.zw, major))
    {
        outColor = frame.metaGridColor;
        return;
    }
    if (onLine(vEye, frame.gridOffset.xy, minor))
    {
        outColor = frame.gridColor;
    }
}
