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


/// @file ActionDrawText.cpp
/// @brief 文本绘制 Action 类的实现

#include <QAction>
#include "ActionDrawText.h"

#include <QMouseEvent>

#include "Debug.h"
#include "DmLine.h"
#include "DmText.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "DmDocument.h"
#include "Preview.h"
#include "Transaction.h"

struct ActionDrawText::Points
{
    DmVector pos;
    DmVector secPos;
};

/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionDrawText::ActionDrawText(DmDocument* doc, GuiDocumentView* docView)
    : PreviewActionInterface("Draw Text", doc, docView)
    , pPoints(new Points{})
    , textChanged(true)
{
    actionType = DM::ActionDrawText;
    reset();
}

ActionDrawText::~ActionDrawText() = default;

/// @brief 初始化 Action 状态
/// @param status 初始状态
void ActionDrawText::init(int status)
{
    ActionInterface::init(status);

    switch (status)
    {
    case ShowDialog:
    {
        reset();

        DmText tmp(nullptr, *data);
        tmp.setDocument(pDocument);
        if (GUIDIALOGFACTORY->requestTextDialog(&tmp))
        {
            data.reset(new TextData(tmp.getData()));
            setStatus(SetPos);
            showOptions();
        }
        else
        {
            hideOptions();
            setFinished();
        }
    }
    break;

    case SetPos:
        GUIDIALOGFACTORY->requestOptions(this, true, true);
        deletePreview();
        preview->setVisible(true);
        preparePreview();
        break;

    default:
        break;
    }
}

/// @brief 重置数据到默认状态
void ActionDrawText::reset()
{
    const QString text = data.get() ? data->getTextString() : "";
    DmTextStyle* pStyle = pDocument->getTextStyleTable()->getActive();
    data.reset(new TextData(DmVector(0.0, 0.0), 1.0, ETextVertMode::kTextBase,
        ETextHorzMode::kTextLeft, text, pStyle, 0.0, EUpdateMode::Update));
}

/// @brief 触发文本创建操作
void ActionDrawText::trigger()
{
    if (pPoints->pos.valid)
    {
        deletePreview();

        Transaction t(tr("Create Text").toStdString(), pDocument);
        t.start();
        DmText* text = new DmText(nullptr, *data);
        text->setDocument(pDocument);
        text->update();
        pDocument->getEntityTable()->add(text);
        t.commit();

        textChanged = true;
        pPoints->secPos = {};
        setStatus(SetPos);
    }
}

/// @brief 准备预览内容
void ActionDrawText::preparePreview()
{
    if (data->getTextMode() != ETextMode::kTextAligned && data->getTextMode() != ETextMode::kTextFit)
    {
        setDataWithOnePoint();
        DmText* text = new DmText(preview->getEntityContainer(), *data);
        text->setDocument(pDocument);
        text->update();
        preview->addEntity(text);
        textChanged = false;
    }
}

/// @brief 鼠标移动事件处理
/// @param e 鼠标事件
void ActionDrawText::mouseMoveEvent(QMouseEvent* e)
{
    if (getStatus() == SetPos)
    {
        DmVector mouse = snapPoint(e);
        DmVector mov = mouse - pPoints->pos;
        pPoints->pos = mouse;
        if (data->getTextMode() != ETextMode::kTextAligned && data->getTextMode() != ETextMode::kTextFit)
        {
            if (textChanged || pPoints->pos.valid == false || preview->isEmpty())
            {
                deletePreview();
                preparePreview();
            }
            else
            {
                preview->move(mov);
            }
            preview->setVisible(true);
        }
        drawPreview();
    }
    else if (getStatus() == SetSecPos)
    {
        pPoints->secPos = snapPoint(e);
        deletePreview();
        preview->appendEntity(new DmLine(nullptr, LineData(pPoints->pos, pPoints->secPos)));
        drawPreview();
    }
}

/// @brief 鼠标释放事件处理
/// @param e 鼠标事件
void ActionDrawText::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        GuiCoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton)
    {
        deletePreview();
        if (getStatus() == SetSecPos)
        {
            init(getStatus() - 1);
        }
        else
        {
            finish(false);
        }
    }
}

/// @brief 坐标事件处理
/// @param e 坐标事件
void ActionDrawText::coordinateEvent(GuiCoordinateEvent* e)
{
    if (e == nullptr)
    {
        return;
    }

    DmVector mouse = e->getCoordinate();

    switch (getStatus())
    {
    case ShowDialog:
        break;

    case SetPos:
        pPoints->pos = mouse;
        // 对齐和布满需要选择2个点
        if (data->getTextMode() == ETextMode::kTextAligned || data->getTextMode() == ETextMode::kTextFit)
        {
            data->setAlignment(mouse);
            setStatus(SetSecPos);
        }
        else
        {
            setDataWithOnePoint();
            trigger();
        }
        break;

    case SetSecPos:
    {
        DmVector tmp = data->getAlignment();
        data->setAlignment(mouse);
        data->setPosition(tmp);
        trigger();
    }
        break;

    default:
        break;
    }
}

/// @brief 命令事件处理
/// @param e 命令事件
void ActionDrawText::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus())
    {
    case SetPos:
        if (checkCommand("text", c))
        {
            deletePreview();
            docView->disableCoordinateInput();
            setStatus(SetText);
        }
        break;

    case SetText:
    {
        setText(e->getCommand());
        GUIDIALOGFACTORY->requestOptions(this, true, true);
        docView->enableCoordinateInput();
        setStatus(SetPos);
    }
        break;

    default:
        break;
    }
}

/// @brief 获取可用命令列表
/// @return 命令字符串列表
QStringList ActionDrawText::getAvailableCommands()
{
    QStringList cmd;
    if (getStatus() == SetPos)
    {
        cmd += command("text");
    }
    return cmd;
}

/// @brief 显示选项面板
void ActionDrawText::showOptions()
{
    ActionInterface::showOptions();

    GUIDIALOGFACTORY->requestOptions(this, true, true);
}

/// @brief 隐藏选项面板
void ActionDrawText::hideOptions()
{
    ActionInterface::hideOptions();

    GUIDIALOGFACTORY->requestOptions(this, false);
}

/// @brief 更新鼠标按钮提示
void ActionDrawText::updateMouseButtonHints()
{
    switch (getStatus())
    {
    case SetPos:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Specify insertion point"), tr("Cancel"));
        break;
    case SetSecPos:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Specify second point"), tr("Cancel"));
        break;
    case ShowDialog:
    case SetText:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Enter text:"), tr("Back"));
        break;
    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

/// @brief 更新鼠标光标
void ActionDrawText::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}

/// @brief 设置文本内容
/// @param t 文本字符串
void ActionDrawText::setText(const QString& t)
{
    data->setTextString(t);
    textChanged = true;
}

/// @brief 获取文本内容
/// @return 文本字符串
QString ActionDrawText::getText() const
{
    return data->getTextString();
}

/// @brief 设置文本角度
/// @param a 角度值
void ActionDrawText::setAngle(double a)
{
    data->setAngle(a);
    textChanged = true;
}

/// @brief 获取文本角度
/// @return 角度值
double ActionDrawText::getAngle() const
{
    return data->getAngle();
}

/// @brief 对单点定位模式设置位置和对齐数据
void ActionDrawText::setDataWithOnePoint()
{
    if (data->getTextMode() != ETextMode::kTextAligned && data->getTextMode() != ETextMode::kTextFit)
    {
        if (data->getTextMode() == ETextMode::kTextLeft)
        {
            // 左对齐的对齐点为0，位置随Position
            data->setPosition(pPoints->pos);
            data->setAlignment(DmVector(0.0, 0.0));
        }
        else
        {
            data->setAlignment(pPoints->pos);
        }
    }
}
