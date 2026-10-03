#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_terminate_invocation : require

// 圆弧的片段：f = |p − c| − r 在局部坐标里求，除以 f 的屏幕梯度长度（dFdx、dFdy）得到到圆（仿射下为椭圆）的
// 像素距离的一阶近似，线宽相对曲率半径很小时误差远小于一个像素（第 4.4 节"仿射变换下的圆弧"）。
// 弧长参数 r × (角度 − 起始角) 精确；两端外的部分按到端点的弧长延伸，画成圆头。
// 每个多重采样点各自判断，写采样掩码（同 gs_line.frag）

#include "gs_frame.glsl"
#include "gs_dash.glsl"

layout(location = 0) in vec2 vLocal;
layout(location = 1) flat in vec4 vArc;
layout(location = 2) flat in vec4 vColor;
layout(location = 3) flat in float vHalfWidth;
layout(location = 4) flat in float vLength;
layout(location = 5) flat in uint vDashMode;
layout(location = 6) flat in uint vLineType;
layout(location = 7) flat in float vLengthScale;

layout(location = 0) out vec4 outColor;

const float kTwoPi = 6.28318530718;

/// @brief 局部坐标里的一点在圆弧上的弧长参数（世界长度），端点外延伸成负数或超过总长
float alongAt(vec2 local, float localToWorld)
{
    float radius = vArc.x;
    float start = vArc.y;
    float sweep = vArc.z;
    float s0 = vArc.w;
    float angle = atan(local.y, local.x) - start;
    angle = angle - kTwoPi * floor(angle / kTwoPi);  // [0, 2π)
    if (sweep >= kTwoPi - 1.0e-6 || angle <= sweep)
    {
        return (s0 + radius * angle) * vLengthScale;
    }
    // 端点外：离哪个端点近，就按到那个端点的弧长延伸
    float beyondEnd = angle - sweep;
    float beforeStart = kTwoPi - angle;
    if (beyondEnd < beforeStart)
    {
        return (s0 + radius * sweep) * vLengthScale + radius * beyondEnd * localToWorld;
    }
    return s0 * vLengthScale - radius * beforeStart * localToWorld;
}

void main()
{
    float radius = vArc.x;
    float f = length(vLocal) - radius;
    // 局部长度每像素的变化（像素中心处），用于把局部的距离换成像素再换成世界长度
    float gradient = length(vec2(dFdx(f), dFdy(f)));
    float wpp = frame.viewport.z;
    float localToWorld = gradient > 0.0 ? wpp / gradient : vLengthScale;

    int mask = 0;
    int samples = int(frame.mode.w);
    for (int i = 0; i < samples; ++i)
    {
        vec2 local = interpolateAtSample(vLocal, i);
        float across = (length(local) - radius) * localToWorld;
        float along = alongAt(local, localToWorld);
        if (insideStroke(along, across, vLength, vDashMode, vLineType, vHalfWidth))
        {
            mask |= 1 << i;
        }
    }
    if (mask == 0)
    {
        terminateInvocation;
    }
    gl_SampleMask[0] = mask;
    outColor = vColor;
}
