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


/// @file ActionDrawLineFree.cpp
/// @brief 自由手绘线交互动作类实现

#include <QAction>
#include "ActionDrawLineFree.h"

#include <QMouseEvent>

#include "DmPolyline.h"
#include "DmLine.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Preview.h"
#include "Transaction.h"

namespace
{
    constexpr int MIN_VERTICES_COUNT = 2;            ///< 自由手绘线最少顶点数阈值
    constexpr double MIN_SNAP_DISTANCE_SQUARED = 1.0; ///< 鼠标移动最短距离平方阈值
}

/// @brief 构造函数，初始化自由手绘线动作
/// @param doc 文档对象指针
/// @param docView 文档视图指针
ActionDrawLineFree::ActionDrawLineFree(DmDocument* doc, GuiDocumentView* docView)
    : PreviewActionInterface("Draw freehand lines", doc, docView)
    , vertex(new DmVector{})
{
    preview->getEntityContainer()->setOwner(false);
    actionType = DM::ActionDrawLineFree;
}

/// @brief 析构函数
ActionDrawLineFree::~ActionDrawLineFree() = default;

/// @brief 触发动作完成，结束当前绘制并将有效的折线添加到文档
void ActionDrawLineFree::trigger()
{
    deleteSnapper();

    if (polyline.get())
    {
        deletePreview();

        const DmVectorSolutions sol = polyline->getRefPoints();

        if (sol.getNumber() > MIN_VERTICES_COUNT)
        {
            Transaction t(tr("Create LineFree").toStdString(), pDocument);
            t.start();
            DmEntity* ent = polyline->clone();
            ent->update();
            ent->setDocument(pDocument);
            pDocument->getEntityTable()->add(ent);
            t.commit();
        }
        else
        {
            // 顶点数不足，不创建实体
        }
        polyline.reset();
    }
    else
    {
        // 折线为空，无需处理
    }
    setStatus(SetStartpoint);
}

/// @brief 鼠标移动事件处理，拖拽状态下根据鼠标位置追加折线顶点
/// @param e 鼠标事件指针
void ActionDrawLineFree::mouseMoveEvent(QMouseEvent* e)
{
    DmVector v = snapPoint(e);
    drawSnapper();

    if (getStatus() == Dragging && polyline.get())
    {
        if ((docView->toGui(v) - docView->toGui(*vertex)).squared() < MIN_SNAP_DISTANCE_SQUARED)
        {

            // 不添加相同的鼠标位置
            return;
        }
        else
        {
            polyline->appendVertex(v);
            DmLine* line = new DmLine(preview->getEntityContainer(), *vertex, v);
            line->setDocument(pDocument);
            preview->appendEntity(line);
            drawPreview();
            *vertex = v;
        }
    }
    else
    {
        // 非拖拽状态或无折线，不处理
    }
}

/// @brief 鼠标按下事件处理，开始新的自由手绘
/// @param e 鼠标事件指针
void ActionDrawLineFree::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        switch (getStatus())
        {
            case SetStartpoint:
                setStatus(Dragging);

                // fall-through：贯穿至下一个 case
            case Dragging:
                *vertex = snapPoint(e);
                polyline.reset(new DmPolyline(nullptr, PolylineData()));
                polyline->setDocument(pDocument);
                polyline->appendVertex(*vertex);
                break;
            default:
                break;
        }
    }
    else
    {
        // 非左键按下，不处理
    }
}

/// @brief 鼠标释放事件处理，结束当前绘制
/// @param e 鼠标事件指针
void ActionDrawLineFree::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        if (getStatus() == Dragging)
        {
            *vertex = {};
            trigger();
        }
        else
        {
            // 非拖拽状态，不处理释放
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        if (polyline.get())
        {
            polyline.reset();
        }
        else
        {
            // 折线为空，无需重置
        }
        init(getStatus() - 1);
    }
    else
    {
        // 其他鼠标按钮，不处理
    }
}

/// @brief 更新鼠标按钮提示信息
void ActionDrawLineFree::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetStartpoint:
        case Dragging:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Click and drag to draw a line"), tr("Cancel"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

/// @brief 更新鼠标光标样式
void ActionDrawLineFree::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}
