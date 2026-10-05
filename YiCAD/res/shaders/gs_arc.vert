#version 450
#extension GL_GOOGLE_include_directive : require

// 圆弧管线（第 4.4 节）：每条记录 48 个顶点（8 段环带），见 gs_arc_vertex.glsl

#define GS_ARC_VERTICES 48
#include "gs_arc_vertex.glsl"
