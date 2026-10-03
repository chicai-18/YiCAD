#version 450

// 叠加层的片段：dash 不为负时按 5 像素划线、5 像素空白（交叉选择框的虚线边）

layout(location = 0) in vec4 vColor;
layout(location = 1) in float vDash;

layout(location = 0) out vec4 outColor;

void main()
{
    // 空白处输出透明，由混合跳过（不用 discard，见 gs_grid.frag）
    bool gap = vDash >= 0.0 && mod(vDash, 10.0) >= 5.0;
    outColor = gap ? vec4(0.0) : vColor;
}
