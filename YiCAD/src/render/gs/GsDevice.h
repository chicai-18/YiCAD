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

/// @file GsDevice.h
/// @brief 图形系统的设备：每个进程一个，持有 RHI 设备、全部管线与图片纹理缓存（RENDER_PLAN.md 第 4.3.1 节）

#ifndef GSDEVICE_H
#define GSDEVICE_H

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <unordered_map>

#include <QImage>
#include <QString>

#include "RhiDevice.h"

/// @brief 着色器程序
enum class GsProgram : std::uint8_t
{
    Segment,
    Hairline,      ///< 线段按线图元画（不显示线宽时的场景通道）
    InfiniteLine,
    Arc,
    ArcSmall,      ///< 小圆弧：每条记录 6 个顶点（第 4.3.10 节）
    Fill,
    Point,
    Image,
    Grid,
    Blit,
    Overlay,
    Count
};

/// @brief 管线的变体：同一程序在不同通道里的状态
enum class GsPipelineVariant : std::uint8_t
{
    Scene,     ///< 场景通道：深度测试与写入（绘图次序）
    Overlay,   ///< 叠加通道里的几何（高亮、预览、块缩略图）：不测深度
    Blend,     ///< 网格与叠加层的动态批次：不测深度，alpha 混合
    Opaque,    ///< 场景底图贴图：不测深度，不混合
};

/// @brief 绑定组布局（与着色器清单 shaders.json 的同名布局一一对应）
enum class GsLayout : std::uint8_t
{
    Frame,
    Model,
    Geometry,
    Image,
    Count
};

/// @brief 图形系统的设备，见文件说明
/// @details 第一个视图在自己的 GL 上下文里初始化时经 acquire() 建立，视图与已上传过数据的模型共同持有；
///          全部持有者释放后销毁。着色器从 <程序目录>/resources/shaders 读，环境变量 YICAD_SHADER_DIR 可以另指（测试用）
class GsDevice : public std::enable_shared_from_this<GsDevice>
{
public:
    /// @brief 取进程里的设备，没有就建；建不成（驱动、着色器）时返回空并记日志
    static std::shared_ptr<GsDevice> acquire();

    ~GsDevice();
    GsDevice(const GsDevice&) = delete;
    GsDevice& operator=(const GsDevice&) = delete;

    RhiDevice& rhi() { return *m_rhi; }

    /// @brief 绑定组布局
    const RhiBindGroupLayoutPtr& layout(GsLayout layout) const { return m_layouts[static_cast<std::size_t>(layout)]; }

    /// @brief 管线：按程序、变体与采样数惰性创建并缓存；颜色目标为 RGBA8，深度为 D24S8
    const RhiPipelinePtr& pipeline(GsProgram program, GsPipelineVariant variant, std::uint32_t sampleCount);

    /// @brief 图片的采样器（三线性、边缘截断）
    const RhiSamplerPtr& imageSampler() const { return m_imageSampler; }

    /// @brief 场景底图贴图的采样器（最近点）
    const RhiSamplerPtr& blitSampler() const { return m_blitSampler; }

    /// @brief 图片纹理：按来源缓存，全进程共享；没有时调 load 解码、生成 mipmap 并上传。用的人都放掉后释放
    RhiTexturePtr imageTexture(const QString& key, const std::function<QImage()>& load);

    /// @brief 着色器所在的目录
    const std::filesystem::path& shaderDirectory() const { return m_shaderDirectory; }

    /// @brief 解码过的图片数（imageTexture 没在缓存里找到而调用 load 的次数，测试用）
    std::uint64_t imageLoads() const { return m_imageLoads; }

private:
    GsDevice() = default;
    bool initialize();

    std::unique_ptr<RhiDevice> m_rhi;
    std::filesystem::path m_shaderDirectory;
    std::array<RhiBindGroupLayoutPtr, static_cast<std::size_t>(GsLayout::Count)> m_layouts;
    struct Program
    {
        RhiShaderPtr vertex;
        RhiShaderPtr fragment;
        std::vector<RhiBindGroupLayoutPtr> layouts;
    };
    std::array<Program, static_cast<std::size_t>(GsProgram::Count)> m_programs;
    std::map<std::uint32_t, RhiPipelinePtr> m_pipelines;  ///< 程序 | 变体 << 8 | 采样数 << 16
    RhiSamplerPtr m_imageSampler;
    RhiSamplerPtr m_blitSampler;
    std::unordered_map<QString, std::weak_ptr<RhiTexture>> m_images;
    std::uint64_t m_imageLoads = 0;
};

#endif // GSDEVICE_H
