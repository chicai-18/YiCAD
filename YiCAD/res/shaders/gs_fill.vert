#version 450
#extension GL_GOOGLE_include_directive : require

// 填充管线（第 4.4 节）：编译时三角剖分好的三角形，每个顶点一个纹素。实心填充、SOLID、三角形、
// 带宽度的多段线、TrueType 字形。选中、高亮时与选中色、高亮色半透明叠色，不加宽

#include "gs_frame.glsl"
#include "gs_vertex.glsl"

// 第 2 组：顶点，每个 RGBA32F：x y 相对分块原点（块与字形为定义坐标）、z 保留、w 图元记录序号的位模式
layout(set = 2, binding = 0) uniform samplerBuffer vertices;

layout(location = 0) flat out vec4 vColor;

void main()
{
    vec4 v = texelFetch(vertices, gl_VertexIndex);
    Prim prim = loadPrim(floatBitsToUint(v.w));
    Style style = resolveStyle(prim);
    if (!style.visible)
    {
        gl_Position = collapsed();
        return;
    }
    vColor = style.color;
    gl_Position = eyeToClip(toEye(v.xy), style.depth);
}
