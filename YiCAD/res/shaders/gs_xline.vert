#version 450
#extension GL_GOOGLE_include_directive : require

// 无限线管线（第 4.4 节）：射线与构造线。顶点着色器用视口矩形（外扩一圈）截出可见的一段，再按线段扩成四边形。
// 基点按 double 拆成高低两个 float（hi + lo），减去同样拆开的视点（每帧常量），远离原点时也不丢精度（R6）；
// 无限线不进分块，也不用实例记录的变换（块里的射线、构造线编译时已变换到世界坐标）。

#include "gs_frame.glsl"
#include "gs_vertex.glsl"

// 第 2 组：每条 2 个 RGBA32F：(基点 x 高位, y 高位, x 低位, y 低位)、(方向 x, y, 种类：1 射线 0 构造线, 图元记录序号的位模式)
layout(set = 2, binding = 0) uniform samplerBuffer lines;

layout(location = 0) out float vAlong;
layout(location = 1) out float vAcross;
layout(location = 2) flat out vec4 vColor;
layout(location = 3) flat out float vHalfWidth;
layout(location = 4) flat out float vLength;
layout(location = 5) flat out uint vStroke;
layout(location = 6) flat out float vDashScale;
layout(location = 7) flat out vec3 vDashParams;

void main()
{
    int k = gl_VertexIndex / 6;
    int corner = gl_VertexIndex % 6;
    vec4 a = texelFetch(lines, k * 2);
    vec4 b = texelFetch(lines, k * 2 + 1);
    Prim prim = loadPrim(floatBitsToUint(b.w));
    Style style = resolveStyle(prim);
    if (!style.visible)
    {
        gl_Position = collapsed();
        return;
    }

    vec2 base = (a.xy - frame.eye.xy) + (a.zw - frame.eye.zw);
    vec2 dir = normalize(b.xy);
    bool ray = b.z > 0.5;

    // 与外扩的视口矩形求交：t 的范围
    float wpp = frame.viewport.z;
    vec2 halfSize = frame.viewport.xy * wpp * 0.5 + vec2(frame.lineStyle.w);
    float tMin = ray ? 0.0 : -1.0e30;
    float tMax = 1.0e30;
    for (int axis = 0; axis < 2; ++axis)
    {
        float o = base[axis];
        float dd = dir[axis];
        if (abs(dd) < 1.0e-12)
        {
            if (abs(o) > halfSize[axis])
            {
                tMax = -1.0;
            }
        }
        else
        {
            float t0 = (-halfSize[axis] - o) / dd;
            float t1 = (halfSize[axis] - o) / dd;
            tMin = max(tMin, min(t0, t1));
            tMax = min(tMax, max(t0, t1));
        }
    }
    if (tMax <= tMin)
    {
        gl_Position = collapsed();
        return;
    }

    float widthPx = lineWidthPixels(resolveLineWeight(prim)) + (style.emphasized ? frame.lineStyle.y : 0.0);
    float halfWidth = widthPx * 0.5 * wpp;
    float extent = halfWidth + frame.viewport.w * wpp;
    vec2 normal = vec2(-dir.y, dir.x);

    bool atEnd = corner == 1 || corner == 2 || corner == 4;
    bool positive = corner == 2 || corner == 4 || corner == 5;
    float t = atEnd ? tMax + extent : tMin - extent;
    float side = positive ? 1.0 : -1.0;
    vec2 eye = base + dir * t + normal * (side * extent);

    // 从基点起周期重复；射线的基点是线端（画圆头），构造线没有端点。无限线不用实例记录（编译时已变换到世界坐标），
    // 块参照的线型比例已乘进图元记录
    Stroke stroke = strokeOf(prim, 1.0, 1.0);
    uint caps = ray ? kStrokeCapStart : 0u;
    vAlong = t;
    vAcross = side * extent;
    vColor = style.color;
    vHalfWidth = halfWidth;
    vLength = 1.0e30;
    vStroke = (stroke.code & ~(kStrokeCapStart | kStrokeCapEnd)) | caps;
    vDashScale = stroke.scale;
    vDashParams = vec3(0.0);
    gl_Position = eyeToClip(eye, style.depth);
}
