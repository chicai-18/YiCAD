#version 450
#extension GL_GOOGLE_include_directive : require

// 点管线（第 4.4 节）：每个点一个屏幕空间的方块（6 个顶点），边长取每帧常量里的点像素（旧渲染器为 2 像素），
// 选中、高亮时加宽。点样式（PDMODE、PDSIZE）预留

#include "gs_frame.glsl"
#include "gs_vertex.glsl"

// 第 2 组：点，每个 RGBA32F：x y 相对分块原点、z 保留、w 图元记录序号的位模式
layout(set = 2, binding = 0) uniform samplerBuffer pointRecords;

layout(location = 0) flat out vec4 vColor;

void main()
{
    int k = gl_VertexIndex / 6;
    int corner = gl_VertexIndex % 6;
    vec4 v = texelFetch(pointRecords, k);
    Prim prim = loadPrim(floatBitsToUint(v.w));
    Style style = resolveStyle(prim);
    if (!style.visible)
    {
        gl_Position = collapsed();
        return;
    }
    float sizePx = frame.lineStyle.z + (style.emphasized ? frame.lineStyle.y : 0.0);
    float half_ = sizePx * 0.5 * frame.viewport.z;
    vec2 offsets[6] = vec2[](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
                             vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));
    vColor = style.color;
    gl_Position = eyeToClip(toEye(v.xy) + offsets[corner] * half_, style.depth);
}
