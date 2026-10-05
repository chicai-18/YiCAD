#version 450
#extension GL_GOOGLE_include_directive : require

// 小圆弧（第 4.3.10 节）：分块里的圆弧在屏幕上都不超过"只画一个四边形"的半径阈值时，图形系统按这个程序画，
// 每条记录 6 个顶点（一个四边形），见 gs_arc_vertex.glsl

#define GS_ARC_VERTICES 6
#include "gs_arc_vertex.glsl"
