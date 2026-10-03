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

/// @file GLRhiCommandList.cpp
/// @brief RHI 命令列表的 OpenGL 实现

#include "GLRhiCommandList.h"

#include <algorithm>

#include "GLRhiDevice.h"
#include "GLRhiResources.h"
#include "GLRhiSurface.h"
#include "YiCadLog.h"

namespace
{
/// @brief 帧缓冲里第 i 个颜色附件的名字（0 号帧缓冲用 GL_COLOR）
GLenum colorAttachment(GLuint framebuffer, std::size_t index)
{
    return framebuffer == 0 ? GL_COLOR : GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(index);
}

/// @brief 按需建上下文里的临时帧缓冲
GLuint scratchFramebuffer(GLuint& slot)
{
    if (slot == 0)
    {
        glGenFramebuffers(1, &slot);
    }
    return slot;
}

GLsizei indexSize(RhiIndexFormat format)
{
    return format == RhiIndexFormat::Uint16 ? 2 : 4;
}

GLenum indexType(RhiIndexFormat format)
{
    return format == RhiIndexFormat::Uint16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
}

#define RHI_ERROR YICAD_LOG(yicad::log::render(), yicad::LogLevel::Error) << "RHI："
}  // namespace

GLRhiCommandList::GLRhiCommandList(GLRhiDevice& device)
    : m_device(device)
{
}

void GLRhiCommandList::begin(GLRhiContextState& state, GLRhiSurface* surface)
{
    m_state = &state;
    m_surface = surface;
    m_inFrame = true;
    m_inPass = false;
    m_pipeline = nullptr;
    m_groups = {};
    m_vertexBuffers = {};
    m_indexBuffer = nullptr;
    m_indexOffset = 0;
    invalidateState();
}

void GLRhiCommandList::end()
{
    if (m_inPass)
    {
        RHI_ERROR << "帧结束时渲染通道还没结束";
        endRenderPass();
    }
    glBindVertexArray(0);
    glUseProgram(0);
    // QOpenGLWidget 在 paintGL 之后用自己的帧缓冲，绑回去
    if (m_surface)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_surface->framebuffer());
    }
    m_pipeline = nullptr;
    m_groups = {};
    m_state = nullptr;
    m_surface = nullptr;
    m_inFrame = false;
}

void GLRhiCommandList::invalidateBindings()
{
    invalidateState();
}

void GLRhiCommandList::invalidateState()
{
    m_pipelineDirty = true;
    m_bindingsDirty = true;
    m_vertexBuffersDirty = true;
}

// ---------------------------------------------------------------------------
// 渲染通道
// ---------------------------------------------------------------------------

void GLRhiCommandList::beginRenderPass(const RhiRenderPassDesc& desc)
{
    if (m_inPass)
    {
        RHI_ERROR << "上一个渲染通道没有结束";
        endRenderPass();
    }

    std::vector<RhiFormat> colorFormats;
    RhiFormat depthFormat = RhiFormat::Undefined;
    if (desc.target)
    {
        const auto& target = static_cast<const GLRhiRenderTarget&>(*desc.target);
        auto it = m_state->framebuffers.find(target.id());
        if (it == m_state->framebuffers.end())
        {
            it = m_state->framebuffers.emplace(target.id(), target.createFramebuffer()).first;
        }
        m_passFramebuffer = it->second;
        m_passWidth = target.width();
        m_passHeight = target.height();
        for (const RhiTexturePtr& texture : target.desc().colorAttachments)
        {
            colorFormats.push_back(texture->format());
        }
        if (target.desc().depthStencil)
        {
            depthFormat = target.desc().depthStencil->format();
        }
    }
    else
    {
        if (!m_surface)
        {
            RHI_ERROR << "离屏帧没有交换链图像，渲染通道要给出目标";
            return;
        }
        m_passFramebuffer = m_surface->framebuffer();
        m_passWidth = m_surface->width();
        m_passHeight = m_surface->height();
        colorFormats.push_back(m_surface->colorFormat());
        depthFormat = m_surface->depthStencilFormat();
    }
    if (desc.target && m_passFramebuffer == 0)
    {
        return;  // 帧缓冲不完整，已记日志
    }

    m_passTarget = desc.target;
    m_passColorCount = colorFormats.size();
    m_passHasDepth = depthFormat != RhiFormat::Undefined;
    m_passDepthStore = desc.depthStore;
    m_passColorOps = desc.colorOps;
    m_passColorOps.resize(m_passColorCount);

    glBindFramebuffer(GL_FRAMEBUFFER, m_passFramebuffer);
    // 清除受裁剪与写掩码影响：先放开，下一次绘制前按管线重新设置
    glDisable(GL_SCISSOR_TEST);
    std::vector<GLenum> discard;
    for (std::size_t i = 0; i < m_passColorCount; ++i)
    {
        const RhiColorAttachmentOps& ops = m_passColorOps[i];
        const GLint drawBuffer = static_cast<GLint>(i);
        if (ops.load == RhiLoadOp::Clear)
        {
            glColorMaski(static_cast<GLuint>(i), GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            if (glRhiFormat(colorFormats[i]).integer)
            {
                const GLuint value[4] = {static_cast<GLuint>(ops.clearColor[0]), static_cast<GLuint>(ops.clearColor[1]),
                                         static_cast<GLuint>(ops.clearColor[2]), static_cast<GLuint>(ops.clearColor[3])};
                glClearBufferuiv(GL_COLOR, drawBuffer, value);
            }
            else
            {
                glClearBufferfv(GL_COLOR, drawBuffer, ops.clearColor.data());
            }
        }
        else if (ops.load == RhiLoadOp::DontCare)
        {
            discard.push_back(colorAttachment(m_passFramebuffer, i));
        }
    }
    if (m_passHasDepth)
    {
        if (desc.depthLoad == RhiLoadOp::Clear)
        {
            glDepthMask(GL_TRUE);
            glStencilMask(0xFF);
            if (depthFormat == RhiFormat::Depth24Stencil8)
            {
                glClearBufferfi(GL_DEPTH_STENCIL, 0, desc.clearDepth, static_cast<GLint>(desc.clearStencil));
            }
            else
            {
                glClearBufferfv(GL_DEPTH, 0, &desc.clearDepth);
            }
        }
        else if (desc.depthLoad == RhiLoadOp::DontCare)
        {
            discard.push_back(depthFormat == RhiFormat::Depth24Stencil8 ? GL_DEPTH_STENCIL_ATTACHMENT
                                                                        : GL_DEPTH_ATTACHMENT);
        }
    }
    if (!discard.empty())
    {
        glInvalidateFramebuffer(GL_FRAMEBUFFER, static_cast<GLsizei>(discard.size()), discard.data());
    }

    glViewport(0, 0, static_cast<GLsizei>(m_passWidth), static_cast<GLsizei>(m_passHeight));
    glDepthRangef(0.0f, 1.0f);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, static_cast<GLsizei>(m_passWidth), static_cast<GLsizei>(m_passHeight));
    invalidateState();
    m_inPass = true;
}

void GLRhiCommandList::endRenderPass()
{
    if (!m_inPass)
    {
        RHI_ERROR << "endRenderPass 之前没有 beginRenderPass";
        return;
    }
    m_inPass = false;

    // 解析多重采样：从本通道的帧缓冲 blit 到挂着解析目标的临时帧缓冲。blit 受裁剪影响
    glDisable(GL_SCISSOR_TEST);
    for (std::size_t i = 0; i < m_passColorCount; ++i)
    {
        const RhiColorAttachmentOps& ops = m_passColorOps[i];
        if (!ops.resolveTarget)
        {
            continue;
        }
        const auto& resolve = static_cast<const GLRhiTexture&>(*ops.resolveTarget);
        if (resolve.sampleCount() != 1 || resolve.width() != m_passWidth || resolve.height() != m_passHeight)
        {
            RHI_ERROR << "解析目标要是与附件同尺寸的单采样纹理";
            continue;
        }
        const GLuint draw = scratchFramebuffer(m_state->scratchDrawFramebuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_passFramebuffer);
        glReadBuffer(m_passFramebuffer == 0 ? GL_BACK : GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolve.name(), 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        const GLint w = static_cast<GLint>(m_passWidth);
        const GLint h = static_cast<GLint>(m_passHeight);
        glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    }

    std::vector<GLenum> discard;
    for (std::size_t i = 0; i < m_passColorCount; ++i)
    {
        if (m_passColorOps[i].store == RhiStoreOp::DontCare)
        {
            discard.push_back(colorAttachment(m_passFramebuffer, i));
        }
    }
    if (m_passHasDepth && m_passDepthStore == RhiStoreOp::DontCare)
    {
        // QOpenGLWidget 的帧缓冲与渲染目标的深度附件都可能是合并的深度模板
        discard.push_back(m_passFramebuffer == 0 ? GL_DEPTH : GL_DEPTH_STENCIL_ATTACHMENT);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, m_passFramebuffer);
    if (!discard.empty())
    {
        glInvalidateFramebuffer(GL_FRAMEBUFFER, static_cast<GLsizei>(discard.size()), discard.data());
    }
    m_passTarget = nullptr;
    invalidateState();
}

// ---------------------------------------------------------------------------
// 绘制状态
// ---------------------------------------------------------------------------

void GLRhiCommandList::setPipeline(const RhiPipeline& pipeline)
{
    m_pipeline = &static_cast<const GLRhiPipeline&>(pipeline);
    invalidateState();
}

void GLRhiCommandList::setBindGroup(std::uint32_t index, const RhiBindGroup& group,
                                    std::span<const std::uint32_t> dynamicOffsets)
{
    if (index >= kMaxBindGroups)
    {
        RHI_ERROR << "绑定组号 " << index << " 超出范围";
        return;
    }
    m_groups[index].group = &static_cast<const GLRhiBindGroup&>(group);
    m_groups[index].dynamicOffsets.assign(dynamicOffsets.begin(), dynamicOffsets.end());
    m_bindingsDirty = true;
}

void GLRhiCommandList::setVertexBuffers(std::uint32_t first, std::span<const RhiVertexBufferBinding> bindings)
{
    if (first + bindings.size() > kMaxVertexBuffers)
    {
        RHI_ERROR << "顶点缓冲槽超出范围";
        return;
    }
    std::copy(bindings.begin(), bindings.end(), m_vertexBuffers.begin() + first);
    m_vertexBuffersDirty = true;
}

void GLRhiCommandList::setIndexBuffer(const RhiBuffer& buffer, std::size_t offset, RhiIndexFormat format)
{
    m_indexBuffer = &buffer;
    m_indexOffset = offset;
    m_indexFormat = format;
    m_vertexBuffersDirty = true;
}

void GLRhiCommandList::setViewport(const RhiViewport& viewport)
{
    if (!m_inPass)
    {
        RHI_ERROR << "setViewport 要在渲染通道内";
        return;
    }
    // RHI 以左上角为原点，GL 以左下角为原点
    const float y = static_cast<float>(m_passHeight) - viewport.y - viewport.height;
    glViewportIndexedf(0, viewport.x, y, viewport.width, viewport.height);
    glDepthRangef(viewport.minDepth, viewport.maxDepth);
}

void GLRhiCommandList::setScissor(const RhiRect& rect)
{
    if (!m_inPass)
    {
        RHI_ERROR << "setScissor 要在渲染通道内";
        return;
    }
    const GLint y = static_cast<GLint>(m_passHeight) - rect.y - static_cast<GLint>(rect.height);
    glScissor(rect.x, y, static_cast<GLsizei>(rect.width), static_cast<GLsizei>(rect.height));
}

bool GLRhiCommandList::flushDrawState()
{
    if (!m_inPass)
    {
        RHI_ERROR << "绘制要在渲染通道内";
        return false;
    }
    if (!m_pipeline)
    {
        RHI_ERROR << "绘制之前没有设置管线";
        return false;
    }
    if (m_pipelineDirty)
    {
        m_pipeline->applyState();
        auto it = m_state->vertexArrays.find(m_pipeline->id());
        if (it == m_state->vertexArrays.end())
        {
            m_state->vertexArrays.emplace(m_pipeline->id(), m_pipeline->createVertexArray());
        }
        else
        {
            glBindVertexArray(it->second);
        }
        m_pipelineDirty = false;
        m_vertexBuffersDirty = true;
        m_bindingsDirty = true;
    }
    if (m_vertexBuffersDirty)
    {
        const std::vector<RhiVertexBufferLayout>& layouts = m_pipeline->desc().vertexBuffers;
        for (std::size_t slot = 0; slot < layouts.size() && slot < kMaxVertexBuffers; ++slot)
        {
            const RhiVertexBufferBinding& binding = m_vertexBuffers[slot];
            const GLuint buffer = binding.buffer ? static_cast<const GLRhiBuffer*>(binding.buffer)->name() : 0;
            glBindVertexBuffer(static_cast<GLuint>(slot), buffer, static_cast<GLintptr>(binding.offset),
                               static_cast<GLsizei>(layouts[slot].stride));
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                     m_indexBuffer ? static_cast<const GLRhiBuffer*>(m_indexBuffer)->name() : 0);
        m_vertexBuffersDirty = false;
    }
    if (m_bindingsDirty)
    {
        bindGroups();
        m_bindingsDirty = false;
    }
    return true;
}

void GLRhiCommandList::bindGroups()
{
    const std::vector<RhiBindGroupLayoutPtr>& layouts = m_pipeline->desc().bindGroupLayouts;
    for (std::size_t set = 0; set < layouts.size() && set < kMaxBindGroups; ++set)
    {
        if (!layouts[set])
        {
            continue;
        }
        const BoundGroup& bound = m_groups[set];
        if (!bound.group)
        {
            RHI_ERROR << "管线 " << m_pipeline->desc().debugName << " 的第 " << set << " 组没有绑定";
            continue;
        }
        if (!bound.group->layout().isCompatible(*layouts[set]))
        {
            RHI_ERROR << "第 " << set << " 组的绑定组与管线 " << m_pipeline->desc().debugName << " 的布局不兼容";
            continue;
        }

        std::size_t dynamicIndex = 0;
        const std::vector<GLRhiBindGroup::Binding>& bindings = bound.group->bindings();
        for (std::size_t i = 0; i < bindings.size(); ++i)
        {
            const GLRhiBindGroup::Binding& binding = bindings[i];
            const GLuint flat = m_pipeline->flatBinding(set, i);
            GLintptr offset = binding.offset;
            if (binding.hasDynamicOffset)
            {
                if (dynamicIndex >= bound.dynamicOffsets.size())
                {
                    RHI_ERROR << "第 " << set << " 组缺少动态偏移";
                    break;
                }
                offset += static_cast<GLintptr>(bound.dynamicOffsets[dynamicIndex++]);
            }
            switch (binding.type)
            {
            case RhiBindingType::UniformBuffer:
                glBindBufferRange(GL_UNIFORM_BUFFER, flat, binding.buffer, offset, binding.size);
                break;
            case RhiBindingType::StorageBuffer:
                glBindBufferRange(GL_SHADER_STORAGE_BUFFER, flat, binding.buffer, offset, binding.size);
                break;
            case RhiBindingType::TexelBuffer:
                glActiveTexture(GL_TEXTURE0 + flat);
                glBindTexture(GL_TEXTURE_BUFFER, binding.texture);
                glBindSampler(flat, 0);
                break;
            case RhiBindingType::CombinedTextureSampler:
                glActiveTexture(GL_TEXTURE0 + flat);
                glBindTexture(binding.textureTarget, binding.texture);
                glBindSampler(flat, binding.sampler);
                break;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 绘制
// ---------------------------------------------------------------------------

void GLRhiCommandList::draw(std::uint32_t vertexCount, std::uint32_t instanceCount,
                            std::uint32_t firstVertex, std::uint32_t firstInstance)
{
    if (!flushDrawState())
    {
        return;
    }
    glDrawArraysInstancedBaseInstance(m_pipeline->mode(), static_cast<GLint>(firstVertex),
                                      static_cast<GLsizei>(vertexCount), static_cast<GLsizei>(instanceCount),
                                      firstInstance);
}

void GLRhiCommandList::drawIndexed(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex,
                                   std::int32_t vertexOffset, std::uint32_t firstInstance)
{
    if (!m_indexBuffer)
    {
        RHI_ERROR << "drawIndexed 之前没有设置索引缓冲";
        return;
    }
    if (!flushDrawState())
    {
        return;
    }
    const std::size_t byteOffset = m_indexOffset + static_cast<std::size_t>(firstIndex) * indexSize(m_indexFormat);
    glDrawElementsInstancedBaseVertexBaseInstance(m_pipeline->mode(), static_cast<GLsizei>(indexCount),
                                                  indexType(m_indexFormat), reinterpret_cast<const void*>(byteOffset),
                                                  static_cast<GLsizei>(instanceCount), vertexOffset, firstInstance);
}

void GLRhiCommandList::drawIndirect(const RhiBuffer& args, std::size_t offset, std::uint32_t drawCount,
                                    std::uint32_t stride)
{
    if (!flushDrawState())
    {
        return;
    }
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, static_cast<const GLRhiBuffer&>(args).name());
    glMultiDrawArraysIndirect(m_pipeline->mode(), reinterpret_cast<const void*>(offset),
                              static_cast<GLsizei>(drawCount), static_cast<GLsizei>(stride));
}

void GLRhiCommandList::drawIndexedIndirect(const RhiBuffer& args, std::size_t offset, std::uint32_t drawCount,
                                           std::uint32_t stride)
{
    if (!m_indexBuffer)
    {
        RHI_ERROR << "drawIndexedIndirect 之前没有设置索引缓冲";
        return;
    }
    // GL 的间接绘制里 firstIndex 从索引缓冲开头算，没有地方再加绑定偏移
    if (m_indexOffset != 0)
    {
        RHI_ERROR << "GL 后端的 drawIndexedIndirect 要求索引缓冲的绑定偏移为 0";
        return;
    }
    if (!flushDrawState())
    {
        return;
    }
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, static_cast<const GLRhiBuffer&>(args).name());
    glMultiDrawElementsIndirect(m_pipeline->mode(), indexType(m_indexFormat), reinterpret_cast<const void*>(offset),
                                static_cast<GLsizei>(drawCount), static_cast<GLsizei>(stride));
}

// ---------------------------------------------------------------------------
// 复制
// ---------------------------------------------------------------------------

void GLRhiCommandList::copyBuffer(const RhiBuffer& src, std::size_t srcOffset, const RhiBuffer& dst,
                                  std::size_t dstOffset, std::size_t size)
{
    if (m_inPass)
    {
        RHI_ERROR << "复制要在渲染通道外";
        return;
    }
    if (!rhiAny(src.desc().usage & RhiBufferUsage::CopySrc) || !rhiAny(dst.desc().usage & RhiBufferUsage::CopyDst)
        || srcOffset + size > src.size() || dstOffset + size > dst.size())
    {
        RHI_ERROR << "copyBuffer 的用途或范围不对";
        return;
    }
    glBindBuffer(GL_COPY_READ_BUFFER, static_cast<const GLRhiBuffer&>(src).name());
    glBindBuffer(GL_COPY_WRITE_BUFFER, static_cast<const GLRhiBuffer&>(dst).name());
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, static_cast<GLintptr>(srcOffset),
                        static_cast<GLintptr>(dstOffset), static_cast<GLsizeiptr>(size));
}

void GLRhiCommandList::copyTextureToBuffer(const RhiTexture& src, const RhiBuffer& dst, std::size_t dstOffset)
{
    if (m_inPass)
    {
        RHI_ERROR << "复制要在渲染通道外";
        return;
    }
    const auto& texture = static_cast<const GLRhiTexture&>(src);
    const std::size_t bytes = static_cast<std::size_t>(texture.width()) * texture.height() * rhiFormatSize(texture.format());
    if (texture.sampleCount() != 1 || rhiIsDepthFormat(texture.format())
        || !rhiAny(texture.desc().usage & RhiTextureUsage::CopySrc))
    {
        RHI_ERROR << "copyTextureToBuffer 的源要是带 CopySrc 用途的单采样颜色纹理";
        return;
    }
    if (!rhiAny(dst.desc().usage & RhiBufferUsage::CopyDst) || dstOffset + bytes > dst.size())
    {
        RHI_ERROR << "copyTextureToBuffer 的目标用途或大小不对";
        return;
    }

    const GLuint read = scratchFramebuffer(m_state->scratchReadFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, read);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.name(), 0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<const GLRhiBuffer&>(dst).name());
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    const GLRhiFormatInfo info = glRhiFormat(texture.format());
    glReadPixels(0, 0, static_cast<GLsizei>(texture.width()), static_cast<GLsizei>(texture.height()), info.format,
                 info.type, reinterpret_cast<void*>(dstOffset));
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
}
