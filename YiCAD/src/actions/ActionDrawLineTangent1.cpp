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


/// @file ActionDrawLineTangent1.cpp
/// @brief 点到圆的切线绘制交互动作的实现

#include "ActionDrawLineTangent1.h"

#include <QAction>
#include <QMouseEvent>

#include "Debug.h"
#include "DmLine.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Preview.h"
#include "Transaction.h"
#include "GeUtility.h"

/// @brief 构造函数
/// @param doc CAD 文档指针
/// @param docView 文档视图指针
ActionDrawLineTangent1::ActionDrawLineTangent1(DmDocument* doc,
                                               GuiDocumentView* docView)
    : PreviewActionInterface("Draw Tangents 1", doc, docView)
    , tangent(nullptr)
    , point(new DmVector{})
    , circle(nullptr)
{
    actionType = DM::ActionDrawLineTangent1;
}

/// @brief 析构函数
ActionDrawLineTangent1::~ActionDrawLineTangent1() = default;

/// @brief 结束动作，清除高亮并调用父类结束
/// @param updateTB 是否更新工具栏
void ActionDrawLineTangent1::finish(bool updateTB)
{
    if (circle)
    {
        circle->setHighlighted(false);
    }
    PreviewActionInterface::finish(updateTB);
}

/// @brief 创建切线
/// @param container 实体容器指针
/// @param coord 当前鼠标坐标
/// @param point 起点坐标
/// @param circle 目标圆或弧实体
/// @return 创建的切线实体，失败返回 nullptr
DmLine* ActionDrawLineTangent1::createTangent1(DmEntityContainer* container,
                                               const DmVector& coord,
                                               const DmVector& point,
                                               DmEntity* circle)
{
    DmLine* ret = nullptr;
    if (!(circle && point.valid))
    {
        return nullptr;
    }
    if (!(GeUtility::isEntityArc(circle)))
    {
        return nullptr;
    }

    // 计算两个切点
    DmVectorSolutions sol = circle->getTangentPoint(point);

    if (!sol.getNumber())
    {
        return nullptr;
    }
    DmVector const vp2(sol.getClosest(coord));
    LineData d;
    if ((vp2 - point).squared() > DM_TOLERANCE2)
    {
        d = LineData(vp2, point);
    }
    else
    {
        d = LineData(point + circle->getTangentDirection(point), point);
    }
    ret = new DmLine(container, d);
    ret->setDocument(pDocument);
    return ret;
}

/// @brief 触发动作，创建切线实体
void ActionDrawLineTangent1::trigger()
{
    PreviewActionInterface::trigger();

    if (tangent)
    {
        DmEntity* newEntity = new DmLine(nullptr, tangent->getData());
        if (circle)
        {
            circle->setHighlighted(false);
            docView->specifyDocumentModified();
            docView->redraw();
        }

        newEntity->setDocument(pDocument);
        Transaction t(tr("Create line tangent").toStdString(), pDocument);
        t.start();
        pDocument->getEntityTable()->add(newEntity);
        t.commit();
        setStatus(SetPoint);
        tangent.reset();
    }
}

/// @brief 鼠标移动事件处理
/// @param e 鼠标事件
void ActionDrawLineTangent1::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse(docView->toGraphX(e->x()), docView->toGraphY(e->y()));

    switch (getStatus())
    {
        case SetPoint:
            *point = snapPoint(e);
            break;

        case SetCircle:
        {
            DmEntity* en = catchEntity(e,
                                       EntityTypeList{ DM::EntityArc,
                                                       DM::EntityCircle,
                                                       DM::EntityEllipse },
                                       DM::ResolveAll);
            if (en)
            {
                if (circle)
                {
                    circle->setHighlighted(false);
                }
                circle = en;
                circle->setHighlighted(true);
                docView->specifyDocumentModified();
                docView->redraw();

                tangent.reset(createTangent1(nullptr,
                                             mouse, *point, circle));

                if (tangent)
                {
                    deletePreview();
                    auto cloneEnt = tangent->clone();
                    cloneEnt->setParent(preview->getEntityContainer());
                    preview->addEntity(cloneEnt);
                    drawPreview();
                }
            }
        }
        break;

        default:
            break;
    }
}

/// @brief 鼠标释放事件处理
/// @param e 鼠标事件
void ActionDrawLineTangent1::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::RightButton)
    {
        deletePreview();
        if (circle)
        {
            circle->setHighlighted(false);
            docView->specifyDocumentModified();
            docView->redraw();
        }
        init(getStatus() - 1);
    }
    else
    {
        switch (getStatus())
        {
            case SetPoint:
            {
                GuiCoordinateEvent ce(snapPoint(e));
                coordinateEvent(&ce);
            }
            break;

            case SetCircle:
                if (tangent)
                {
                    trigger();
                }
                break;

            default:
                break;
        }
    }
}

/// @brief 坐标输入事件处理
/// @param e 坐标事件
void ActionDrawLineTangent1::coordinateEvent(GuiCoordinateEvent* e)
{
    if (!e)
    {
        return;
    }
    switch (getStatus())
    {
        case SetPoint:
            *point = e->getCoordinate();
            docView->moveRelativeZero(*point);
            setStatus(SetCircle);
            break;

        default:
            break;
    }
}

/// @brief 更新鼠标按钮提示
void ActionDrawLineTangent1::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetPoint:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify point"), tr("Cancel"));
            break;
        case SetCircle:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select circle, arc or ellipse"), tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

/// @brief 更新鼠标光标样式
void ActionDrawLineTangent1::updateMouseCursor()
{
    switch (getStatus())
    {
        case SetPoint:
            docView->setMouseCursor(DM::CadCursor);
            break;
        case SetCircle:
            docView->setMouseCursor(DM::SelectCursor);
            break;
        default:
            break;
    }
}
