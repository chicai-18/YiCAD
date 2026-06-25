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


/// @file ActionModifyEntity.cpp
/// @brief 修改实体属性的交互动作类实现

#include "ActionModifyEntity.h"

#include <QAction>
#include <QMouseEvent>

#include "ActionModifyMText.h"
#include "Debug.h"
#include "DmMText.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"

/// @brief 构造函数
/// @param [in] doc 文档指针
/// @param [in] docView 文档视图指针
ActionModifyEntity::ActionModifyEntity(DmDocument* doc, GuiDocumentView* docView) :
    ActionInterface("Modify Entity", doc, docView),
    m_currentEntity(nullptr)
{
    actionType = DM::ActionModifyEntity;
}

/// @brief 触发修改操作
void ActionModifyEntity::trigger()
{
    if (m_currentEntity)
    {
        // 多行文字的编辑不是模态对话框，且与Action关联，需要特殊处理
        if (m_currentEntity->getEntityType() == DM::EntityMText)
        {
            ActionModifyMText* action = new ActionModifyMText(pDocument, docView);
            action->setText(static_cast<DmMText*>(m_currentEntity));
            docView->setCurrentAction(action);
            return;
        }

        // 不是多行文字类型处理
        GUIDIALOGFACTORY->requestModifyEntityDialog(m_currentEntity);
    }
}

/// @brief 鼠标释放事件处理
/// @param [in] e 鼠标事件指针
void ActionModifyEntity::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::RightButton)
    {
        init(getStatus() - 1);
    }
    else
    {
        m_currentEntity = catchEntity(e);
        if (m_currentEntity)
        {
            m_currentEntity->setSelected(true);
            docView->emitSelectedChanged();
        }
        trigger();
    }
}

/// @brief 更新鼠标光标为选择样式
void ActionModifyEntity::updateMouseCursor()
{
    docView->setMouseCursor(DM::SelectCursor);
}

/// @brief 更新鼠标按键提示
void ActionModifyEntity::updateMouseButtonHints()
{
    GUIDIALOGFACTORY->updateMouseWidget(tr("Click on entity to modify"), tr("Cancel"));
}
