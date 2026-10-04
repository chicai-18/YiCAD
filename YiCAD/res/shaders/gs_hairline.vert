#version 450
#extension GL_GOOGLE_include_directive : require

// 细线管线：不显示线宽时场景里的线段按 GPU 的线图元画，每段 2 个顶点（线段管线是 6 个顶点的四边形），
// 片段只判划线。点缓冲与线段管线相同：顶点序号 / 2 是段号 k，读点 k 与 k + 1，点 k 带"断开"标志时这一段不存在。
// 加宽的（选中）在这里折叠，由线段管线另画（GsView 的加宽列表）。

#include "gs_frame.glsl"
#include "gs_vertex.glsl"

// 第 2 组：点，同 gs_segment.vert
layout(set = 2, binding = 0) uniform samplerBuffer points;

layout(location = 0) out float vAlong;          // 弧长参数（世界长度）
layout(location = 1) flat out vec4 vColor;
layout(location = 2) flat out float vLength;    // 这一段线的长度（世界长度）
layout(location = 3) flat out uint vStroke;     // 线型的画法（Stroke::code）
layout(location = 4) flat out float vDashScale;
layout(location = 5) flat out vec3 vDashParams;

void main()
{
    int k = gl_VertexIndex >> 1;
    bool atEnd = (gl_VertexIndex & 1) != 0;
    vec4 p0 = texelFetch(points, k);
    uint code = floatBitsToUint(p0.w);
    if ((code & 0x80000000u) != 0u)
    {
        gl_Position = collapsed();
        return;
    }
    Prim prim = loadPrim(code & 0x7FFFFFFFu);
    Style style = resolveStyle(prim);
    if (!style.visible || style.emphasized)
    {
        gl_Position = collapsed();
        return;
    }
    vec4 p = atEnd ? texelFetch(points, k + 1) : p0;
    float lengthScale = iTranslate.z;
    Stroke stroke = strokeOf(prim, lengthScale, iTranslate.w);
    vAlong = p.z * lengthScale;
    vColor = style.color;
    vLength = stroke.len;
    vStroke = stroke.code;
    vDashScale = stroke.scale;
    vDashParams = stroke.params;
    gl_Position = eyeToClip(toEye(p.xy), style.depth);
}
