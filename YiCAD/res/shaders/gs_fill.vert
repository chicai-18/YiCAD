#version 450
#extension GL_GOOGLE_include_directive : require

// 填充管线（第 4.4 节）：编译时三角剖分好的三角形，每个顶点一个纹素。实心填充、SOLID、三角形、
// 带宽度的多段线、TrueType 字形。选中、高亮时与选中色、高亮色半透明叠色，不加宽

#include "gs_frame.glsl"
#include "gs_vertex.glsl"

// 第 2 组：顶点，每个 RGBA32F：x y 相对分块原点（块与字形为定义坐标）、z 保留、w 图元记录序号的位模式
layout(set = 2, binding = 0) uniform samplerBuffer vertices;

layout(location = 0) flat out vec4 vColor;
layout(location = 1) flat out float vCoverage;  // 覆盖率（过密填充图案的替身；其余为 1），片段着色器按它抖动写采样掩码
layout(location = 2) flat out float vDither;    // 抖动的错开量（替身的图元记录序号），各族的采样不重叠

void main()
{
    if (smallGlyph())
    {
        gl_Position = collapsed();
        return;
    }
    vec4 v = texelFetch(vertices, gl_VertexIndex);
    Prim prim = loadPrim(floatBitsToUint(v.w));
    vCoverage = 1.0;
    vDither = 0.0;
    if ((prim.flags & kPrimFlagHatchCover) != 0u)
    {
        // 填充图案过密时的替身：线距不够密时不画；画的时候按平均覆盖率（划线占周期的比例、点，乘线宽、除以线距）
        vec4 cover = loadPrimDash(prim.index);
        if (!denseHatch(cover.x))
        {
            gl_Position = collapsed();
            return;
        }
        float wpp = frame.viewport.z;
        float scale = instanceScale();
        float widthPx = lineWidthPixels(resolveLineWeight(prim));
        float spacingPx = max(cover.x * scale / wpp, 1.0e-6);
        float marksPerPixel = cover.z * wpp / max(scale, 1.0e-30);
        // 图案的周期在屏幕上过密时图案线画成实线（第 4.5.4 节），替身同样按实线算，两边在阈值处衔接；
        // 否则是划线占的比例加上每条划线、每个点的圆头（共一个线宽）
        bool solid = cover.w * scale < frame.strokeStyle.y * wpp;
        float ink = solid ? 1.0 : clamp(cover.y + marksPerPixel * widthPx, 0.0, 1.0);
        vCoverage = clamp(ink * widthPx / spacingPx, 0.0, 1.0);
        // 抖动按图元记录错开：每族一条替身的图元记录，各族的采样不重叠
        vDither = float(prim.index);
    }
    Style style = resolveStyle(prim);
    if (!style.visible)
    {
        gl_Position = collapsed();
        return;
    }
    vColor = style.color;
    vec2 eye = toEye(v.xy);
    if (style.tiny)
    {
        // 亚像素的对象画成一个点：三角形可能一个采样也盖不到，按顶点在三角形里的位置撑开到一个像素以上
        const vec2 spread[3] = vec2[](vec2(-1.0, -1.0), vec2(2.0, -1.0), vec2(-1.0, 2.0));
        eye += spread[gl_VertexIndex % 3] * frame.strokeStyle.z * frame.viewport.z * 0.5;
    }
    gl_Position = eyeToClip(eye, style.depth);
}
