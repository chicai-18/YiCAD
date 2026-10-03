#version 450

// 图片的片段：纹素乘颜色（选中、高亮时颜色已与选中色、高亮色混合）

layout(set = 3, binding = 0) uniform sampler2D image;

layout(location = 0) in vec2 vUv;
layout(location = 1) flat in vec4 vColor;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(image, vUv) * vColor;
}
