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

/// @file DrawLineRectangleCommand.cpp
/// @brief 画矩形命令 ext.draw.line_rectangle，取代原 ActionDrawLineRectangle：两个对角点画闭合多段线

#include <vector>

#include <QCoreApplication>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "DrawCommands.h"
#include "Commands.h"
#include "DmDocument.h"
#include "DmPolyline.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "PlaceCommand.h"
#include "Transaction.h"

namespace
{
constexpr int RECTANGLE_VERTEX_COUNT = 4; ///< 矩形顶点数
constexpr int RECTANGLE_WEIGHT_COUNT = 8; ///< 矩形线宽数（每段起止各一）

/// @brief 两个对角点构成的闭合多段线
DmPolyline* createRectangle(DmEntityContainer* container, const DmVector& corner1, const DmVector& corner2)
{
    DmVector p1(corner1);
    DmVector p2(corner2.x, corner1.y);
    DmVector p3(corner2);
    DmVector p4(corner1.x, corner2.y);
    std::vector<DmVector> pts{p1, p2, p3, p4};
    std::vector<double> bulges(RECTANGLE_VERTEX_COUNT, 0.0);
    std::vector<double> weights(RECTANGLE_WEIGHT_COUNT, 0.0);
    return new DmPolyline(container, PolylineData(pts, bulges, weights, true));
}
}  // namespace

/// @brief 画矩形命令；交互由 DrawLineRectangleTool 驱动
class DrawLineRectangleCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawLineRectangleCommand)

public:
    /// @brief 预览两个对角点构成的矩形
    void previewRectangle(const DmVector& corner1, const DmVector& corner2)
    {
        preview().clear();
        DmPolyline* poly = createRectangle(preview().entities().getEntityContainer(), corner1, corner2);
        poly->setDocument(document());
        poly->update();
        preview().entities().addEntity(poly);
        preview().draw();
    }

    /// @brief 提交矩形，相对零点移到第二个角点
    void commitRectangle(const DmVector& corner1, const DmVector& corner2)
    {
        preview().clear();
        DmPolyline* polyline = createRectangle(nullptr, corner1, corner2);
        polyline->setDocument(document());
        polyline->update();
        Transaction t(tr("Create line rectangle").toStdString(), document());
        t.start();
        document()->getEntityTable()->add(polyline);
        t.commit();
        view()->moveRelativeZero(corner2);
    }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

namespace
{
/// @brief 画矩形工具：指定第一个角点，再指定第二个角点
class DrawLineRectangleTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetCorner1, ///< 设置第一个角点
        SetCorner2  ///< 设置第二个角点
    };

    DrawLineRectangleTool(DrawLineRectangleCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetCorner1:
            GUIDIALOGFACTORY->updateMouseWidget(DrawLineRectangleCommand::tr("Specify first corner"),
                                                DrawLineRectangleCommand::tr("Cancel"));
            break;
        case SetCorner2:
            GUIDIALOGFACTORY->updateMouseWidget(DrawLineRectangleCommand::tr("Specify second corner"),
                                                DrawLineRectangleCommand::tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        DmVector mouse = pointAt(e);
        if (SetCorner2 == status() && m_corner1.valid)
        {
            m_corner2 = mouse;
            m_command.previewRectangle(m_corner1, m_corner2);
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (Qt::LeftButton == e->button())
        {
            onCoordinate(pointAt(e));
        }
        else if (Qt::RightButton == e->button())
        {
            m_command.preview().clear();
            stepBack();
        }
    }

    void onCoordinate(const DmVector& mouse) override
    {
        switch (status())
        {
        case SetCorner1:
            m_corner1 = mouse;
            view()->moveRelativeZero(mouse);
            setStatus(SetCorner2);
            break;
        case SetCorner2:
            m_corner2 = mouse;
            m_command.commitRectangle(m_corner1, m_corner2);
            finishIfOrthogonal();
            setStatus(SetCorner1);
            break;
        default:
            break;
        }
    }

    void onCommand(GuiCommandEvent* e) override
    {
        // 与原 ActionDrawLineRectangle 一致：help 只列出命令（没有可用命令），不接受
        if (Commands::checkCommand("help", e->getCommand().toLower()))
        {
            GUIDIALOGFACTORY->commandMessage(Commands::msgAvailableCommands());
        }
    }

private:
    /// @brief 鼠标位置：正交限制下取原始坐标，否则取捕捉点
    DmVector pointAt(QMouseEvent* e)
    {
        DmVector pos = snapper()->snapPoint(e);
        if (DM::RestrictOrthogonal == snapper()->getSnapMode()->restriction)
        {
            pos = view()->toGraph(e->x(), e->y());
        }
        return pos;
    }

    DrawLineRectangleCommand& m_command;
    DmVector m_corner1; ///< 第一个角点
    DmVector m_corner2; ///< 第二个角点
};
}  // namespace

std::unique_ptr<BasePlaceTool> DrawLineRectangleCommand::createTool()
{
    return std::make_unique<DrawLineRectangleTool>(*this, document(), view());
}

ExclusiveCommandFactory DrawCommands::lineRectangle()
{
    return exclusiveCommandFactory<DrawLineRectangleCommand>();
}
