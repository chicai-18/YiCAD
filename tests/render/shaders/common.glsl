// RHI 一致性测试的着色器共用的声明，经 #include 引入（顺带检查工具链对 GL_GOOGLE_include_directive 的支持）

// 第 0 组：每次绘制的常量，带动态偏移
layout(set = 0, binding = 0, std140) uniform Frame
{
    mat4 transform;  // 投影 × RhiDevice::clipSpaceCorrection()
    vec4 color;      // solid 的颜色；records 的色调
    vec4 sampleUv;   // records 采样纹理的位置
} frame;
