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

/// @file GsView.cpp
/// @brief GsOverlay 与 GsView 实现

#include "GsView.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numeric>

#include "DmBlock.h"
#include "GsDevice.h"
#include "ScopedTimer.h"

namespace
{

constexpr std::size_t kHighlightBitmapThreshold = 1000;  ///< 高亮集超过它时改走状态位图（第 4.3.8 节）
/// @brief 选中的实体不超过它时场景里的线段按细线画、选中的另按四边形画；超过时全部按四边形画
constexpr std::size_t kEmphasisLimit = 20000;
constexpr std::uint32_t kFrameSlots = 4;
constexpr std::uint32_t kSlotScene = 0;
constexpr std::uint32_t kSlotHighlight = 1;
constexpr std::uint32_t kSlotTransient = 2;
constexpr std::uint32_t kSlotPlain = 3;
constexpr double kGripSize = 15.0;          ///< 夹点边长（像素），与旧渲染器的选中控制点相同
constexpr float kEmphasisPixels = 4.0f;     ///< 选中、高亮加宽的像素，与旧渲染器的 HIGHLIGHT_WIDTH 相同
constexpr float kPointPixels = 2.0f;        ///< 点的像素，与旧渲染器的 DEFAULT_POINT_SIZE 相同
/// @brief 显示线宽时每单位线宽代码（毫米 × 100）的像素：线宽按 5 像素/毫米显示，不随缩放变（第 4.6 节，用户定的固定换算）
constexpr float kLineWidthPerCode = 0.05f;
constexpr float kMinDashPeriodPixels = 2.0f;  ///< 线型的周期在屏幕上短于它时画实线（第 4.5.4 节）
// LOD 的阈值（像素，第 4.3.10 节）
constexpr float kLodTextPixels = 2.0f;        ///< 字高小于它时字形不画，画沿基线的细条
constexpr float kLodHatchPixels = 2.0f;       ///< 填充图案的线距小于它时不画图案线，画按覆盖率的实心
constexpr float kLodObjectPixels = 1.0f;      ///< 对象包围框小于它时画成一个点
constexpr float kLodArcPixels = 4.0f;         ///< 圆弧（含线宽外扩前）的半径不超过它时只画一个四边形，不画 8 段环带
// 渐进绘制（第 4.3.10 节）
constexpr double kSceneBudgetMs = 10.0;          ///< 每帧场景通道的 GPU 时间预算（60 帧的一帧 16.7 毫秒，留出贴图与叠加层）
constexpr double kDefaultVertexRate = 4.0e6;     ///< 还没测出时假定的每毫秒顶点数（本机独显约 4×10⁶，阶段 6 的测量）
constexpr double kMinBudgetVertices = 2.0e5;     ///< 每帧至少画这么多顶点：再慢的机器也要往前走
constexpr double kMinTimedVertices = 2.0e5;      ///< 一帧画的顶点少于它时不拿来估速度（固定开销占大头）
constexpr std::uint32_t kTimestampSlots = 8;     ///< 时间戳环：同时在等结果的帧最多这么多

std::uint32_t packColor(const QColor& c)
{
    return gsPackColor(c.red(), c.green(), c.blue(), c.alpha());
}

std::array<float, 4> toFloat4(const QColor& c)
{
    return {static_cast<float>(c.redF()), static_cast<float>(c.greenF()), static_cast<float>(c.blueF()),
            static_cast<float>(c.alphaF())};
}

GsProgram programOf(GsClass c)
{
    switch (c)
    {
    case GsClass::Segment: return GsProgram::Segment;
    case GsClass::Arc: return GsProgram::Arc;
    case GsClass::Fill: return GsProgram::Fill;
    case GsClass::Point: return GsProgram::Point;
    case GsClass::Image: return GsProgram::Image;
    case GsClass::InfiniteLine: return GsProgram::InfiniteLine;
    case GsClass::Count: break;
    }
    return GsProgram::Segment;
}

/// @brief 画的先后：同一对象的填充在线之下（绘图次序相同的深度时，后画的在上）
constexpr std::array<GsClass, 5> kDrawOrder = {GsClass::Fill, GsClass::Segment, GsClass::Arc, GsClass::Point,
                                               GsClass::InfiniteLine};

/// @brief 视点对间距取模（double，结果在 [0, spacing)）
double positiveMod(double v, double spacing)
{
    return v - spacing * std::floor(v / spacing);
}

}  // namespace

// ---------------------------------------------------------------------------
// GsOverlay
// ---------------------------------------------------------------------------

void GsOverlay::triangle(const GsOverlayVertex& a, const GsOverlayVertex& b, const GsOverlayVertex& c)
{
    m_vertices.push_back(a);
    m_vertices.push_back(b);
    m_vertices.push_back(c);
}

void GsOverlay::line(double x0, double y0, double x1, double y1, double width, const QColor& color, bool dashed)
{
    const double dx = x1 - x0;
    const double dy = y1 - y0;
    const double len = std::hypot(dx, dy);
    const double ux = len > 0.0 ? dx / len : 1.0;
    const double uy = len > 0.0 ? dy / len : 0.0;
    const double h = width * 0.5;
    const double nx = -uy * h;
    const double ny = ux * h;
    // 端点各延长半个线宽，两条首尾相接的线段在角上不留缺口
    const double ex = ux * h;
    const double ey = uy * h;
    const std::uint32_t c = packColor(color);
    const float d0 = dashed ? 0.0f : -1.0f;
    const float d1 = dashed ? static_cast<float>(len) : -1.0f;
    const GsOverlayVertex a{static_cast<float>(x0 - ex + nx), static_cast<float>(y0 - ey + ny), c, d0};
    const GsOverlayVertex b{static_cast<float>(x0 - ex - nx), static_cast<float>(y0 - ey - ny), c, d0};
    const GsOverlayVertex e{static_cast<float>(x1 + ex + nx), static_cast<float>(y1 + ey + ny), c, d1};
    const GsOverlayVertex f{static_cast<float>(x1 + ex - nx), static_cast<float>(y1 + ey - ny), c, d1};
    triangle(a, b, f);
    triangle(a, f, e);
}

void GsOverlay::fillRect(double x0, double y0, double x1, double y1, const QColor& color)
{
    const std::uint32_t c = packColor(color);
    const GsOverlayVertex a{static_cast<float>(x0), static_cast<float>(y0), c, -1.0f};
    const GsOverlayVertex b{static_cast<float>(x1), static_cast<float>(y0), c, -1.0f};
    const GsOverlayVertex d{static_cast<float>(x1), static_cast<float>(y1), c, -1.0f};
    const GsOverlayVertex e{static_cast<float>(x0), static_cast<float>(y1), c, -1.0f};
    triangle(a, b, d);
    triangle(a, d, e);
}

// ---------------------------------------------------------------------------
// GsView
// ---------------------------------------------------------------------------

GsView::GsView() = default;

GsView::~GsView()
{
    release();
}

void GsView::release()
{
    m_sceneList.clear();
    m_highlightList.clear();
    m_transientList.clear();
    m_frameBuffer.reset();
    m_cellOffsets.reset();
    m_transientCellOffsets.reset();
    m_viewBitsBuffer.reset();
    m_indirect.reset();
    m_sceneIndirect.reset();
    m_overlayVertices.reset();
    m_blockInstanceBuffer.reset();
    m_blitGroup.reset();
    m_sceneTarget.reset();
    m_sceneColor.reset();
    m_sceneDepth.reset();
    m_sceneResolve.reset();
    m_targetWidth = m_targetHeight = m_targetSamples = 0;
    invalidateScene();
    m_timestamps.reset();
    m_pendingTimings.clear();
    m_model.reset();
    m_device.reset();
}

void GsView::setModel(std::shared_ptr<GsModel> model, bool selection)
{
    m_model = std::move(model);
    m_selection = selection;
    m_block = nullptr;
    invalidateScene();
    m_viewBitsDirty = true;
    m_highlightDirty = true;
}

void GsView::setTransient(GsModel* transient)
{
    m_transient = transient;
}

void GsView::setBlock(std::shared_ptr<GsModel> model, const DmBlock* block)
{
    m_model = std::move(model);
    m_block = block;
    m_selection = false;
    invalidateScene();
}

void GsView::setCamera(const DmVector& center, double worldPerPixel)
{
    m_center = center;
    m_worldPerPixel = worldPerPixel;
}

void GsView::setStyle(const GsViewStyle& style)
{
    m_style = style;
}

void GsView::setHighlighted(std::vector<DmEntity*> entities)
{
    const bool wasBitmap = m_useBitmapHighlight;
    m_highlighted = std::move(entities);
    m_useBitmapHighlight = m_highlighted.size() > kHighlightBitmapThreshold;
    m_highlightDirty = true;
    if (m_useBitmapHighlight || wasBitmap)
    {
        m_viewBitsDirty = true;
        invalidateScene();
    }
}

void GsView::setHidden(std::vector<DmEntity*> entities)
{
    if (entities.empty() && m_hidden.empty())
    {
        return;
    }
    m_hidden = std::move(entities);
    m_viewBitsDirty = true;
    invalidateScene();
}

void GsView::setGrips(std::vector<DmVector> grips)
{
    m_grips = std::move(grips);
}

bool GsView::ensureDevice()
{
    if (!m_device)
    {
        m_device = GsDevice::acquire();
    }
    return m_device != nullptr;
}

RhiBufferPtr GsView::ensureBuffer(RhiBufferPtr& buffer, std::size_t bytes, RhiBufferUsage usage, const char* name)
{
    bytes = std::max<std::size_t>(bytes, 256);
    if (!buffer || buffer->size() < bytes)
    {
        RhiBufferDesc desc;
        desc.size = bytes * 2;
        desc.usage = usage;
        desc.debugName = name;
        buffer = m_device->rhi().createBuffer(desc);
    }
    return buffer;
}

bool GsView::ensureTargets(std::uint32_t width, std::uint32_t height, std::uint32_t samples)
{
    if (m_sceneTarget && m_targetWidth == width && m_targetHeight == height && m_targetSamples == samples)
    {
        return true;
    }
    RhiDevice& rhi = m_device->rhi();
    m_sceneTarget.reset();
    m_blitGroup.reset();
    RhiTextureDesc color;
    color.width = width;
    color.height = height;
    color.format = RhiFormat::RGBA8Unorm;
    color.sampleCount = samples;
    color.usage = samples > 1 ? RhiTextureUsage::ColorTarget : (RhiTextureUsage::ColorTarget | RhiTextureUsage::Sampled);
    color.debugName = "gs scene";
    m_sceneColor = rhi.createTexture(color);
    RhiTextureDesc depth = color;
    depth.format = RhiFormat::Depth24Stencil8;
    depth.usage = RhiTextureUsage::DepthStencil;
    depth.debugName = "gs scene depth";
    m_sceneDepth = rhi.createTexture(depth);
    if (samples > 1)
    {
        RhiTextureDesc resolve = color;
        resolve.sampleCount = 1;
        resolve.usage = RhiTextureUsage::ColorTarget | RhiTextureUsage::Sampled | RhiTextureUsage::CopySrc;
        resolve.debugName = "gs scene resolve";
        m_sceneResolve = rhi.createTexture(resolve);
    }
    else
    {
        m_sceneResolve = m_sceneColor;
    }
    if (!m_sceneColor || !m_sceneDepth || !m_sceneResolve)
    {
        return false;
    }
    RhiRenderTargetDesc target;
    target.colorAttachments = {m_sceneColor};
    target.depthStencil = m_sceneDepth;
    target.debugName = "gs scene";
    m_sceneTarget = rhi.createRenderTarget(target);
    RhiBindGroupDesc blit;
    blit.layout = m_device->layout(GsLayout::Image);
    RhiBindGroupEntry entry;
    entry.binding = 0;
    entry.texture = m_sceneResolve;
    entry.sampler = m_device->blitSampler();
    blit.entries = {entry};
    blit.debugName = "gs scene blit";
    m_blitGroup = rhi.createBindGroup(blit);
    m_targetWidth = width;
    m_targetHeight = height;
    m_targetSamples = samples;
    invalidateScene();
    return m_sceneTarget && m_blitGroup;
}

void GsView::prepareViewBits()
{
    const std::uint32_t capacity = m_model ? m_model->slotCapacity() : 0;
    const std::size_t words = std::max<std::size_t>((capacity + 15) / 16, 4);
    m_viewBits.assign(words, 0u);
    if (m_model)
    {
        auto set = [this](const std::vector<DmEntity*>& entities, std::uint32_t bit) {
            for (const DmEntity* e : entities)
            {
                const std::uint32_t slot = m_model->slotOf(e);
                if (slot != kGsNoSlot)
                {
                    m_viewBits[slot >> 4] |= bit << ((slot & 15u) * 2u);
                }
            }
        };
        if (m_useBitmapHighlight)
        {
            set(m_highlighted, 1u);
        }
        set(m_hidden, 2u);
    }
    ensureBuffer(m_viewBitsBuffer, m_viewBits.size() * sizeof(std::uint32_t), RhiBufferUsage::Texel, "gs view bits");
    if (m_viewBitsBuffer)
    {
        m_device->rhi().upload(*m_viewBitsBuffer, 0,
                               std::span<const std::byte>(reinterpret_cast<const std::byte*>(m_viewBits.data()),
                                                          m_viewBits.size() * sizeof(std::uint32_t)));
    }
}

void GsView::uploadCellOffsets(const std::vector<DmVector>& origins, RhiBufferPtr& buffer, std::vector<float>& uploaded,
                               double eyeX, double eyeY)
{
    std::vector<float> offsets(std::max<std::size_t>(origins.size(), 1) * 2, 0.0f);
    for (std::size_t i = 0; i < origins.size(); ++i)
    {
        offsets[i * 2] = static_cast<float>(origins[i].x - eyeX);
        offsets[i * 2 + 1] = static_cast<float>(origins[i].y - eyeY);
    }
    const RhiBuffer* previous = buffer.get();
    ensureBuffer(buffer, offsets.size() * sizeof(float), RhiBufferUsage::Texel, "gs cell offsets");
    // 相机与分块都没变时（只移动光标的帧）不重传
    if (buffer && (buffer.get() != previous || offsets != uploaded))
    {
        m_device->rhi().upload(*buffer, 0,
                               std::span<const std::byte>(reinterpret_cast<const std::byte*>(offsets.data()),
                                                          offsets.size() * sizeof(float)));
        uploaded = std::move(offsets);
    }
}

RhiBindGroupPtr GsView::frameGroup(std::uint32_t slot, const RhiBufferPtr& cells)
{
    const std::size_t stride = (std::max<std::size_t>(sizeof(GsFrameConstants),
                                                      m_device->rhi().caps().uniformBufferOffsetAlignment)
                                + 255) / 256 * 256;
    RhiBindGroupDesc desc;
    desc.layout = m_device->layout(GsLayout::Frame);
    RhiBindGroupEntry frame;
    frame.binding = 0;
    frame.buffer = m_frameBuffer;
    frame.offset = stride * slot;
    frame.size = sizeof(GsFrameConstants);
    RhiBindGroupEntry offsets;
    offsets.binding = 1;
    offsets.buffer = cells;
    offsets.texelFormat = RhiFormat::RG32Float;
    RhiBindGroupEntry bits;
    bits.binding = 2;
    bits.buffer = m_viewBitsBuffer;
    bits.texelFormat = RhiFormat::R32Uint;
    desc.entries = {frame, offsets, bits};
    desc.debugName = "gs frame";
    return m_device->rhi().createBindGroup(desc);
}

GsView::DrawRange GsView::wholeRange(const GsDrawList& list)
{
    DrawRange r;
    for (std::size_t i = 0; i < kGsClassCount; ++i)
    {
        r.end[i] = static_cast<std::uint32_t>(list.commands[i].size());
    }
    r.smallArcsEnd = static_cast<std::uint32_t>(list.smallArcs.size());
    r.imagesEnd = static_cast<std::uint32_t>(list.images.size());
    return r;
}

GsView::DrawRange GsView::chunkRange(const GsDrawList& list, std::size_t first, std::size_t last)
{
    DrawRange r;
    if (first > 0)
    {
        const GsDrawList::Chunk& before = list.chunks[first - 1];
        r.begin = before.end;
        r.smallArcsBegin = before.smallArcsEnd;
        r.imagesBegin = before.imagesEnd;
    }
    const GsDrawList::Chunk& through = list.chunks[last - 1];
    r.end = through.end;
    r.smallArcsEnd = through.smallArcsEnd;
    r.imagesEnd = through.imagesEnd;
    return r;
}

void GsView::drawList(RhiCommandList& commands, const GsDrawList& list, const DrawRange& range, const RhiBuffer& indirect,
                      std::size_t indirectBase, const GsModel& model, const RhiBindGroup& frame,
                      const RhiBuffer& instances, std::uint32_t samples, bool scene, bool hairlines)
{
    const GsPipelineVariant variant = scene ? GsPipelineVariant::Scene : GsPipelineVariant::Overlay;
    // 间接参数在缓冲里按管线类依次排列，最后是小圆弧（与 render() 填写的顺序相同）
    std::array<std::size_t, kGsClassCount> starts{};
    std::size_t cursor = indirectBase;
    for (std::size_t i = 0; i < kGsClassCount; ++i)
    {
        starts[i] = cursor;
        cursor += list.commands[i].size();
    }
    const std::size_t smallArcStart = cursor;
    const RhiVertexBufferBinding binding{&instances, 0};

    // 图片先画（与填充同为面，线画在它们之上）
    if (range.imagesEnd > range.imagesBegin && model.geometryGroup(GsClass::Image))
    {
        const RhiPipelinePtr& pipeline = m_device->pipeline(GsProgram::Image, variant, samples);
        if (pipeline)
        {
            commands.setPipeline(*pipeline);
            commands.setBindGroup(0, frame);
            commands.setBindGroup(1, *model.modelGroup());
            commands.setBindGroup(2, *model.geometryGroup(GsClass::Image));
            commands.setVertexBuffers(0, std::span<const RhiVertexBufferBinding>(&binding, 1));
            for (std::uint32_t k = range.imagesBegin; k < range.imagesEnd; ++k)
            {
                const GsDrawList::ImageDraw& image = list.images[k];
                commands.setBindGroup(3, *image.textureGroup);
                commands.draw(image.args.vertexCount, image.args.instanceCount, image.args.firstVertex,
                              image.args.firstInstance);
            }
        }
    }
    bool smallArcsDrawn = false;
    for (GsClass c : kDrawOrder)
    {
        const std::size_t i = static_cast<std::size_t>(c);
        const std::size_t count = range.end[i] - range.begin[i];
        if (count == 0 || !model.geometryGroup(c))
        {
            continue;
        }
        const GsProgram program = hairlines && c == GsClass::Segment ? GsProgram::Hairline : programOf(c);
        const RhiPipelinePtr& pipeline = m_device->pipeline(program, variant, samples);
        if (!pipeline)
        {
            continue;
        }
        commands.setPipeline(*pipeline);
        commands.setBindGroup(0, frame);
        commands.setBindGroup(1, *model.modelGroup());
        commands.setBindGroup(2, *model.geometryGroup(c));
        commands.setVertexBuffers(0, std::span<const RhiVertexBufferBinding>(&binding, 1));
        commands.drawIndirect(indirect, (starts[i] + range.begin[i]) * sizeof(RhiDrawIndirectArgs),
                              static_cast<std::uint32_t>(count));
        if (c == GsClass::Arc)
        {
            drawSmallArcs(commands, range, indirect, smallArcStart, model, frame, binding, samples, scene);
            smallArcsDrawn = true;
        }
    }
    if (!smallArcsDrawn)
    {
        drawSmallArcs(commands, range, indirect, smallArcStart, model, frame, binding, samples, scene);
    }
}

void GsView::drawSmallArcs(RhiCommandList& commands, const DrawRange& range, const RhiBuffer& indirect, std::size_t start,
                           const GsModel& model, const RhiBindGroup& frame, const RhiVertexBufferBinding& binding,
                           std::uint32_t samples, bool scene)
{
    const std::uint32_t count = range.smallArcsEnd - range.smallArcsBegin;
    if (count == 0 || !model.geometryGroup(GsClass::Arc))
    {
        return;
    }
    const RhiPipelinePtr& pipeline =
        m_device->pipeline(GsProgram::ArcSmall, scene ? GsPipelineVariant::Scene : GsPipelineVariant::Overlay, samples);
    if (!pipeline)
    {
        return;
    }
    commands.setPipeline(*pipeline);
    commands.setBindGroup(0, frame);
    commands.setBindGroup(1, *model.modelGroup());
    commands.setBindGroup(2, *model.geometryGroup(GsClass::Arc));
    commands.setVertexBuffers(0, std::span<const RhiVertexBufferBinding>(&binding, 1));
    commands.drawIndirect(indirect, (start + range.smallArcsBegin) * sizeof(RhiDrawIndirectArgs), count);
}

void GsView::readSceneTimings()
{
    // 时间戳要几帧之后才可读：按写的先后读，读不出就等下一帧（不等待 GPU）
    std::size_t done = 0;
    for (const PendingTiming& t : m_pendingTimings)
    {
        std::array<std::uint64_t, 2> times{};
        if (!m_timestamps || !m_device->rhi().readTimestamps(*m_timestamps, t.slot * 2, times))
        {
            break;
        }
        ++done;
        const double ms = static_cast<double>(times[1] - times[0]) / 1.0e6;
        if (t.vertices >= kMinTimedVertices && ms > 0.0)
        {
            const double rate = t.vertices / ms;
            m_vertexRate = m_vertexRate > 0.0 ? m_vertexRate * 0.7 + rate * 0.3 : rate;
        }
    }
    m_pendingTimings.erase(m_pendingTimings.begin(), m_pendingTimings.begin() + static_cast<std::ptrdiff_t>(done));
}

double GsView::sceneBudget() const
{
    if (m_fixedBudget > 0)
    {
        return static_cast<double>(m_fixedBudget);
    }
    const double rate = m_vertexRate > 0.0 ? m_vertexRate : kDefaultVertexRate;
    return std::max(rate * kSceneBudgetMs, kMinBudgetVertices);
}

bool GsView::render(RhiSurface& surface, double dpr)
{
    if (!ensureDevice())
    {
        return false;
    }
    RhiDevice& rhi = m_device->rhi();
    const std::uint32_t width = std::max(surface.width(), 1u);
    const std::uint32_t height = std::max(surface.height(), 1u);
    const std::uint32_t samples = std::max(surface.sampleCount(), 1u);
    const double wpp = m_worldPerPixel / std::max(dpr, 1.0e-6);  // 每设备像素的世界长度
    const double eyeX = m_center.x;
    const double eyeY = m_center.y;
    // 收集命令时整组判断的 LOD 阈值，与每帧常量里着色器的阈值相同（设备像素换成世界长度）
    GsLod lod;
    if (m_style.lod)
    {
        lod.textHeight = kLodTextPixels * dpr * wpp;
        lod.arcRadius = kLodArcPixels * dpr * wpp;
    }

    // 1. 模型处理累积的变更
    if (m_model)
    {
        m_model->update(*m_device);
    }
    if (m_transient)
    {
        m_transient->update(*m_device);
    }
    if (!ensureTargets(width, height, samples))
    {
        return false;
    }

    // 2. 场景是否作废
    const std::uint64_t modelVersion = m_model ? m_model->version() : 0;
    // 高亮色只在高亮走状态位图（画在场景里）时影响场景底图
    GsViewStyle sceneStyle = m_style;
    if (!m_useBitmapHighlight)
    {
        sceneStyle.highlight = m_sceneStyle.highlight;
    }
    if (modelVersion != m_sceneModelVersion || m_sceneCenter.x != eyeX || m_sceneCenter.y != eyeY
        || m_sceneWorldPerPixel != wpp || m_sceneWidth != width || m_sceneHeight != height || !(m_sceneStyle == sceneStyle))
    {
        invalidateScene();
    }
    if (modelVersion != m_highlightModelVersion)
    {
        m_highlightDirty = true;
        m_viewBitsDirty = true;
    }
    // 从头画场景，或者接着画上一帧没画完的（渐进绘制，第 4.3.10 节）
    const bool redrawScene = !m_sceneValid && !m_sceneInProgress;
    const bool drawScene = redrawScene || m_sceneInProgress;
    m_lastFrameRedrewScene = drawScene;
    readSceneTimings();

    // 每视图的缓冲
    if (m_viewBitsDirty || !m_viewBitsBuffer)
    {
        prepareViewBits();
        m_viewBitsDirty = false;
    }
    std::vector<GsInstanceRecord> blockInstances;
    if (redrawScene)
    {
        m_sceneList.clear();
        if (m_model && m_block)
        {
            m_model->collectBlock(m_block, GiTransform(), m_sceneList, blockInstances);
            m_model->update(*m_device);  // 新编译的块几何要上传
        }
        else if (m_model)
        {
            const double margin = 16.0 * wpp;
            const double halfW = width * 0.5 * wpp + margin;
            const double halfH = height * 0.5 * wpp + margin;
            m_model->collectVisible(DmVector(eyeX - halfW, eyeY - halfH), DmVector(eyeX + halfW, eyeY + halfH), lod,
                                    m_sceneList);
            // 放大后弦高超过半个像素的曲线重新离散（第 4.3.10 节）：这一帧先用旧结果画，好了模型的版本变，场景重画
            m_model->refineVisible(DmVector(eyeX - halfW, eyeY - halfH), DmVector(eyeX + halfW, eyeY + halfH), wpp);
        }
        // 不显示线宽时线段按细线画（每段 2 个顶点，片段只判划线），选中的线另按四边形画在上面；
        // 显示线宽、高亮走状态位图或选中的太多时全部按四边形画
        m_emphasisList.clear();
        m_sceneHairlines = m_model && !m_style.lineWidths && !m_useBitmapHighlight
                           && (!m_selection || m_block || m_model->collectSelected(m_emphasisList, kEmphasisLimit));
        if (!m_sceneHairlines)
        {
            m_emphasisList.clear();
        }

        // 场景的间接参数只在重画场景时上传（只移动光标的帧不传）。缓冲留着复用：大图纸上有几万条命令，
        // 每次新建要向系统要几百 KB 的新内存页，缺页的开销比复制本身大得多
        std::vector<RhiDrawIndirectArgs>& sceneIndirect = m_sceneIndirectData;
        sceneIndirect.clear();
        for (std::size_t i = 0; i < kGsClassCount; ++i)
        {
            for (RhiDrawIndirectArgs a : m_sceneList.commands[i])
            {
                if (m_sceneHairlines && static_cast<GsClass>(i) == GsClass::Segment)
                {
                    // 四边形每段 6 个顶点，细线 2 个
                    a.vertexCount /= 3;
                    a.firstVertex /= 3;
                }
                sceneIndirect.push_back(a);
            }
        }
        sceneIndirect.insert(sceneIndirect.end(), m_sceneList.smallArcs.begin(), m_sceneList.smallArcs.end());
        const auto& emphasis = m_emphasisList.commands[static_cast<std::size_t>(GsClass::Segment)];
        m_emphasisBase = sceneIndirect.size();
        m_emphasisCount = emphasis.size();
        sceneIndirect.insert(sceneIndirect.end(), emphasis.begin(), emphasis.end());
        ensureBuffer(m_sceneIndirect, std::max<std::size_t>(sceneIndirect.size(), 1) * sizeof(RhiDrawIndirectArgs),
                     RhiBufferUsage::Indirect, "gs scene indirect");
        if (m_sceneIndirect && !sceneIndirect.empty())
        {
            rhi.upload(*m_sceneIndirect, 0,
                       std::span<const std::byte>(reinterpret_cast<const std::byte*>(sceneIndirect.data()),
                                                  sceneIndirect.size() * sizeof(RhiDrawIndirectArgs)));
        }

        // 各段的顶点数（渐进绘制按它分批）：细线的线段每段 2 个顶点
        m_chunkCost.assign(m_sceneList.chunks.size(), 0.0);
        auto cost = [](const RhiDrawIndirectArgs& a) {
            return static_cast<double>(a.vertexCount) * static_cast<double>(a.instanceCount);
        };
        for (std::size_t k = 0; k < m_sceneList.chunks.size(); ++k)
        {
            const DrawRange range = chunkRange(m_sceneList, k, k + 1);
            double vertices = 0.0;
            for (std::size_t i = 0; i < kGsClassCount; ++i)
            {
                const double factor = m_sceneHairlines && static_cast<GsClass>(i) == GsClass::Segment ? 1.0 / 3.0 : 1.0;
                for (std::uint32_t j = range.begin[i]; j < range.end[i]; ++j)
                {
                    vertices += cost(m_sceneList.commands[i][j]) * factor;
                }
            }
            for (std::uint32_t j = range.smallArcsBegin; j < range.smallArcsEnd; ++j)
            {
                vertices += cost(m_sceneList.smallArcs[j]);
            }
            for (std::uint32_t j = range.imagesBegin; j < range.imagesEnd; ++j)
            {
                vertices += cost(m_sceneList.images[j].args);
            }
            m_chunkCost[k] = vertices;
        }
        m_nextChunk = 0;
        m_sceneInProgress = true;
        m_sceneModelVersion = modelVersion;
        m_sceneCenter = DmVector(eyeX, eyeY);
        m_sceneWorldPerPixel = wpp;
        m_sceneWidth = width;
        m_sceneHeight = height;
        m_sceneStyle = m_style;
    }
    if (m_highlightDirty)
    {
        YICAD_SCOPED_TIMER(yicad::counters::regenHighlight());
        m_highlightList.clear();
        if (m_model && !m_useBitmapHighlight && !m_highlighted.empty())
        {
            // 选中优先：已选中的按选中色画在场景里，不再高亮
            m_model->collectEntities(m_highlighted, m_highlightList, true);
        }
        m_highlightModelVersion = modelVersion;
        m_highlightDirty = false;
    }
    m_transientList.clear();
    if (m_transient)
    {
        m_transient->collectVisible(DmVector(-1.0e300, -1.0e300), DmVector(1.0e300, 1.0e300), lod, m_transientList);
    }

    // 叠加通道的间接参数：高亮、临时模型依次排，每帧上传
    std::vector<RhiDrawIndirectArgs>& indirect = m_indirectData;
    indirect.clear();
    auto append = [&indirect](const GsDrawList& list) {
        const std::size_t base = indirect.size();
        for (const auto& c : list.commands)
        {
            indirect.insert(indirect.end(), c.begin(), c.end());
        }
        indirect.insert(indirect.end(), list.smallArcs.begin(), list.smallArcs.end());
        return base;
    };
    const std::size_t highlightBase = append(m_highlightList);
    const std::size_t transientBase = append(m_transientList);
    ensureBuffer(m_indirect, std::max<std::size_t>(indirect.size(), 1) * sizeof(RhiDrawIndirectArgs),
                 RhiBufferUsage::Indirect, "gs indirect");
    if (!indirect.empty())
    {
        rhi.upload(*m_indirect, 0,
                   std::span<const std::byte>(reinterpret_cast<const std::byte*>(indirect.data()),
                                              indirect.size() * sizeof(RhiDrawIndirectArgs)));
    }

    // 每帧常量：四个通道
    const std::size_t stride = (std::max<std::size_t>(sizeof(GsFrameConstants), rhi.caps().uniformBufferOffsetAlignment)
                                + 255) / 256 * 256;
    ensureBuffer(m_frameBuffer, stride * kFrameSlots, RhiBufferUsage::Uniform, "gs frame");
    std::vector<std::byte> frameBytes(stride * kFrameSlots);
    auto frame = [&](std::uint32_t slot, std::uint32_t pass, bool selection, bool bits) {
        GsFrameConstants f;
        const glm::mat4& correction = rhi.clipSpaceCorrection();
        std::memcpy(f.clipCorrection.data(), &correction[0][0], sizeof(float) * 16);
        f.viewport = {static_cast<float>(width), static_cast<float>(height), static_cast<float>(wpp), 1.0f};
        const float hx = static_cast<float>(eyeX);
        const float hy = static_cast<float>(eyeY);
        f.eye = {hx, hy, static_cast<float>(eyeX - hx), static_cast<float>(eyeY - hy)};
        f.background = toFloat4(m_style.background);
        f.selectedColor = toFloat4(m_style.selected);
        f.highlightColor = toFloat4(m_style.highlight);
        // 以像素计的量都乘设备像素比（第 4.6 节）；抗锯齿外扩（viewport.w）按设备像素
        const float px = static_cast<float>(dpr);
        f.lineStyle = {m_style.lineWidths ? kLineWidthPerCode * px : 0.0f, kEmphasisPixels * px, kPointPixels * px,
                       static_cast<float>(16.0 * wpp)};
        f.mode = {pass, bits ? 1u : 0u, selection ? 1u : 0u, samples};
        // LTSCALE 取场景的模型（文档的变量），预览等叠加的临时模型也按它画
        f.strokeStyle = {static_cast<float>(m_model ? m_model->globalLineTypeScale() : 1.0), kMinDashPeriodPixels * px, px,
                         0.0f};
        const double spacing = m_style.gridSpacing;
        const bool grid = m_style.gridOn && spacing > 0.0 && std::isfinite(spacing);
        f.grid = {static_cast<float>(spacing), static_cast<float>(spacing * 5.0), grid ? 1.0f : 0.0f, 0.0f};
        if (grid)
        {
            f.gridOffset = {static_cast<float>(positiveMod(eyeX, spacing)), static_cast<float>(positiveMod(eyeY, spacing)),
                            static_cast<float>(positiveMod(eyeX, spacing * 5.0)),
                            static_cast<float>(positiveMod(eyeY, spacing * 5.0))};
        }
        f.gridColor = toFloat4(m_style.grid);
        f.metaGridColor = toFloat4(m_style.metaGrid);
        if (m_style.lod)
        {
            f.lod = {kLodTextPixels * px, kLodHatchPixels * px, kLodObjectPixels * px, kLodArcPixels * px};
        }
        std::memcpy(frameBytes.data() + stride * slot, &f, sizeof(f));
    };
    frame(kSlotScene, m_block ? kGsPassPlain : kGsPassScene, m_selection, true);
    frame(kSlotHighlight, kGsPassHighlight, false, true);
    frame(kSlotTransient, kGsPassPlain, false, false);
    frame(kSlotPlain, kGsPassPlain, false, false);
    rhi.upload(*m_frameBuffer, 0, frameBytes);

    // 分块偏移：模型各分块原点相对视点（块缩略图只有一个原点为 (0, 0) 的"分块"）
    static const std::vector<DmVector> kOrigin{DmVector(0.0, 0.0)};
    uploadCellOffsets(m_model && !m_block ? m_model->cellOrigins() : kOrigin, m_cellOffsets, m_cellOffsetsUploaded, eyeX,
                      eyeY);
    if (m_transient)
    {
        uploadCellOffsets(m_transient->cellOrigins(), m_transientCellOffsets, m_transientCellOffsetsUploaded, eyeX, eyeY);
    }
    if (redrawScene && m_block)
    {
        m_blockInstances = std::move(blockInstances);
        ensureBuffer(m_blockInstanceBuffer, std::max<std::size_t>(m_blockInstances.size(), 1) * sizeof(GsInstanceRecord),
                     RhiBufferUsage::Vertex, "gs block instances");
        if (!m_blockInstances.empty())
        {
            rhi.upload(*m_blockInstanceBuffer, 0,
                       std::span<const std::byte>(reinterpret_cast<const std::byte*>(m_blockInstances.data()),
                                                  m_blockInstances.size() * sizeof(GsInstanceRecord)));
        }
    }

    // 叠加层：夹点在前（画在下面），画布的叠加层在后
    std::vector<GsOverlayVertex> overlay;
    {
        GsOverlay grips;
        if (!m_grips.empty())
        {
            const double half = kGripSize * 0.5;
            for (const DmVector& g : m_grips)
            {
                const double px = (g.x - eyeX) / m_worldPerPixel + width / dpr * 0.5;
                const double py = height / dpr * 0.5 - (g.y - eyeY) / m_worldPerPixel;
                grips.fillRect(px - half, py - half, px + half, py + half, QColor(0, 0, 255));
            }
        }
        overlay = grips.vertices();
        overlay.insert(overlay.end(), m_overlay.vertices().begin(), m_overlay.vertices().end());
        for (GsOverlayVertex& v : overlay)
        {
            v.x = static_cast<float>(v.x * dpr);
            v.y = static_cast<float>(v.y * dpr);
        }
    }
    if (!overlay.empty())
    {
        ensureBuffer(m_overlayVertices, overlay.size() * sizeof(GsOverlayVertex), RhiBufferUsage::Vertex, "gs overlay");
        rhi.upload(*m_overlayVertices, 0,
                   std::span<const std::byte>(reinterpret_cast<const std::byte*>(overlay.data()),
                                              overlay.size() * sizeof(GsOverlayVertex)));
    }

    // 绑定组（引用的缓冲在上面可能换过，每帧按当前的建）
    RhiBindGroupPtr sceneGroup = frameGroup(kSlotScene, m_cellOffsets);
    RhiBindGroupPtr highlightGroup = frameGroup(kSlotHighlight, m_cellOffsets);
    RhiBindGroupPtr transientGroup = m_transient ? frameGroup(kSlotTransient, m_transientCellOffsets) : nullptr;
    RhiBindGroupPtr plainGroup = frameGroup(kSlotPlain, m_cellOffsets);
    if (!sceneGroup || !highlightGroup || !plainGroup)
    {
        return false;
    }

    // 3. 录制
    RhiCommandList& commands = rhi.beginFrame(surface);
    if (drawScene)
    {
        YICAD_SCOPED_TIMER(yicad::counters::scene());
        // 这一帧画哪几段：从上次停下的地方起，至少一段，累计的顶点数不超过预算（整张表没分段时一次画完）
        DrawRange range = wholeRange(m_sceneList);
        double drawnVertices = 0.0;
        std::size_t last = m_nextChunk;
        const std::size_t chunkCount = m_sceneList.chunks.size();
        if (chunkCount > 0)
        {
            const double budget = sceneBudget();
            while (last < chunkCount && (last == m_nextChunk || drawnVertices + m_chunkCost[last] <= budget))
            {
                drawnVertices += m_chunkCost[last];
                ++last;
            }
            range = chunkRange(m_sceneList, m_nextChunk, last);
        }
        else
        {
            drawnVertices = std::accumulate(m_chunkCost.begin(), m_chunkCost.end(), 0.0);
        }

        RhiRenderPassDesc pass;
        pass.target = m_sceneTarget.get();
        RhiColorAttachmentOps ops;
        // 接着画时保留已经画好的颜色与深度（深度表达绘图次序，后面几段照样按次序叠上去）
        ops.load = redrawScene ? RhiLoadOp::Clear : RhiLoadOp::Load;
        ops.clearColor = {static_cast<float>(m_style.background.redF()), static_cast<float>(m_style.background.greenF()),
                          static_cast<float>(m_style.background.blueF()), 1.0f};
        ops.resolveTarget = m_targetSamples > 1 ? m_sceneResolve.get() : nullptr;
        pass.colorOps = {ops};
        pass.depthLoad = redrawScene ? RhiLoadOp::Clear : RhiLoadOp::Load;
        pass.depthStore = RhiStoreOp::Store;
        // 场景通道的 GPU 耗时：前后各写一个时间戳，几帧后读出，估每毫秒能画的顶点数（第 4.3.10 节）
        std::uint32_t timestampSlot = kTimestampSlots;
        if (rhi.caps().timestampQueries && m_pendingTimings.size() < kTimestampSlots)
        {
            if (!m_timestamps)
            {
                RhiQuerySetDesc desc;
                desc.count = kTimestampSlots * 2;
                desc.debugName = "gs scene timestamps";
                m_timestamps = rhi.createQuerySet(desc);
            }
            if (m_timestamps)
            {
                timestampSlot = m_nextTimestampSlot;
                m_nextTimestampSlot = (m_nextTimestampSlot + 1) % kTimestampSlots;
                commands.writeTimestamp(*m_timestamps, timestampSlot * 2);
            }
        }
        commands.beginRenderPass(pass);
        if (redrawScene && m_style.gridOn && m_style.gridSpacing > 0.0)
        {
            const RhiPipelinePtr& grid = m_device->pipeline(GsProgram::Grid, GsPipelineVariant::Blend, samples);
            if (grid)
            {
                commands.setPipeline(*grid);
                commands.setBindGroup(0, *plainGroup);
                commands.draw(3, 1, 0, 0);
            }
        }
        if (m_model && m_model->isReady())
        {
            const RhiBuffer* instances = m_block ? m_blockInstanceBuffer.get() : m_model->instanceBuffer();
            if (instances)
            {
                drawList(commands, m_sceneList, range, *m_sceneIndirect, 0, *m_model, *sceneGroup, *instances, samples, true,
                         m_sceneHairlines);
                // 选中的线：按四边形加宽、换色，深度前移，画在细线之上（第一批就画，选中的不等）
                const RhiPipelinePtr& quads = m_device->pipeline(GsProgram::Segment, GsPipelineVariant::Scene, samples);
                const RhiBindGroup* segments = m_model->geometryGroup(GsClass::Segment);
                if (redrawScene && m_emphasisCount > 0 && quads && segments)
                {
                    const RhiVertexBufferBinding binding{instances, 0};
                    commands.setPipeline(*quads);
                    commands.setBindGroup(0, *sceneGroup);
                    commands.setBindGroup(1, *m_model->modelGroup());
                    commands.setBindGroup(2, *segments);
                    commands.setVertexBuffers(0, std::span<const RhiVertexBufferBinding>(&binding, 1));
                    commands.drawIndirect(*m_sceneIndirect, m_emphasisBase * sizeof(RhiDrawIndirectArgs),
                                          static_cast<std::uint32_t>(m_emphasisCount));
                }
            }
        }
        commands.endRenderPass();
        if (timestampSlot < kTimestampSlots)
        {
            commands.writeTimestamp(*m_timestamps, timestampSlot * 2 + 1);
            m_pendingTimings.push_back({timestampSlot, drawnVertices});
        }
        m_nextChunk = last;
        if (chunkCount == 0 || last >= chunkCount)
        {
            m_sceneInProgress = false;
            m_sceneValid = true;
        }
    }

    RhiRenderPassDesc pass;
    pass.target = nullptr;
    RhiColorAttachmentOps ops;
    ops.clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    pass.colorOps = {ops};
    commands.beginRenderPass(pass);
    const RhiPipelinePtr& blit = m_device->pipeline(GsProgram::Blit, GsPipelineVariant::Opaque, samples);
    if (blit && m_blitGroup)
    {
        commands.setPipeline(*blit);
        commands.setBindGroup(0, *plainGroup);
        commands.setBindGroup(3, *m_blitGroup);
        commands.draw(3, 1, 0, 0);
    }
    if (m_model && m_model->isReady() && !m_highlightList.empty() && m_model->instanceBuffer())
    {
        drawList(commands, m_highlightList, wholeRange(m_highlightList), *m_indirect, highlightBase, *m_model, *highlightGroup,
                 *m_model->instanceBuffer(), samples, false, false);
    }
    if (m_transient && m_transient->isReady() && transientGroup && m_transient->instanceBuffer())
    {
        drawList(commands, m_transientList, wholeRange(m_transientList), *m_indirect, transientBase, *m_transient,
                 *transientGroup, *m_transient->instanceBuffer(), samples, false, false);
    }
    if (!overlay.empty() && m_overlayVertices)
    {
        const RhiPipelinePtr& pipeline = m_device->pipeline(GsProgram::Overlay, GsPipelineVariant::Blend, samples);
        if (pipeline)
        {
            commands.setPipeline(*pipeline);
            commands.setBindGroup(0, *plainGroup);
            const RhiVertexBufferBinding binding{m_overlayVertices.get(), 0};
            commands.setVertexBuffers(0, std::span<const RhiVertexBufferBinding>(&binding, 1));
            commands.draw(static_cast<std::uint32_t>(overlay.size()), 1, 0, 0);
        }
    }
    commands.endRenderPass();
    rhi.endFrame();
    return true;
}
