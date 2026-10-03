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

/// @file GsArena.cpp
/// @brief GsRangeAllocator 与 GsArena 实现

#include "GsArena.h"

#include <algorithm>
#include <utility>

#include "YiCadLog.h"

namespace
{
constexpr std::uint32_t kMinCapacity = 1024;
}

// ---------------------------------------------------------------------------
// GsRangeAllocator
// ---------------------------------------------------------------------------

std::optional<std::uint32_t> GsRangeAllocator::allocate(std::uint32_t count)
{
    if (count == 0)
    {
        return 0u;
    }
    auto bySize = m_freeBySize.lower_bound(count);
    if (bySize == m_freeBySize.end())
    {
        return std::nullopt;
    }
    const std::uint32_t offset = bySize->second;
    const std::uint32_t size = bySize->first;
    eraseFree(m_freeByOffset.find(offset));
    if (size > count)
    {
        insertFree(offset + count, size - count);
    }
    m_used += count;
    return offset;
}

void GsRangeAllocator::free(std::uint32_t offset, std::uint32_t count)
{
    if (count == 0)
    {
        return;
    }
    m_used -= std::min(m_used, count);
    std::uint32_t start = offset;
    std::uint32_t size = count;
    // 与后面的空闲段合并
    auto next = m_freeByOffset.lower_bound(offset);
    if (next != m_freeByOffset.end() && next->first == offset + count)
    {
        size += next->second;
        eraseFree(next);
    }
    // 与前面的空闲段合并
    auto after = m_freeByOffset.lower_bound(offset);
    if (after != m_freeByOffset.begin())
    {
        auto prev = std::prev(after);
        if (prev->first + prev->second == offset)
        {
            start = prev->first;
            size += prev->second;
            eraseFree(prev);
        }
    }
    insertFree(start, size);
}

void GsRangeAllocator::grow(std::uint32_t capacity)
{
    if (capacity <= m_capacity)
    {
        return;
    }
    const std::uint32_t oldCapacity = m_capacity;
    m_capacity = capacity;
    // 新增部分作为一段"已用"再释放，顺带与末尾的空闲段合并
    m_used += capacity - oldCapacity;
    free(oldCapacity, capacity - oldCapacity);
}

void GsRangeAllocator::reset()
{
    m_freeByOffset.clear();
    m_freeBySize.clear();
    m_used = 0;
    if (m_capacity > 0)
    {
        insertFree(0, m_capacity);
    }
}

void GsRangeAllocator::insertFree(std::uint32_t offset, std::uint32_t count)
{
    m_freeByOffset.emplace(offset, count);
    m_freeBySize.emplace(count, offset);
}

void GsRangeAllocator::eraseFree(std::map<std::uint32_t, std::uint32_t>::iterator it)
{
    auto range = m_freeBySize.equal_range(it->second);
    for (auto s = range.first; s != range.second; ++s)
    {
        if (s->second == it->first)
        {
            m_freeBySize.erase(s);
            break;
        }
    }
    m_freeByOffset.erase(it);
}

// ---------------------------------------------------------------------------
// GsArena
// ---------------------------------------------------------------------------

GsArena::GsArena(std::string name, std::uint32_t elementSize, RhiBufferUsage usage, RhiFormat texelFormat)
    : m_name(std::move(name))
    , m_elementSize(elementSize)
    , m_usage(usage | RhiBufferUsage::CopySrc | RhiBufferUsage::CopyDst)
    , m_texelFormat(texelFormat)
{
}

GsRange GsArena::allocate(std::uint32_t count)
{
    if (count == 0)
    {
        return {};
    }
    std::optional<std::uint32_t> offset = m_allocator.allocate(count);
    while (!offset)
    {
        const std::uint32_t capacity = std::max({m_allocator.capacity() * 2, m_allocator.used() + count * 2, kMinCapacity});
        m_allocator.grow(capacity);
        offset = m_allocator.allocate(count);
    }
    return {*offset, count};
}

void GsArena::free(GsRange& range)
{
    m_allocator.free(range.offset, range.count);
    range = {};
}

void GsArena::reset()
{
    m_allocator.reset();
}

void GsArena::reserve(std::uint32_t capacity)
{
    m_allocator.grow(std::max(capacity, kMinCapacity));
}

bool GsArena::needsCopy() const
{
    return m_buffer && m_buffer->size() < static_cast<std::size_t>(capacity()) * m_elementSize;
}

bool GsArena::sync(RhiDevice& device, RhiCommandList* copies)
{
    m_retired.reset();
    if (capacity() == 0)
    {
        reserve(kMinCapacity);
    }
    const std::size_t bytes = static_cast<std::size_t>(capacity()) * m_elementSize;
    if (m_buffer && m_buffer->size() >= bytes)
    {
        return false;
    }
    RhiBufferDesc desc;
    desc.size = bytes;
    desc.usage = m_usage;
    desc.debugName = m_name;
    RhiBufferPtr buffer = device.createBuffer(desc);
    if (!buffer)
    {
        YICAD_LOG(yicad::log::render(), yicad::LogLevel::Warning) << "GS 数据区 " << m_name << " 建不成 " << bytes << " 字节的缓冲";
        return false;
    }
    if (m_buffer && copies)
    {
        copies->copyBuffer(*m_buffer, 0, *buffer, 0, m_buffer->size());
        m_retired = m_buffer;
    }
    m_buffer = std::move(buffer);
    ++m_generation;
    return true;
}

void GsArena::write(RhiDevice& device, const GsRange& range, std::span<const std::byte> data)
{
    if (range.empty() || !m_buffer || data.empty())
    {
        return;
    }
    device.upload(*m_buffer, static_cast<std::size_t>(range.offset) * m_elementSize,
                  data.first(std::min<std::size_t>(data.size(), static_cast<std::size_t>(range.count) * m_elementSize)));
}
