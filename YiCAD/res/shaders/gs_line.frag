#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_terminate_invocation : require

// 线段与无限线的片段：每个多重采样点按线型与圆头判断是否在线上，写采样掩码（gs_dash.glsl 的说明）；
// 一个采样也不覆盖时丢弃片段。terminateInvocation 经 spirv-cross 成为 GL 的 discard
// （普通的 discard 在 Vulkan 1.3 的 SPIR-V 里是 spirv-cross 转不成 GLSL 430 的 demote）

#include "gs_frame.glsl"
#include "gs_dash.glsl"

layout(location = 0) in float vAlong;
layout(location = 1) in float vAcross;
layout(location = 2) flat in vec4 vColor;
layout(location = 3) flat in float vHalfWidth;
layout(location = 4) flat in float vLength;
layout(location = 5) flat in uint vStroke;
layout(location = 6) flat in float vDashScale;
layout(location = 7) flat in vec3 vDashParams;

layout(location = 0) out vec4 outColor;

void main()
{
    int mask = 0;
    int samples = int(frame.mode.w);
    for (int i = 0; i < samples; ++i)
    {
        float along = interpolateAtSample(vAlong, i);
        float across = interpolateAtSample(vAcross, i);
        if (insideStroke(along, across, vLength, vStroke, vDashScale, vDashParams, vHalfWidth))
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
