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

/// @file EntityTableCmd.h
/// @brief 实体表命令，包含添加、移除、修改实体的 Undo/Redo 命令

#ifndef ENTITYTABLECMD_H
#define ENTITYTABLECMD_H

#include "Cmd.h"
#include "DmEntity.h"
#include <string>

class EntityTable;
class DmEntity;

/// @brief 添加实体命令
class EntityTableAddCmd :public ICmd
{
public:
    EntityTableAddCmd():m_table(nullptr),m_addedEnt(nullptr),m_isOwnByCommand(false) { }
    /// @brief 构造添加实体命令
    /// @param table 实体表
    /// @param addedEnt 要添加的实体
    EntityTableAddCmd(EntityTable* table, DmEntity* addedEnt);
    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;
    DmObject* getObject() override {return m_addedEnt;}
    CmdType getCmdType() const override{return CmdType::EntityTableAddCmd;}

protected:
    EntityTable* m_table;           ///< 实体表
    DmEntity* m_addedEnt;           ///< 被添加的实体
    bool m_isOwnByCommand;           ///< 实体是否由该命令所有
};

/// @brief 移除实体命令
class EntityTableRemoveCmd :public ICmd
{
public:
    EntityTableRemoveCmd():m_table(nullptr),m_removedEnt(nullptr){}
    /// @brief 构造移除实体命令
    /// @param table 实体表
    /// @param removedEnt 要移除的实体
    EntityTableRemoveCmd(EntityTable* table, DmEntity* removedEnt);
    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;
    DmObject* getObject() override {return m_removedEnt;}
    CmdType getCmdType() const override{return CmdType::EntityTableRemoveCmd;}

protected:
    EntityTable* m_table;           ///< 实体表
    DmEntity* m_removedEnt;         ///< 被移除的实体
};

/// @brief 修改实体命令
class EntityTableModifyCmd :public ICmd
{
public:
    EntityTableModifyCmd():m_table(nullptr),m_modifiedEnt(nullptr){}
    /// @brief 构造修改实体命令
    /// @param table 实体表
    /// @param modifiedEnt 要修改的实体
    EntityTableModifyCmd(EntityTable* table, DmEntity* modifiedEnt);
    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;
    DmObject* getObject() override {return m_modifiedEnt;}
    CmdType getCmdType() const override{return CmdType::EntityTableModifyCmd;}

protected:
    std::string m_originData;       ///< 实体的成员序列化后的原始数据
    std::string m_newData;          ///< 实体的成员序列化后的新数据
    DmEntity* m_modifiedEnt;        ///< 被修改的实体
    EntityTable* m_table;           ///< 实体表
};

#endif //ENTITYTABLECMD_H
