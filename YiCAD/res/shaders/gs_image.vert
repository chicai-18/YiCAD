#version 450
#extension GL_GOOGLE_include_directive : require

// 图片管线（第 4.4 节）：每张图片一个平行四边形（6 个顶点），纹理按图片来源在 GsDevice 里缓存。
// 颜色与旧渲染器相同：纹素乘实体颜色

#include "gs_frame.glsl"
#include "gs_vertex.glsl"

// 第 2 组：每张图片 2 个 RGBA32F：(原点 x, y, u 边 x, y)、(v 边 x, y, 保留, 图元记录序号的位模式)
layout(set = 2, binding = 0) uniform samplerBuffer images;

layout(location = 0) out vec2 vUv;
layout(location = 1) flat out vec4 vColor;

void main()
{
    int k = gl_VertexIndex / 6;
    int corner = gl_VertexIndex % 6;
    vec4 a = texelFetch(images, k * 2);
    vec4 b = texelFetch(images, k * 2 + 1);
    Prim prim = loadPrim(floatBitsToUint(b.w));
    Style style = resolveStyle(prim);
    if (!style.visible)
    {
        gl_Position = collapsed();
        return;
    }
    vec2 uvs[6] = vec2[](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0),
                         vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));
    vec2 uv = uvs[corner];
    vec2 local = a.xy + a.zw * uv.x + b.xy * uv.y;
    vUv = uv;
    vColor = style.color;
    gl_Position = eyeToClip(toEye(local), style.depth);
}
