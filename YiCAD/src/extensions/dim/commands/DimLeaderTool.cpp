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

/// @file DimLeaderTool.cpp
/// @brief DimLeaderCommand 与引线工具（从原 ActionDimLeader 机械改写）

#include <memory>
#include <vector>

#include <QInputDialog>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QStringList>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "Commands.h"
#include "DimCommands.h"
#include "DmDimensionStyle.h"
#include "DmDimensionStyleTable.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "DmLeader.h"
#include "DmLine.h"
#include "DmMText.h"
#include "DmTextStyle.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Preview.h"
#include "Transaction.h"

namespace
{
/// @brief 引线工具：目标点，再逐个指定折点；右键、小键盘回车或命令行空行完成一条，可连续
class DimLeaderTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetStartpoint, ///< 设置起点
        SetEndpoint    ///< 设置下一点
    };

    DimLeaderTool(DimLeaderCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

protected:
    void updateHints() override;
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;
    void onKeyPress(QKeyEvent* e) override;
    void onCoordinate(const DmVector& coord) override;
    void onCommand(GuiCommandEvent* e) override;

private:
    /// @brief 已设置的点
    struct Points
    {
        std::vector<DmVector> points;
    };

    /// @brief 回到某一状态并清空已设置的点（原 init(status)）；status < 0 时结束命令
    void init(int s)
    {
        if (s < 0)
        {
            command().finish();
            return;
        }
        restart(s);
        reset();
    }

    void reset();
    void trigger();
    QStringList availableCommands() const;

    DimLeaderCommand& m_command;
    Points m_points; ///< 已设置的点
};
}  // namespace

void DimLeaderTool::reset()
{
    m_points.points.clear();
}

void DimLeaderTool::trigger()
{
    m_command.preview().clear();

    if (m_points.points.size() >= 2)
    {
        bool ok = false;
        QString text = QInputDialog::getText(nullptr,
                    DimLeaderCommand::tr("Set leader text"), DimLeaderCommand::tr("Leader text:"),
                    QLineEdit::Normal, QString(), &ok);
        DmDimensionStyle* pDimStyle =
                    view()->getDocument()->getDimStyleTable()->getActive();

        DmLeader* leader = new DmLeader(
                    nullptr, DmLeaderData(pDimStyle, m_points.points));
        leader->setDocument(document());
        leader->update();

        Transaction t(DimLeaderCommand::tr("Add leader").toStdString(), document());
        t.start();
        document()->getEntityTable()->add(leader);

        // 追加文字
        DmMText* mtext = nullptr;
        if (ok && !text.isEmpty())
        {
            DmTextStyle* pStyle = pDimStyle->getDataRef().textStyle();
            double height = pDimStyle->getValidTextHeight();
            MTextData textData(DmVector(0.0, 0.0), height,
                        EMTextVertMode::kTextTop, EMTextHorzMode::kTextLeft,
                        1.0, 10.0, text, pStyle, 0.0);
            textData.setJustification(EMTextMode::kTextMiddleLeft);
            textData.setLineSpacingFactor(1.0);
            mtext = new DmMText(nullptr, textData);
            mtext->setDocument(document());
            mtext->update();
            DmVector textSize = mtext->getWidthHeight();
            DmVector lastPt = m_points.points.back();
            DmVector lastSecPt = m_points.points.at(
                        m_points.points.size() - 2);
            DmVector lastDir = (lastPt - lastSecPt).normalize();
            if (lastDir.x > 0)
            {
                mtext->move(lastPt + DmVector(0.0, 0.0));
            }
            else
            {
                mtext->move(lastPt
                            + DmVector(-textSize.x - height / 2.0, 0.0));
            }
            document()->getEntityTable()->add(mtext);
        }
        t.commit();

        m_command.preview().clear();
        DmVector rz = view()->getRelativeZero();
        view()->moveRelativeZero(rz);
    }
}

void DimLeaderTool::onMouseMove(QMouseEvent* e)
{
    DmVector mouse = snapper()->snapPoint(e);
    if (status() == SetEndpoint
        && !m_points.points.empty())
    {
        m_command.preview().clear();
        std::vector<DmVector> pts(m_points.points);
        pts.emplace_back(mouse);
        DmDimensionStyleTable* pTable =
                    view()->getDocument()->getDimStyleTable();
        DmLeaderData ldata(pTable->getActive(), pts);

        DmLeader* leader = new DmLeader(
                    m_command.preview().entities().getEntityContainer(), ldata);
        leader->setDocument(document());
        leader->update();
        m_command.preview().entities().addEntity(leader);
        m_command.preview().draw();
    }
}

void DimLeaderTool::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        onCoordinate(snapper()->snapPoint(e));
    }
    else if (e->button() == Qt::RightButton)
    {
        if (status() == SetEndpoint
            && m_points.points.size() >= 2)
        {
            trigger();
            finishIfOrthogonal();
            reset();
            setStatus(SetStartpoint);
        }
        else
        {
            m_command.preview().clear();
            init(status() - 1);
        }
    }
}

void DimLeaderTool::onKeyPress(QKeyEvent* e)
{
    if (status() == SetEndpoint
        && e->key() == Qt::Key_Enter)
    {
        trigger();
        reset();
        setStatus(SetStartpoint);
    }
    else
    {
        // 与原 ActionInterface::keyPressEvent 一样不接受
        e->ignore();
    }
}

void DimLeaderTool::onCoordinate(const DmVector& coord)
{
    DmVector mouse = coord;

    switch (status())
    {
        case SetStartpoint:
            m_points.points.clear();
            m_points.points.push_back(mouse);
            setStatus(SetEndpoint);
            view()->moveRelativeZero(mouse);
            break;

        case SetEndpoint:
            m_points.points.push_back(mouse);
            view()->moveRelativeZero(mouse);
            break;

        default:
            break;
    }
}

void DimLeaderTool::onCommand(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (Commands::checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(
                    Commands::msgAvailableCommands()
                    + availableCommands().join(", "));
        return;
    }

    // 回车完成
    if (c == "")
    {
        trigger();
        reset();
        setStatus(SetStartpoint);
    }
}

QStringList DimLeaderTool::availableCommands() const
{
    QStringList cmd;

    return cmd;
}

void DimLeaderTool::updateHints()
{
    switch (status())
    {
        case SetStartpoint:
            GUIDIALOGFACTORY->updateMouseWidget(
                        DimLeaderCommand::tr("Specify target point"), DimLeaderCommand::tr("Cancel"));
            break;
        case SetEndpoint:
            GUIDIALOGFACTORY->updateMouseWidget(
                        DimLeaderCommand::tr("Specify next point"), DimLeaderCommand::tr("Finish"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

std::unique_ptr<BasePlaceTool> DimLeaderCommand::createTool()
{
    return std::make_unique<DimLeaderTool>(*this, document(), view());
}
