#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_terminate_invocation : require

// 细线的片段：覆盖由线图元的多重采样光栅化给出，这里只按线型判断是否在划线上（半线宽为半个像素），
// 不在就丢弃。terminateInvocation 经 spirv-cross 成为 GL 的 discard（见 gs_line.frag）

#include "gs_frame.glsl"
#include "gs_dash.glsl"

layout(location = 0) in float vAlong;
layout(location = 1) flat in vec4 vColor;
layout(location = 2) flat in float vLength;
layout(location = 3) flat in uint vStroke;
layout(location = 4) flat in float vDashScale;
layout(location = 5) flat in vec3 vDashParams;

layout(location = 0) out vec4 outColor;

void main()
{
    if ((vStroke & 15u) != kStrokeSolid
        && !insideStroke(vAlong, 0.0, vLength, vStroke, vDashScale, vDashParams, 0.5 * frame.viewport.z))
    {
        terminateInvocation;
    }
    outColor = vColor;
}
