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

/// @file GLRhiResources.cpp
/// @brief RHI 资源的 OpenGL 实现

#include "GLRhiResources.h"

#include "GLRhiDevice.h"
#include "YiCadLog.h"

namespace
{
GLenum glBlendFactor(RhiBlendFactor factor)
{
    switch (factor)
    {
    case RhiBlendFactor::Zero: return GL_ZERO;
    case RhiBlendFactor::One: return GL_ONE;
    case RhiBlendFactor::SrcColor: return GL_SRC_COLOR;
    case RhiBlendFactor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
    case RhiBlendFactor::DstColor: return GL_DST_COLOR;
    case RhiBlendFactor::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
    case RhiBlendFactor::SrcAlpha: return GL_SRC_ALPHA;
    case RhiBlendFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
    case RhiBlendFactor::DstAlpha: return GL_DST_ALPHA;
    case RhiBlendFactor::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
    }
    return GL_ONE;
}

GLenum glBlendOp(RhiBlendOp op)
{
    switch (op)
    {
    case RhiBlendOp::Add: return GL_FUNC_ADD;
    case RhiBlendOp::Subtract: return GL_FUNC_SUBTRACT;
    case RhiBlendOp::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
    case RhiBlendOp::Min: return GL_MIN;
    case RhiBlendOp::Max: return GL_MAX;
    }
    return GL_FUNC_ADD;
}

GLenum glCompareOp(RhiCompareOp op)
{
    switch (op)
    {
    case RhiCompareOp::Never: return GL_NEVER;
    case RhiCompareOp::Less: return GL_LESS;
    case RhiCompareOp::Equal: return GL_EQUAL;
    case RhiCompareOp::LessOrEqual: return GL_LEQUAL;
    case RhiCompareOp::Greater: return GL_GREATER;
    case RhiCompareOp::NotEqual: return GL_NOTEQUAL;
    case RhiCompareOp::GreaterOrEqual: return GL_GEQUAL;
    case RhiCompareOp::Always: return GL_ALWAYS;
    }
    return GL_LESS;
}

GLenum glTopology(RhiPrimitiveTopology topology)
{
    switch (topology)
    {
    case RhiPrimitiveTopology::PointList: return GL_POINTS;
    case RhiPrimitiveTopology::LineList: return GL_LINES;
    case RhiPrimitiveTopology::LineStrip: return GL_LINE_STRIP;
    case RhiPrimitiveTopology::TriangleList: return GL_TRIANGLES;
    case RhiPrimitiveTopology::TriangleStrip: return GL_TRIANGLE_STRIP;
    }
    return GL_TRIANGLES;
}

/// @brief 顶点格式：分量数、GL 类型、是否整数、是否归一化
struct VertexFormatInfo
{
    GLint components;
    GLenum type;
    bool integer;
    bool normalized;
};

VertexFormatInfo glVertexFormat(RhiVertexFormat format)
{
    switch (format)
    {
    case RhiVertexFormat::Float: return {1, GL_FLOAT, false, false};
    case RhiVertexFormat::Float2: return {2, GL_FLOAT, false, false};
    case RhiVertexFormat::Float3: return {3, GL_FLOAT, false, false};
    case RhiVertexFormat::Float4: return {4, GL_FLOAT, false, false};
    case RhiVertexFormat::Uint: return {1, GL_UNSIGNED_INT, true, false};
    case RhiVertexFormat::Uint2: return {2, GL_UNSIGNED_INT, true, false};
    case RhiVertexFormat::Uint4: return {4, GL_UNSIGNED_INT, true, false};
    case RhiVertexFormat::Sint: return {1, GL_INT, true, false};
    case RhiVertexFormat::UByte4Norm: return {4, GL_UNSIGNED_BYTE, false, true};
    }
    return {1, GL_FLOAT, false, false};
}
}  // namespace

GLRhiFormatInfo glRhiFormat(RhiFormat format)
{
    switch (format)
    {
    case RhiFormat::R8Unorm: return {GL_R8, GL_RED, GL_UNSIGNED_BYTE, false};
    case RhiFormat::RGBA8Unorm: return {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, false};
    case RhiFormat::R32Float: return {GL_R32F, GL_RED, GL_FLOAT, false};
    case RhiFormat::RG32Float: return {GL_RG32F, GL_RG, GL_FLOAT, false};
    case RhiFormat::RGBA32Float: return {GL_RGBA32F, GL_RGBA, GL_FLOAT, false};
    case RhiFormat::R32Uint: return {GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, true};
    case RhiFormat::RG32Uint: return {GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT, true};
    case RhiFormat::RGBA32Uint: return {GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, true};
    case RhiFormat::Depth24Stencil8: return {GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, false};
    case RhiFormat::Depth32Float: return {GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, false};
    case RhiFormat::Undefined: break;
    }
    return {};
}

// ---------------------------------------------------------------------------

GLRhiResourceBase::GLRhiResourceBase(GLRhiDevice& device)
    : m_device(&device)
    , m_id(device.nextObjectId())
{
}

GLRhiResourceBase::~GLRhiResourceBase()
{
    if (m_device)
    {
        m_device->onResourceDestroyed();
    }
}

GLRhiBuffer::GLRhiBuffer(GLRhiDevice& device, const RhiBufferDesc& desc, GLuint name)
    : RhiBuffer(desc)
    , GLRhiResourceBase(device)
    , m_name(name)
{
}

GLRhiBuffer::~GLRhiBuffer()
{
    if (m_device)
    {
        m_device->dropPendingUploads(this);
        glDeleteBuffers(1, &m_name);
    }
}

GLRhiTexture::GLRhiTexture(GLRhiDevice& device, const RhiTextureDesc& desc, GLuint name)
    : RhiTexture(desc)
    , GLRhiResourceBase(device)
    , m_name(name)
{
}

GLRhiTexture::~GLRhiTexture()
{
    if (m_device)
    {
        m_device->dropPendingUploads(this);
        glDeleteTextures(1, &m_name);
    }
}

GLenum GLRhiTexture::target() const
{
    return sampleCount() > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
}

GLRhiSampler::GLRhiSampler(GLRhiDevice& device, const RhiSamplerDesc& desc, GLuint name)
    : RhiSampler(desc)
    , GLRhiResourceBase(device)
    , m_name(name)
{
}

GLRhiSampler::~GLRhiSampler()
{
    if (m_device)
    {
        glDeleteSamplers(1, &m_name);
    }
}

GLRhiShader::GLRhiShader(GLRhiDevice& device, RhiShaderStage stage, GLuint name)
    : RhiShader(stage)
    , GLRhiResourceBase(device)
    , m_name(name)
{
}

GLRhiShader::~GLRhiShader()
{
    if (m_device)
    {
        glDeleteShader(m_name);
    }
}

GLRhiBindGroupLayout::GLRhiBindGroupLayout(GLRhiDevice& device, std::span<const RhiBindGroupLayoutEntry> entries)
    : RhiBindGroupLayout(entries)
    , GLRhiResourceBase(device)
{
}

GLRhiBindGroup::GLRhiBindGroup(GLRhiDevice& device, RhiBindGroupDesc desc, std::vector<Binding> bindings)
    : RhiBindGroup(std::move(desc))
    , GLRhiResourceBase(device)
    , m_bindings(std::move(bindings))
{
}

GLRhiBindGroup::~GLRhiBindGroup()
{
    if (!m_device)
    {
        return;
    }
    for (const Binding& binding : m_bindings)
    {
        if (binding.type == RhiBindingType::TexelBuffer && binding.texture != 0)
        {
            glDeleteTextures(1, &binding.texture);
        }
    }
}

// ---------------------------------------------------------------------------

GLRhiPipeline::GLRhiPipeline(GLRhiDevice& device, RhiPipelineDesc desc, GLuint program,
                             std::vector<std::vector<GLuint>> flatBindings)
    : RhiPipeline(std::move(desc))
    , GLRhiResourceBase(device)
    , m_program(program)
    , m_mode(glTopology(this->desc().topology))
    , m_flatBindings(std::move(flatBindings))
{
}

GLRhiPipeline::~GLRhiPipeline()
{
    if (m_device)
    {
        m_device->forgetPipeline(id());
        glDeleteProgram(m_program);
    }
}

std::vector<std::vector<GLuint>> GLRhiPipeline::computeFlatBindings(const std::vector<RhiBindGroupLayoutPtr>& layouts)
{
    GLuint uniformBuffers = 0;
    GLuint storageBuffers = 0;
    GLuint textureUnits = 0;
    std::vector<std::vector<GLuint>> result(layouts.size());
    for (std::size_t set = 0; set < layouts.size(); ++set)
    {
        if (!layouts[set])
        {
            continue;
        }
        for (const RhiBindGroupLayoutEntry& entry : layouts[set]->entries())
        {
            switch (entry.type)
            {
            case RhiBindingType::UniformBuffer:
                result[set].push_back(uniformBuffers++);
                break;
            case RhiBindingType::StorageBuffer:
                result[set].push_back(storageBuffers++);
                break;
            case RhiBindingType::TexelBuffer:
            case RhiBindingType::CombinedTextureSampler:
                result[set].push_back(textureUnits++);
                break;
            }
        }
    }
    return result;
}

GLuint GLRhiPipeline::createVertexArray() const
{
    GLuint vertexArray = 0;
    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);
    const std::vector<RhiVertexBufferLayout>& buffers = desc().vertexBuffers;
    for (std::size_t slot = 0; slot < buffers.size(); ++slot)
    {
        const RhiVertexBufferLayout& layout = buffers[slot];
        const GLuint binding = static_cast<GLuint>(slot);
        glVertexBindingDivisor(binding, layout.stepMode == RhiVertexStepMode::Instance ? 1 : 0);
        for (const RhiVertexAttribute& attribute : layout.attributes)
        {
            const VertexFormatInfo info = glVertexFormat(attribute.format);
            glEnableVertexAttribArray(attribute.location);
            if (info.integer)
            {
                glVertexAttribIFormat(attribute.location, info.components, info.type, attribute.offset);
            }
            else
            {
                glVertexAttribFormat(attribute.location, info.components, info.type,
                                     info.normalized ? GL_TRUE : GL_FALSE, attribute.offset);
            }
            glVertexAttribBinding(attribute.location, binding);
        }
    }
    return vertexArray;
}

void GLRhiPipeline::applyState() const
{
    glUseProgram(m_program);

    const RhiPipelineDesc& d = desc();
    for (std::size_t i = 0; i < d.colorTargets.size(); ++i)
    {
        const RhiColorTargetState& target = d.colorTargets[i];
        const GLuint index = static_cast<GLuint>(i);
        if (target.blendEnabled)
        {
            glEnablei(GL_BLEND, index);
            glBlendFuncSeparatei(index, glBlendFactor(target.color.srcFactor), glBlendFactor(target.color.dstFactor),
                                 glBlendFactor(target.alpha.srcFactor), glBlendFactor(target.alpha.dstFactor));
            glBlendEquationSeparatei(index, glBlendOp(target.color.op), glBlendOp(target.alpha.op));
        }
        else
        {
            glDisablei(GL_BLEND, index);
        }
        glColorMaski(index,
                     rhiAny(target.writeMask & RhiColorWriteMask::Red) ? GL_TRUE : GL_FALSE,
                     rhiAny(target.writeMask & RhiColorWriteMask::Green) ? GL_TRUE : GL_FALSE,
                     rhiAny(target.writeMask & RhiColorWriteMask::Blue) ? GL_TRUE : GL_FALSE,
                     rhiAny(target.writeMask & RhiColorWriteMask::Alpha) ? GL_TRUE : GL_FALSE);
    }

    const bool hasDepth = d.depthStencil.format != RhiFormat::Undefined;
    if (hasDepth && d.depthStencil.depthTestEnabled)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(glCompareOp(d.depthStencil.depthCompare));
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(hasDepth && d.depthStencil.depthWriteEnabled ? GL_TRUE : GL_FALSE);
}

// ---------------------------------------------------------------------------

GLRhiRenderTarget::GLRhiRenderTarget(GLRhiDevice& device, RhiRenderTargetDesc desc)
    : RhiRenderTarget(std::move(desc))
    , GLRhiResourceBase(device)
{
}

GLRhiRenderTarget::~GLRhiRenderTarget()
{
    if (m_device)
    {
        m_device->forgetRenderTarget(id());
    }
}

GLuint GLRhiRenderTarget::createFramebuffer() const
{
    GLuint framebuffer = 0;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    const RhiRenderTargetDesc& d = desc();
    std::vector<GLenum> drawBuffers;
    for (std::size_t i = 0; i < d.colorAttachments.size(); ++i)
    {
        const auto* texture = static_cast<const GLRhiTexture*>(d.colorAttachments[i].get());
        const GLenum attachment = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i);
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, texture->target(), texture->name(), 0);
        drawBuffers.push_back(attachment);
    }
    if (d.depthStencil)
    {
        const auto* texture = static_cast<const GLRhiTexture*>(d.depthStencil.get());
        const GLenum attachment = texture->format() == RhiFormat::Depth24Stencil8
                                      ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, texture->target(), texture->name(), 0);
    }
    if (drawBuffers.empty())
    {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }
    else
    {
        glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
        glReadBuffer(GL_COLOR_ATTACHMENT0);
    }

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        YICAD_LOG(yicad::log::render(), yicad::LogLevel::Error)
            << "渲染目标 " << d.debugName << " 的帧缓冲不完整，状态 0x" << std::hex << status;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &framebuffer);
        return 0;
    }
    return framebuffer;
}
