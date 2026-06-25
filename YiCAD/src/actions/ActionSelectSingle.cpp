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


/// @file ActionSelectSingle.cpp
/// @brief 单选实体Action实现

#include "ActionSelectSingle.h"

#include <QAction>
#include <QMouseEvent>

#include "Debug.h"
#include "GuiDialogFactory.h"
#include "Selection.h"

/// @brief 构造函数，初始化单选操作
/// @param [in] doc 文档指针
/// @param [in] docView 文档视图指针
/// @param [in] action_select 调用此选择动作的父Action指针
/// @param [in] entityTypeList 允许选择的实体类型列表
ActionSelectSingle::ActionSelectSingle(DmDocument* doc,
                                       GuiDocumentView* docView,
                                       ActionInterface* action_select,
                                       std::list<DM::EntityType> const& entityTypeList)
    : ActionInterface("Select Entities", doc, docView)
    , entityTypeList(entityTypeList)
    , en(nullptr)
    , actionSelect(action_select)
{
    actionType = DM::ActionSelectSingle;
}

/// @brief 初始化单选操作，并禁用点捕捉以避免实体选择落到端点/中点等提示
void ActionSelectSingle::init(int status)
{
    ActionInterface::init(status);
    getSnapMode()->clear();
}

/// @brief 执行选择操作：如果捕获到匹配的实体，执行单选
void ActionSelectSingle::trigger()
{
    bool typeMatch = true;
    if (en && entityTypeList.size())
    {
        typeMatch = (std::find(entityTypeList.begin(), entityTypeList.end(),
                               en->getEntityType())
                     != entityTypeList.end());
    }
    if (en && typeMatch)
    {
        Selection s(pDocument, docView);
        s.selectSingle(en);

        GUIDIALOGFACTORY->updateSelectionWidget(
            pDocument->getEntityTable()->countSelect());
    }
}

/// @brief 处理键盘按键事件，Esc取消选择，Enter确认选择
/// @param [in] e 键盘事件指针
void ActionSelectSingle::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape)
    {
        finish(false);
        actionSelect->keyPressEvent(e);
    }

    if (pDocument->getEntityTable()->hasSelect()
        && e->key() == Qt::Key_Enter)
    {
        finish(false);
        actionSelect->keyPressEvent(e);
    }
}

/// @brief 处理鼠标释放事件，执行实体捕获与选择
/// @param [in] e 鼠标事件指针
void ActionSelectSingle::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::RightButton)
    {
        finish();
        if (actionSelect->getEntityType() == DM::ActionSelect)
        {
            actionSelect->finish();
        }
        else
        {
            actionSelect->mouseReleaseEvent(e);
        }
    }
    else
    {
        if (entityTypeList.size())
        {
            en = catchEntity(e, entityTypeList);
        }
        else
        {
            en = catchEntity(e);
        }
        trigger();
    }
}

/// @brief 更新鼠标光标样式为选择光标
void ActionSelectSingle::updateMouseCursor()
{
    docView->setMouseCursor(DM::SelectCursor);
}
