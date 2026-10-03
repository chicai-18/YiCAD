#version 450
#extension GL_GOOGLE_include_directive : require

// 盖满画布的一个三角形（3 个顶点）：网格与场景底图贴图共用。vEye 是相对视点的世界坐标，vUv 是纹理坐标

#include "gs_frame.glsl"

layout(location = 0) out vec2 vEye;
layout(location = 1) out vec2 vUv;

void main()
{
    vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    vec2 p = positions[gl_VertexIndex % 3];
    vEye = p * frame.viewport.xy * frame.viewport.z * 0.5;
    vUv = p * 0.5 + 0.5;
    gl_Position = frame.clipCorrection * vec4(p, 0.0, 1.0);
}
