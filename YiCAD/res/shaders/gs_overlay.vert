#version 450
#extension GL_GOOGLE_include_directive : require

// 叠加层的动态批次（第 4.3.9 节）：原点标记、选择框、光标、捕捉标记、夹点。顶点由 CPU 每帧生成，
// 坐标是以画布左上角为原点的像素，三角形列表；dash 为沿线的像素长度（负数为实线）

#include "gs_frame.glsl"

layout(location = 0) in vec2 position;   // 像素，左上角为原点
layout(location = 1) in vec4 color;
layout(location = 2) in float dash;

layout(location = 0) out vec4 vColor;
layout(location = 1) out float vDash;

void main()
{
    vec2 ndc = vec2(position.x / frame.viewport.x * 2.0 - 1.0, 1.0 - position.y / frame.viewport.y * 2.0);
    vColor = color;
    vDash = dash;
    gl_Position = frame.clipCorrection * vec4(ndc, 0.0, 1.0);
}
