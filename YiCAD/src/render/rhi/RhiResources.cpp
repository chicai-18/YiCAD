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

/// @file RhiResources.cpp
/// @brief RHI 资源基类中与后端无关的部分

#include "RhiResources.h"

#include <algorithm>

RhiBindGroupLayout::RhiBindGroupLayout(std::span<const RhiBindGroupLayoutEntry> entries)
    : m_entries(entries.begin(), entries.end())
{
    std::sort(m_entries.begin(), m_entries.end(),
        [](const RhiBindGroupLayoutEntry& a, const RhiBindGroupLayoutEntry& b) { return a.binding < b.binding; });
}

bool RhiBindGroupLayout::isCompatible(const RhiBindGroupLayout& other) const
{
    if (this == &other)
    {
        return true;
    }
    return std::equal(m_entries.begin(), m_entries.end(), other.m_entries.begin(), other.m_entries.end(),
        [](const RhiBindGroupLayoutEntry& a, const RhiBindGroupLayoutEntry& b)
        {
            return a.binding == b.binding && a.type == b.type && a.stages == b.stages
                && a.hasDynamicOffset == b.hasDynamicOffset;
        });
}

namespace
{
/// @brief 渲染目标的第一个附件，尺寸与采样数以它为准
const RhiTexture* firstAttachment(const RhiRenderTargetDesc& desc)
{
    if (!desc.colorAttachments.empty() && desc.colorAttachments.front())
    {
        return desc.colorAttachments.front().get();
    }
    return desc.depthStencil.get();
}
}  // namespace

std::uint32_t RhiRenderTarget::width() const
{
    const RhiTexture* texture = firstAttachment(m_desc);
    return texture ? texture->width() : 0;
}

std::uint32_t RhiRenderTarget::height() const
{
    const RhiTexture* texture = firstAttachment(m_desc);
    return texture ? texture->height() : 0;
}

std::uint32_t RhiRenderTarget::sampleCount() const
{
    const RhiTexture* texture = firstAttachment(m_desc);
    return texture ? texture->sampleCount() : 1;
}
