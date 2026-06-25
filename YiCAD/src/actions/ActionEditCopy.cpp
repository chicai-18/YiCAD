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


/// @file ActionEditCopy.cpp
/// @brief 编辑复制/剪切 Action 类的实现

#include "ActionEditCopy.h"

#include <QAction>
#include <QMouseEvent>

#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Modification.h"

/// @brief 构造函数
/// @param copy 为 true 时复制，为 false 时剪切
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionEditCopy::ActionEditCopy(bool copy, DmDocument* doc, GuiDocumentView* docView) :
    ActionInterface("Edit Copy", doc, docView), copy{copy}, referencePoint{new DmVector{}}
{
}

ActionEditCopy::~ActionEditCopy() = default;

/// @brief 初始化 Action 状态
/// @param status 初始状态
void ActionEditCopy::init(int status)
{
    ActionInterface::init(status);
    // trigger();
}

/// @brief 触发复制/剪切操作
void ActionEditCopy::trigger()
{
    Modification m(docView);
    m.copy(*referencePoint, !copy);

    // docView->redraw();
    finish(false);
    docView->killSelectActions();
    // init(getStatus()-1);
    GUIDIALOGFACTORY->updateSelectionWidget(pDocument->getEntityTable()->countSelect());
}

/// @brief 鼠标移动事件处理
/// @param e 鼠标事件
void ActionEditCopy::mouseMoveEvent(QMouseEvent* e)
{
    if (getStatus() == SetReferencePoint)
    {
        (void)snapPoint(e);
    }
    else
    {
        deleteSnapper();
    }
}

/// @brief 鼠标释放事件处理
/// @param e 鼠标事件
void ActionEditCopy::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        GuiCoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton)
    {
        init(getStatus() - 1);
    }
}

/// @brief 坐标事件处理
/// @param e 坐标事件
void ActionEditCopy::coordinateEvent(GuiCoordinateEvent* e)
{
    if (!e)
    {
        return;
    }

    *referencePoint = e->getCoordinate();
    trigger();
}

/// @brief 更新鼠标按钮提示
void ActionEditCopy::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetReferencePoint:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify reference point"), tr("Cancel"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

/// @brief 更新鼠标光标
void ActionEditCopy::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}
