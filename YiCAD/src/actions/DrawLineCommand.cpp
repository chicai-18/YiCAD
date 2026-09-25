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

/// @file DrawLineCommand.cpp
/// @brief 画直线命令与画直线工具的实现

#include "DrawLineCommand.h"

#include <algorithm>
#include <vector>

#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "Commands.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "EditUndoCommand.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "LineData.h"
#include "Transaction.h"

namespace
{
/// @brief Shift 键吸附角度（度）
constexpr double ANGLE_SNAP_DEGREES = 15.0;
}  // namespace

/// @brief 画直线工具：指定起点，再连续指定下一点；维护撤销/重做历史
class DrawLineTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetStartpoint, ///< 设置起点
        SetEndpoint    ///< 设置终点
    };

    DrawLineTool(DrawLineCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
        snapper()->drawSnapper();
    }

    /// @brief 闭合当前线段组
    void close()
    {
        if (SetEndpoint != status())
        {
            return;
        }
        if (1 < m_startOffset && 0 <= m_historyIndex - m_startOffset)
        {
            History h(m_history.at(index(-m_startOffset)));
            if ((m_data.getStartPoint() - h.currPt).squared() > DM_TOLERANCE2)
            {
                m_data.setEndPoint(h.currPt);
                addHistory(HA_Close, m_data.getStartPoint(), m_data.getEndPoint(), m_startOffset);
                commit();
                setStatus(SetStartpoint);
            }
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(DrawLineCommand::tr(
                "Cannot close sequence of lines: Not enough entities defined yet, or already closed."));
        }
    }

    /// @brief 撤销上一步
    void undo()
    {
        if (0 <= m_historyIndex)
        {
            History h(m_history.at(index()));

            --m_historyIndex;
            m_command.preview().clear();
            view()->moveRelativeZero(h.prevPt);

            switch (h.histAct)
            {
            case HA_SetStartpoint:
                setStatus(SetStartpoint);
                break;

            case HA_SetEndpoint:
            case HA_Close:
                // 原先嵌套启动 ActionEditUndo，现在直接撤销文档的一步
                EditUndoCommand::run(document(), true);
                m_data.setStartPoint(h.prevPt);
                setStatus(SetEndpoint);
                break;

            case HA_Next:
                m_data.setStartPoint(h.prevPt);
                setStatus(SetEndpoint);
                break;

            default:
                break;
            }

            // 从新的当前历史记录获取 close 的索引
            h = m_history.at(index());
            m_startOffset = h.startOffset;
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(DrawLineCommand::tr("Cannot undo: Begin of history reached"));
        }
    }

    /// @brief 重做下一步
    void redo()
    {
        if (m_history.size() > (index() + 1))
        {
            ++m_historyIndex;
            History h(m_history.at(index()));
            m_command.preview().clear();
            view()->moveRelativeZero(h.currPt);
            m_data.setStartPoint(h.currPt);
            m_startOffset = h.startOffset;
            switch (h.histAct)
            {
            case HA_SetStartpoint:
                setStatus(SetEndpoint);
                break;

            case HA_SetEndpoint:
                EditUndoCommand::run(document(), false);
                setStatus(SetEndpoint);
                break;

            case HA_Close:
                EditUndoCommand::run(document(), false);
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
            GUIDIALOGFACTORY->commandMessage(DrawLineCommand::tr("Cannot redo: End of history reached"));
        }
    }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetStartpoint:
            GUIDIALOGFACTORY->updateMouseWidget(DrawLineCommand::tr("Specify first point"),
                                                DrawLineCommand::tr("Cancel"));
            break;
        case SetEndpoint:
        {
            QString msg;
            if (m_startOffset >= 2)
            {
                msg += Commands::command("close");
            }
            if (index() + 1 < m_history.size())
            {
                if (msg.size() > 0)
                {
                    msg += "/";
                }
                msg += Commands::command("redo");
            }
            if (m_historyIndex >= 1)
            {
                if (msg.size() > 0)
                {
                    msg += "/";
                }
                msg += Commands::command("undo");
            }

            if (m_historyIndex >= 1)
            {
                GUIDIALOGFACTORY->updateMouseWidget(DrawLineCommand::tr("Specify next point or [%1]").arg(msg),
                                                    DrawLineCommand::tr("Back"));
            }
            else
            {
                GUIDIALOGFACTORY->updateMouseWidget(DrawLineCommand::tr("Specify next point"),
                                                    DrawLineCommand::tr("Back"));
            }
            break;
        }
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        DmVector mouse = snapper()->snapPoint(e);
        m_mouse = mouse;
        if (status() == SetEndpoint && m_data.getStartPoint().valid)
        {
            // 按下 Shift 键时吸附到角度
            if (e->modifiers() & Qt::ShiftModifier)
            {
                mouse = snapper()->snapToAngle(mouse, m_data.getStartPoint(), ANGLE_SNAP_DEGREES);
            }
            m_command.previewLine(m_data.getStartPoint(), mouse);
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
        {
            DmVector snapped = snapper()->snapPoint(e);
            if ((e->modifiers() & Qt::ShiftModifier) && status() == SetEndpoint)
            {
                snapped = snapper()->snapToAngle(snapped, m_data.getStartPoint(), ANGLE_SNAP_DEGREES);
            }
            onCoordinate(snapped);
        }
        else if (e->button() == Qt::RightButton)
        {
            m_command.preview().clear();
            switch (status())
            {
            default:
            case SetStartpoint:
                stepBack();
                break;
            case SetEndpoint:
                next();
                break;
            }
        }
    }

    void onCoordinate(const DmVector& mouse) override
    {
        if (!m_data.getStartPoint().valid && status() == SetEndpoint)
        {
            setStatus(SetStartpoint);
            m_startOffset = 0;
        }

        switch (status())
        {
        case SetStartpoint:
            m_data.setStartPoint(mouse);
            m_startOffset = 0;
            addHistory(HA_SetStartpoint, view()->getRelativeZero(), mouse, m_startOffset);
            setStatus(SetEndpoint);
            view()->moveRelativeZero(mouse);
            updateHints();
            break;

        case SetEndpoint:
            addLine(mouse);
            break;

        default:
            break;
        }
    }

    void onCommand(GuiCommandEvent* e) override
    {
        QString c = e->getCommand().toLower();
        bool isLength = false;
        double length = c.toDouble(&isLength);

        switch (status())
        {
        case SetStartpoint:
            if (Commands::checkCommand("help", c))
            {
                GUIDIALOGFACTORY->commandMessage(Commands::msgAvailableCommands() + availableCommands().join(", "));
                e->accept();
                return;
            }
            break;

        case SetEndpoint:
            if (Commands::checkCommand("close", c))
            {
                close();
                e->accept();
                updateHints();
                return;
            }
            if (Commands::checkCommand("undo", c))
            {
                undo();
                e->accept();
                updateHints();
                return;
            }
            if (isLength)
            {
                // 输入长度：沿起点指向鼠标的方向（鼠标位置无效时沿 X 轴正向）画线
                // 与原 ActionDrawLine 一致，数值不接受，随后仍被当作新命令解析
                DmVector startPt = m_data.getStartPoint();
                DmVector vec(1.0, 0.0);
                if (m_mouse.valid)
                {
                    vec = (m_mouse - startPt).normalize();
                }
                addLine(startPt + vec * length);
            }
            break;

        default:
            return;
        }

        if (Commands::checkCommand("redo", c))
        {
            redo();
            e->accept();
            updateHints();
        }
    }

private:
    /// @brief 历史动作
    enum HistoryAction
    {
        HA_SetStartpoint, ///< 设置起点
        HA_SetEndpoint,   ///< 设置终点
        HA_Close,         ///< 闭合线段组
        HA_Next           ///< 开始新线段组
    };

    /// @brief 一条历史记录
    struct History
    {
        History(HistoryAction a, const DmVector& p, const DmVector& c, int s)
            : histAct(a)
            , prevPt(p)
            , currPt(c)
            , startOffset(s)
        {
        }

        HistoryAction histAct; ///< 要撤销/重做的动作
        DmVector prevPt;       ///< 前一坐标
        DmVector currPt;       ///< 当前坐标
        int startOffset;       ///< close 方法的起点偏移量
    };

    /// @brief 历史记录下标
    size_t index(int offset = 0) const
    {
        return static_cast<size_t>(std::max(0, m_historyIndex + offset));
    }

    /// @brief 当前状态下可用的命令行命令（help 列出）
    QStringList availableCommands() const
    {
        QStringList cmd;
        if (index() + 1 < m_history.size())
        {
            cmd += Commands::command("redo");
        }
        if (status() == SetEndpoint)
        {
            if (m_historyIndex >= 1)
            {
                cmd += Commands::command("undo");
            }
            if (m_startOffset >= 2)
            {
                cmd += Commands::command("close");
            }
        }
        return cmd;
    }

    /// @brief 开始下一线段组
    void next()
    {
        addHistory(HA_Next, m_data.getStartPoint(), m_data.getEndPoint(), m_startOffset);
        setStatus(SetStartpoint);
    }

    /// @brief 添加历史记录
    void addHistory(HistoryAction a, const DmVector& p, const DmVector& c, int s)
    {
        if (m_historyIndex < -1)
        {
            m_historyIndex = -1;
        }

        // TODO: 确认 historyIndex == 1 的条件是否正确覆盖了所有需要清除重做历史的场景
        if (m_historyIndex == 1)
        {
            m_history.erase(m_history.begin() + m_historyIndex + 1, m_history.end());
        }
        m_history.push_back(History(a, p, c, s));
        m_historyIndex = static_cast<int>(m_history.size() - 1);
    }

    /// @brief 添加一条线段；拒绝零长度直线
    void addLine(const DmVector& endPt)
    {
        if ((endPt - m_data.getStartPoint()).squared() > DM_TOLERANCE2)
        {
            m_data.setEndPoint(endPt);
            ++m_startOffset;
            addHistory(HA_SetEndpoint, m_data.getStartPoint(), endPt, m_startOffset);
            commit();
            m_data.setStartPoint(m_data.getEndPoint());
            if (m_history.size() >= 2)
            {
                updateHints();
            }
        }
    }

    /// @brief 提交当前线段，相对零点移到本次历史记录的点（原 trigger()）
    void commit()
    {
        m_command.commitLine(m_data);
        view()->moveRelativeZero(m_history.at(index()).currPt);
    }

    DrawLineCommand& m_command;
    LineData m_data;              ///< 当前线段
    int m_historyIndex = -1;      ///< 历史记录索引（undo/redo 指针）
    int m_startOffset = 0;        ///< close 方法的起点偏移量
    std::vector<History> m_history; ///< 历史记录（undo/redo 缓冲区）
    DmVector m_mouse;             ///< 鼠标位置
};

DrawLineCommand::DrawLineCommand() = default;

DrawLineCommand::~DrawLineCommand() = default;

std::unique_ptr<BasePlaceTool> DrawLineCommand::createTool()
{
    return std::make_unique<DrawLineTool>(*this, document(), view());
}

DrawLineTool* DrawLineCommand::tool() const
{
    return static_cast<DrawLineTool*>(placeTool());
}

void DrawLineCommand::previewLine(const DmVector& start, const DmVector& end)
{
    preview().clear();
    DmLine* line = new DmLine(preview().entities().getEntityContainer(), start, end);
    line->setDocument(document());
    preview().entities().addEntity(line);
    preview().draw();
}

void DrawLineCommand::commitLine(const LineData& data)
{
    preview().clear();
    Transaction t(tr("Create Line").toStdString(), document());
    t.start();
    DmLine* line = new DmLine(nullptr, data);
    line->setDocument(document());
    document()->getEntityTable()->add(line);
    t.commit();
}

void DrawLineCommand::close()
{
    if (tool())
    {
        tool()->close();
    }
}

void DrawLineCommand::undo()
{
    if (tool())
    {
        tool()->undo();
    }
}

void DrawLineCommand::redo()
{
    if (tool())
    {
        tool()->redo();
    }
}

void DrawLineCommand::showOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, true);
}

void DrawLineCommand::hideOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, false);
}

namespace
{
const bool g_registered = CommandRegistry::instance().registerExclusiveCommand(
    QStringLiteral("draw.line"), exclusiveCommandFactory<DrawLineCommand>());
}  // namespace
