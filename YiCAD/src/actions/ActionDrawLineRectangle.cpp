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


/// @file ActionDrawLineRectangle.cpp
/// @brief 矩形绘制动作类的实现

#include "ActionDrawLineRectangle.h"

#include <QAction>
#include <QMouseEvent>

#include "Debug.h"
#include "DmLine.h"
#include "DmPolyline.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Preview.h"
#include "Transaction.h"

namespace
{
    /// @brief 矩形顶点数量
    constexpr int RECTANGLE_VERTEX_COUNT = 4;

    /// @brief 矩形多段线权重数量（每顶点2个权重）
    constexpr int RECTANGLE_WEIGHT_COUNT = 8;
}

struct ActionDrawLineRectangle::Points
{
    DmVector corner1; ///< 第一个角点
    DmVector corner2; ///< 第二个角点
};

/// @brief 构造函数
/// @param doc 文档对象指针
/// @param docView 文档视图指针
ActionDrawLineRectangle::ActionDrawLineRectangle(
        DmDocument* doc, GuiDocumentView* docView)
    : PreviewActionInterface("Draw rectangles", doc, docView)
    , pPoints(new Points{})
{
    actionType = DM::ActionDrawLineRectangle;
}

/// @brief 析构函数
ActionDrawLineRectangle::~ActionDrawLineRectangle() = default;

/// @brief 触发动作执行，创建矩形并添加到文档
void ActionDrawLineRectangle::trigger()
{
    PreviewActionInterface::trigger();

    // TODO: 变量 weights 声明后未使用，请确认是否需要删除
    std::vector<double> weights(RECTANGLE_WEIGHT_COUNT, 0.0);
    DmPolyline* polyline = createRectangle(nullptr);
    polyline->setDocument(pDocument);
    polyline->update();
    Transaction t(tr("Create line rectangle").toStdString(), pDocument);
    t.start();
    pDocument->getEntityTable()->add(polyline);
    t.commit();

    docView->moveRelativeZero(pPoints->corner2);
}

/// @brief 鼠标移动事件处理，实时预览矩形
/// @param e 鼠标事件指针
void ActionDrawLineRectangle::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = snapPoint(e);
    SnapMode* mode = getSnapMode();

    if (DM::RestrictOrthogonal == mode->restriction)
    {
        mouse = docView->toGraph(e->x(), e->y());
    }

    if (SetCorner2 == getStatus() && pPoints->corner1.valid)
    {
        pPoints->corner2 = mouse;
        deletePreview();
        DmPolyline* poly = createRectangle(preview->getEntityContainer());
        poly->setDocument(pDocument);
        poly->update();
        preview->addEntity(poly);
        drawPreview();
    }
}

/// @brief 鼠标释放事件处理，确定角点位置
/// @param e 鼠标事件指针
void ActionDrawLineRectangle::mouseReleaseEvent(QMouseEvent* e)
{
    if (Qt::LeftButton == e->button())
    {
        SnapMode* mode = getSnapMode();
        DmVector pos = snapPoint(e);

        if (DM::RestrictOrthogonal == mode->restriction)
        {
            pos = docView->toGraph(e->x(), e->y());
        }

        GuiCoordinateEvent ce(pos);
        coordinateEvent(&ce);
    }
    else if (Qt::RightButton == e->button())
    {
        deletePreview();
        init(getStatus() - 1);
    }
    else
    {
        // 其他按钮不处理
    }
}

/// @brief 坐标输入事件处理，设置角点并触发绘制
/// @param e 坐标事件指针
void ActionDrawLineRectangle::coordinateEvent(GuiCoordinateEvent* e)
{
    if (!e)
    {
        return;
    }

    DmVector mouse = e->getCoordinate();

    switch (getStatus())
    {
        case SetCorner1:
            pPoints->corner1 = mouse;
            docView->moveRelativeZero(mouse);
            setStatus(SetCorner2);
            break;

        case SetCorner2:
            pPoints->corner2 = mouse;
            trigger();
            finishOrthogonal();
            setStatus(SetCorner1);
            break;

        default:
            break;
    }
}

/// @brief 命令事件处理
/// @param e 命令事件指针
void ActionDrawLineRectangle::commandEvent(GuiCommandEvent* e)
{
    if (!e)
    {
        return;
    }

    QString const& c = e->getCommand().toLower();

    if (checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(
            msgAvailableCommands() + getAvailableCommands().join(", "));
        return;
    }
}

/// @brief 更新鼠标按钮提示文本
void ActionDrawLineRectangle::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetCorner1:
            GUIDIALOGFACTORY->updateMouseWidget(
                tr("Specify first corner"), tr("Cancel"));
            break;

        case SetCorner2:
            GUIDIALOGFACTORY->updateMouseWidget(
                tr("Specify second corner"), tr("Back"));
            break;

        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

/// @brief 更新鼠标光标样式
void ActionDrawLineRectangle::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}

/// @brief 创建矩形多段线
/// @param container 实体容器指针，可为 nullptr
/// @return 创建的矩形多段线指针
DmPolyline* ActionDrawLineRectangle::createRectangle(
        DmEntityContainer* container)
{
    DmVector p1(pPoints->corner1);
    DmVector p2(pPoints->corner2.x, pPoints->corner1.y);
    DmVector p3(pPoints->corner2);
    DmVector p4(pPoints->corner1.x, pPoints->corner2.y);

    std::vector<DmVector> pts{ p1, p2, p3, p4 };
    std::vector<double> bulges(RECTANGLE_VERTEX_COUNT, 0.0);
    std::vector<double> weights(RECTANGLE_WEIGHT_COUNT, 0.0);

    DmPolyline* polyline = new DmPolyline(
        container, PolylineData(pts, bulges, weights, true));

    return polyline;
}
