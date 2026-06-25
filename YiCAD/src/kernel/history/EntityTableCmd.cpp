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

/// @file EntityTableCmd.cpp
/// @brief 实体表命令实现

#include "EntityTableCmd.h"
#include "EntityTable.h"
#include "DmEntity.h"
#include "EntityData.h"
#include "Stream.h"
#include <sstream>

EntityTableAddCmd::EntityTableAddCmd(EntityTable *table, DmEntity *addedEnt)
        :m_table(table)
        ,m_addedEnt(addedEnt)
{
}

/// @brief 执行添加实体
void EntityTableAddCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    DmId id = m_addedEnt->getId();
    auto e = m_table->find(id);
    if (e == nullptr)
    {
        m_isOwnByCommand = true;
        m_table->add_direct(m_addedEnt);
        e = m_addedEnt;
    }
    else
    {
        // 在"添加"->"删除"->"添加"时发生
        m_isOwnByCommand = false;
        e->setErased(false);
        m_table->m_searchTree.insert(e);
    }
    e->setSelected(false);//设置非选中状态，undo后是非选中状态
    e->setHighlighted(false);
    ICmd::execute();
}

/// @brief 撤销添加实体
void EntityTableAddCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    DmId id = m_addedEnt->getId();
    auto e = m_table->find(id);
    if (e == nullptr)
    {
        return;
    }
    else
    {
        e->setErased(true);
        m_table->m_searchTree.remove(e);
    }
    ICmd::undo();
}

/// @brief 重做添加实体
void EntityTableAddCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    DmId id = m_addedEnt->getId();
    auto e = m_table->find(id);
    if (e == nullptr)
    {
        m_table->add_direct(m_addedEnt);
    }
    else
    {
        // 一般不出现
        e->setErased(false);
        m_table->m_searchTree.insert(e);
    }
    ICmd::redo();
}

/// @brief 清空（释放资源）
void EntityTableAddCmd::clear()
{
    DmId id = m_addedEnt->getId();
    auto obj = m_table->find(id);
    if (m_isOwnByCommand)
    {
        m_table->remove_direct(obj);
    }
}

EntityTableRemoveCmd::EntityTableRemoveCmd(EntityTable *table, DmEntity *removedEnt)
:m_table(table)
,m_removedEnt(removedEnt)
{

}

/// @brief 执行移除实体
void EntityTableRemoveCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_removedEnt->getId());
    if (e == nullptr)
    {
        // 一般不发生
        return;
    }
    else
    {
        e->setSelected(false);//删除前设置非选中状态，undo后是非选中状态
        e->setHighlighted(false);
        e->setErased(true);
        m_table->m_searchTree.remove(e);
    }
    ICmd::execute();
}

/// @brief 撤销移除实体
void EntityTableRemoveCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    auto e = m_table->find(m_removedEnt->getId());
    if (e == nullptr)
    {
        return;
    }
    else
    {
        e->setErased(false);
        m_table->m_searchTree.insert(e);
    }
    ICmd::undo();
}

/// @brief 重做移除实体
void EntityTableRemoveCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    execute();
    ICmd::redo();
}

void EntityTableRemoveCmd::clear()
{
}

EntityTableModifyCmd::EntityTableModifyCmd(EntityTable *table, DmEntity *modifiedEnt)
:m_table(table)
,m_modifiedEnt(modifiedEnt)
{
    modifiedEnt->setSelected(false);//设置非选中状态，undo后是非选中状态
    modifiedEnt->setHighlighted(false);
    std::ostringstream oss;
    OutputStream str(oss);
    m_modifiedEnt->saveStream(str);
    m_originData = oss.str();
}

/// @brief 执行修改实体
void EntityTableModifyCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    std::ostringstream oss;
    OutputStream str(oss);
    m_modifiedEnt->saveStream(str);
    m_newData = oss.str();
    m_modifiedEnt->update();
    m_table->m_searchTree.update(m_modifiedEnt);//更新到空间树
    ICmd::execute();
}

/// @brief 撤销修改实体
void EntityTableModifyCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    std::istringstream iss(m_originData);
    InputStream str(iss);
    m_modifiedEnt->restoreStream(str);
    m_modifiedEnt->update();
    m_table->m_searchTree.update(m_modifiedEnt);
    ICmd::undo();
}

/// @brief 重做修改实体
void EntityTableModifyCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    std::istringstream iss(m_newData);
    InputStream str(iss);
    m_modifiedEnt->restoreStream(str);
    m_modifiedEnt->update();
    m_table->m_searchTree.update(m_modifiedEnt);
    ICmd::redo();
}

void EntityTableModifyCmd::clear()
{

}
