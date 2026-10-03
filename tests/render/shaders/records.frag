#version 450
#extension GL_GOOGLE_include_directive : require

// 颜色 = 存储缓冲里该记录的颜色 × 纹理在 frame.sampleUv 处的纹素 × 色调

#include "common.glsl"

// 第 1 组：每条记录的颜色；存储缓冲只在片段阶段用
layout(set = 1, binding = 1, std430) readonly buffer Colors
{
    vec4 colors[];
};

// 第 2 组：纹理与采样器合一
layout(set = 2, binding = 0) uniform sampler2D image;

layout(location = 0) flat in uint vRecord;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = colors[vRecord] * texture(image, frame.sampleUv.xy) * frame.color;
}
