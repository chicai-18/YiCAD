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


/// @file ActionDrawXline.cpp
/// @brief 构造线绘制 Action 类的实现

#include "ActionDrawXline.h"

#include <QMouseEvent>

#include "Debug.h"
#include "DmXline.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Preview.h"
#include "Snapper.h"
#include "Transaction.h"

/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionDrawXline::ActionDrawXline(DmDocument* doc, GuiDocumentView* docView)
    : PreviewActionInterface("Draw ray", doc, docView)
    , data(new XLineData{})
{
    actionType = DM::ActionDrawXline;
}

ActionDrawXline::~ActionDrawXline()
{
}

/// @brief 重置数据
void ActionDrawXline::reset()
{
    data.reset(new XLineData{});
}

/// @brief 初始化 Action 状态
/// @param status 初始状态
void ActionDrawXline::init(int status)
{
    PreviewActionInterface::init(status);

    reset();
}

/// @brief 触发构造线创建操作
void ActionDrawXline::trigger()
{
    PreviewActionInterface::trigger();

    Transaction t(tr("Draw Construction Line").toStdString(), pDocument);
    t.start();
    DmXline* xline = new DmXline(nullptr, *data);
    xline->setDocument(pDocument);
    pDocument->getEntityTable()->add(xline);
    t.commit();

    docView->moveRelativeZero(data->getBasePoint());
}

/// @brief 鼠标移动事件处理
/// @param e 鼠标事件
void ActionDrawXline::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = snapPoint(e);
    if (getStatus() == SetDir && data->getBasePoint().valid)
    {
        deletePreview();
        data->setDirection(mouse - data->getBasePoint());
        DmXline* xline = new DmXline(preview->getEntityContainer(), *data);
        xline->setDocument(pDocument);
        preview->addEntity(xline);
        drawPreview();
    }
}

/// @brief 鼠标释放事件处理
/// @param e 鼠标事件
void ActionDrawXline::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        GuiCoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton)
    {
        deletePreview();
        switch (getStatus())
        {
        default:
        case SetBasePoint:
            init(getStatus() - 1);
            break;

        case SetDir:
            setStatus(SetBasePoint);
            break;
        }
    }
}

/// @brief 坐标事件处理
/// @param e 坐标事件
void ActionDrawXline::coordinateEvent(GuiCoordinateEvent* e)
{
    if (e == nullptr)
    {
        return;
    }
    DmVector mousePos = e->getCoordinate();
    switch (getStatus())
    {
    case SetBasePoint:
        data->setBasePoint(mousePos);
        setStatus(SetDir);
        docView->moveRelativeZero(mousePos);
        break;
    case SetDir:
        data->setDirection(mousePos - data->getBasePoint());
        trigger();
        break;
    default:
        break;
    }
}

/// @brief 命令事件处理
/// @param e 命令事件
void ActionDrawXline::commandEvent(GuiCommandEvent* e)
{
    QString str = e->getCommand().toLower();
    switch (getStatus())
    {
    case SetBasePoint:
        break;

    case SetDir:
        break;

    default:
        break;
    }
}

/// @brief 获取可用命令列表
/// @return 命令字符串列表（当前无可用命令）
QStringList ActionDrawXline::getAvailableCommands()
{
    return {};
}

/// @brief 更新鼠标按钮提示
void ActionDrawXline::updateMouseButtonHints()
{
    switch (getStatus())
    {
    case SetBasePoint:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Specify first point"), tr("Cancel"));
        break;
    case SetDir:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Specify direction"), tr("Back"));
        break;
    default:
        break;
    }
}

/// @brief 显示选项面板
void ActionDrawXline::showOptions()
{
    ActionInterface::showOptions();
}

/// @brief 隐藏选项面板
void ActionDrawXline::hideOptions()
{
    ActionInterface::hideOptions();
}

/// @brief 更新鼠标光标
void ActionDrawXline::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}
