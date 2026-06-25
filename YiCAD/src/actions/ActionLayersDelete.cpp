/****************************************************************************
**
 * Toggle whether a layer is a construction layer
 * Construction layer doesn't appear on printout
 * and have straight lines of infinite length

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/


/// @file ActionLayersDelete.cpp
/// @brief 图层删除 Action 类的实现

#include "ActionLayersDelete.h"
#include "DmLayer.h"
#include "DmDocument.h"
#include "ApplicationWindow.h"
#include "CustomComboboxItem.h"
#include "GuiDocumentView.h"
#include "SARibbonComboBox.h"
#include "MDIWindow.h"
#include "Transaction.h"

#include <QMessageBox>
#include <QListWidget>

/// @brief 构造函数
/// @param sender 发送者对象指针
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionLayersDelete::ActionLayersDelete(QObject* sender, DmDocument* doc, GuiDocumentView* docView) :
    ActionInterface("Delete Layer", doc, docView)
    , layer(nullptr)
    , theButton(sender)
{
}

/// @brief 初始化并立即触发
/// @param status 初始状态
void ActionLayersDelete::init(int status)
{
    ActionInterface::init(status);
    trigger();
}

/// @brief 触发图层删除操作
void ActionLayersDelete::trigger()
{
    ApplicationWindow* appWin = ApplicationWindow::getAppWindow();
    auto cbxItems = appWin->getLayerComboboxItems();

    // 按钮触发的action，获得按钮所在的图层，判断需要隐藏还是显示
    if (theButton && nullptr == layer)
    {
        // 图层下拉列表按钮触发
        for (auto& item : cbxItems)
        {
            auto data = item->getData();
            if (data->btnDelete == theButton)
            {
                setLayer(data->getLayerName());
            }
        }
    }

    if (layer)
    {
        bool canRemove = canLayerRemove(layer);
        QString layerName = layer->getName();
        if (!canRemove)
        {
            QMessageBox::information(nullptr, QObject::tr("Tips"), tr("The layer: %1 can not be remove. The following layers can not be removed: \n1. \"0\" layer; \n2. current layer; \n3. layer contains entities").arg(layerName));
        }
        else
        {
            Transaction t(tr("Delete Layer").toStdString(), pDocument);
            t.start();
            pDocument->getLayerTable()->remove(layer);
            t.commit();
        }
    }
    finish();
}

/// @brief 根据图层名称设置图层
/// @param layerName 图层名称
void ActionLayersDelete::setLayer(const QString& layerName)
{
    layer = pDocument->getLayerTable()->find(layerName);
}

/// @brief 检查图层是否可删除
/// @param layer 图层指针
/// @return 可删除返回 true，否则返回 false
bool ActionLayersDelete::canLayerRemove(DmLayer* layer)
{
    if (layer->isZeroLayer())
    {
        return false;
    }
    // 是否为当前图层
    auto layerTable = pDocument->getLayerTable();
    if (layerTable->getActive() == layer)
    {
        return false;
    }
    auto entTable = pDocument->getEntityTable();
    // 检查是否存在实体在该图层
    for (auto it = entTable->begin(); it != entTable->end(); ++it)
    {
        if ((*it)->getLayer() == layer)
        {
            return false;
        }
    }

    auto blkTable = pDocument->getBlockTable();
    // 检查是否存在块里的实体在该图层
    // TODO : 待确认
    for (auto it = blkTable->begin(); it != blkTable->end(); ++it)
    {
        auto blk = *it;
        for (auto ent: blk->getEntityTable())
        {
            auto c = dynamic_cast<DmEntityContainer*>(ent);
            if (c)
            {
                for (DmEntity* e = c->firstEntity(DM::ResolveAll); e; e = c->nextEntity(DM::ResolveAll))
                {
                    if (e->getLayer() == layer)
                    {
                        return false;
                    }
                }
            }
            else
            {
                if (ent->getLayer() == layer)
                {
                    return false;
                }
            }
        }

    }
    return true;
}
