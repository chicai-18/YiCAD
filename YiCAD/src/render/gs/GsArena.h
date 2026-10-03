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

/// @file GsArena.h
/// @brief GPU 数据区：一个按需增长的缓冲与它上面的区段分配器（RENDER_PLAN.md 第 4.3.4 节）
/// @details 每个管线类的几何、图元记录、实例记录各一个数据区，分块与共享几何在里面各占一段连续的区段。
///          方案写的是"按页增长"；这里是单个缓冲翻倍增长、旧内容在 GPU 上复制过去：一个管线类的全部绘制
///          用同一个缓冲，一次多重间接绘制就能画完，不用按页分组。

#ifndef GSARENA_H
#define GSARENA_H

#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>

#include "RhiDevice.h"

/// @brief 一段区间：起点与长度，单位是元素
struct GsRange
{
    std::uint32_t offset = 0;
    std::uint32_t count = 0;

    bool empty() const { return count == 0; }
    std::uint32_t end() const { return offset + count; }
};

/// @brief 区段分配器：按元素分配，最合适优先，释放时与相邻空闲段合并
class GsRangeAllocator
{
public:
    /// @brief 分配 count 个元素；放不下时返回空
    std::optional<std::uint32_t> allocate(std::uint32_t count);

    /// @brief 释放一段（必须是 allocate 给出的）
    void free(std::uint32_t offset, std::uint32_t count);

    /// @brief 容量扩到 capacity（只增不减），新增部分并入空闲
    void grow(std::uint32_t capacity);

    /// @brief 全部清空，容量不变
    void reset();

    std::uint32_t capacity() const { return m_capacity; }
    std::uint32_t used() const { return m_used; }

private:
    void insertFree(std::uint32_t offset, std::uint32_t count);
    void eraseFree(std::map<std::uint32_t, std::uint32_t>::iterator it);

    std::map<std::uint32_t, std::uint32_t> m_freeByOffset;      ///< 空闲段：起点 -> 长度
    std::multimap<std::uint32_t, std::uint32_t> m_freeBySize;   ///< 空闲段：长度 -> 起点
    std::uint32_t m_capacity = 0;
    std::uint32_t m_used = 0;
};

/// @brief 数据区：缓冲与分配器
/// @details 分配只动 CPU 上的分配器，容量不够时翻倍；GPU 缓冲在 sync() 时按容量重建，旧内容复制过去。
///          所以一批改动的顺序是：分配（allocate）→ sync → 写入（write）
class GsArena
{
public:
    /// @param name 调试名
    /// @param elementSize 一个元素的字节数
    /// @param usage 缓冲用途（另加复制的源与目标）
    /// @param texelFormat 作纹素缓冲时的格式；不作纹素缓冲时为 Undefined
    GsArena(std::string name, std::uint32_t elementSize, RhiBufferUsage usage, RhiFormat texelFormat);

    /// @brief 分配一段；容量不够时翻倍
    GsRange allocate(std::uint32_t count);

    /// @brief 释放一段
    void free(GsRange& range);

    /// @brief 全部清空（全部重建）
    void reset();

    /// @brief 预留容量（全部重建时按总量一次给够）
    void reserve(std::uint32_t capacity);

    /// @brief 让 GPU 缓冲赶上容量：没有缓冲时新建，容量变了就建大的并把旧内容复制过去
    /// @param device 设备
    /// @param copies 需要复制时在它上面录制（帧外），见 GsModel::syncArenas
    /// @return 缓冲是否换了（绑定组要重建）
    bool sync(RhiDevice& device, RhiCommandList* copies);

    /// @brief 是否需要复制旧内容（sync 前判断要不要开一个复制用的帧）
    bool needsCopy() const;

    /// @brief 把 data 写进 range（元素个数相同）
    void write(RhiDevice& device, const GsRange& range, std::span<const std::byte> data);

    const RhiBufferPtr& buffer() const { return m_buffer; }
    std::uint32_t elementSize() const { return m_elementSize; }
    RhiFormat texelFormat() const { return m_texelFormat; }
    std::uint32_t capacity() const { return m_allocator.capacity(); }
    std::uint32_t used() const { return m_allocator.used(); }
    /// @brief 缓冲换过几次；绑定组按它判断要不要重建
    std::uint64_t generation() const { return m_generation; }

private:
    std::string m_name;
    std::uint32_t m_elementSize = 0;
    RhiBufferUsage m_usage = RhiBufferUsage::None;
    RhiFormat m_texelFormat = RhiFormat::Undefined;
    GsRangeAllocator m_allocator;
    RhiBufferPtr m_buffer;
    RhiBufferPtr m_retired;          ///< 换下来的旧缓冲，复制录制完之前要留着
    std::uint64_t m_generation = 0;
};

#endif // GSARENA_H
