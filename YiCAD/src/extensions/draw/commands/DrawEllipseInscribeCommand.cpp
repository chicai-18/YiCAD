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

/// @file DrawEllipseInscribeCommand.cpp
/// @brief 内切椭圆命令 ext.draw.ellipse_inscribe，取代原 ActionDrawEllipseInscribe：
///        选四条直线，画内切于它们围成的四边形的椭圆

#include <memory>
#include <vector>

#include <QCoreApplication>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "DrawCommands.h"
#include "DmDocument.h"
#include "DmEllipse.h"
#include "DmLine.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "PlaceCommand.h"
#include "Transaction.h"

namespace
{
/// @brief 内切椭圆命令；交互由 DrawEllipseInscribeTool 驱动
class DrawEllipseInscribeCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawEllipseInscribeCommand)

public:
    /// @brief 提交椭圆
    void commitEllipse(const EllipseData& data)
    {
        DmEllipse* ellipse = new DmEllipse(nullptr, data);
        ellipse->setDocument(document());
        ellipse->update();
        preview().clear();
        Transaction t(tr("Add ellipse").toStdString(), document());
        t.start();
        document()->getEntityTable()->add(ellipse);
        t.commit();
    }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 内切椭圆工具：依次选四条直线
class DrawEllipseInscribeTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetLine1, ///< 选择第一条边线
        SetLine2, ///< 选择第二条边线
        SetLine3, ///< 选择第三条边线
        SetLine4  ///< 选择第四条边线
    };

    DrawEllipseInscribeTool(DrawEllipseInscribeCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
        // 原 init(0)：选线不捕捉
        snapper()->suspend();
    }

    std::optional<DM::CursorType> getCursor() const override { return DM::SelectCursor; }

protected:
    void updateHints() override;
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;
    void onFinish() override { clearLines(false); }

private:
    /// @brief 选中的直线与求出的椭圆
    struct Points
    {
        std::vector<DmLine*> lines; ///< 已选中的直线
        EllipseData eData;          ///< 求出的椭圆
        bool valid{false};          ///< 椭圆是否有效
    };

    /// @brief 回到某一状态（原 init(status)）：status < 0 时结束命令
    void init(int s)
    {
        if (s < 0)
        {
            command().finish();
            return;
        }
        restart(s);
        snapper()->suspend();
        clearLines(true);
    }

    /// @brief 提交后取消高亮：正交限制下结束命令，否则回到第一步（原 trigger()）
    void commit()
    {
        m_command.commitEllipse(m_points.eData);
        for (DmLine* const p : m_points.lines)
        {
            if (p)
            {
                p->setHighlighted(false);
                view()->redraw();
            }
        }
        snapper()->drawSnapper();
        clearLines(false);
        if (snapper()->getSnapMode()->restriction == DM::RestrictOrthogonal)
        {
            command().finish();
        }
        else
        {
            setStatus(SetLine1);
        }
    }

    void clearLines(bool checkStatus);
    bool preparePreview();

    DrawEllipseInscribeCommand& m_command;
    Points m_points;
};

void DrawEllipseInscribeTool::clearLines(bool checkStatus)
{
    while (m_points.lines.size())
    {
        if (checkStatus && static_cast<int>(m_points.lines.size()) <= status())
        {
            break;
        }
        m_points.lines.back()->setHighlighted(false);
        view()->redraw();
        m_points.lines.pop_back();
    }
}

void DrawEllipseInscribeTool::onMouseMove(QMouseEvent* e)
{
    if (status() == SetLine4)
    {
        DmEntity* en = snapper()->catchEntity(e, DM::EntityLine, DM::ResolveAll);
        if (!en)
        {
            return;
        }
        if (!(en->isVisible() && en->getEntityType() == DM::EntityLine))
        {
            return;
        }
        for (auto p : m_points.lines)
        {
            if (en == p)
            {
                return; // 不重复选择同一条线
            }
        }

        clearLines(true);
        m_points.lines.push_back(static_cast<DmLine*>(en));
        if (preparePreview())
        {
            m_points.lines.back()->setHighlighted(true);
            view()->redraw();
            DmEllipse* ellipse = new DmEllipse(m_command.preview().entities().getEntityContainer(), m_points.eData);
            ellipse->setDocument(document());
            m_command.preview().entities().addEntity(ellipse);
            m_command.preview().draw();
        }
    }
}

bool DrawEllipseInscribeTool::preparePreview()
{
    m_points.valid = false;
    if (status() == SetLine4)
    {
        DmEllipse e(nullptr, EllipseData());
        m_points.valid = e.createInscribeQuadrilateral(m_points.lines);
        if (m_points.valid)
        {
            m_points.eData = e.getData();
        }
        else if (GUIDIALOGFACTORY)
        {
            GUIDIALOGFACTORY->commandMessage(DrawEllipseInscribeCommand::tr("Can not determine uniquely an ellipse"));
        }
    }
    return m_points.valid;
}

void DrawEllipseInscribeTool::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        DmEntity* en = snapper()->catchEntity(e, DM::EntityLine, DM::ResolveAll);
        if (!en)
        {
            return;
        }
        if (!(en->isVisible() && en->getEntityType() == DM::EntityLine))
        {
            return;
        }
        for (int i = 0; i < status(); ++i)
        {
            if (en->getId() == m_points.lines[i]->getId())
            {
                return; // 不重复选择同一条线
            }
        }
        clearLines(true);
        m_points.lines.push_back(static_cast<DmLine*>(en));

        switch (status())
        {
        case SetLine1:
        case SetLine2:
        case SetLine3:
            {
                DmEntity* li = en->clone();
                li->setDocument(document());
                li->setHighlighted(true);
                li->setParent(nullptr);
                m_command.preview().entities().addEntity(li);
                setStatus(status() + 1);
                break;
            }

        case SetLine4:
            if (preparePreview())
            {
                commit();
                finishIfOrthogonal();
            }
            m_command.preview().clear();
        default:
            break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        // 返回上一个状态
        if (status() > 0)
        {
            clearLines(true);
            m_points.lines.back()->setHighlighted(false);
            view()->redraw();
            m_points.lines.pop_back();
            m_command.preview().clear();
        }
        init(status() - 1);
    }
}

void DrawEllipseInscribeTool::updateHints()
{
    switch (status())
    {
    case SetLine1:
        GUIDIALOGFACTORY->updateMouseWidget(DrawEllipseInscribeCommand::tr("Specify the first line"), DrawEllipseInscribeCommand::tr("Cancel"));
        break;

    case SetLine2:
        GUIDIALOGFACTORY->updateMouseWidget(DrawEllipseInscribeCommand::tr("Specify the second line"), DrawEllipseInscribeCommand::tr("Back"));
        break;

    case SetLine3:
        GUIDIALOGFACTORY->updateMouseWidget(DrawEllipseInscribeCommand::tr("Specify the third line"), DrawEllipseInscribeCommand::tr("Back"));
        break;

    case SetLine4:
        GUIDIALOGFACTORY->updateMouseWidget(DrawEllipseInscribeCommand::tr("Specify the fourth line"), DrawEllipseInscribeCommand::tr("Back"));
        break;

    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}
std::unique_ptr<BasePlaceTool> DrawEllipseInscribeCommand::createTool()
{
    return std::make_unique<DrawEllipseInscribeTool>(*this, document(), view());
}

}  // namespace

ExclusiveCommandFactory DrawCommands::ellipseInscribe()
{
    return exclusiveCommandFactory<DrawEllipseInscribeCommand>();
}
