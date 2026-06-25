/****************************************************************************
**
 * This action class can handle user events to draw tangents normal to lines

Copyright (C) 2011-2012 Dongxu Li (dongxuli2011@gmail.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/


/// @file ActionDrawLineOrthTan.cpp
/// @brief 正交切线绘制动作类实现

#include <QAction>
#include "ActionDrawLineOrthTan.h"

#include <QMouseEvent>

#include "DmLine.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Preview.h"
#include "Selection.h"
#include "Transaction.h"
#include "GeUtility.h"


/// @brief 构造函数，初始化正交切线绘制动作
ActionDrawLineOrthTan::ActionDrawLineOrthTan(DmDocument* doc, GuiDocumentView* docView) :
    PreviewActionInterface("Draw Tangent Orthogonal", doc, docView),
    normal(nullptr),
    tangent(nullptr),
    circle(nullptr)
{
    actionType = DM::ActionDrawLineOrthTan;
}

/// @brief 完成动作，清理选中实体和预览
void ActionDrawLineOrthTan::finish(bool updateTB)
{
    clearLines();
    PreviewActionInterface::finish(updateTB);
}

/// @brief 触发动作，创建切线实体并添加到文档
void ActionDrawLineOrthTan::trigger()
{
    if (!tangent)
    {
        return;
    }

    PreviewActionInterface::trigger();

    deletePreview();
    if (circle)
    {
        circle->setHighlighted(false);
    }
    circle = nullptr;

    Transaction t(tr("Create LineOrthTan").toStdString(), pDocument);
    t.start();
    DmEntity* newEntity = new DmLine(nullptr, tangent->getData());
    newEntity->setDocument(pDocument);
    pDocument->getEntityTable()->add(newEntity);
    t.commit();

    setStatus(SetCircle);
}

/// @brief 处理鼠标移动事件，根据当前状态选择圆弧或椭圆
void ActionDrawLineOrthTan::mouseMoveEvent(QMouseEvent* e)
{
    e->accept();
    DmVector mouse(docView->toGraphX(e->x()), docView->toGraphY(e->y()));

    switch (getStatus())
    {
        case SetLine:
        {
            return;
        }
        case SetCircle:
        {
            DmEntity* en = catchEntity(e, { DM::EntityArc, DM::EntityCircle, DM::EntityEllipse }, DM::ResolveAll);
            if (!en)
            {
                return;
            }
            if (circle)
            {
                circle->setHighlighted(false);
            }
            circle = en;
            circle->setHighlighted(true);
            docView->specifyDocumentModified();
            docView->redraw();
            deletePreview();
            tangent = createLineOrthTan(mouse, normal, circle);
            auto l = new DmLine(preview->getEntityContainer(), tangent->getData());
            l->setDocument(pDocument);
            preview->addEntity(l);
            drawPreview();
            break;
        }
        default:
        {
            break;
        }
    }
}

/// @brief 创建与给定法线正交的切线
DmLine* ActionDrawLineOrthTan::createLineOrthTan(const DmVector& coord, DmLine* normal, DmEntity* circle)
{
    DmLine* ret = nullptr;

    // check given entities:
    if (!(circle && normal))
    {
        return ret;
    }

    if (!GeUtility::isEntityArc(circle))
    {
        return ret;
    }
    DmVector const& t0 = circle->getNearestOrthTan(coord, *normal, false);
    if (!t0.valid)
    {
        return ret;
    }
    DmVector const& vp = normal->getNearestPointOnEntity(t0, false);
    ret = new DmLine(nullptr, vp, t0);
    ret->setDocument(pDocument);
    return ret;
}

/// @brief 清除选中的直线和预览
void ActionDrawLineOrthTan::clearLines()
{
    for (DmEntity* p : {static_cast<DmEntity*>(normal), circle})
    {
        if (p)
        {
            p->setHighlighted(false);
        }
    }
    docView->specifyDocumentModified();
    docView->redraw();
    if (circle)
    {
        circle = nullptr;
    }
    deletePreview();
}

/// @brief 处理鼠标释放事件
void ActionDrawLineOrthTan::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::RightButton)
    {
        clearLines();
        if (getStatus() == SetLine)
        {
            finish(true);
        }
        else
        {
            init(getStatus() - 1);
        }
    }
    else
    {
        switch (getStatus())
        {
            case SetLine:
            {
                DmEntity* en = catchEntity(e, DM::EntityLine);
                if (en)
                {
                    if (en->getLength() < DM_TOLERANCE)
                    {
                        // ignore lines not long enough
                        break;
                    }
                    if (normal)
                    {
                        normal->setHighlighted(false);
                    }
                    normal = static_cast<DmLine*>(en);
                    normal->setHighlighted(true);
                    docView->specifyDocumentModified();
                    docView->redraw();
                    setStatus(SetCircle);
                }
                break;
            }

            case SetCircle:
            {
                if (tangent)
                {
                    trigger();
                }
                break;
            }

            default:
            {
                break;
            }
        }
    }
}

/// @brief 更新鼠标按钮提示文本
void ActionDrawLineOrthTan::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetLine:
        {
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select a line"), tr("Cancel"));
            break;
        }
        case SetCircle:
        {
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select circle, arc or ellipse"), tr("Back"));
            break;
        }
        default:
        {
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }
}

/// @brief 更新鼠标光标样式
void ActionDrawLineOrthTan::updateMouseCursor()
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

// EOF
