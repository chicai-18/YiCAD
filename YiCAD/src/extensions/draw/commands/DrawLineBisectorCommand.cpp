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

/// @file DrawLineBisectorCommand.cpp
/// @brief 画角平分线命令与工具的实现

#include "DrawLineBisectorCommand.h"

#include <vector>

#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "DrawCommands.h"
#include "Commands.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Information.h"
#include "Math2d.h"
#include "Transaction.h"

namespace
{
/// @brief 角平分线最大创建数量
constexpr int MAX_BISECTOR_COUNT = 200;

/// @brief 两条直线夹角内的 num 条等分线；不是两条相交的直线时返回空
std::vector<DmLine*> createBisectors(DmEntityContainer* container, DmDocument* doc, const DmVector& coord1,
                                     const DmVector& coord2, double length, int num, DmLine* l1, DmLine* l2)
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
        newLine->setDocument(doc);
        res.emplace_back(newLine);
    }
    return res;
}

/// @brief 画角平分线工具：选第一条直线，再选第二条直线；命令行可改长度与数量
class DrawLineBisectorTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetLine1,  ///< 选择第一条线
        SetLine2,  ///< 选择第二条线
        SetLength, ///< 在命令行中设置长度
        SetNumber  ///< 在命令行中设置数量
    };

    DrawLineBisectorTool(DrawLineBisectorCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
        // 原 init(0)：捕捉器挂起（选线不捕捉），刷新高亮
        snapper()->suspend();
        view->specifyDocumentModified();
        view->redraw();
    }

    std::optional<DM::CursorType> getCursor() const override { return DM::SelectCursor; }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetLine1:
            GUIDIALOGFACTORY->updateMouseWidget(DrawLineBisectorCommand::tr("Select first line"),
                                                DrawLineBisectorCommand::tr("Cancel"));
            break;
        case SetLine2:
            GUIDIALOGFACTORY->updateMouseWidget(DrawLineBisectorCommand::tr("Select second line"),
                                                DrawLineBisectorCommand::tr("Back"));
            break;
        case SetLength:
            GUIDIALOGFACTORY->updateMouseWidget(DrawLineBisectorCommand::tr("Enter bisector length:"),
                                                DrawLineBisectorCommand::tr("Back"));
            break;
        case SetNumber:
            GUIDIALOGFACTORY->updateMouseWidget(DrawLineBisectorCommand::tr("Enter number of bisectors:"),
                                                DrawLineBisectorCommand::tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        DmVector mouse(view()->toGraphX(e->pos().x()), view()->toGraphY(e->pos().y()));
        switch (status())
        {
        case SetLine1:
        {
            // 悬停的直线高亮
            DmEntity* en = snapper()->catchEntity(e, DM::ResolveAll);
            if (en && en->getEntityType() == DM::EntityLine)
            {
                if (m_hovered != en)
                {
                    if (m_hovered && m_hovered->isHighlighted())
                    {
                        m_hovered->setHighlighted(false);
                    }
                    m_hovered = en;
                    m_hovered->setHighlighted(true);
                    view()->specifyDocumentModified();
                    view()->redraw();
                }
            }
            else if (m_hovered && m_hovered->isHighlighted())
            {
                m_hovered->setHighlighted(false);
                view()->specifyDocumentModified();
                view()->redraw();
            }
            break;
        }

        case SetLine2:
        {
            m_coord2 = mouse;
            DmEntity* en = snapper()->catchEntity(e, DM::ResolveAll);
            if (en == m_line1)
            {
                break;
            }
            if (en && en->getEntityType() == DM::EntityLine)
            {
                if (m_line2 && m_line2->isHighlighted())
                {
                    m_line2->setHighlighted(false);
                }
                m_line2 = static_cast<DmLine*>(en);
                m_line2->setHighlighted(true);
                view()->specifyDocumentModified();
                view()->redraw();
                m_command.previewBisectors(m_coord1, m_coord2, m_line1, m_line2);
            }
            else
            {
                m_command.preview().clear();
                if (m_line2 && m_line2->isHighlighted())
                {
                    m_line2->setHighlighted(false);
                    view()->specifyDocumentModified();
                    view()->redraw();
                }
                m_line2 = nullptr;
            }
            break;
        }

        default:
            break;
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::RightButton)
        {
            m_command.preview().clear();
            init(status() - 1);
            return;
        }

        DmVector mouse(view()->toGraphX(e->pos().x()), view()->toGraphY(e->pos().y()));
        switch (status())
        {
        case SetLine1:
        {
            m_coord1 = mouse;
            DmEntity* en = snapper()->catchEntity(e, DM::ResolveAll);
            if (en && en->getEntityType() == DM::EntityLine)
            {
                m_line1 = static_cast<DmLine*>(en);
                m_line1->setHighlighted(true);
                view()->specifyDocumentModified();
                view()->redraw();
                m_line2 = nullptr;
                setStatus(SetLine2);
            }
            break;
        }

        case SetLine2:
            m_coord2 = mouse;
            for (DmLine* line : {m_line1, m_line2})
            {
                if (line && line->isHighlighted())
                {
                    line->setHighlighted(false);
                }
            }
            m_command.commitBisectors(m_coord1, m_coord2, m_line1, m_line2);
            setStatus(SetLine1);
            break;

        default:
            break;
        }
    }

    void onCommand(GuiCommandEvent* e) override
    {
        QString c = e->getCommand().toLower();

        if (Commands::checkCommand("help", c))
        {
            // 与原 Action 一致：列出命令后不接受
            GUIDIALOGFACTORY->commandMessage(Commands::msgAvailableCommands() + availableCommands().join(", "));
            return;
        }

        switch (status())
        {
        case SetLine1:
        case SetLine2:
            // 与原 Action 一致：进入输入状态，但不接受这段文本
            m_lastStatus = static_cast<Status>(status());
            if (Commands::checkCommand("length", c))
            {
                m_command.preview().clear();
                setStatus(SetLength);
            }
            else if (Commands::checkCommand("number", c))
            {
                m_command.preview().clear();
                setStatus(SetNumber);
            }
            break;

        case SetLength:
        {
            bool ok = false;
            double l = Math2d::eval(c, &ok);
            if (ok)
            {
                e->accept();
                m_command.setLength(l);
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(DrawLineBisectorCommand::tr("Not a valid expression"));
            }
            GUIDIALOGFACTORY->requestCommandOptions(&m_command, true, true);
            setStatus(m_lastStatus);
            break;
        }

        case SetNumber:
        {
            bool ok = false;
            int n = static_cast<int>(Math2d::eval(c, &ok));
            if (ok)
            {
                e->accept();
                if (n > 0 && n <= MAX_BISECTOR_COUNT)
                {
                    m_command.setNumber(n);
                }
                else
                {
                    GUIDIALOGFACTORY->commandMessage(
                        DrawLineBisectorCommand::tr("Number sector lines not in range: ",
                                                    "number of bisector to create must be in [1, 200]")
                        + QString::number(n));
                }
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(DrawLineBisectorCommand::tr("Not a valid expression"));
            }
            GUIDIALOGFACTORY->requestCommandOptions(&m_command, true, true);
            setStatus(m_lastStatus);
            break;
        }

        default:
            break;
        }
    }

    void onFinish() override
    {
        // 原先悬停高亮记在函数内的静态变量里、命令结束时不取消；现在记在工具里，结束时取消
        if (m_hovered && m_hovered->isHighlighted())
        {
            m_hovered->setHighlighted(false);
        }
    }

private:
    /// @brief 回到某一状态（原 init(status)）：status < 0 时结束命令；退回第一步时取消第二条线的高亮
    void init(int s)
    {
        if (s < 0)
        {
            for (DmLine* line : {m_line2, m_line1})
            {
                if (line && line->isHighlighted())
                {
                    line->setHighlighted(false);
                }
            }
            view()->specifyDocumentModified();
            view()->redraw();
            command().finish();
            return;
        }

        restart(s);
        snapper()->suspend();
        if (s < SetLine2)
        {
            if (m_line2 && m_line2->isHighlighted())
            {
                m_line2->setHighlighted(false);
            }
            view()->specifyDocumentModified();
            view()->redraw();
        }
    }

    /// @brief 当前状态下可用的命令行命令（help 列出）
    QStringList availableCommands() const
    {
        QStringList cmd;
        if (status() == SetLine1 || status() == SetLine2)
        {
            cmd += Commands::command("length");
            cmd += Commands::command("number");
        }
        return cmd;
    }

    DrawLineBisectorCommand& m_command;
    DmLine* m_line1 = nullptr;      ///< 第一条选中的线
    DmLine* m_line2 = nullptr;      ///< 第二条选中的线
    DmEntity* m_hovered = nullptr;  ///< 选第一条线时悬停高亮的线
    DmVector m_coord1;              ///< 选择第一条线时的鼠标位置
    DmVector m_coord2;              ///< 选择第二条线时的鼠标位置
    Status m_lastStatus = SetLine1; ///< 进入长度或数量设置前的状态
};
}  // namespace

std::unique_ptr<BasePlaceTool> DrawLineBisectorCommand::createTool()
{
    return std::make_unique<DrawLineBisectorTool>(*this, document(), view());
}

void DrawLineBisectorCommand::previewBisectors(const DmVector& coord1, const DmVector& coord2, DmLine* line1,
                                               DmLine* line2)
{
    preview().clear();
    for (auto line : createBisectors(preview().entities().getEntityContainer(), document(), coord1, coord2, m_length,
                                     m_number, line1, line2))
    {
        preview().entities().addEntity(line);
    }
    preview().draw();
}

void DrawLineBisectorCommand::commitBisectors(const DmVector& coord1, const DmVector& coord2, DmLine* line1,
                                              DmLine* line2)
{
    preview().clear();
    // 事务名沿用原 ActionDrawLineBisector 的"Add cloud line"
    Transaction t(tr("Add cloud line").toStdString(), document());
    t.start();
    for (auto line : createBisectors(nullptr, document(), coord1, coord2, m_length, m_number, line1, line2))
    {
        document()->getEntityTable()->add(line);
    }
    t.commit();
}

void DrawLineBisectorCommand::showOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, true);
}

void DrawLineBisectorCommand::hideOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, false);
}

ExclusiveCommandFactory DrawCommands::lineBisector()
{
    return exclusiveCommandFactory<DrawLineBisectorCommand>();
}
