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


/// @file ActionDrawLineBisector.cpp
/// @brief 角平分线绘制操作类实现

#include <QAction>
#include "ActionDrawLineBisector.h"

#include <QMouseEvent>

#include "Debug.h"
#include "DmLine.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Math2d.h"
#include "Preview.h"
#include "Information.h"
#include "Transaction.h"

constexpr int MAX_BISECTOR_COUNT = 200; ///< 角平分线最大创建数量

struct ActionDrawLineBisector::Points
{
    DmVector coord1; ///< 选择第一条线时的鼠标位置
    DmVector coord2; ///< 选择第二条线时的鼠标位置
};

/// @brief 构造函数
/// @param [in] doc 文档指针
/// @param [in] docView 文档视图指针
ActionDrawLineBisector::ActionDrawLineBisector(DmDocument* doc, GuiDocumentView* docView) :
    PreviewActionInterface("Draw Bisectors", doc, docView), bisector(nullptr), line1(nullptr), line2(nullptr),
    length(10.0), number(1), pPoints(new Points{}), lastStatus(SetLine1)
{
    actionType = DM::ActionDrawLineBisector;
}

/// @brief 析构函数
ActionDrawLineBisector::~ActionDrawLineBisector() = default;

/// @brief 设置角平分线长度
/// @param [in] l 长度值
void ActionDrawLineBisector::setLength(double l)
{
    length = l;
}

/// @brief 获取角平分线长度
/// @return 当前长度值
double ActionDrawLineBisector::getLength() const
{
    return length;
}

/// @brief 设置角平分线数量
/// @param [in] n 数量值
void ActionDrawLineBisector::setNumber(int n)
{
    number = n;
}

/// @brief 获取角平分线数量
/// @return 当前数量值
int ActionDrawLineBisector::getNumber() const
{
    return number;
}

/// @brief 创建角平分线实体
/// @param [in] container 实体容器
/// @param [in] coord1 第一条线的鼠标坐标
/// @param [in] coord2 第二条线的鼠标坐标
/// @param [in] length 角平分线长度
/// @param [in] num 角平分线数量
/// @param [in] l1 第一条线
/// @param [in] l2 第二条线
/// @return 生成的角平分线列表
std::vector<DmLine*> ActionDrawLineBisector::createBisector(DmEntityContainer* container,
                                                             const DmVector& coord1,
                                                             const DmVector& coord2,
                                                             double length,
                                                             int num,
                                                             DmLine* l1,
                                                             DmLine* l2)
{
    std::vector<DmLine*> res;
    if (!(l1 && l2))
    {
        return res;
    }
    if (!(l1->getEntityType() == DM::EntityLine && l2->getEntityType() == DM::EntityLine))
    {
        return res;
    }

    // intersection between entities:
    DmVectorSolutions const& sol = Information::getIntersection(l1, l2, false);
    DmVector inters = sol.get(0);
    if (!inters.valid)
    {
        return res;
    }

    double startAngle = inters.angleTo(l1->getNearestPointOnEntity(coord1));
    double endAngle = inters.angleTo(l2->getNearestPointOnEntity(coord2));
    double angleDiff = Math2d::getAngleDifference(startAngle, endAngle);
    if (angleDiff > M_PI)
    {
        angleDiff = angleDiff - 2. * M_PI;
    }

    for (int n = 1; n <= num; ++n)
    {
        double angle = startAngle + (angleDiff / (num + 1) * n);
        DmVector const& v = DmVector::polar(length, angle);
        DmLine* newLine = new DmLine(container, inters, inters + v);
        newLine->setDocument(pDocument);
        res.emplace_back(newLine);
    }
    return res;
}

/// @brief 初始化操作状态
/// @param [in] status 初始状态值
void ActionDrawLineBisector::init(int status)
{
    PreviewActionInterface::init(status);
    if (status >= 0)
    {
        Snapper::suspend();
    }

    if (status < SetLine2)
    {
        if (line2 && line2->isHighlighted())
        {
            line2->setHighlighted(false);
        }
        if (status < 0 && line1 && line1->isHighlighted())
        {
            line1->setHighlighted(false);
        }
        docView->specifyDocumentModified();
        docView->redraw();
    }
}

/// @brief 执行绘制操作，生成角平分线并添加到文档
void ActionDrawLineBisector::trigger()
{
    PreviewActionInterface::trigger();

    for (auto p : {line1, line2})
    {
        if (p && p->isHighlighted())
        {
            p->setHighlighted(false);
        }
    }

    Transaction t(tr("Add cloud line").toStdString(), pDocument);
    t.start();
    auto ents = createBisector(nullptr, pPoints->coord1, pPoints->coord2, length, number, line1, line2);
    for (auto e : ents)
    {
        pDocument->getEntityTable()->add(e);
    }
    t.commit();
}

/// @brief 处理鼠标移动事件
/// @param [in] e 鼠标事件指针
void ActionDrawLineBisector::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = DmVector(docView->toGraphX(e->x()), docView->toGraphY(e->y()));

    switch (getStatus())
    {
        case SetLine1:
        {
            DmEntity* en = catchEntity(e, DM::ResolveAll);
            static DmEntity* line = nullptr;
            if (en && en->getEntityType() == DM::EntityLine)
            {
                if ((line != nullptr && line != en) || line == nullptr)
                {
                    if (line && line->isHighlighted())
                    {
                        line->setHighlighted(false);
                    }
                    line = static_cast<DmLine*>(en);
                    line->setHighlighted(true);
                    docView->specifyDocumentModified();
                    docView->redraw();
                }
            }
            else
            {
                if (line && line->isHighlighted())
                {
                    line->setHighlighted(false);
                    docView->specifyDocumentModified();
                    docView->redraw();
                }
            }
        }
        break;

        case SetLine2:
        {
            pPoints->coord2 = mouse;
            DmEntity* en    = catchEntity(e, DM::ResolveAll);
            if (en == line1)
            {
                break;
            }
            if (en && en->getEntityType() == DM::EntityLine)
            {
                if (line2 && line2->isHighlighted())
                {
                    line2->setHighlighted(false);
                }
                line2 = static_cast<DmLine*>(en);
                line2->setHighlighted(true);
                docView->specifyDocumentModified();
                docView->redraw();

                deletePreview();
                std::vector<DmLine*> lines = createBisector(preview->getEntityContainer(), pPoints->coord1, pPoints->coord2, length, number, line1, line2);
                for (auto e : lines)
                {
                    preview->addEntity(e);
                }
                drawPreview();
            }
            else
            {
                deletePreview();
                if (line2 && line2->isHighlighted())
                {
                    line2->setHighlighted(false);
                    docView->specifyDocumentModified();
                    docView->redraw();
                }
                line2 = nullptr;
            }
        }
        break;

        default:
            break;
    }
}

/// @brief 处理鼠标释放事件
/// @param [in] e 鼠标事件指针
void ActionDrawLineBisector::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::RightButton)
    {
        deletePreview();
        init(getStatus() - 1);
    }
    else
    {
        DmVector mouse = DmVector(docView->toGraphX(e->x()), docView->toGraphY(e->y()));

        switch (getStatus())
        {
            case SetLine1:
            {
                pPoints->coord1 = mouse;
                DmEntity* en    = catchEntity(e, DM::ResolveAll);
                if (en && en->getEntityType() == DM::EntityLine)
                {
                    line1 = static_cast<DmLine*>(en);
                    line1->setHighlighted(true);
                    docView->specifyDocumentModified();
                    docView->redraw();
                    line2 = nullptr;
                    setStatus(SetLine2);
                }
            }
            break;

            case SetLine2:
            {
                pPoints->coord2 = mouse;
                trigger();
                setStatus(SetLine1);
            }
            break;

            default:
                break;
        }
    }
}

/// @brief 处理命令行事件
/// @param [in] e 命令事件指针
void ActionDrawLineBisector::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus())
    {
        case SetLine1:
        case SetLine2:
            lastStatus = static_cast<Status>(getStatus());
            if (checkCommand("length", c))
            {
                deletePreview();
                setStatus(SetLength);
            }
            else if (checkCommand("number", c))
            {
                deletePreview();
                setStatus(SetNumber);
            }
            else
            {
                // 未识别的命令，不作处理
            }
            break;

        case SetLength:
        {
            bool ok = false;
            double l = Math2d::eval(c, &ok);
            if (ok)
            {
                e->accept();
                length = l;
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            GUIDIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

        case SetNumber:
        {
            bool ok = false;
            int n = static_cast<int>(Math2d::eval(c, &ok));
            if (ok)
            {
                e->accept();
                if (n > 0 && n <= MAX_BISECTOR_COUNT)
                {
                    number = n;
                }
                else
                {
                    GUIDIALOGFACTORY->commandMessage(
                        tr("Number sector lines not in range: ", "number of bisector to create must be in [1, 200]") +
                        QString::number(n));
                }
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            GUIDIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

        default:
            break;
    }
}

/// @brief 获取可用命令列表
/// @return 可用命令的字符串列表
QStringList ActionDrawLineBisector::getAvailableCommands()
{
    QStringList cmd;

    switch (getStatus())
    {
        case SetLine1:
        case SetLine2:
            cmd += command("length");
            cmd += command("number");
            break;
        default:
            break;
    }

    return cmd;
}

/// @brief 更新鼠标按钮提示
void ActionDrawLineBisector::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetLine1:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select first line"), tr("Cancel"));
            break;
        case SetLine2:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select second line"), tr("Back"));
            break;
        case SetLength:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Enter bisector length:"), tr("Back"));
            break;
        case SetNumber:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Enter number of bisectors:"), tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

/// @brief 显示选项界面
void ActionDrawLineBisector::showOptions()
{
    ActionInterface::showOptions();

    GUIDIALOGFACTORY->requestOptions(this, true);
}

/// @brief 隐藏选项界面
void ActionDrawLineBisector::hideOptions()
{
    ActionInterface::hideOptions();

    GUIDIALOGFACTORY->requestOptions(this, false);
}

/// @brief 更新鼠标光标样式
void ActionDrawLineBisector::updateMouseCursor()
{
    docView->setMouseCursor(DM::SelectCursor);
}

// EOF
