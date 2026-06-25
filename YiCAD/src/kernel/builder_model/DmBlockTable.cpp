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


/// @file DmBlockTable.cpp
/// @brief 块表的实现，包括块的增删查改、激活、重命名等操作

#include "DmBlockTable.h"

#include <set>
#include <iostream>
#include <algorithm>
#include <QString>
#include <QRegExp>

#include "Debug.h"
#include "DmBlock.h"
#include "DmDocument.h"

DmBlockTable::DmBlockTable()
{
    m_pActiveBlock = nullptr;
}

void DmBlockTable::setDocument(DmDocument* pDoc)
{
    ITable::setDocument(pDoc);
}

void DmBlockTable::startModify(DmObject* e)
{
    DmBlock* block = static_cast<DmBlock*>(e);
    BlockTableModifyCmd* cmd = new BlockTableModifyCmd(this, block);
    m_pDoc->getCmdManager()->addToCurrentCmd(cmd);
}

// 清空块表中的所有块。
void DmBlockTable::clear()
{
    m_blocks.clear();
    m_blockMap.clear();
    m_pActiveBlock = nullptr;
}

// 激活指定块。
void DmBlockTable::activate(const QString& name)
{
    activate(find(name));
}

// 激活指定块。
void DmBlockTable::activate(DmBlock* block)
{
    m_pActiveBlock = block;
}

void DmBlockTable::activate_direct(const QString& name)
{
    activate_direct(find(name));
}

void DmBlockTable::activate_direct(DmBlock* block)
{
    m_pActiveBlock = block;
}

/// @brief 向块列表添加块。如果已存在同名块，则添加失败。
/// @param block 要添加的块
/// @param notify true 表示需要通知监听者
/// @return false 表示已存在同名块，添加失败
bool DmBlockTable::add(DmBlock* block, bool notify)
{
    if (!block)
    {
        return false;
    }

    if (!m_pDoc)
    {
        return false;
    }

    // 检查块是否已存在
    DmBlock* b = find(block->getName());
    if (b)
    {
        return false;
    }

    // 通过命令系统添加
    DmId id = block->getId();
    if (!id.isValid())
    {
        id = m_pDoc->getIdManager()->assignID(block);
    }
    BlockTableAddCmd* cmd = new BlockTableAddCmd(this, block);
    m_pDoc->getCmdManager()->addAndExecuteCmd(cmd);

    return true;
}

// 从列表中移除块。
// 块从列表移除后、对象真正删除前会通知监听者。
void DmBlockTable::remove(DmBlock* block)
{
    if (!block)
    {
        return;
    }

    if (!m_pDoc)
    {
        return;
    }

    // 通过命令系统标记为已删除
    BlockTableRemoveCmd* cmd = new BlockTableRemoveCmd(this, block);
    m_pDoc->getCmdManager()->addAndExecuteCmd(cmd);

    // 必要时取消当前激活块
    if (m_pActiveBlock == block)
    {
        m_pActiveBlock = nullptr;
    }
}

void DmBlockTable::add_direct(DmBlock* block)
{
    m_blocks.emplace_back(block);
    m_blockMap[block->getId()] = block;
}

void DmBlockTable::remove_direct(DmBlock* block)
{
    auto it2 = std::find(m_blocks.begin(), m_blocks.end(), block);
    if (it2 != m_blocks.end())
    {
        m_blocks.erase(it2);
    }
    m_blockMap.erase(block->getId());
}

/// @brief 尝试将指定块重命名为 name，块表中的块名必须唯一。
/// @param block 要重命名的块
/// @param name 新名称
/// @retval true 重命名成功
/// @retval false 重命名失败
bool DmBlockTable::rename(DmBlock* block, const QString& name)
{
    if (block)
    {
        if (!find(name))
        {
            block->setName(name);
            return true;
        }
    }

    return false;
}

/// @return 返回指定名称的块指针；如果未找到则返回 nullptr
DmBlock* DmBlockTable::find(const QString& name)
{
    for (DmBlock* b : m_blocks)
    {
        if (b->isErased())
        {
            continue;
        }
        if (b->getName() == name)
        {
            return b;
        }
    }
    return nullptr;
}

DmBlock* DmBlockTable::find(const DmId& id)
{
    auto it = m_blockMap.find(id);
    if (it == m_blockMap.end())
    {
        return nullptr;
    }
    return it->second;
}

/// @brief 生成一个新的唯一块名。
/// @param suggestion 新块名的建议前缀
QString DmBlockTable::newName(const QString& suggestion)
{
    if (!find(suggestion))
    {
        return suggestion;
    }

    QString name = suggestion;
    QRegExp const rx(R"(-\d+$)");
    int index = name.lastIndexOf(rx);
    int i = -1;
    if (index > 0)
    {
        i = name.mid(index + 1).toInt();
        name = name.mid(0, index);
    }
    for (DmBlock* b : m_blocks)
    {
        if (b->isErased())
        {
            continue;
        }
        index = b->getName().lastIndexOf(rx);
        if (index < 0)
        {
            continue;
        }
        QString const part1 = b->getName().mid(0, index);
        if (part1 != name)
        {
            continue;
        }
        i = std::max(b->getName().mid(index + 1).toInt(), i);
    }
    return QString("%1-%2").arg(name).arg(i + 1);
}

// 切换指定块的开关状态。
void DmBlockTable::toggle(const QString& name)
{
    toggle(find(name));
}

// 切换指定块的开关状态。
void DmBlockTable::toggle(DmBlock* block)
{
    if (!block)
    {
        return;
    }

    block->toggle();
}

unsigned int DmBlockTable::count() const
{
    unsigned int size = 0;
    for (auto block : m_blocks)
    {
        if (!block->isErased())
        {
            size++;
        }
    }
    return size;
}

DmBlockTable::iterator DmBlockTable::begin()
{
    return DmBlockTable::iterator(m_blocks.begin(), m_blocks.end());
}

DmBlockTable::iterator DmBlockTable::end()
{
    return DmBlockTable::iterator(m_blocks.end());
}

/// @return 当前激活的块；如果没有激活块则返回 nullptr
DmBlock* DmBlockTable::getActive()
{
    return m_pActiveBlock;
}
