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


/// @file ActionInfoAngle.cpp
/// @brief 角度测量 Action 类的实现

#include "ActionInfoAngle.h"

#include <QAction>
#include <QMouseEvent>
#include <cmath>

#include "Debug.h"
#include "DmDocument.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Information.h"
#include "Preview.h"

namespace
{
    constexpr double TWO_PI = 2.0 * M_PI; ///< 2π常量
    constexpr double ANGLE_ZERO = 0.0;     ///< 角度零值
}

struct ActionInfoAngle::Points
{
    DmVector point1;
    DmVector point2;

    DmVector intersection;
};

/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionInfoAngle::ActionInfoAngle(DmDocument* doc, GuiDocumentView* docView) :
    PreviewActionInterface("Info Angle", doc, docView), entity1(nullptr), entity2(nullptr),
    pPoints(new Points{})
{
    actionType = DM::ActionInfoAngle;
}

ActionInfoAngle::~ActionInfoAngle()
{
    unhighlightEntity();
}

/// @brief 初始化 Action 状态
/// @param status 初始状态
void ActionInfoAngle::init(int status)
{
    ActionInterface::init(status);
}

void ActionInfoAngle::unhighlightEntity()
{
    if (prevHighlighted)
    {
        prevHighlighted->setHighlighted(false);
        docView->specifyDocumentModified();
        docView->redraw();
        prevHighlighted = nullptr;
    }
}

/// @brief 触发角度计算和显示
void ActionInfoAngle::trigger()
{
    if (entity1 && entity2)
    {
        const DmVectorSolutions& sol = Information::getIntersection(entity1, entity2, false);

        if (sol.hasValid())
        {
            pPoints->intersection = sol.get(0);

            if (pPoints->intersection.valid && pPoints->point1.valid && pPoints->point2.valid)
            {
                double startAngle = pPoints->intersection.angleTo(pPoints->point1);
                double endAngle = pPoints->intersection.angleTo(pPoints->point2);
                double angle = remainder(endAngle - startAngle, TWO_PI);

                QString str = DmUnits::formatAngle(angle, pDocument->getAngleFormat(), pDocument->getAnglePrecision());

                if (angle < ANGLE_ZERO)
                {
                    str += " or ";
                    str += DmUnits::formatAngle(angle + TWO_PI, pDocument->getAngleFormat(), pDocument->getAnglePrecision());
                }
                GUIDIALOGFACTORY->commandMessage(tr("Angle: %1").arg(str));
            }
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(tr("Lines are parallel"));
        }
    }
}

/// @brief 鼠标移动事件处理，实现实体高亮
/// @param e 鼠标事件
void ActionInfoAngle::mouseMoveEvent(QMouseEvent* e)
{
    DmEntity* se = catchEntity(e, { DM::EntityLine, DM::EntityPolyline }, DM::ResolveAllButTextImage);
    switch (getStatus())
    {
        case SetEntity1:
        {
            if (se != prevHighlighted)
            {
                unhighlightEntity();
                entity1 = se;
                if (entity1)
                {
                    entity1->setHighlighted(true);
                    docView->specifyDocumentModified();
                    docView->redraw();
                    prevHighlighted = entity1;
                }
            }
        }
        break;

        case SetEntity2:
        {
            if (se != prevHighlighted)
            {
                if (prevHighlighted && prevHighlighted != entity1)
                {
                    prevHighlighted->setHighlighted(false);
                    docView->specifyDocumentModified();
                    docView->redraw();
                }
                entity2 = se;
                if (entity2 && entity2 != entity1)
                {
                    entity2->setHighlighted(true);
                    docView->specifyDocumentModified();
                    docView->redraw();
                    prevHighlighted = entity2;
                }
                else
                {
                    prevHighlighted = entity1;
                }
            }
            else
            {
                entity2 = se;
            }
        }
        break;

        default:
            break;
    }
}

/// @brief 鼠标释放事件处理
/// @param e 鼠标事件
void ActionInfoAngle::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        DmVector mouse{docView->toGraphX(e->x()), docView->toGraphY(e->y())};

        switch (getStatus())
        {
            case SetEntity1:
                entity1 = catchEntity(e, DM::ResolveAll);
                if (entity1 && (entity1->getEntityType() == DM::EntityLine || entity1->getEntityType() == DM::EntityPolyline))
                {
                    pPoints->point1 = entity1->getNearestPointOnEntity(mouse);
                    setStatus(SetEntity2);
                }
                break;

            case SetEntity2:
                entity2 = catchEntity(e, DM::ResolveAll);
                if (entity2 && (entity2->getEntityType() == DM::EntityLine || entity2->getEntityType() == DM::EntityPolyline))
                {
                    pPoints->point2 = entity2->getNearestPointOnEntity(mouse);
                    setStatus(SetEntity1);
                    trigger();
                }
                break;

            default:
                break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        unhighlightEntity();
        deletePreview();
        init(getStatus() - 1);
    }
}

/// @brief 更新鼠标按钮提示
void ActionInfoAngle::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetEntity1:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify first line"), tr("Cancel"));
            break;
        case SetEntity2:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify second line"), tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

/// @brief 更新鼠标光标
void ActionInfoAngle::updateMouseCursor()
{
    docView->setMouseCursor(DM::SelectCursor);
}
