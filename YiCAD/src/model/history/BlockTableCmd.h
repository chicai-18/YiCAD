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

/// @file BlockTableCmd.h
/// @brief 块表操作命令，包含添加、移除、修改块的 Undo/Redo 命令

#ifndef BLOCKTABLECMD_H
#define BLOCKTABLECMD_H

#include "TableBase.h"
#include "DmBlock.h"

class DmBlockTable;
class DmBlock;

/// @brief 添加块命令
class BlockTableAddCmd :public ICmd
{
public:
    /// @brief 构造添加块命令
    /// @param table 块表
    /// @param addedBlock 要添加的块
    BlockTableAddCmd(DmBlockTable* table, DmBlock* addedBlock);
    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;
    DmObject* getObject() override {return m_addedBlock;}
    CmdType getCmdType() const override{return CmdType::BlockTableAddCmd;}

protected:
    DmBlockTable* m_table;          ///< 块表
    DmBlock* m_addedBlock;          ///< 被添加的块
    bool m_isOwnByCommand;           ///< 是否由该命令拥有
};

/// @brief 移除块命令
class BlockTableRemoveCmd :public ICmd
{
public:
    /// @brief 构造移除块命令
    /// @param table 块表
    /// @param removedBlock 要移除的块
    BlockTableRemoveCmd(DmBlockTable* table, DmBlock* removedBlock);
    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;
    DmObject* getObject() override {return m_removedBlock;}
    CmdType getCmdType() const override{return CmdType::BlockTableRemoveCmd;}

protected:
    DmBlockTable* m_table;          ///< 块表
    DmBlock* m_removedBlock;        ///< 被移除的块
};

/// @brief 修改块命令
class BlockTableModifyCmd :public ICmd
{
public:
    BlockTableModifyCmd():m_table(nullptr),m_modifiedBlock(nullptr){}
    /// @brief 构造修改块命令
    /// @param table 块表
    /// @param modifiedBlock 要修改的块
    BlockTableModifyCmd(DmBlockTable* table, DmBlock* modifiedBlock);
    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;
    /// @brief 设置新数据（针对块修改的特殊情况）
    /// @param newData 新的序列化数据
    void setNewData(const std::string& newData){m_newData = newData;}
    DmObject* getObject() override {return m_modifiedBlock;}
    CmdType getCmdType() const override{return CmdType::BlockTableModifyCmd;}
    /// @brief 设置已执行状态
    void setExecuted(bool executed) { m_isExecuted = executed; }

protected:
    std::string m_originData;           ///< 原始序列化数据
    std::string m_newData;              ///< 新序列化数据
    DmBlock* m_modifiedBlock;           ///< 被修改的块
    DmBlockTable* m_table;              ///< 块表
};

#endif //BLOCKTABLECMD_H
