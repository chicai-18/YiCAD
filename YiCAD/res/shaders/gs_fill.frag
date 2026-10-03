#version 450

// 填充与点的片段：纯色；边缘由多重采样抗锯齿

layout(location = 0) flat in vec4 vColor;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vColor;
}
