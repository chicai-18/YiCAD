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


/// @file ActionLayersLockAll.cpp
/// @brief 图层全部锁定/解锁 Action 类的实现

#include "ActionLayersLockAll.h"

#include <QAction>

#include "Debug.h"
#include "DmDocument.h"
#include "ApplicationWindow.h"
#include "CustomComboboxItem.h"
#include "GuiDocumentView.h"
#include "Transaction.h"

/// @brief 构造函数
/// @param lock true 表示锁定所有图层，false 表示解锁所有图层
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionLayersLockAll::ActionLayersLockAll(const bool lock, DmDocument* doc, GuiDocumentView* docView) :
    ActionInterface("Lock all Layers", doc, docView)
    , lock(lock)
{
}

/// @brief 触发全部锁定/解锁图层操作
void ActionLayersLockAll::trigger()
{
    if (pDocument)
    {
        // 如果是锁定操作，先取消选择所有实体
        if (lock)
        {
            auto table = pDocument->getEntityTable();
            for (auto it = table->begin(); it != table->end(); ++it)
            {
                (*it)->setSelected(false);
            }
        }

        Transaction t(tr("Lock All Layers").toStdString(), pDocument);
        t.start();
        auto layerTable = pDocument->getLayerTable();
        for (auto it = layerTable->begin(); it != layerTable->end(); ++it)
        {
            layerTable->startModify(*it);
            (*it)->lock(lock);
        }
        t.commit();
    }
    finish(false);
}

/// @brief 初始化并立即触发
/// @param status 初始状态
void ActionLayersLockAll::init(int status)
{
    ActionInterface::init(status);
    trigger();
}
