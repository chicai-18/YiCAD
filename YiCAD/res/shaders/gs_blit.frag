#version 450

// 场景底图贴到画布上（第 4.3.8 节的叠加通道第一步）：底图已解析成单采样纹理，逐像素取

layout(set = 3, binding = 0) uniform sampler2D scene;

layout(location = 0) in vec2 vEye;
layout(location = 1) in vec2 vUv;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(texture(scene, vUv).rgb, 1.0);
}
