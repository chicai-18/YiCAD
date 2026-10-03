#version 450
#extension GL_GOOGLE_include_directive : require

// 线段管线（第 4.4 节）：点缓冲里相邻两点连成一段，每段 6 个顶点（两个三角形）。
// 顶点序号 / 6 是段号 k，读点 k 与 k + 1；点 k 带"断开"标志时这一段不存在（折线的末点），折叠掉。
// 四边形沿线方向两端与法向各外扩 半线宽 + 1 像素，端点与划线两端在片段着色器里按距离画成圆头。

#include "gs_frame.glsl"
#include "gs_vertex.glsl"

// 第 2 组：点，每个 RGBA32F：x y 相对分块原点（块与字形为定义坐标）、z 弧长参数、w 图元记录序号（位模式，最高位为断开）
layout(set = 2, binding = 0) uniform samplerBuffer points;

layout(location = 0) out float vAlong;          // 弧长参数（世界长度）
layout(location = 1) out float vAcross;         // 到中心线的有向距离（世界长度）
layout(location = 2) flat out vec4 vColor;
layout(location = 3) flat out float vHalfWidth; // 半线宽（世界长度）
layout(location = 4) flat out float vLength;    // 这一段线的总长（世界长度）
layout(location = 5) flat out uint vDashMode;
layout(location = 6) flat out uint vLineType;

void main()
{
    int k = gl_VertexIndex / 6;
    int corner = gl_VertexIndex % 6;
    vec4 p0 = texelFetch(points, k);
    vec4 p1 = texelFetch(points, k + 1);
    uint code = floatBitsToUint(p0.w);
    if ((code & 0x80000000u) != 0u)
    {
        gl_Position = collapsed();
        return;
    }
    Prim prim = loadPrim(code & 0x7FFFFFFFu);
    Style style = resolveStyle(prim);
    if (!style.visible)
    {
        gl_Position = collapsed();
        return;
    }

    float wpp = frame.viewport.z;
    float widthPx = lineWidthPixels(resolveLineWeight(prim)) + (style.emphasized ? frame.lineStyle.y : 0.0);
    float halfWidth = widthPx * 0.5 * wpp;
    float extent = halfWidth + frame.viewport.w * wpp;
    float lengthScale = iTranslate.z;

    vec2 a = toEye(p0.xy);
    vec2 b = toEye(p1.xy);
    vec2 d = b - a;
    float len = length(d);
    vec2 dir = len > 0.0 ? d / len : vec2(1.0, 0.0);
    vec2 normal = vec2(-dir.y, dir.x);

    bool atEnd = corner == 1 || corner == 2 || corner == 4;
    bool positive = corner == 2 || corner == 4 || corner == 5;
    vec2 base = atEnd ? b + dir * extent : a - dir * extent;
    float side = positive ? 1.0 : -1.0;
    vec2 eye = base + normal * (side * extent);

    float s0 = p0.z * lengthScale;
    float s1 = p1.z * lengthScale;
    // 弧长参数沿线性插值；外扩部分按世界长度延伸，片段着色器据此画圆头
    float sLen = s1 - s0;
    float along = atEnd ? s1 + (len > 0.0 ? extent * sLen / len : extent) : s0 - (len > 0.0 ? extent * sLen / len : extent);

    vAlong = along;
    vAcross = side * extent;
    vColor = style.color;
    vHalfWidth = halfWidth;
    vLength = prim.runLength * lengthScale;
    vDashMode = prim.dashMode;
    vLineType = resolveLineType(prim);
    gl_Position = eyeToClip(eye, style.depth);
}
