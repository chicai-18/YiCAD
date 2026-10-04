#version 450
#extension GL_GOOGLE_include_directive : require

// 圆弧管线（第 4.4 节，D7）：圆、圆弧、多段线的凸度段解析绘制。每条记录 48 个顶点：覆盖圆弧的环带分 8 段，
// 每段两个三角形。环带在局部坐标（块与字形为定义坐标）里生成，内外半径各外扩"半线宽 + 1 像素"折算的局部长度，
// 外半径另按 r / cos(θ/2) 保守外扩；折算用实例变换的最小奇异值，仿射（非等比、错切）下也能盖住。
// 片段着色器把像素位置换回局部坐标，按圆方程求到圆的距离，再除以它的屏幕梯度得到像素距离，任何缩放下都是真圆。

#include "gs_frame.glsl"
#include "gs_vertex.glsl"

// 第 2 组：每条 2 个 RGBA32F：(圆心 x, y, 半径, 起始角)、(扫角, 弧长参数起点, 保留, 图元记录序号的位模式)
layout(set = 2, binding = 0) uniform samplerBuffer arcs;

layout(location = 0) out vec2 vLocal;          // 相对圆心的局部坐标
layout(location = 1) flat out vec4 vArc;       // 半径、起始角、扫角、弧长参数起点
layout(location = 2) flat out vec4 vColor;
layout(location = 3) flat out float vHalfWidth; // 半线宽（世界长度）
layout(location = 4) flat out float vLength;    // 这一段线的长度（世界长度）
layout(location = 5) flat out uint vStroke;     // 线型的画法（Stroke::code）
layout(location = 6) flat out float vDashScale; // 图案长度到弧长参数的比例
layout(location = 7) flat out vec3 vDashParams; // 相位、头部划线终点、尾部划线起点
layout(location = 8) flat out float vLengthScale;

const int kSegments = 8;
const float kTwoPi = 6.28318530718;

void main()
{
    int k = gl_VertexIndex / 48;
    int local = gl_VertexIndex % 48;
    int segment = local / 6;
    int corner = local % 6;
    vec4 a = texelFetch(arcs, k * 2);
    vec4 b = texelFetch(arcs, k * 2 + 1);
    Prim prim = loadPrim(floatBitsToUint(b.w));
    Style style = resolveStyle(prim);
    if (!style.visible)
    {
        gl_Position = collapsed();
        return;
    }

    float wpp = frame.viewport.z;
    float widthPx = lineWidthPixels(resolveLineWeight(prim)) + (style.emphasized ? frame.lineStyle.y : 0.0);
    float halfWidth = widthPx * 0.5 * wpp;
    float extentWorld = halfWidth + frame.viewport.w * wpp;

    // 局部长度 × 最小奇异值 = 世界长度的下界，所以局部外扩 = 世界外扩 / 最小奇异值
    mat2 m = mat2(iLinear.x, iLinear.y, iLinear.z, iLinear.w);
    float p = dot(iLinear, iLinear) * 0.5;
    float q = abs(determinant(m));
    float sigmaMin = sqrt(max(p - sqrt(max(p * p - q * q, 0.0)), 1.0e-30));
    float extent = extentWorld / sigmaMin;

    float radius = a.z;
    float start = a.w;
    float sweep = b.x;
    // 两端外扩一个角度放下圆头；外扩后接近整圆时按整圆画
    float pad = radius > 0.0 ? min(extent / radius, 3.14159) : 3.14159;
    float a0 = start - pad;
    float span = sweep + 2.0 * pad;
    if (span >= kTwoPi)
    {
        a0 = start;
        span = kTwoPi;
    }
    float theta = span / float(kSegments);
    float inner = max(radius - extent, 0.0);
    float outer = (radius + extent) / cos(theta * 0.5);

    bool second = corner == 1 || corner == 2 || corner == 4;
    bool outside = corner == 2 || corner == 4 || corner == 5;
    float angle = a0 + theta * float(segment + (second ? 1 : 0));
    float r = outside ? outer : inner;
    vec2 offset = vec2(cos(angle), sin(angle)) * r;

    Stroke stroke = strokeOf(prim, iTranslate.z, iTranslate.w);
    vLocal = offset;
    vArc = vec4(radius, start, sweep, b.y);
    vColor = style.color;
    vHalfWidth = halfWidth;
    vLength = stroke.len;
    vStroke = stroke.code;
    vDashScale = stroke.scale;
    vDashParams = stroke.params;
    vLengthScale = iTranslate.z;
    gl_Position = eyeToClip(toEye(a.xy + offset), style.depth);
}
