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


/// @file ActionDrawLine.cpp
/// @brief 直线绘制交互动作的实现

#include "ActionDrawLine.h"

#include <cmath>
#include <vector>

#include <QAction>
#include <QMouseEvent>

#include "ActionEditUndo.h"
#include "Commands.h"
#include "Debug.h"
#include "DmLine.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Preview.h"
#include "Transaction.h"

namespace
{
    constexpr double ANGLE_SNAP_DEGREES = 15.0; ///< Shift 键吸附角度（度）
}

struct ActionDrawLine::History
{
    explicit History(const ActionDrawLine::HistoryAction a, const DmVector& p, const DmVector& c, const int s)
        : histAct(a)
        , prevPt(p)
        , currPt(c)
        , startOffset(s)
    {
    }

    explicit History(const History& h)
        : histAct(h.histAct)
        , prevPt(h.prevPt)
        , currPt(h.currPt)
        , startOffset(h.startOffset)
    {
    }

    History& operator=(const History& rho)
    {
        histAct = rho.histAct;
        prevPt = rho.prevPt;
        currPt = rho.currPt;
        startOffset = rho.startOffset;
        return *this;
    }

    ActionDrawLine::HistoryAction histAct; ///< 要撤销/重做的动作
    DmVector prevPt; ///< 前一坐标
    DmVector currPt; ///< 当前坐标
    int startOffset; ///< close 方法的起点偏移量
};

struct ActionDrawLine::Points
{
    LineData data; ///< 已定义的线段数据
    int historyIndex = -1; ///< 历史记录索引（undo/redo 指针）
    int startOffset = 0; ///< close 方法的起点偏移量

    std::vector<History> history; ///< 历史记录（undo/redo 缓冲区）

    /// 历史索引包装函数，避免有符号/无符号转换时的编译器警告。
    /// offset 参数用于 close 方法中查找起始点。
    size_t index(const int offset = 0);

    DmVector mouse; ///< 鼠标位置
};

size_t ActionDrawLine::Points::index(const int offset /*= 0*/)
{
    return static_cast<size_t>(std::max(0, historyIndex + offset));
}

ActionDrawLine::ActionDrawLine(DmDocument* doc, GuiDocumentView* docView)
    : PreviewActionInterface("Draw lines", doc, docView)
    , pPoints(new Points{})
{
    actionType = DM::ActionDrawLine;
}

ActionDrawLine::~ActionDrawLine() = default;

void ActionDrawLine::reset()
{
    pPoints.reset(new Points{});
}

void ActionDrawLine::init(const int status)
{
    PreviewActionInterface::init(status);

    reset();
    drawSnapper();
}

void ActionDrawLine::trigger()
{
    PreviewActionInterface::trigger();

    Transaction t(tr("Create Line").toStdString(), pDocument);
    t.start();
    DmLine* line = new DmLine(nullptr, pPoints->data);
    line->setDocument(pDocument);
    pDocument->getEntityTable()->add(line);
    t.commit();

    docView->moveRelativeZero(pPoints->history.at(pPoints->index()).currPt);
}

void ActionDrawLine::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = snapPoint(e);
    pPoints->mouse = mouse;
    if (getStatus() == SetEndpoint && pPoints->data.getStartPoint().valid)
    {

        // 按下 Shift 键时吸附到角度
        if (e->modifiers() & Qt::ShiftModifier)
        {
            mouse = snapToAngle(mouse, pPoints->data.getStartPoint(), ANGLE_SNAP_DEGREES);
        }
        else
        {
            // 未按 Shift 键，不进行角度吸附
        }

        deletePreview();
        DmLine* line = new DmLine(preview->getEntityContainer(), pPoints->data.getStartPoint(), mouse);
        line->setDocument(pDocument);
        preview->addEntity(line);
        drawPreview();
    }
    else
    {
        // 不在设置终点状态，无需处理
    }
}

void ActionDrawLine::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        DmVector snapped = snapPoint(e);

        // 按下 Shift 键时吸附到角度
        if ((e->modifiers() & Qt::ShiftModifier) && getStatus() == SetEndpoint)
        {
            snapped = snapToAngle(snapped, pPoints->data.getStartPoint(), ANGLE_SNAP_DEGREES);
        }
        else
        {
            // 未按 Shift 键或不在设置终点状态，不进行角度吸附
        }

        GuiCoordinateEvent ce(snapped);
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton)
    {
        deletePreview();
        switch (getStatus())
        {
        default:
        case SetStartpoint:
            init(getStatus() - 1);
            break;

        case SetEndpoint:
            next();
            break;
        }
    }
    else
    {
        // 其他按键忽略
    }
}

void ActionDrawLine::coordinateEvent(GuiCoordinateEvent* e)
{
    if (nullptr == e)
    {
        return;
    }

    DmVector mouse = e->getCoordinate();
    if (pPoints->data.getStartPoint().valid == false && getStatus() == SetEndpoint)
    {
        setStatus(SetStartpoint);
        pPoints->startOffset = 0;
    }

    switch (getStatus())
    {
    case SetStartpoint:
        pPoints->data.setStartPoint(mouse);
        pPoints->startOffset = 0;
        addHistory(HA_SetStartpoint, docView->getRelativeZero(), mouse, pPoints->startOffset);
        setStatus(SetEndpoint);
        docView->moveRelativeZero(mouse);
        updateMouseButtonHints();
        break;

    case SetEndpoint:
        addLine(mouse);
        break;

    default:
        break;
    }
}

void ActionDrawLine::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();
    bool isLength = false;
    double length = c.toDouble(&isLength);
    DmVector endPt;
    DmVector startPt;
    DmVector vec(1.0, 0.0);

    switch (getStatus())
    {
    case SetStartpoint:
        if (checkCommand("help", c))
        {
            GUIDIALOGFACTORY->commandMessage(
                msgAvailableCommands() + getAvailableCommands().join(", "));
            e->accept();
            return;
        }
        break;

    case SetEndpoint:
        if (checkCommand("close", c))
        {
            close();
            e->accept();
            updateMouseButtonHints();
            return;
        }

        if (checkCommand("undo", c))
        {
            undo();
            e->accept();
            updateMouseButtonHints();
            return;
        }

        if (isLength)
        {
            startPt = pPoints->data.getStartPoint();
            if (pPoints->mouse.valid)
            {
                vec = (pPoints->mouse - startPt).normalize();
            }
            else
            {
                // 鼠标位置无效，使用默认方向 (1.0, 0.0)
            }
            endPt = startPt + vec * length;
            addLine(endPt);
        }
        else
        {
            // 不是长度命令，继续检查其他命令
        }
        break;

    default:
        return;
    }

    if (checkCommand("redo", c))
    {
        redo();
        e->accept();
        updateMouseButtonHints();
    }
    else
    {
        // 不是重做命令，忽略
    }
}

QStringList ActionDrawLine::getAvailableCommands()
{
    QStringList cmd;
    if (pPoints->index() + 1 < pPoints->history.size())
    {
        cmd += command("redo");
    }
    else
    {
        // 没有可重做的历史操作
    }

    switch (getStatus())
    {
    case SetStartpoint:
        break;
    case SetEndpoint:
        if (pPoints->historyIndex >= 1)
        {
            cmd += command("undo");
        }
        else
        {
            // 没有可撤销的历史操作
        }
        if (pPoints->startOffset >= 2)
        {
            cmd += command("close");
        }
        else
        {
            // 线段数不足，无法闭合
        }
        break;
    default:
        break;
    }

    return cmd;
}

void ActionDrawLine::updateMouseButtonHints()
{
    switch (getStatus())
    {
    case SetStartpoint:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Specify first point"), tr("Cancel"));
        break;
    case SetEndpoint:
    {
        QString msg;

        if (pPoints->startOffset >= 2)
        {
            msg += COMMANDS->command("close");
        }
        else
        {
            // 偏移量不足，不添加 close 命令
        }
        if (pPoints->index() + 1 < pPoints->history.size())
        {
            if (msg.size() > 0)
            {
                msg += "/";
            }
            else
            {
                // 消息为空，不需要分隔符
            }
            msg += COMMANDS->command("redo");
        }
        else
        {
            // 没有可重做的历史操作
        }
        if (pPoints->historyIndex >= 1)
        {
            if (msg.size() > 0)
            {
                msg += "/";
            }
            else
            {
                // 消息为空，不需要分隔符
            }
            msg += COMMANDS->command("undo");
        }
        else
        {
            // 没有可撤销的历史操作
        }

        if (pPoints->historyIndex >= 1)
        {
            GUIDIALOGFACTORY->updateMouseWidget(
                tr("Specify next point or [%1]").arg(msg), tr("Back"));
        }
        else
        {
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify next point"), tr("Back"));
        }
        break;
    }
    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void ActionDrawLine::showOptions()
{
    ActionInterface::showOptions();
    GUIDIALOGFACTORY->requestOptions(this, true);
}

void ActionDrawLine::hideOptions()
{
    ActionInterface::hideOptions();
    GUIDIALOGFACTORY->requestOptions(this, false);
}

void ActionDrawLine::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}

void ActionDrawLine::close()
{
    if (SetEndpoint != getStatus())
    {
        return;
    }
    if (1 < pPoints->startOffset && 0 <= pPoints->historyIndex - pPoints->startOffset)
    {
        History h(pPoints->history.at(pPoints->index(-pPoints->startOffset)));
        if ((pPoints->data.getStartPoint() - h.currPt).squared() > DM_TOLERANCE2)
        {
            pPoints->data.setEndPoint(h.currPt);
            addHistory(HA_Close, pPoints->data.getStartPoint(), pPoints->data.getEndPoint(),
                       pPoints->startOffset);
            trigger();
            setStatus(SetStartpoint);
        }
        else
        {
            // 起点与当前点距离过近，无法闭合（起点和终点重合）
        }
    }
    else
    {
        GUIDIALOGFACTORY->commandMessage(
            tr("Cannot close sequence of lines: Not enough entities defined yet, or already closed."));
    }
}

void ActionDrawLine::next()
{
    addHistory(HA_Next, pPoints->data.getStartPoint(), pPoints->data.getEndPoint(), pPoints->startOffset);
    setStatus(SetStartpoint);
}

void ActionDrawLine::addHistory(const ActionDrawLine::HistoryAction a, const DmVector& p, const DmVector& c, const int s)
{
    if (pPoints->historyIndex < -1)
    {
        pPoints->historyIndex = -1;
    }

    // TODO: 确认 historyIndex == 1 的条件是否正确覆盖了所有需要清除重做历史的场景
    if (pPoints->historyIndex == 1)
    {
        pPoints->history.erase(pPoints->history.begin() + pPoints->historyIndex + 1, pPoints->history.end());
    }
    pPoints->history.push_back(History(a, p, c, s));
    pPoints->historyIndex = static_cast<int>(pPoints->history.size() - 1);
}

void ActionDrawLine::undo()
{
    if (0 <= pPoints->historyIndex)
    {
        History h(pPoints->history.at(pPoints->index()));

        --pPoints->historyIndex;
        deletePreview();
        docView->moveRelativeZero(h.prevPt);

        switch (h.histAct)
        {
        case HA_SetStartpoint:
            setStatus(SetStartpoint);
            break;

        case HA_SetEndpoint:
        case HA_Close:
            docView->setCurrentAction(new ActionEditUndo(true, pDocument, docView));
            pPoints->data.setStartPoint(h.prevPt);
            setStatus(SetEndpoint);
            break;

        case HA_Next:
            pPoints->data.setStartPoint(h.prevPt);
            setStatus(SetEndpoint);
            break;

        default:
            break;
        }

        // 从新的当前历史记录获取 close 的索引
        h = pPoints->history.at(pPoints->index());
        pPoints->startOffset = h.startOffset;
    }
    else
    {
        GUIDIALOGFACTORY->commandMessage(tr("Cannot undo: Begin of history reached"));
    }
}

void ActionDrawLine::redo()
{
    if (pPoints->history.size() > (pPoints->index() + 1))
    {
        ++pPoints->historyIndex;
        History h(pPoints->history.at(pPoints->index()));
        deletePreview();
        docView->moveRelativeZero(h.currPt);
        pPoints->data.setStartPoint(h.currPt);
        pPoints->startOffset = h.startOffset;
        switch (h.histAct)
        {
        case HA_SetStartpoint:
            setStatus(SetEndpoint);
            break;

        case HA_SetEndpoint:
            docView->setCurrentAction(new ActionEditUndo(false, pDocument, docView));
            setStatus(SetEndpoint);
            break;

        case HA_Close:
            docView->setCurrentAction(new ActionEditUndo(false, pDocument, docView));
            setStatus(SetStartpoint);
            break;

        case HA_Next:
            setStatus(SetStartpoint);
            break;

        default:
            break;
        }
    }
    else
    {
        GUIDIALOGFACTORY->commandMessage(tr("Cannot redo: End of history reached"));
    }
}

void ActionDrawLine::addLine(const DmVector& endPt)
{
    if ((endPt - pPoints->data.getStartPoint()).squared() > DM_TOLERANCE2)
    {

        // 拒绝零长度直线
        pPoints->data.setEndPoint(endPt);
        ++pPoints->startOffset;
        addHistory(HA_SetEndpoint, pPoints->data.getStartPoint(), endPt, pPoints->startOffset);
        trigger();
        pPoints->data.setStartPoint(pPoints->data.getEndPoint());
        if (pPoints->history.size() >= 2)
        {
            updateMouseButtonHints();
        }
        else
        {
            // 历史记录不足，无需更新按钮提示
        }
    }
    else
    {
        // 拒绝零长度直线，不做任何操作
    }
}
