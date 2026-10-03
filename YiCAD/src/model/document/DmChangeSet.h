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

/// @file DmChangeSet.h
/// @brief 文档的变更集与变更跟踪器（RENDER_PLAN.md 第 4.3.6 节、第 4.1 步）
///
/// 实体表与各符号表在增、删、改时向文档的跟踪器登记对象（含 add_direct 一类不走命令的路径），
/// 撤销系统在提交、撤销、重做、回滚时补登命令直接改动的对象。文档在这些时刻与读盘结束时把累积的登记
/// 打包成 DmChangeSet，经 DmDocumentListener::entitiesChanged() 交给监听者（图形系统据此只更新变化的部分）。
///
/// 实体不区分增、删、改：接收方按实体当前是否已删除（isErased）判断是去掉还是重建。
/// 释放了的实体另列，接收方只拿它的地址当键，不能解引用；同一地址之后又分配给新实体时，
/// 新实体同时出现在 entities 里，接收方先处理释放的，再处理改动的。

#ifndef DMCHANGESET_H
#define DMCHANGESET_H

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class DmBlock;
class DmEntity;
class DmObject;

/// @brief 改动过的实体与它所在的实体表
struct DmEntityChange
{
    DmEntity* entity = nullptr;         ///< 实体，在变更集交出时仍然存在
    const DmBlock* ownerBlock = nullptr; ///< 所在的块定义；为空表示模型空间（文档的实体表）
};

/// @brief 一批变更
struct DmChangeSet
{
    std::vector<DmEntityChange> entities;          ///< 增、删、改过的实体，每个只出现一次，按首次登记的顺序
    std::vector<const void*> destroyedEntities;    ///< 已释放的实体的地址，只能当键用

    std::vector<const DmBlock*> blocks;            ///< 内容或记录改过的块定义（含新加的），仍然存在
    std::vector<const void*> destroyedBlocks;      ///< 已释放的块定义的地址，只能当键用

    bool layersChanged = false;            ///< 图层表有增、删、改
    bool lineTypesChanged = false;         ///< 线型表有增、删、改
    bool textStylesChanged = false;        ///< 文字样式表有增、删、改
    bool dimensionStylesChanged = false;   ///< 标注样式表有增、删、改
    bool variablesChanged = false;         ///< 文档变量改过

    /// @brief 全部重建：读盘、新建之后，此时其余各项为空
    bool fullRebuild = false;

    /// @brief 是否什么也没变
    bool isEmpty() const;
};

/// @brief 符号表的种类，见 DmChangeTracker::touchTable()
enum class DmSymbolTableKind : std::uint8_t
{
    Layer,
    LineType,
    TextStyle,
    DimensionStyle,
};

/// @brief 文档的变更跟踪器：累积登记，由文档在提交、撤销、重做、读盘等时刻取走（takeChanges）
/// @details 文档没有监听者时不记录（没有人取，记下来只会越攒越多）。批量期间（读盘）不逐个记录，
///          结束时记为全部重建。登记实体、块定义时顺带递增对象的修订号（DmObject::revision）
class DmChangeTracker
{
public:
    /// @brief 是否记录登记（文档有监听者时为真）
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    /// @brief 实体增、删、改：记下它与所在的实体表，递增它的修订号
    /// @param entity 实体
    /// @param ownerBlock 所在的块定义；为空表示模型空间。非空时块定义同时记为改过
    void touchEntity(DmEntity* entity, const DmBlock* ownerBlock);

    /// @brief 实体即将释放
    void destroyEntity(const DmEntity* entity, const DmBlock* ownerBlock);

    /// @brief 块定义的内容或记录改过，或新加入块表
    void touchBlock(const DmBlock* block);

    /// @brief 块定义即将释放
    void destroyBlock(const DmBlock* block);

    /// @brief 符号表有增、删、改
    void touchTable(DmSymbolTableKind kind);

    /// @brief 文档变量改过
    void touchVariables();

    /// @brief 开始批量改动（读盘）：期间不逐个记录；可以嵌套
    void beginBulk();

    /// @brief 结束批量改动：最外层结束时丢掉已记录的，记为全部重建
    void endBulk();

    /// @brief 记为全部重建（REGEN）
    void requestFullRebuild();

    /// @brief 有没有还没取走的变更
    bool hasPendingChanges() const;

    /// @brief 实体是否已登记、还没取走（图形系统的修订号校验跳过它们）
    bool isPending(const DmEntity* entity) const;

    /// @brief 取走累积的变更，跟踪器清空
    DmChangeSet takeChanges();

private:
    bool recording() const { return m_enabled && m_bulkDepth == 0; }

    DmChangeSet m_pending;
    std::unordered_map<const DmEntity*, std::size_t> m_touchedEntities;  ///< m_pending.entities 里的实体 -> 下标
    std::unordered_set<const DmBlock*> m_touchedBlocks;                  ///< m_pending.blocks 里的块
    bool m_enabled = false;
    int m_bulkDepth = 0;
};

#endif // DMCHANGESET_H
