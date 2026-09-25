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

/// @file DrawPointCommand.cpp
/// @brief 画点命令 draw.point，取代原 ActionDrawPoint：每指定一个位置画一个点

#include <memory>

#include <QCoreApplication>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandRegistry.h"
#include "Commands.h"
#include "DmDocument.h"
#include "DmPoint.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "PlaceCommand.h"
#include "Transaction.h"

/// @brief 画点命令；交互由 DrawPointTool 驱动
class DrawPointCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawPointCommand)

public:
    /// @brief 提交一个点，相对零点移到该点
    void commitPoint(const DmVector& pos)
    {
        if (!pos.valid)
        {
            return;
        }
        Transaction t(tr("Create Point").toStdString(), document());
        t.start();
        DmPoint* point = new DmPoint(nullptr, PointData(pos));
        point->setDocument(document());
        document()->getEntityTable()->add(point);
        t.commit();
        view()->moveRelativeZero(pos);
    }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

namespace
{
/// @brief 画点工具：只有一步
class DrawPointTool : public BasePlaceTool
{
public:
    DrawPointTool(DrawPointCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

protected:
    void updateHints() override
    {
        if (status() == 0)
        {
            GUIDIALOGFACTORY->updateMouseWidget(DrawPointCommand::tr("Specify location"),
                                                DrawPointCommand::tr("Cancel"));
        }
        else
        {
            GUIDIALOGFACTORY->updateMouseWidget();
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        // 只为显示捕捉标记
        snapper()->snapPoint(e);
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
        {
            onCoordinate(snapper()->snapPoint(e));
        }
        else if (e->button() == Qt::RightButton)
        {
            stepBack();
        }
    }

    void onCoordinate(const DmVector& pos) override
    {
        m_command.commitPoint(pos);
    }

    void onCommand(GuiCommandEvent* e) override
    {
        // 与原 Action 一致：help 只列出命令（没有可用命令），不接受
        if (Commands::checkCommand("help", e->getCommand().toLower()))
        {
            GUIDIALOGFACTORY->commandMessage(Commands::msgAvailableCommands());
        }
    }

private:
    DrawPointCommand& m_command;
};
}  // namespace

std::unique_ptr<BasePlaceTool> DrawPointCommand::createTool()
{
    return std::make_unique<DrawPointTool>(*this, document(), view());
}

namespace
{
const bool g_registered = CommandRegistry::instance().registerExclusiveCommand(
    QStringLiteral("draw.point"), exclusiveCommandFactory<DrawPointCommand>());
}  // namespace
