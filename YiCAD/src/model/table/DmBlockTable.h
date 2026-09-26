/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


/// @file DmBlockTable.h
/// @brief 块表，用来维护和管理文档中所有块模板

#ifndef DMBLOCKTABLE_H
#define DMBLOCKTABLE_H

#include <vector>
#include <unordered_map>

#include "TableBase.h"
#include "BlockTableCmd.h"
#include "DmId.h"

class QString;
class DmBlock;
class DmDocument;

/// @class DmBlockTable
/// @brief 块表
/// @description 用来维护块模板
class DmBlockTable : public ITable
{
    friend class BlockTableAddCmd;
    friend class BlockTableRemoveCmd;
    friend class BlockTableModifyCmd;

public:
    using iterator = FilterIterator<std::vector<DmBlock*>::iterator>;

public:
    DmBlockTable();
    virtual ~DmBlockTable() = default;

    void setDocument(DmDocument* pDoc) override;
    void startModify(DmObject* e) override;

    void clear();
    /// @return 当前未删除的块数量
    unsigned int count() const;

    /// @brief 用于 range-based for 遍历
    iterator begin();
    iterator end();

    void activate(const QString& name);
    void activate(DmBlock* block);
    void activate_direct(const QString& name);
    void activate_direct(DmBlock* block);
    /// @return 当前激活的块；若没有激活块则返回 nullptr
    DmBlock* getActive();

    virtual bool add(DmBlock* block, bool notify = true);
    virtual void remove(DmBlock* block);

    /// @brief 表中不存在该块，直接添加（供命令类使用）
    void add_direct(DmBlock* block);
    /// @brief 直接从表中移除块（供命令类使用）
    void remove_direct(DmBlock* block);
    virtual bool rename(DmBlock* block, const QString& name);
    DmBlock* find(const QString& name);
    DmBlock* find(const DmId& id);
    QString newName(const QString& suggestion = "");
    void toggle(const QString& name);
    void toggle(DmBlock* block);

private:
    std::vector<DmBlock*>           m_blocks;           ///< 文档中的块列表
    std::unordered_map<DmId, DmBlock*> m_blockMap;      ///< 通过 ID 查找块的映射
    DmBlock*                        m_pActiveBlock = nullptr;  ///< 当前激活的块
};

#endif
