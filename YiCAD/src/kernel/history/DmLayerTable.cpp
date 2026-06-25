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

/// @file DmLayerTable.cpp
/// @brief 图层表实现

#include "DmLayerTable.h"

#include "Debug.h"
#include "DmLayer.h"
#include "DmDocument.h"
#include "LayerTableCmd.h"

DmLayerTable::DmLayerTable()
{
}

DmLayerTable::~DmLayerTable()
{
    for (auto l : m_layers)
    {
        delete l;
    }
    m_layers.clear();
}

unsigned int DmLayerTable::count() const
{
    unsigned int size = 0;
    for (auto layer : m_layers)
    {
        if (!layer->isErased())
        {
            size++;
        }
    }
    return size;
}

/// @brief 按名称激活图层
void DmLayerTable::activate(const QString& name)
{
    activate(find(name));
}

/// @brief 激活图层
void DmLayerTable::activate(DmLayer* layer)
{
    LayerTableActivateCmd* cmd = new LayerTableActivateCmd(this, layer);
    m_pDoc->getCmdManager()->addAndExecuteCmd(cmd);
}

/// @brief 获取当前激活的图层
DmLayer* DmLayerTable::getActive()
{
    return m_pActiveLayer;
}

/// @brief 查找图层（不能获得已删除的）
DmLayer* DmLayerTable::find(const QString& name)
{
    DmLayer* ret = NULL;

    for (int i = 0; i < m_layers.size(); ++i)
    {
        DmLayer* l = m_layers.at(i);
        if (l->isErased())
            continue;
        if (l->getName() == name)
        {
            ret = l;
            break;
        }
    }
    return ret;
}

/// @brief 开始修改图层
void DmLayerTable::startModify(DmObject *e)
{
    DmLayer* ent = static_cast<DmLayer*>(e);
    LayerTableModifyCmd* cmd = new LayerTableModifyCmd(this, ent);
    m_pDoc->getCmdManager()->addToCurrentCmd(cmd);
}

/// @brief 添加图层
void DmLayerTable::add(DmLayer *e)
{
    if (!m_pDoc)
        return;
    DmId id = e->getId();
    if (!id.isValid())
    {
        id = m_pDoc->getIdManager()->assignID(e);
    }
    LayerTableAddCmd* cmd = new LayerTableAddCmd(this, e);
    m_pDoc->getCmdManager()->addAndExecuteCmd(cmd);
}

/// @brief 通过 id 移除图层
void DmLayerTable::remove(DmId id)
{
    if (!m_pDoc)
        return;
    auto it = m_layerMap.find(id);
    if (it == m_layerMap.end())
        return;
    LayerTableRemoveCmd* cmd = new LayerTableRemoveCmd(this, it->second);
    m_pDoc->getCmdManager()->addAndExecuteCmd(cmd);
}

/// @brief 移除图层
void DmLayerTable::remove(DmLayer *e)
{
    remove(e->getId());
}

/// @brief 直接添加图层
bool DmLayerTable::add_direct(DmLayer *e)
{
    if (!m_pDoc)
        return false;
    DmId id = e->getId();
    if (!id.isValid())
    {
        id = m_pDoc->getIdManager()->assignID(e);
    }
    auto it = m_layerMap.find(id);
    if (it != m_layerMap.end())
        return false;
    m_layerMap[id] = e;
    m_layers.emplace_back(e);
    return true;
}

/// @brief 直接删除图层
bool DmLayerTable::remove_direct(DmLayer *e)
{
    auto it2 = std::find(m_layers.begin(), m_layers.end(), e);
    m_layers.erase(it2);
    m_layerMap.erase(e->getId());
    m_pDoc->getIdManager()->removeID(e->getId());
    delete e;
    return true;
}

DmLayerTable::iterator DmLayerTable::begin()
{
    return DmLayerTable::iterator(m_layers.begin(), m_layers.end());
}

DmLayerTable::iterator DmLayerTable::end()
{
    return DmLayerTable::iterator(m_layers.end());
}

/// @brief 直接激活图层
void DmLayerTable::activate_direct(DmLayer *layer)
{
    m_pActiveLayer = layer;
}

/// @brief 直接激活图层（按名称）
void DmLayerTable::activate_direct(const QString& name)
{
    activate_direct(find(name));
}

/// @brief 通过 id 查找图层
DmLayer *DmLayerTable::find(const DmId &id)
{
    auto it = m_layerMap.find(id);
    if (it == m_layerMap.end())
        return nullptr;
    return it->second;
}

/// @brief 设置关联文档并初始化
void DmLayerTable::setDocument(DmDocument *pDoc)
{
    ITable::setDocument(pDoc);
    // 做一些初始化操作，添加 "0" 图层
    DmLayer* newLayer = new DmLayer("0");
    newLayer->setDocument(pDoc);
    add_direct(newLayer);
    m_pActiveLayer = newLayer;
}
