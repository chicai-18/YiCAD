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

/// @file RhiCommandList.h
/// @brief RHI 的命令列表（RENDER_PLAN.md 第 4.7.2 节）

#ifndef RHICOMMANDLIST_H
#define RHICOMMANDLIST_H

#include "RhiResources.h"

/// @brief 一帧的命令列表，由 RhiDevice::beginFrame() 给出，endFrame() 之后失效
/// @details 绘制与绑定只能在渲染通道内；复制只能在渲染通道外。资源经裸指针或引用传入，
///          调用方在录制期间持有它们的句柄即可：句柄释放后对象进延迟释放队列，等本帧完成才销毁
class RhiCommandList
{
public:
    virtual ~RhiCommandList() = default;

    /// @brief 开始渲染通道；视口与裁剪矩形重置为整个目标
    virtual void beginRenderPass(const RhiRenderPassDesc& desc) = 0;
    /// @brief 结束渲染通道：按存储动作处理附件，解析多重采样
    virtual void endRenderPass() = 0;

    virtual void setPipeline(const RhiPipeline& pipeline) = 0;
    /// @brief 绑定第 index 组；dynamicOffsets 按布局里带动态偏移的项的绑定号顺序给出
    virtual void setBindGroup(std::uint32_t index, const RhiBindGroup& group,
                              std::span<const std::uint32_t> dynamicOffsets = {}) = 0;
    /// @brief 从第 first 个槽起依次设置顶点缓冲
    virtual void setVertexBuffers(std::uint32_t first, std::span<const RhiVertexBufferBinding> bindings) = 0;
    virtual void setIndexBuffer(const RhiBuffer& buffer, std::size_t offset, RhiIndexFormat format) = 0;
    virtual void setViewport(const RhiViewport& viewport) = 0;
    virtual void setScissor(const RhiRect& rect) = 0;

    virtual void draw(std::uint32_t vertexCount, std::uint32_t instanceCount,
                      std::uint32_t firstVertex, std::uint32_t firstInstance) = 0;
    virtual void drawIndexed(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex,
                             std::int32_t vertexOffset, std::uint32_t firstInstance) = 0;
    /// @brief 多重间接绘制；参数是 RhiDrawIndirectArgs 数组
    virtual void drawIndirect(const RhiBuffer& args, std::size_t offset, std::uint32_t drawCount,
                              std::uint32_t stride = sizeof(RhiDrawIndirectArgs)) = 0;
    /// @brief 多重间接绘制；参数是 RhiDrawIndexedIndirectArgs 数组
    virtual void drawIndexedIndirect(const RhiBuffer& args, std::size_t offset, std::uint32_t drawCount,
                                     std::uint32_t stride = sizeof(RhiDrawIndexedIndirectArgs)) = 0;

    /// @brief 缓冲间复制（渲染通道外）；源要有 CopySrc 用途，目标要有 CopyDst
    virtual void copyBuffer(const RhiBuffer& src, std::size_t srcOffset, const RhiBuffer& dst,
                            std::size_t dstOffset, std::size_t size) = 0;
    /// @brief 把单采样颜色纹理第 0 级整个复制进缓冲（渲染通道外），按行紧密排列；测试出图用
    /// @details 行序按后端：RhiCaps::framebufferOriginBottomLeft 为真时第 0 行是画面底部
    virtual void copyTextureToBuffer(const RhiTexture& src, const RhiBuffer& dst, std::size_t dstOffset = 0) = 0;
    /// @brief GPU 执行到这里时把时间戳写进 set 的第 index 项（RhiCaps::timestampQueries），用 RhiDevice::readTimestamps 读出
    virtual void writeTimestamp(const RhiQuerySet& set, std::uint32_t index) = 0;
};

#endif // RHICOMMANDLIST_H
