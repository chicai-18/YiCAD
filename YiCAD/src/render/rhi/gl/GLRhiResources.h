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

/// @file GLRhiResources.h
/// @brief RHI 资源的 OpenGL 实现

#ifndef GLRHIRESOURCES_H
#define GLRHIRESOURCES_H

#include <GL/glew.h>

#include <array>
#include <vector>

#include "RhiResources.h"

class GLRhiDevice;

/// @brief GL 格式三元组
struct GLRhiFormatInfo
{
    GLenum internalFormat = GL_NONE;
    GLenum format = GL_NONE;  ///< 上传、读回时的像素格式
    GLenum type = GL_NONE;
    bool integer = false;     ///< 整数格式：清除用 glClearBufferuiv，着色器里是 usampler
};

/// @brief RHI 格式到 GL 格式
GLRhiFormatInfo glRhiFormat(RhiFormat format);

/// @brief GL 资源共有的部分：所属设备与序号
/// @details 析构时删除 GL 对象；调用方（设备的延迟释放）保证共享组里有上下文是当前的。
///          设备先于句柄销毁时句柄的删除器先调 abandon()，析构不再碰 GL
class GLRhiResourceBase
{
public:
    void abandon() { m_device = nullptr; }
    std::uint64_t id() const { return m_id; }

protected:
    explicit GLRhiResourceBase(GLRhiDevice& device);
    ~GLRhiResourceBase();

    GLRhiDevice* m_device;
    std::uint64_t m_id;
};

class GLRhiBuffer final : public RhiBuffer, public GLRhiResourceBase
{
public:
    GLRhiBuffer(GLRhiDevice& device, const RhiBufferDesc& desc, GLuint name);
    ~GLRhiBuffer() override;
    GLuint name() const { return m_name; }

private:
    GLuint m_name;
};

class GLRhiTexture final : public RhiTexture, public GLRhiResourceBase
{
public:
    GLRhiTexture(GLRhiDevice& device, const RhiTextureDesc& desc, GLuint name);
    ~GLRhiTexture() override;
    GLuint name() const { return m_name; }
    /// @brief GL_TEXTURE_2D 或 GL_TEXTURE_2D_MULTISAMPLE
    GLenum target() const;

private:
    GLuint m_name;
};

class GLRhiSampler final : public RhiSampler, public GLRhiResourceBase
{
public:
    GLRhiSampler(GLRhiDevice& device, const RhiSamplerDesc& desc, GLuint name);
    ~GLRhiSampler() override;
    GLuint name() const { return m_name; }

private:
    GLuint m_name;
};

class GLRhiShader final : public RhiShader, public GLRhiResourceBase
{
public:
    GLRhiShader(GLRhiDevice& device, RhiShaderStage stage, GLuint name);
    ~GLRhiShader() override;
    GLuint name() const { return m_name; }

private:
    GLuint m_name;
};

class GLRhiBindGroupLayout final : public RhiBindGroupLayout, public GLRhiResourceBase
{
public:
    GLRhiBindGroupLayout(GLRhiDevice& device, std::span<const RhiBindGroupLayoutEntry> entries);
};

/// @brief 绑定组：解析好的 GL 对象，按布局项的顺序（绑定号升序）排列
class GLRhiBindGroup final : public RhiBindGroup, public GLRhiResourceBase
{
public:
    struct Binding
    {
        RhiBindingType type = RhiBindingType::UniformBuffer;
        bool hasDynamicOffset = false;
        GLuint buffer = 0;
        GLintptr offset = 0;
        GLsizeiptr size = 0;
        GLuint texture = 0;         ///< 纹素缓冲：本绑定组自己建的缓冲纹理；合一绑定：纹理
        GLenum textureTarget = GL_NONE;
        GLuint sampler = 0;
    };

    GLRhiBindGroup(GLRhiDevice& device, RhiBindGroupDesc desc, std::vector<Binding> bindings);
    ~GLRhiBindGroup() override;
    const std::vector<Binding>& bindings() const { return m_bindings; }

private:
    std::vector<Binding> m_bindings;
};

/// @brief 管线：链接好的程序与固定的绘制状态
/// @details 绑定点平铺：按组号、组内按绑定号依次编号，常量缓冲、存储缓冲、纹理单元（纹素缓冲与纹理）各自从 0 起。
///          tools/compile_shaders.py 用同一规则改写着色器的 layout(binding)，建管线时再按程序反射核对
class GLRhiPipeline final : public RhiPipeline, public GLRhiResourceBase
{
public:
    GLRhiPipeline(GLRhiDevice& device, RhiPipelineDesc desc, GLuint program,
                  std::vector<std::vector<GLuint>> flatBindings);
    ~GLRhiPipeline() override;

    GLuint program() const { return m_program; }
    GLenum mode() const { return m_mode; }
    /// @brief 第 set 组第 entryIndex 项（布局里的顺序）的 GL 绑定点
    GLuint flatBinding(std::size_t set, std::size_t entryIndex) const { return m_flatBindings[set][entryIndex]; }
    /// @brief 在当前上下文里建本管线的 VAO（只有顶点格式，缓冲在绘制时绑定）
    GLuint createVertexArray() const;
    /// @brief 设置固定状态：程序、混合、深度、颜色写掩码
    void applyState() const;

    /// @brief 按平铺规则算出各组各项的绑定点
    static std::vector<std::vector<GLuint>> computeFlatBindings(const std::vector<RhiBindGroupLayoutPtr>& layouts);

private:
    GLuint m_program;
    GLenum m_mode;
    std::vector<std::vector<GLuint>> m_flatBindings;
};

class GLRhiRenderTarget final : public RhiRenderTarget, public GLRhiResourceBase
{
public:
    GLRhiRenderTarget(GLRhiDevice& device, RhiRenderTargetDesc desc);
    ~GLRhiRenderTarget() override;
    /// @brief 在当前上下文里建 FBO；不完整时返回 0
    GLuint createFramebuffer() const;
};

/// @brief 一组时间戳查询：查询对象按上下文放在 GLRhiContextState::querySets 里，这里只有序号
class GLRhiQuerySet final : public RhiQuerySet, public GLRhiResourceBase
{
public:
    GLRhiQuerySet(GLRhiDevice& device, RhiQuerySetDesc desc);
    ~GLRhiQuerySet() override;
};

#endif // GLRHIRESOURCES_H
