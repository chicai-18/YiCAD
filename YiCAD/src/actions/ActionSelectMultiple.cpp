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


/// @file ActionSelectMultiple.cpp
/// @brief 支持单选与框选的选择Action实现

#include <QMouseEvent>
#include <QKeyEvent>
#include "ActionSelectMultiple.h"
#include "Debug.h"
#include "GuiDocumentView.h"
#include "Selection.h"
#include "GuiDialogFactory.h"
#include "Preview.h"

/// @brief 触发选择窗口的最小拖拽距离（GUI像素），防止误触
constexpr int MIN_DRAG_DISTANCE = 10;

/// @brief 构造函数，初始化选择操作
/// @param [in] doc 文档指针
/// @param [in] docView 文档视图指针
/// @param [in] actionSelect 调用此选择动作的父Action指针
/// @param [in] entityTypeList 允许选择的实体类型列表
ActionSelectMultiple::ActionSelectMultiple(DmDocument* doc,
                                           GuiDocumentView* docView,
                                           ActionInterface* actionSelect,
                                           std::list<DM::EntityType> const& entityTypeList) :
    PreviewActionInterface("Select Entities", doc, docView),
    entityTypeList(entityTypeList), actionSelect(actionSelect)
{
    actionType = DM::ActionSelectMultiple;
}

/// @brief 初始化操作状态，重置选择窗口数据
/// @param [in] status 初始状态值，默认为0
void ActionSelectMultiple::init(int status)
{
    PreviewActionInterface::init(status);
    getSnapMode()->clear();
    pPoints.reset(new Points{});
    setStatus(Neutral);
}

/// @brief 执行选择操作：当窗口足够大时执行窗口选择
void ActionSelectMultiple::trigger()
{
    PreviewActionInterface::trigger();

    if (pPoints->v1.valid && pPoints->v2.valid)
    {
        if (docView->toGuiDX(pPoints->v1.distanceTo(pPoints->v2))
            > MIN_DRAG_DISTANCE)
        {
            bool cross = (pPoints->v1.x > pPoints->v2.x);

            Selection s(pDocument, docView);
            s.selectWindow(pPoints->v1, pPoints->v2, select, cross);

            GUIDIALOGFACTORY->updateSelectionWidget(
                pDocument->getEntityTable()->countSelect());

            init();
        }
    }
}

/// @brief 处理键盘按键事件，Esc取消选择，Enter确认选择
/// @param [in] e 键盘事件指针
void ActionSelectMultiple::keyPressEvent(QKeyEvent* e)
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

/// @brief 处理鼠标移动事件，更新拖拽状态或选择窗口第二角
/// @param [in] e 鼠标事件指针
void ActionSelectMultiple::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = docView->toGraph(e->x(), e->y());
    snapPoint(e);
    if (getStatus() == Dragging)
    {
        pPoints->v2 = mouse;
        if (docView->toGuiDX(pPoints->v1.distanceTo(pPoints->v2))
            > MIN_DRAG_DISTANCE)
        {
            setStatus(SetCorner2);
        }
    }
    else if (getStatus() == SetCorner2 && pPoints->v1.valid)
    {
        //pPoints->v2 = docView->toGraph(e->x(), e->y());
        pPoints->v2 = snapFree(e);
        docView->setOverlayCorners(pPoints->v1, pPoints->v2);
        docView->redraw();
    }
}

/// @brief 处理鼠标按下事件，开始拖拽操作
/// @param [in] e 鼠标事件指针
void ActionSelectMultiple::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        if (getStatus() == Neutral)
        {
            pPoints->v1 = docView->toGraph(e->x(), e->y());
            setStatus(Dragging);
        }
    }
}

/// @brief 处理鼠标释放事件，执行实体选择或窗口选择
/// @param [in] e 鼠标事件指针
void ActionSelectMultiple::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::RightButton)
    {
        // 右键结束
        if (getStatus() == SetCorner2)
        {
            deletePreview();
        }

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
    else if (e->button() == Qt::LeftButton)
    {
        if (getStatus() == Dragging)
        {
            // 单选模式：捕获单个实体
            DmEntity* ent = nullptr;
            if (entityTypeList.size())
            {
                ent = catchEntity(e, entityTypeList);
            }
            else
            {
                ent = catchEntity(e);
            }

            if (ent)
            {
                deletePreview();
                Selection s(pDocument, docView);
                s.selectSingle(ent);
                GUIDIALOGFACTORY->updateSelectionWidget(
                    pDocument->getEntityTable()->countSelect());
                e->accept();
                setStatus(Neutral);
            }
            else
            {
                setStatus(SetCorner2);
            }
        }
        else if (getStatus() == SetCorner2)
        {
            // 框选模式：完成选择窗口
            pPoints->v2 = docView->toGraph(e->x(), e->y());
            deletePreview();
            bool cross = (pPoints->v1.x > pPoints->v2.x);
            Selection s(pDocument, docView);
            // 默认选择，Shift+点击=取消选择
            bool selectLocal = (e->modifiers() & Qt::ShiftModifier)
                               ? false : true;
            s.selectWindow(pPoints->v1, pPoints->v2, selectLocal, cross,
                           entityTypeList);
            GUIDIALOGFACTORY->updateSelectionWidget(
                pDocument->getEntityTable()->countSelect());

            // 调用init()而不是直接设置状态，可避免结束时绘制十字长线
            init();
            //setStatus(Neutral);
            //e->accept();
        }
    }
}

/// @brief 更新鼠标按钮提示文本
void ActionSelectMultiple::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case Neutral:
            GUIDIALOGFACTORY->updateMouseWidget(
                tr("Click and drag for the selection window"),
                tr("Cancel"));
            break;

        case SetCorner2:
            GUIDIALOGFACTORY->updateMouseWidget(
                tr("Choose second edge"), tr("Back"));
            break;

        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

/// @brief 更新鼠标光标样式为选择光标
void ActionSelectMultiple::updateMouseCursor()
{
    docView->setMouseCursor(DM::SelectCursor);
}
