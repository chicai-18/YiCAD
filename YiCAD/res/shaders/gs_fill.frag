#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_terminate_invocation : require

// 填充与点的片段：纯色，边缘由多重采样抗锯齿。过密填充图案的替身（覆盖率小于 1）按有序抖动只写一部分采样
// （4×4 Bayer 矩阵错开各像素，再按采样序号均分阈值），整体的平均覆盖率等于图案线的覆盖率；
// 只写采样掩码、不混合，所以深度（绘图次序）照常起作用

#include "gs_frame.glsl"

layout(location = 0) flat in vec4 vColor;
layout(location = 1) flat in float vCoverage;
layout(location = 2) flat in float vDither;

layout(location = 0) out vec4 outColor;

const float kBayer[16] = float[](0.0, 8.0, 2.0, 10.0, 12.0, 4.0, 14.0, 6.0, 3.0, 11.0, 1.0, 9.0, 15.0, 7.0, 13.0, 5.0);

void main()
{
    int mask = -1;
    if (vCoverage < 1.0)
    {
        ivec2 p = ivec2(gl_FragCoord.xy) & 3;
        float base = (kBayer[p.y * 4 + p.x] + 0.5) / 16.0 + vDither * 0.618034;
        int samples = max(int(frame.mode.w), 1);
        mask = 0;
        for (int i = 0; i < samples; ++i)
        {
            if (fract(base + float(i) / float(samples)) < vCoverage)
            {
                mask |= 1 << i;
            }
        }
        if (mask == 0)
        {
            terminateInvocation;
        }
    }
    gl_SampleMask[0] = mask;
    outColor = vColor;
}
