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

/// @file GLRhiCommandList.h
/// @brief RHI 命令列表的 OpenGL 实现

#ifndef GLRHICOMMANDLIST_H
#define GLRHICOMMANDLIST_H

#include <GL/glew.h>

#include <array>
#include <vector>

#include "RhiCommandList.h"

class GLRhiDevice;
class GLRhiPipeline;
class GLRhiBindGroup;
class GLRhiSurface;
struct GLRhiContextState;

/// @brief GL 的命令列表：调用即执行（GL 本来就是立即模式）
/// @details 绑定在绘制时才落到 GL 上（管线换了要按新管线的平铺绑定点重新绑定）。
///          视口与裁剪矩形按 RHI 的左上角原点给出，这里按目标高度换算成 GL 的左下角原点
class GLRhiCommandList final : public RhiCommandList
{
public:
    explicit GLRhiCommandList(GLRhiDevice& device);

    /// @brief 一帧开始：本帧的上下文状态与表面（离屏帧为空）
    void begin(GLRhiContextState& state, GLRhiSurface* surface);
    /// @brief 一帧结束
    void end();
    /// @brief 设备在帧内建资源时动过纹理、缓冲的绑定：下一次绘制前全部重新绑定
    void invalidateBindings();

    void beginRenderPass(const RhiRenderPassDesc& desc) override;
    void endRenderPass() override;
    void setPipeline(const RhiPipeline& pipeline) override;
    void setBindGroup(std::uint32_t index, const RhiBindGroup& group,
                      std::span<const std::uint32_t> dynamicOffsets) override;
    void setVertexBuffers(std::uint32_t first, std::span<const RhiVertexBufferBinding> bindings) override;
    void setIndexBuffer(const RhiBuffer& buffer, std::size_t offset, RhiIndexFormat format) override;
    void setViewport(const RhiViewport& viewport) override;
    void setScissor(const RhiRect& rect) override;
    void draw(std::uint32_t vertexCount, std::uint32_t instanceCount,
              std::uint32_t firstVertex, std::uint32_t firstInstance) override;
    void drawIndexed(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex,
                     std::int32_t vertexOffset, std::uint32_t firstInstance) override;
    void drawIndirect(const RhiBuffer& args, std::size_t offset, std::uint32_t drawCount,
                      std::uint32_t stride) override;
    void drawIndexedIndirect(const RhiBuffer& args, std::size_t offset, std::uint32_t drawCount,
                             std::uint32_t stride) override;
    void copyBuffer(const RhiBuffer& src, std::size_t srcOffset, const RhiBuffer& dst,
                    std::size_t dstOffset, std::size_t size) override;
    void copyTextureToBuffer(const RhiTexture& src, const RhiBuffer& dst, std::size_t dstOffset) override;
    void writeTimestamp(const RhiQuerySet& set, std::uint32_t index) override;

private:
    static constexpr std::size_t kMaxBindGroups = 4;
    static constexpr std::size_t kMaxVertexBuffers = 16;

    /// @brief 绘制前把管线状态、顶点与索引缓冲、绑定组落到 GL 上；不能绘制时返回 false
    bool flushDrawState();
    void bindGroups();
    /// @brief 清除、解析等会改掉的状态，下一次绘制前重新设置
    void invalidateState();

    struct BoundGroup
    {
        const GLRhiBindGroup* group = nullptr;
        std::vector<std::uint32_t> dynamicOffsets;
    };

    GLRhiDevice& m_device;
    GLRhiContextState* m_state = nullptr;
    GLRhiSurface* m_surface = nullptr;
    bool m_inFrame = false;

    // 渲染通道
    bool m_inPass = false;
    GLuint m_passFramebuffer = 0;
    std::uint32_t m_passWidth = 0;
    std::uint32_t m_passHeight = 0;
    std::vector<RhiColorAttachmentOps> m_passColorOps;
    std::size_t m_passColorCount = 0;
    bool m_passHasDepth = false;
    RhiStoreOp m_passDepthStore = RhiStoreOp::DontCare;
    const RhiRenderTarget* m_passTarget = nullptr;

    // 绘制状态
    const GLRhiPipeline* m_pipeline = nullptr;
    bool m_pipelineDirty = true;
    bool m_bindingsDirty = true;
    bool m_vertexBuffersDirty = true;
    std::array<BoundGroup, kMaxBindGroups> m_groups;
    std::array<RhiVertexBufferBinding, kMaxVertexBuffers> m_vertexBuffers{};
    const RhiBuffer* m_indexBuffer = nullptr;
    std::size_t m_indexOffset = 0;
    RhiIndexFormat m_indexFormat = RhiIndexFormat::Uint32;
};

#endif // GLRHICOMMANDLIST_H
