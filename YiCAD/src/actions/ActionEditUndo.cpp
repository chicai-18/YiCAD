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


/// @file ActionEditUndo.cpp
/// @brief 撤销/重做 Action 类的实现

#include "ActionEditUndo.h"

#include <QAction>

#include "DmDocument.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"

/// @brief 构造函数
/// @param undo true 为撤销，false 为重做
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionEditUndo::ActionEditUndo(bool undo, DmDocument* doc, GuiDocumentView* docView) :
    ActionInterface("Edit Undo", doc, docView), undo(undo)
{
}

/// @brief 初始化并立即执行
/// @param status 初始状态
void ActionEditUndo::init(int status)
{
    ActionInterface::init(status);
    trigger();
}

/// @brief 触发撤销或重做操作
void ActionEditUndo::trigger()
{
    if (!pDocument)
    {
        qWarning("undo: pDocument is null");
        return;
    }

    if (undo)
    {
        pDocument->undo();
    }
    else
    {
        pDocument->redo();
    }

    //docView->redraw();
    finish(false);
    //GUIDIALOGFACTORY->updateSelectionWidget(pDocument->getEntityContainer()->countSelected(), pDocument->getEntityContainer()->totalSelectedLength());
}
