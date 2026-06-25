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


/// @file ActionModifyCut2P.cpp
/// @brief 两点裁剪 Action 类的实现

#include "ActionModifyCut2P.h"

#include <QAction>
#include <QMouseEvent>

#include "Debug.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Modification.h"
#include "DmCircle.h"
#include "DmArc.h"
#include "Preview.h"
#include "DmEllipse.h"
#include "Tools.h"

/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionModifyCut2P::ActionModifyCut2P(DmDocument* doc, GuiDocumentView* docView)
    : PreviewActionInterface("Cut Entity", doc, docView)
    , cutEntity(nullptr)
    , firstCoord(new DmVector{})
    , secondCoord(new DmVector{})
{
    actionType = DM::ActionModifyCut2P;
}

/// @brief 析构函数
ActionModifyCut2P::~ActionModifyCut2P() = default;

/// @brief 初始化 Action 状态
/// @param status 初始状态
void ActionModifyCut2P::init(int status)
{
    ActionInterface::init(status);
}

/// @brief 执行裁剪操作
void ActionModifyCut2P::trigger()
{
    if (cutEntity && firstCoord->valid && secondCoord->valid)
    {
        cutEntity->setHighlighted(false);
        cutEntity->setVisible(true);
        docView->redraw();

        Modification m(docView);
        m.cut2P(*firstCoord, *secondCoord, cutEntity);

        cutEntity = nullptr;
        *firstCoord = DmVector(false);
        *secondCoord = DmVector(false);
        finish();

        GUIDIALOGFACTORY->updateSelectionWidget(pDocument->getEntityTable()->countSelect());
    }
}

/// @brief 鼠标移动事件处理
/// @param e 鼠标事件指针
void ActionModifyCut2P::mouseMoveEvent(QMouseEvent* e)
{
    DmVector pt;
    // TODO: startAngle 和 endAngle 变量已移除，原声明未使用，如需角度计算逻辑请在此处添加
    switch (getStatus())
    {
    case ChooseCutEntity:
        deleteSnapper();
        break;

    case SetCutCoord:
    {
        pt = snapPoint(e);
        cutEntity->setVisible(true);
        deletePreview();
        *secondCoord = cutEntity->getNearestPointOnEntity(pt);
        std::vector<DmEntity*> remainEnts;
        DmEntity* deleteEnt = nullptr;
        const bool res = Modification::tryCut2P(*firstCoord, *secondCoord, cutEntity, remainEnts, deleteEnt);
        if (res)
        {
            preview->getEntityContainer()->addEntity(deleteEnt);
            for (const auto& e : remainEnts)
            {
                preview->getEntityContainer()->addEntity(e);
            }
            cutEntity->setVisible(false);
        }
        drawPreview();
    }
        break;
    default:
        break;
    }
}

/// @brief 鼠标释放事件处理
/// @param e 鼠标事件指针
void ActionModifyCut2P::mouseReleaseEvent(QMouseEvent* e)
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
            else if (Modification::isCutableEntity(cutEntity))
            {
                docView->redraw();
                setStatus(SetCutCoord);
                DmVector pt = snapPoint(e);
                *firstCoord = cutEntity->getNearestPointOnEntity(pt);
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Entity must be a line, arc, circle, ellipse or polyline."));
            }
            break;
        case SetCutCoord:
        {
            DmVector pt = snapPoint(e);

            *secondCoord = cutEntity->getNearestPointOnEntity(pt);
            if (cutEntity == nullptr)
            {
                GUIDIALOGFACTORY->commandMessage(tr("No Entity found."));
            }
            else if (!firstCoord->valid)
            {
                GUIDIALOGFACTORY->commandMessage(tr("Cutting point is invalid."));
            }
            else if (!cutEntity->isPointOnEntity(*secondCoord))
            {
                GUIDIALOGFACTORY->commandMessage(tr("Cutting point is not on entity."));
            }
            else
            {
                trigger();
                deleteSnapper();
            }
            break;
        }
        default:
            break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        if (cutEntity)
        {
            cutEntity->setVisible(true);    //还原为可见
            deletePreview();
            docView->redraw();
        }
        init(getStatus() - 1);
    }
}

/// @brief 更新鼠标光标样式
void ActionModifyCut2P::updateMouseCursor()
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
