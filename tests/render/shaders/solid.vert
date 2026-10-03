#version 450
#extension GL_GOOGLE_include_directive : require

// 纯色：只用第 0 组的常量缓冲

#include "common.glsl"

layout(location = 0) in vec2 position;

void main()
{
    gl_Position = frame.transform * vec4(position, 0.0, 1.0);
}
