/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file RhiDevice.h
/// @brief RHI 的设备（RENDER_PLAN.md 第 4.7.2 节）

#ifndef RHIDEVICE_H
#define RHIDEVICE_H

#include <glm/glm.hpp>

#include "RhiCommandList.h"

/// @brief GPU 设备。GL 下对应一组共享的上下文，Vulkan 下对应 VkDevice
/// @details 单线程使用（第 4.7.1 节）。创建失败的资源返回空指针，原因记在 render 日志里。
///          全部资源句柄要在设备销毁之前释放
class RhiDevice
{
public:
    virtual ~RhiDevice() = default;

    virtual const RhiCaps& caps() const = 0;

    virtual RhiBufferPtr createBuffer(const RhiBufferDesc& desc) = 0;
    virtual RhiTexturePtr createTexture(const RhiTextureDesc& desc) = 0;
    virtual RhiSamplerPtr createSampler(const RhiSamplerDesc& desc) = 0;
    /// @brief 输入为构建期生成的本后端着色器（RhiCaps::shaderLanguage）
    virtual RhiShaderPtr createShader(const RhiShaderDesc& desc) = 0;
    virtual RhiBindGroupLayoutPtr createBindGroupLayout(const RhiBindGroupLayoutDesc& desc) = 0;
    virtual RhiBindGroupPtr createBindGroup(const RhiBindGroupDesc& desc) = 0;
    /// @brief 着色器用到的绑定与 desc.bindGroupLayouts 不一致时失败
    virtual RhiPipelinePtr createPipeline(const RhiPipelineDesc& desc) = 0;
    virtual RhiRenderTargetPtr createRenderTarget(const RhiRenderTargetDesc& desc) = 0;

    /// @brief 经暂存环形缓冲把数据写入缓冲；在下一次 beginFrame 录制的命令之前生效
    /// @details 数据当场复制，调用返回后即可改写 data
    virtual void upload(RhiBuffer& dst, std::size_t offset, std::span<const std::byte> data) = 0;
    /// @brief 同上，写入纹理的一块区域；数据按行紧密排列，见 RhiTextureRegion
    virtual void upload(RhiTexture& dst, const RhiTextureRegion& region, std::span<const std::byte> data) = 0;

    /// @brief 开始画到表面的一帧
    virtual RhiCommandList& beginFrame(RhiSurface& surface) = 0;
    /// @brief 开始只画离屏目标的一帧（没有交换链图像，渲染通道必须给出目标）
    virtual RhiCommandList& beginOffscreenFrame() = 0;
    /// @brief 提交、呈现（GL 下由 QOpenGLWidget 呈现）、推进延迟释放队列
    virtual void endFrame() = 0;

    /// @brief 等已提交的帧全部完成，并销毁可以销毁的资源
    virtual void waitIdle() = 0;
    /// @brief 读出 Readback 缓冲的内容；先等写它的帧完成
    virtual bool readBuffer(const RhiBuffer& buffer, std::size_t offset, std::span<std::byte> out) = 0;

    /// @brief 把 GL 风格的投影（y 向上、深度 -1..1）变换到本后端裁剪空间（Vulkan 的 y 向下、深度 0..1）
    virtual const glm::mat4& clipSpaceCorrection() const = 0;
};

#endif // RHIDEVICE_H
