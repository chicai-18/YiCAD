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


/// @file ActionSelect.cpp
/// @brief 实体选择操作实现

#include <QMouseEvent>
#include "ActionSelect.h"

#include "ActionSelectMultiple.h"
#include "ActionSelectSingle.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"

/// @brief 构造函数，初始化选择操作
/// @param [in] a_handler UI动作处理器指针
/// @param [in] doc 文档指针
/// @param [in] docView 文档视图指针
/// @param [in] nextAction 选择完成后要执行的后续动作类型
/// @param [in] entityTypeList 允许选择的实体类型列表
ActionSelect::ActionSelect(UIActionHandler* a_handler, DmDocument* doc, GuiDocumentView* docView,
                           DM::ActionType nextAction, std::list<DM::EntityType> const& entityTypeList) :
    ActionInterface("Select Entities", doc, docView),
    entityTypeList(entityTypeList), nextAction(nextAction), action_handler(a_handler)
{
    actionType = DM::ActionSelect;
}

/// @brief 初始化操作状态，创建框选子Action
/// @param [in] status 初始状态值
void ActionSelect::init(int status)
{
    ActionInterface::init(status);
    if (status >= 0)
    {
        // 当前使用框选模式；如需单选模式可切换为 ActionSelectSingle
        //docView->setCurrentAction(new ActionSelectSingle(*container, *docView, this, entityTypeList));
        docView->setCurrentAction(new ActionSelectMultiple(pDocument, docView, this, entityTypeList));
    }

    deleteSnapper();
}

/// @brief 恢复操作
void ActionSelect::resume()
{
    ActionInterface::resume();
    deleteSnapper();
}

/// @brief 处理鼠标释放事件，右键返回上一状态
/// @param [in] e 鼠标事件指针
void ActionSelect::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::RightButton)
    {
        init(getStatus() - 1);
    }
}

/// @brief 获取已选中的实体数量
/// @return 已选中的实体数量；若为0则显示提示消息
int ActionSelect::countSelected()
{
    int ret = pDocument->getEntityTable()->countSelect();
    if (ret == 0)
    {
        GUIDIALOGFACTORY->commandMessage(tr("No entity selected!"));
    }

    return ret;
}

/// @brief 更新鼠标按钮提示文本，根据后续动作类型显示不同提示
void ActionSelect::updateMouseButtonHints()
{
    switch (nextAction)
    {
        case DM::ActionModifyDeleteNoSelect:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select to delete"), tr("Cancel"));
            break;

        case DM::ActionModifyMoveNoSelect:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select to move"), tr("Cancel"));
            break;

        case DM::ActionEditCopyNoSelect:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select to copy"), tr("Cancel"));
            break;

        case DM::ActionEditCutNoSelect:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select to cut"), tr("Cancel"));
            break;

        case DM::ActionModifyRotateNoSelect:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select to rotate"), tr("Cancel"));
            break;

        case DM::ActionModifyScaleNoSelect:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select to scale"), tr("Cancel"));
            break;

        case DM::ActionModifyMirrorNoSelect:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select to mirror"), tr("Cancel"));
            break;

        case DM::ActionModifyExplodeNoSelect:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select to explode"), tr("Cancel"));
            break;

        case DM::ActionBlocksCreateNoSelect:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select to create block"), tr("Cancel"));
            break;

        case DM::ActionNoSelectCopyToLayer:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select objects to copy"), tr("Cancel"));
            // TODO: 此case无break，会落入default分支，请确认是否为预期行为
            // fall through

        default:
            GUIDIALOGFACTORY->updateMouseWidget(tr(""), tr(""));
            break;
    }
}

/// @brief 更新鼠标光标样式
void ActionSelect::updateMouseCursor()
{
    if (docView)
    {
        if (isFinished())
        {
            docView->setMouseCursor(DM::ArrowCursor);
        }
        else
        {
            docView->setMouseCursor(DM::SelectCursor);
        }
    }
}

/// @brief 处理键盘按键事件，回车键确认选择
/// @param [in] e 键盘事件指针
void ActionSelect::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Enter && countSelected() > 0)
    {
        finish();
        action_handler->setCurrentAction(nextAction);
    }

    ActionInterface::keyPressEvent(e);
}
