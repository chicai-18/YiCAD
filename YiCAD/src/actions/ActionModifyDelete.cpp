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


/// @file ActionModifyDelete.cpp
/// @brief 删除选中实体的交互动作类实现

#include "ActionModifyDelete.h"

#include <QAction>

#include "Debug.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Modification.h"

/// @brief 构造函数
/// @param [in] doc 文档指针
/// @param [in] docView 文档视图指针
ActionModifyDelete::ActionModifyDelete(DmDocument* doc, GuiDocumentView* docView)
    : ActionInterface("Delete Entities", doc, docView)
{
    actionType = DM::ActionModifyDelete;
}

/// @brief 初始化动作，直接触发删除
/// @param [in] status 初始状态
void ActionModifyDelete::init(int status)
{
    ActionInterface::init(status);

    trigger();
}

/// @brief 触发删除操作
void ActionModifyDelete::trigger()
{
    Modification m(docView);
    m.remove();

    finish(false);

    GUIDIALOGFACTORY->updateSelectionWidget(pDocument->getEntityTable()->countSelect());
}

/// @brief 更新鼠标按键提示
void ActionModifyDelete::updateMouseButtonHints()
{
    GUIDIALOGFACTORY->updateMouseWidget();
}

/// @brief 更新鼠标光标为删除样式
void ActionModifyDelete::updateMouseCursor()
{
    docView->setMouseCursor(DM::DelCursor);
}
