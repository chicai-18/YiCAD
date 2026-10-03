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

/// @file RhiResources.h
/// @brief RHI 的资源对象：缓冲、纹理、采样器、着色器、管线、绑定组、渲染目标、表面
/// @details 资源只由 RhiDevice 创建，经 Rhi*Ptr 持有。基类保存创建时的描述，后端派生类持有 API 对象

#ifndef RHIRESOURCES_H
#define RHIRESOURCES_H

#include <utility>
#include <vector>

#include "RhiTypes.h"

/// @brief 资源基类
class RhiResource
{
public:
    virtual ~RhiResource() = default;
    RhiResource(const RhiResource&) = delete;
    RhiResource& operator=(const RhiResource&) = delete;

protected:
    RhiResource() = default;
};

class RhiBuffer : public RhiResource
{
public:
    const RhiBufferDesc& desc() const { return m_desc; }
    std::size_t size() const { return m_desc.size; }

protected:
    explicit RhiBuffer(RhiBufferDesc desc) : m_desc(std::move(desc)) {}

private:
    RhiBufferDesc m_desc;
};

class RhiTexture : public RhiResource
{
public:
    const RhiTextureDesc& desc() const { return m_desc; }
    std::uint32_t width() const { return m_desc.width; }
    std::uint32_t height() const { return m_desc.height; }
    RhiFormat format() const { return m_desc.format; }
    std::uint32_t sampleCount() const { return m_desc.sampleCount; }

protected:
    explicit RhiTexture(RhiTextureDesc desc) : m_desc(std::move(desc)) {}

private:
    RhiTextureDesc m_desc;
};

class RhiSampler : public RhiResource
{
public:
    const RhiSamplerDesc& desc() const { return m_desc; }

protected:
    explicit RhiSampler(const RhiSamplerDesc& desc) : m_desc(desc) {}

private:
    RhiSamplerDesc m_desc;
};

class RhiShader : public RhiResource
{
public:
    RhiShaderStage stage() const { return m_stage; }

protected:
    explicit RhiShader(RhiShaderStage stage) : m_stage(stage) {}

private:
    RhiShaderStage m_stage;
};

/// @brief 绑定组布局；项按绑定号升序保存
class RhiBindGroupLayout : public RhiResource
{
public:
    const std::vector<RhiBindGroupLayoutEntry>& entries() const { return m_entries; }

    /// @brief 两个布局的项完全相同（Vulkan 的"布局兼容"）
    bool isCompatible(const RhiBindGroupLayout& other) const;

protected:
    explicit RhiBindGroupLayout(std::span<const RhiBindGroupLayoutEntry> entries);

private:
    std::vector<RhiBindGroupLayoutEntry> m_entries;
};

/// @brief 绑定组：按布局给出的一组资源，持有这些资源的引用
class RhiBindGroup : public RhiResource
{
public:
    const RhiBindGroupDesc& desc() const { return m_desc; }
    const RhiBindGroupLayout& layout() const { return *m_desc.layout; }

protected:
    explicit RhiBindGroup(RhiBindGroupDesc desc) : m_desc(std::move(desc)) {}

private:
    RhiBindGroupDesc m_desc;
};

class RhiPipeline : public RhiResource
{
public:
    const RhiPipelineDesc& desc() const { return m_desc; }

protected:
    explicit RhiPipeline(RhiPipelineDesc desc) : m_desc(std::move(desc)) {}

private:
    RhiPipelineDesc m_desc;
};

class RhiRenderTarget : public RhiResource
{
public:
    const RhiRenderTargetDesc& desc() const { return m_desc; }
    std::uint32_t width() const;
    std::uint32_t height() const;
    std::uint32_t sampleCount() const;

protected:
    explicit RhiRenderTarget(RhiRenderTargetDesc desc) : m_desc(std::move(desc)) {}

private:
    RhiRenderTargetDesc m_desc;
};

/// @brief 呈现表面（交换链）：GL 下是一个 QOpenGLWidget 的帧缓冲，Vulkan 下是窗口的交换链（第 4.7.6 节）
/// @details 不归设备所有，由视图持有；尺寸是设备像素
class RhiSurface
{
public:
    virtual ~RhiSurface() = default;
    virtual std::uint32_t width() const = 0;
    virtual std::uint32_t height() const = 0;
    virtual RhiFormat colorFormat() const = 0;
    virtual RhiFormat depthStencilFormat() const = 0;
    virtual std::uint32_t sampleCount() const = 0;
};

#endif // RHIRESOURCES_H
