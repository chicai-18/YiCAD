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


/// @file ActionLayersAdd.cpp
/// @brief 图层添加 Action 类的实现

#include "ActionLayersAdd.h"

#include <QAction>

#include "Debug.h"
#include "DmDocument.h"
#include "GuiDialogFactory.h"
#include "ApplicationWindow.h"
#include "Transaction.h"

/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionLayersAdd::ActionLayersAdd(DmDocument* doc, GuiDocumentView* docView)
    : ActionInterface("Add Layer", doc, docView)
{
}

/// @brief 触发图层添加操作
void ActionLayersAdd::trigger()
{
    if (pDocument)
    {
        DmLayer* layer = GUIDIALOGFACTORY->requestNewLayerDialog(pDocument->getLayerTable());
        if (layer)
        {
            Transaction t("Add Layer", pDocument);
            t.start();
            pDocument->getLayerTable()->add(layer);
            t.commit();
        }
    }
    finish(false);
}

/// @brief 初始化并立即触发
/// @param status 初始状态
void ActionLayersAdd::init(int status)
{
    ActionInterface::init(status);
    trigger();
}
