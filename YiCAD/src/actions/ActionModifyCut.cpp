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


/// @file ActionModifyCut.cpp
/// @brief 裁剪实体操作实现 —— 选择实体并指定剪切点将其一分为二。

#include "ActionModifyCut.h"

#include <QAction>
#include <QMouseEvent>

#include "Debug.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Modification.h"

/// @brief 构造函数，初始化裁剪操作所需的成员变量。
/// @param doc CAD文档对象。
/// @param docView 文档视图对象。
ActionModifyCut::ActionModifyCut(DmDocument* doc, GuiDocumentView* docView)
    : ActionInterface("Cut Entity", doc, docView)
    , cutEntity(nullptr)
    , cutCoord(new DmVector{})
{
    actionType = DM::ActionModifyCut;
}

/// @brief 析构函数，使用默认实现。
ActionModifyCut::~ActionModifyCut() = default;

/// @brief 初始化操作到指定状态。
/// @param status 目标状态索引。
void ActionModifyCut::init(int status)
{
    ActionInterface::init(status);
}

/// @brief 结束操作，清理实体高亮状态。
/// @param updateTB 是否更新工具栏。
void ActionModifyCut::finish(bool updateTB)
{
    if (cutEntity)
    {
        cutEntity->setHighlighted(false);
    }
    ActionInterface::finish(updateTB);
}

/// @brief 判断实体是否可裁剪（支持直线、圆弧、椭圆、多段线、样条曲线）。
/// @param e 待检查的实体指针。
/// @return 若实体类型可裁剪则返回 true。
bool ActionModifyCut::entityTrimmable(DmEntity* e) const
{
    switch (e->getEntityType())
    {
    case DM::EntityArc:
    case DM::EntityEllipse:
    case DM::EntityLine:
    case DM::EntityPolyline:
    case DM::EntitySpline:
        return true;
    default:
        return false;
    }
}

/// @brief 执行裁剪操作：在裁剪点处将实体一分为二。
void ActionModifyCut::trigger()
{
    if (cutEntity && cutCoord->valid && cutEntity->isPointOnEntity(*cutCoord))
    {
        cutEntity->setHighlighted(false);
        docView->redraw();

        Modification m(docView);
        m.cut(*cutCoord, cutEntity);

        cutEntity = nullptr;
        *cutCoord = DmVector(false);
        setStatus(ChooseCutEntity);
    }
}

/// @brief 处理鼠标移动事件。
/// @param e 鼠标事件对象。
void ActionModifyCut::mouseMoveEvent(QMouseEvent* e)
{
    switch (getStatus())
    {
    case ChooseCutEntity:
        deleteSnapper();
        break;

    case SetCutCoord:
        snapPoint(e);
        break;

    default:
        break;
    }
}

/// @brief 处理鼠标释放事件：左键选择实体/设置裁剪点，右键回退状态。
/// @param e 鼠标事件对象。
void ActionModifyCut::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        switch (getStatus())
        {
        case ChooseCutEntity:
            cutEntity = catchEntity(e);
            if (cutEntity == nullptr)
            {
                GUIDIALOGFACTORY->commandMessage(tr("No Entity found."));
            }
            else if (entityTrimmable(cutEntity))
            {
                cutEntity->setHighlighted(true);
                docView->redraw();
                setStatus(SetCutCoord);
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Entity must be a line, arc, ellipse or polyline."));
            }
            break;

        case SetCutCoord:
            *cutCoord = snapPoint(e);
            if (cutEntity == nullptr)
            {
                GUIDIALOGFACTORY->commandMessage(tr("No Entity found."));
            }
            else if (!cutCoord->valid)
            {
                cutEntity->setHighlighted(false);
                GUIDIALOGFACTORY->commandMessage(tr("Cutting point is invalid."));
            }
            else if (!cutEntity->isPointOnEntity(*cutCoord))
            {
                cutEntity->setHighlighted(false);
                GUIDIALOGFACTORY->commandMessage(tr("Cutting point is not on entity."));
            }
            else
            {
                trigger();
                deleteSnapper();
            }
            break;

        default:
            break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        if (cutEntity)
        {
            cutEntity->setHighlighted(false);
            docView->redraw();
        }
        init(getStatus() - 1);
    }
}

/// @brief 更新鼠标按钮提示文本。
void ActionModifyCut::updateMouseButtonHints()
{
    switch (getStatus())
    {
    case ChooseCutEntity:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Specify entity to cut"), tr("Cancel"));
        break;
    case SetCutCoord:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Specify cutting point"), tr("Back"));
        break;
    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

/// @brief 根据当前操作状态更新鼠标光标样式。
void ActionModifyCut::updateMouseCursor()
{
    switch (getStatus())
    {
    case ChooseCutEntity:
        docView->setMouseCursor(DM::SelectCursor);
        break;
    case SetCutCoord:
        docView->setMouseCursor(DM::CadCursor);
        break;
    default:
        break;
    }
}
