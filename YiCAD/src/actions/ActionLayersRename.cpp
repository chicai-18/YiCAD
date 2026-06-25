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


/// @file ActionLayersRename.cpp
/// @brief 图层重命名 Action 类的实现

#include "ActionLayersRename.h"

#include "DmDocument.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "ApplicationWindow.h"
#include "Transaction.h"

/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionLayersRename::ActionLayersRename(DmDocument* doc, GuiDocumentView* docView) :
    ActionInterface("Rename Layer", doc, docView)
{
}

/// @brief 初始化并立即触发
/// @param status 初始状态
void ActionLayersRename::init(int status)
{
    ActionInterface::init(status);
    trigger();
}

/// @brief 触发图层重命名操作，通过编辑对话框修改图层数据
void ActionLayersRename::trigger()
{
    if (pDocument)
    {
        // 通过编辑对话框获取用户修改后的图层数据
        DmLayer* layer = GUIDIALOGFACTORY->requestEditLayerDialog(pDocument->getLayerTable());
        if (layer)
        {
            Transaction t(tr("Rename Layer").toStdString(), pDocument);
            t.start();
            auto activeLayer = pDocument->getLayerTable()->getActive();
            pDocument->getLayerTable()->startModify(activeLayer);
            activeLayer->setData(layer->getData());
            t.commit();
            delete layer;
        }
        else
        {
            // 用户取消编辑对话框，无需执行任何操作
        }
    }
    else
    {
        // 文档指针为空，无法执行操作
    }
    finish(false);
}
