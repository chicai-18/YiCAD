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

/// @file DmLineTypeTable.cpp
/// @brief 线型表实现

#include "DmLineTypeTable.h"
#include "LineTypeTableCmd.h"
#include "DmDocument.h"

DmLineType* DmLineTypeTable::ByLayer = new DmLineType(DmLineTypeData(LineType::ByLayer, "ByLayer __________", "__________", {}));
DmLineType* DmLineTypeTable::ByBlock = new DmLineType(DmLineTypeData(LineType::ByBlock, "ByBlock __________", "__________", {}));
DmLineType* DmLineTypeTable::Continuous = new DmLineType(DmLineTypeData(LineType::Continuous, "Continuous __________", "__________", {}));
DmLineType* DmLineTypeTable::DashLine = new DmLineType(DmLineTypeData(LineType::DashLine, "DashLine _ _ _ _ _ ", "_ _ _ _ _ ", { 5,-5 }));

DmLineTypeTable::DmLineTypeTable()
    : m_lineTypes(),
      m_pActLineType(nullptr)
{
}

DmLineTypeTable::~DmLineTypeTable()
{
    for (auto l : m_lineTypes)
    {
        delete l;
    }
    m_lineTypes.clear();
}

/// @brief 设置关联文档并初始化
void DmLineTypeTable::setDocument(DmDocument *pDoc)
{
    ITable::setDocument(pDoc);
    // 做一些初始化操作
    DmLineType* pByLayer = new DmLineType(DmLineTypeTable::ByLayer);
    add_direct(pByLayer);
    DmLineType* pByBlock = new DmLineType(DmLineTypeTable::ByBlock);
    add_direct(pByBlock);
    DmLineType* pContinuous = new DmLineType(DmLineTypeTable::Continuous);
    add_direct(pContinuous);
    activate_direct(pByLayer); // 与初始（第一个）一致
}

/// @brief 开始修改线型
void DmLineTypeTable::startModify(DmObject* e)
{
    DmLineType* ent = static_cast<DmLineType*>(e);
    LineTypeTableModifyCmd* cmd = new LineTypeTableModifyCmd(this, ent);
    m_pDoc->getCmdManager()->addToCurrentCmd(cmd);
}

/// @brief 添加线型
void DmLineTypeTable::add(DmLineType* e)
{
    if (!m_pDoc)
        return;
    DmId id = e->getId();
    if (!id.isValid())
    {
        id = m_pDoc->getIdManager()->assignID(e);
    }
    LineTypeTableAddCmd* cmd = new LineTypeTableAddCmd(this, e);
    m_pDoc->getCmdManager()->addAndExecuteCmd(cmd);
}

/// @brief 通过 id 移除线型
void DmLineTypeTable::remove(DmId id)
{
    if (!m_pDoc)
        return;
    auto it = m_lineTypeMap.find(id);
    if (it == m_lineTypeMap.end())
        return;
    LineTypeTableRemoveCmd* cmd = new LineTypeTableRemoveCmd(this, it->second);
    m_pDoc->getCmdManager()->addAndExecuteCmd(cmd);
}

/// @brief 移除线型
void DmLineTypeTable::remove(DmLineType* e)
{
    remove(e->getId());
}

/// @brief 查找线型
DmLineType* DmLineTypeTable::find(const QString& name)
{
    DmLineType* ret = NULL;

    for (int i = 0; i < m_lineTypes.size(); ++i)
    {
        DmLineType* l = m_lineTypes.at(i);
        if (l->isErased())
            continue;
        if (l->getLineTypeName() == name)
        {
            ret = l;
            break;
        }
    }
    return ret;
}

/// @brief 通过 id 查找线型
DmLineType* DmLineTypeTable::find(const DmId& id)
{
    auto it = m_lineTypeMap.find(id);
    if (it == m_lineTypeMap.end())
        return nullptr;
    return it->second;
}

/// @brief 直接添加线型
bool DmLineTypeTable::add_direct(DmLineType* e)
{
    if (!m_pDoc)
        return false;
    DmId id = e->getId();
    if (!id.isValid())
    {
        id = m_pDoc->getIdManager()->assignID(e);
    }
    auto it = m_lineTypeMap.find(id);
    if (it != m_lineTypeMap.end())
        return false;
    m_lineTypeMap[id] = e;
    m_lineTypes.emplace_back(e);
    return true;
}

/// @brief 直接删除线型
bool DmLineTypeTable::remove_direct(DmLineType* e)
{
    auto it2 = std::find(m_lineTypes.begin(), m_lineTypes.end(), e);
    m_lineTypes.erase(it2);
    m_lineTypeMap.erase(e->getId());
    m_pDoc->getIdManager()->removeID(e->getId());
    delete e;
    return true;
}

DmLineTypeTable::iterator DmLineTypeTable::begin()
{
    return DmLineTypeTable::iterator(m_lineTypes.begin(), m_lineTypes.end());
}

DmLineTypeTable::iterator DmLineTypeTable::end()
{
    return DmLineTypeTable::iterator(m_lineTypes.end());
}

unsigned int DmLineTypeTable::count() const
{
    unsigned int size = 0;
    for (auto lineType : m_lineTypes)
    {
        if (!lineType->isErased())
        {
            size++;
        }
    }
    return size;
}

/// @brief 按名称激活线型
void DmLineTypeTable::activate(const QString& name)
{
    activate(find(name));
}

/// @brief 激活线型
void DmLineTypeTable::activate(DmLineType* lineType)
{
    LineTypeTableActivateCmd* cmd = new LineTypeTableActivateCmd(this, lineType);
    m_pDoc->getCmdManager()->addAndExecuteCmd(cmd);
}

/// @brief 直接激活线型
void DmLineTypeTable::activate_direct(DmLineType* layer)
{
    m_pActLineType = layer;
}

/// @brief 获取当前激活的线型
DmLineType* DmLineTypeTable::getActive()
{
    return m_pActLineType;
}

/// @brief 删除所有静态线型实例
void DmLineTypeTable::deleteStaticLineTypes()
{
    if (ByLayer)
    {
        delete ByLayer;
        ByLayer = nullptr;
    }
    if (ByBlock)
    {
        delete ByBlock;
        ByBlock = nullptr;
    }
    if (Continuous)
    {
        delete Continuous;
        Continuous = nullptr;
    }
    if (DashLine)
    {
        delete DashLine;
        DashLine = nullptr;
    }
}
