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

/// @file GsDevice.cpp
/// @brief GsDevice 实现

#include "GsDevice.h"

#include <algorithm>
#include <cstdlib>

#include <QCoreApplication>
#include <QDir>

#include "GLRhiDevice.h"
#include "GsShaders.h"
#include "GsTypes.h"
#include "RhiShaderLibrary.h"
#include "YiCadLog.h"

#define GS_WARNING YICAD_LOG(yicad::log::render(), yicad::LogLevel::Warning) << "GS："

namespace
{

std::weak_ptr<GsDevice>& instance()
{
    static std::weak_ptr<GsDevice> device;
    return device;
}

/// @brief 着色器目录：YICAD_SHADER_DIR，否则 <程序目录>/resources/shaders（cmake --install 复制）
std::filesystem::path defaultShaderDirectory()
{
    const QString env = qEnvironmentVariable("YICAD_SHADER_DIR");
    if (!env.isEmpty())
    {
        return std::filesystem::path(env.toStdWString());
    }
    const QString dir = QCoreApplication::applicationDirPath() + QStringLiteral("/resources/shaders");
    return std::filesystem::path(QDir::toNativeSeparators(dir).toStdWString());
}

const GsShaders::Program& programInfo(GsProgram program)
{
    switch (program)
    {
    case GsProgram::Segment: return GsShaders::gs_segment;
    case GsProgram::Hairline: return GsShaders::gs_hairline;
    case GsProgram::InfiniteLine: return GsShaders::gs_xline;
    case GsProgram::Arc: return GsShaders::gs_arc;
    case GsProgram::Fill: return GsShaders::gs_fill;
    case GsProgram::Point: return GsShaders::gs_point;
    case GsProgram::Image: return GsShaders::gs_image;
    case GsProgram::Grid: return GsShaders::gs_grid;
    case GsProgram::Blit: return GsShaders::gs_blit;
    case GsProgram::Overlay: return GsShaders::gs_overlay;
    case GsProgram::Count: break;
    }
    return GsShaders::gs_segment;
}

/// @brief 着色器清单里的布局 -> GsLayout（按条目相同判断）
const std::span<const RhiBindGroupLayoutEntry> layoutEntries(GsLayout layout)
{
    switch (layout)
    {
    case GsLayout::Frame: return GsShaders::Frame;
    case GsLayout::Model: return GsShaders::Model;
    case GsLayout::Geometry: return GsShaders::Geometry;
    case GsLayout::Image: return GsShaders::Image;
    case GsLayout::Count: break;
    }
    return {};
}

/// @brief 几何管线的实例记录（GsInstanceRecord）作为步进为 1 的实例属性
RhiVertexBufferLayout instanceLayout()
{
    RhiVertexBufferLayout layout;
    layout.stride = sizeof(GsInstanceRecord);
    layout.stepMode = RhiVertexStepMode::Instance;
    layout.attributes = {{0, RhiVertexFormat::Float4, 0},
                         {1, RhiVertexFormat::Float4, 16},
                         {2, RhiVertexFormat::Uint4, 32},
                         {3, RhiVertexFormat::Uint4, 48}};
    return layout;
}

/// @brief 叠加层动态批次的顶点（GsOverlayVertex）
RhiVertexBufferLayout overlayLayout()
{
    RhiVertexBufferLayout layout;
    layout.stride = sizeof(GsOverlayVertex);
    layout.stepMode = RhiVertexStepMode::Vertex;
    layout.attributes = {{0, RhiVertexFormat::Float2, 0},
                         {1, RhiVertexFormat::UByte4Norm, 8},
                         {2, RhiVertexFormat::Float, 12}};
    return layout;
}

}  // namespace

std::shared_ptr<GsDevice> GsDevice::acquire()
{
    std::shared_ptr<GsDevice> device = instance().lock();
    if (device)
    {
        return device;
    }
    device.reset(new GsDevice());
    if (!device->initialize())
    {
        return nullptr;
    }
    instance() = device;
    return device;
}

GsDevice::~GsDevice()
{
    m_images.clear();
    m_pipelines.clear();
    m_programs = {};
    m_layouts = {};
    m_imageSampler.reset();
    m_blitSampler.reset();
    if (m_rhi)
    {
        m_rhi->waitIdle();
    }
}

bool GsDevice::initialize()
{
    std::unique_ptr<GLRhiDevice> rhi = GLRhiDevice::create();
    if (!rhi)
    {
        GS_WARNING << "建不成 RHI 设备";
        return false;
    }
    m_rhi = std::move(rhi);
    m_shaderDirectory = defaultShaderDirectory();

    for (std::size_t i = 0; i < m_layouts.size(); ++i)
    {
        RhiBindGroupLayoutDesc desc;
        desc.entries = layoutEntries(static_cast<GsLayout>(i));
        desc.debugName = "gs layout";
        m_layouts[i] = m_rhi->createBindGroupLayout(desc);
        if (!m_layouts[i])
        {
            GS_WARNING << "建不成绑定组布局 " << i;
            return false;
        }
    }

    for (std::size_t i = 0; i < m_programs.size(); ++i)
    {
        const GsShaders::Program& info = programInfo(static_cast<GsProgram>(i));
        RhiProgramShaders shaders = rhiLoadProgram(*m_rhi, m_shaderDirectory, info.name);
        if (!shaders.isValid())
        {
            GS_WARNING << "着色器程序 " << std::string(info.name) << " 读不进来";
            return false;
        }
        Program& program = m_programs[i];
        program.vertex = shaders.vertex;
        program.fragment = shaders.fragment;
        // 按组号放共用的布局：同名布局的条目相同，绑定组可以在程序之间共用
        for (const std::span<const RhiBindGroupLayoutEntry>& entries : info.bindGroups)
        {
            RhiBindGroupLayoutPtr layout;
            for (std::size_t l = 0; l < m_layouts.size(); ++l)
            {
                if (!entries.empty() && entries.data() == layoutEntries(static_cast<GsLayout>(l)).data())
                {
                    layout = m_layouts[l];
                }
            }
            program.layouts.push_back(layout);
        }
    }

    RhiSamplerDesc image;
    image.minFilter = RhiFilter::Linear;
    image.magFilter = RhiFilter::Linear;
    image.mipmapMode = RhiMipmapMode::Linear;
    m_imageSampler = m_rhi->createSampler(image);
    RhiSamplerDesc blit;
    blit.minFilter = RhiFilter::Nearest;
    blit.magFilter = RhiFilter::Nearest;
    m_blitSampler = m_rhi->createSampler(blit);
    return m_imageSampler && m_blitSampler;
}

const RhiPipelinePtr& GsDevice::pipeline(GsProgram program, GsPipelineVariant variant, std::uint32_t sampleCount)
{
    const std::uint32_t key = static_cast<std::uint32_t>(program) | (static_cast<std::uint32_t>(variant) << 8)
                            | (sampleCount << 16);
    auto it = m_pipelines.find(key);
    if (it != m_pipelines.end())
    {
        return it->second;
    }
    const Program& p = m_programs[static_cast<std::size_t>(program)];
    RhiPipelineDesc desc;
    desc.vertexShader = p.vertex;
    desc.fragmentShader = p.fragment;
    desc.bindGroupLayouts = p.layouts;
    desc.topology = program == GsProgram::Hairline ? RhiPrimitiveTopology::LineList : RhiPrimitiveTopology::TriangleList;
    desc.sampleCount = sampleCount;
    desc.debugName = std::string(programInfo(program).name);
    if (program == GsProgram::Overlay)
    {
        desc.vertexBuffers = {overlayLayout()};
    }
    else if (program != GsProgram::Grid && program != GsProgram::Blit)
    {
        desc.vertexBuffers = {instanceLayout()};
    }

    RhiColorTargetState color;
    color.format = RhiFormat::RGBA8Unorm;
    desc.depthStencil.format = RhiFormat::Depth24Stencil8;
    switch (variant)
    {
    case GsPipelineVariant::Scene:
        desc.depthStencil.depthTestEnabled = true;
        desc.depthStencil.depthWriteEnabled = true;
        desc.depthStencil.depthCompare = RhiCompareOp::LessOrEqual;
        break;
    case GsPipelineVariant::Overlay:
        break;
    case GsPipelineVariant::Blend:
        color.blendEnabled = true;
        color.color = {RhiBlendFactor::SrcAlpha, RhiBlendFactor::OneMinusSrcAlpha, RhiBlendOp::Add};
        color.alpha = {RhiBlendFactor::One, RhiBlendFactor::OneMinusSrcAlpha, RhiBlendOp::Add};
        break;
    case GsPipelineVariant::Opaque:
        break;
    }
    desc.colorTargets = {color};
    RhiPipelinePtr pipeline = m_rhi->createPipeline(desc);
    if (!pipeline)
    {
        GS_WARNING << "建不成管线 " << desc.debugName;
    }
    return m_pipelines.emplace(key, std::move(pipeline)).first->second;
}

RhiTexturePtr GsDevice::imageTexture(const QString& key, const std::function<QImage()>& load)
{
    auto it = m_images.find(key);
    if (it != m_images.end())
    {
        if (RhiTexturePtr texture = it->second.lock())
        {
            return texture;
        }
    }
    // 第 0 行是图片的底部（采样坐标 v = 0 在图片原点一侧），与原先旧渲染器的 mirrored() 相同
    ++m_imageLoads;
    QImage image = load().convertToFormat(QImage::Format_RGBA8888).mirrored();
    if (image.isNull() || image.width() == 0 || image.height() == 0)
    {
        image = QImage(1, 1, QImage::Format_RGBA8888);
        image.fill(Qt::white);
    }
    const std::uint32_t maxSize = std::max(m_rhi->caps().maxTextureSize, 1u);
    if (static_cast<std::uint32_t>(image.width()) > maxSize || static_cast<std::uint32_t>(image.height()) > maxSize)
    {
        // 超过最大纹理尺寸：等比缩到放得下（方案说的切片留待以后）
        image = image.scaled(static_cast<int>(maxSize), static_cast<int>(maxSize), Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
    }
    std::uint32_t levels = 1;
    for (std::uint32_t s = static_cast<std::uint32_t>(std::max(image.width(), image.height())); s > 1; s >>= 1)
    {
        ++levels;
    }
    RhiTextureDesc desc;
    desc.width = static_cast<std::uint32_t>(image.width());
    desc.height = static_cast<std::uint32_t>(image.height());
    desc.format = RhiFormat::RGBA8Unorm;
    desc.mipLevels = levels;
    desc.usage = RhiTextureUsage::Sampled;
    desc.debugName = "gs image";
    RhiTexturePtr texture = m_rhi->createTexture(desc);
    if (!texture)
    {
        GS_WARNING << "建不成图片纹理";
        return nullptr;
    }
    QImage level = image;
    for (std::uint32_t l = 0; l < levels; ++l)
    {
        if (l > 0)
        {
            level = level.scaled(std::max(level.width() / 2, 1), std::max(level.height() / 2, 1), Qt::IgnoreAspectRatio,
                                 Qt::SmoothTransformation);
        }
        RhiTextureRegion region;
        region.mipLevel = l;
        region.width = static_cast<std::uint32_t>(level.width());
        region.height = static_cast<std::uint32_t>(level.height());
        // QImage 的行可能有填充，逐行拷成紧密排列
        std::vector<std::byte> pixels(static_cast<std::size_t>(level.width()) * level.height() * 4);
        for (int y = 0; y < level.height(); ++y)
        {
            std::memcpy(pixels.data() + static_cast<std::size_t>(y) * level.width() * 4, level.constScanLine(y),
                        static_cast<std::size_t>(level.width()) * 4);
        }
        m_rhi->upload(*texture, region, pixels);
    }
    m_images[key] = texture;
    return texture;
}
