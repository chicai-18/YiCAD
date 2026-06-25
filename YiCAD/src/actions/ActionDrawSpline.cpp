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


/// @file ActionDrawSpline.cpp
/// @brief ActionDrawSpline 类实现，处理样条曲线绘制的交互逻辑

#include <QAction>
#include "ActionDrawSpline.h"

#include <QMouseEvent>

#include "Commands.h"
#include "Debug.h"
#include "DmPoint.h"
#include "DmSpline.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Preview.h"
#include "Transaction.h"
#include "Math2d.h"
#include "DmLine.h"

struct ActionDrawSpline::Points
{
    SplineData data;            ///< 当前已定义的样条曲线数据
    DmSpline* spline = nullptr; ///< 正在操作的样条曲线实体
    QList<DmVector> history;    ///< 控制点历史记录（用于撤销）
};

/// @brief 构造函数
/// @param [in] doc 文档对象指针
/// @param [in] docView 文档视图指针
ActionDrawSpline::ActionDrawSpline(DmDocument* doc, GuiDocumentView* docView) :
    PreviewActionInterface("Draw splines", doc, docView),
    pPoints(new Points{})
{
    actionType = DM::ActionDrawSpline;
    reset();
}

/// @brief 析构函数
ActionDrawSpline::~ActionDrawSpline() = default;

/// @brief 重置交互状态，清除当前样条曲线和历史控制点
void ActionDrawSpline::reset()
{
    pPoints->spline = nullptr;
    pPoints->history.clear();
}

/// @brief 初始化交互状态
/// @param [in] status 初始状态
void ActionDrawSpline::init(int status)
{
    PreviewActionInterface::init(status);

    reset();
}

/// @brief 提交当前样条曲线到文档
void ActionDrawSpline::trigger()
{
    PreviewActionInterface::trigger();

    if (!pPoints->spline)
    {
        return;
    }

    Transaction t(tr("Add spline").toStdString(), pDocument);
    t.start();
    setControlPointsKnotsByClose(
        pPoints->spline, pPoints->spline->isClosed());
    pDocument->getEntityTable()->add(pPoints->spline);
    t.commit();

    // 更新视图
    const DmVector relativeZero = docView->getRelativeZero();
    docView->moveRelativeZero(relativeZero);

    pPoints->spline = nullptr;
}

/// @brief 处理鼠标移动事件
/// @param [in] e 鼠标事件指针
void ActionDrawSpline::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = snapPoint(e);
    // TODO: 确认 point.valid 检查是否需要恢复
    if (getStatus() == SetNextPoint && pPoints->spline)
    {
        deletePreview();
        DmSpline* tmpSpline =
            static_cast<DmSpline*>(pPoints->spline->clone());
        tmpSpline->setDocument(pDocument);
        tmpSpline->setParent(preview->getEntityContainer());
        setControlPointsKnotsByClose(
            tmpSpline, tmpSpline->isClosed(), mouse);
        preview->addEntity(tmpSpline);

        const auto& controlPoints = tmpSpline->getControlPoints();
        for (const DmVector& vp : controlPoints)
        {
            preview->addEntity(new DmPoint(nullptr, PointData(vp)));
        }

        drawPreview();
    }
}

/// @brief 处理鼠标释放事件
/// @param [in] e 鼠标事件指针
void ActionDrawSpline::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        GuiCoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton)
    {
        if (getStatus() == SetNextPoint && pPoints->spline
            && pPoints->spline->getNumberOfControlPoints()
                >= pPoints->spline->getDegree() + 1)
        {
            trigger();
        }
        deletePreview();
        init(getStatus() - 1);
    }
    else
    {
        // 其他鼠标按键暂不处理
    }
}

/// @brief 处理坐标输入事件
/// @param [in] e 坐标事件指针
void ActionDrawSpline::coordinateEvent(GuiCoordinateEvent* e)
{
    if (!e)
    {
        return;
    }

    DmVector mouse = e->getCoordinate();

    switch (getStatus())
    {
        case SetStartpoint:
            pPoints->history.clear();
            pPoints->history.append(mouse);
            if (!pPoints->spline)
            {
                pPoints->spline =
                    new DmSpline(nullptr, pPoints->data);
                pPoints->spline->setDocument(pDocument);
                setControlPointsKnotsByClose(
                    pPoints->spline, pPoints->spline->isClosed());
            }
            setStatus(SetNextPoint);
            docView->moveRelativeZero(mouse);
            updateMouseButtonHints();
            break;

        case SetNextPoint:
            docView->moveRelativeZero(mouse);
            pPoints->history.append(mouse);
            if (pPoints->spline)
            {
                setControlPointsKnotsByClose(
                    pPoints->spline, pPoints->spline->isClosed());

                deletePreview();
                DmSpline* tmpSpline =
                    static_cast<DmSpline*>(pPoints->spline->clone());
                tmpSpline->setDocument(pDocument);
                tmpSpline->setParent(preview->getEntityContainer());
                preview->addEntity(tmpSpline);
                drawPreview();
            }
            updateMouseButtonHints();
            break;

        default:
            break;
    }
}

/// @brief 处理命令输入事件
/// @param [in] e 命令事件指针
void ActionDrawSpline::commandEvent(GuiCommandEvent* e)
{
    QString commandStr = e->getCommand().toLower();

    switch (getStatus())
    {
        case SetStartpoint:
            if (checkCommand("help", commandStr))
            {
                GUIDIALOGFACTORY->commandMessage(
                    msgAvailableCommands()
                    + getAvailableCommands().join(", "));
                return;
            }
            break;

        case SetNextPoint:
            if (checkCommand("undo", commandStr))
            {
                undo();
                updateMouseButtonHints();
                return;
            }
            break;

        default:
            break;
    }
}

/// @brief 获取当前状态下可用的命令列表
/// @return 可用命令字符串列表
QStringList ActionDrawSpline::getAvailableCommands()
{
    QStringList cmd;

    switch (getStatus())
    {
        case SetStartpoint:
            break;
        case SetNextPoint:
            if (pPoints->history.size() >= 2)
            {
                cmd += command("undo");
            }
            else if (pPoints->history.size() >= 3)
            {
                cmd += command("close");
            }
            break;
        default:
            break;
    }

    return cmd;
}

/// @brief 更新鼠标按键提示
void ActionDrawSpline::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetStartpoint:
            GUIDIALOGFACTORY->updateMouseWidget(
                tr("Specify first control point"), tr("Cancel"));
            break;
        case SetNextPoint:
        {
            QString msg = QString();

            if (pPoints->history.size() >= 3)
            {
                msg += COMMANDS->command("close");
                msg += "/";
            }
            if (pPoints->history.size() >= 2)
            {
                msg += COMMANDS->command("undo");
                GUIDIALOGFACTORY->updateMouseWidget(
                    tr("Specify next control point or [%1]")
                        .arg(msg),
                    tr("Back"));
            }
            else
            {
                GUIDIALOGFACTORY->updateMouseWidget(
                    tr("Specify next control point"),
                    tr("Back"));
            }
        }
        break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

/// @brief 显示选项面板
void ActionDrawSpline::showOptions()
{
    ActionInterface::showOptions();

    GUIDIALOGFACTORY->requestOptions(this, true);
}

/// @brief 隐藏选项面板
void ActionDrawSpline::hideOptions()
{
    ActionInterface::hideOptions();

    GUIDIALOGFACTORY->requestOptions(this, false);
}

/// @brief 更新鼠标光标样式
void ActionDrawSpline::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}

/// @brief 撤销最后一个控制点
void ActionDrawSpline::undo()
{
    if (pPoints->history.size() > 1)
    {
        pPoints->history.removeLast();
        deletePreview();
        if (pPoints->spline)
        {
            setControlPointsKnotsByClose(
                pPoints->spline, pPoints->spline->isClosed());
            DmSpline* tmpSpline =
                static_cast<DmSpline*>(pPoints->spline->clone());
            tmpSpline->setDocument(pDocument);
            tmpSpline->setParent(preview->getEntityContainer());
            preview->addEntity(tmpSpline);
            if (!pPoints->history.isEmpty())
            {
                const DmVector lastPoint = pPoints->history.last();
                docView->moveRelativeZero(lastPoint);
            }
        }
        drawPreview();
    }
    else
    {
        GUIDIALOGFACTORY->commandMessage(
            tr("Cannot undo: Not enough entities defined yet."));
    }
}

/// @brief 设置样条曲线的阶数
/// @param [in] deg 阶数值
void ActionDrawSpline::setDegree(int deg)
{
    pPoints->data.setDegree(deg);
    if (pPoints->spline)
    {
        pPoints->spline->setDegree(deg);
        setControlPointsKnotsByClose(
            pPoints->spline, pPoints->spline->isClosed());
    }
}

/// @brief 获取样条曲线的阶数
/// @return 当前阶数
int ActionDrawSpline::getDegree()
{
    return pPoints->data.getDegree();
}

/// @brief 设置样条曲线是否闭合
/// @param [in] c 是否闭合
void ActionDrawSpline::setClosed(bool c)
{
    pPoints->data.setIsClosed(c);
    if (pPoints->spline)
    {
        bool isOriginClosed = pPoints->spline->isClosed();
        if (isOriginClosed == c)
        {
            return;
        }

        setControlPointsKnotsByClose(pPoints->spline, c);
    }
}

/// @brief 查询样条曲线是否闭合
/// @return 是否闭合
bool ActionDrawSpline::isClosed()
{
    return pPoints->data.getIsClosed();
}

/// @brief 根据闭合状态及动态点设置样条曲线控制点和节点
/// @param [in,out] spline 要设置的样条曲线对象
/// @param [in] isClosed 是否闭合
/// @param [in] dynamicPt 动态控制点，默认为无效点
void ActionDrawSpline::setControlPointsKnotsByClose(
    DmSpline* spline, bool isClosed, DmVector dynamicPt)
{
    std::vector<DmVector> controlPts;
    for (const auto& p : pPoints->history)
    {
        controlPts.emplace_back(p);
    }
    if (dynamicPt.valid)
    {
        controlPts.emplace_back(dynamicPt);
    }

    DmSpline::setControlPointsKnotsByClose(
        spline, isClosed, controlPts);
}
