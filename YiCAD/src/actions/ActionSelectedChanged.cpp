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


/// @file ActionSelectedChanged.cpp
/// @brief 选择变更Action实现

#include "ActionSelectedChanged.h"
#include "DmLayer.h"
#include "DmDocument.h"
#include "DmMText.h"
#include "ApplicationWindow.h"
#include "CustomComboboxItem.h"
#include "GuiDocumentView.h"
#include "GuiDialogFactory.h"
#include "ActionModifyMText.h"

/// @brief 构造函数，初始化选择变更处理器
/// @param [in] doc 文档指针
/// @param [in] docView 文档视图指针
ActionSelectedChanged::ActionSelectedChanged(DmDocument* doc,
                                             GuiDocumentView* docView) :
    ActionInterface("Selected Changed", doc, docView)
    , m_selectedNumType(0)
    , m_singleSelectedEnt(nullptr)
{
}

/// @brief 初始化操作状态，立即触发选择变更处理
/// @param [in] status 初始状态值
void ActionSelectedChanged::init(int status)
{
    ActionInterface::init(status);
    trigger();
}

/// @brief 执行选择变更处理：更新UI图层信息，并触发单选实体的特殊处理
void ActionSelectedChanged::trigger()
{
    // 更新图层信息到UI
    updateToUI();

    // 根据不同类型做特殊处理
    if (m_selectedNumType == 1)
    {
        triggleSingleSelectedAction();
    }

    finish();
}

/// @brief 更新选择信息到UI，统计选中实体及其所属图层
void ActionSelectedChanged::updateToUI()
{
    // 获得选择实体个数
    DmLayer* firstSelectedLayer = nullptr;
    // 0表示无选择，1表示1个图层选择，2表示多于1个图层选择
    m_selectedNumType = 0;
    m_singleSelectedEnt = nullptr;
    for (auto ent : *pDocument->getEntityTable())
    {
        if (ent->isSelected())
        {
            DmLayer* layer = ent->getLayer();
            if (firstSelectedLayer == nullptr)
            {
                firstSelectedLayer = layer;
                m_selectedNumType = 1;
                m_singleSelectedEnt = ent;
            }
            else if (firstSelectedLayer != layer)
            {
                m_selectedNumType = 2;
                break;
            }
        }
    }

    // 更新底部状态栏
    GUIDIALOGFACTORY->updateSelectionWidget(
        pDocument->getEntityTable()->countSelect());
}

/// @brief 触发单选实体的特殊处理（如激活多行文字编辑）
void ActionSelectedChanged::triggleSingleSelectedAction()
{
    //if (m_singleSelectedEnt->getEntityType() == DM::EntityArrayRect)
    //{
    //	ActionArrayRect* action = new ActionArrayRect(pDocument, docView, true);
    //	docView->setCurrentAction(action);
    //}

    //if (m_singleSelectedEnt->getEntityType() == DM::EntityArrayPolar)
    //{
    //	ActionArrayPolar* action = new ActionArrayPolar(pDocument, docView, true);
    //	docView->setCurrentAction(action);
    //}

    // 选择的多行文字，激活属性编辑选项
    if (m_singleSelectedEnt->getEntityType() == DM::EntityMText)
    {
        ActionModifyMText* action = new ActionModifyMText(pDocument, docView);
        action->setText(static_cast<DmMText*>(m_singleSelectedEnt));
        docView->setCurrentAction(action);
    }
}
