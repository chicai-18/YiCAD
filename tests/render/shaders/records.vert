#version 450
#extension GL_GOOGLE_include_directive : require

// 按记录画：记录序号经步进为 1 的实例属性传入，读取遵守 firstInstance（GL 的 baseInstance），
// 不用内建实例序号（RENDER_PLAN.md 第 4.7.3 节）。顶点阶段只用常量缓冲与纹素缓冲

#include "common.glsl"

// 第 1 组：每条记录一个 vec4，xy 平移、z 缩放
layout(set = 1, binding = 0) uniform samplerBuffer records;

layout(location = 0) in vec2 position;
layout(location = 1) in uint record;

layout(location = 0) flat out uint vRecord;

void main()
{
    vec4 placement = texelFetch(records, int(record));
    gl_Position = frame.transform * vec4(position * placement.z + placement.xy, 0.0, 1.0);
    vRecord = record;
}
