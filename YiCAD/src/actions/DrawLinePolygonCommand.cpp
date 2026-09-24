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

/// @file DrawLinePolygonCommand.cpp
/// @brief 画正多边形命令与工具的实现

#include "DrawLinePolygonCommand.h"

#include <cmath>

#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "Commands.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "DmLine.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Transaction.h"

namespace
{
constexpr double TWO_PI = 2.0 * M_PI;
/// @brief 中心+角点命令行输入边数的上限（不含）
constexpr int MAX_CEN_COR_EDGES = 10000;
/// @brief 中心+切点命令行输入边数的上限（含）
constexpr int MAX_CEN_TAN_SIDES = 9999;

/// @brief 画正多边形工具：指定中心，再指定第二点；命令行可输入 number 改边数
class LinePolygonTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetCenter, ///< 设置中心点
        SetPoint,  ///< 设置角点或切点
        SetNumber  ///< 命令行输入边数
    };

    LinePolygonTool(LinePolygonCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

protected:
    void updateHints() override
    {
        // 与原 Action 一致：右键没有提示
        switch (status())
        {
        case SetCenter:
            GUIDIALOGFACTORY->updateMouseWidget(LinePolygonCommand::tr("Specify center"), "");
            break;
        case SetPoint:
            GUIDIALOGFACTORY->updateMouseWidget(m_command.secondPointHint(), "");
            break;
        case SetNumber:
            GUIDIALOGFACTORY->updateMouseWidget(LinePolygonCommand::tr("Enter number:"), "");
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        DmVector mouse = snapper()->snapPoint(e);
        if (status() == SetPoint && m_center.valid)
        {
            m_point = mouse;
            m_command.previewPolygon(m_center, m_point);
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
        {
            onCoordinate(snapper()->snapPoint(e));
        }
        else if (e->button() == Qt::RightButton)
        {
            m_command.preview().clear();
            stepBack();
        }
    }

    void onCoordinate(const DmVector& mouse) override
    {
        switch (status())
        {
        case SetCenter:
            m_center = mouse;
            setStatus(SetPoint);
            view()->moveRelativeZero(mouse);
            break;
        case SetPoint:
            m_point = mouse;
            m_command.commitPolygon(m_center, m_point);
            finishIfOrthogonal();
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
        case SetCenter:
        case SetPoint:
            // 与原 Action 一致：进入输入边数的状态，但不接受这段文本
            if (Commands::checkCommand("number", c))
            {
                m_command.preview().clear();
                m_lastStatus = static_cast<Status>(status());
                setStatus(SetNumber);
            }
            break;

        case SetNumber:
        {
            bool ok = false;
            int n = c.toInt(&ok);
            if (ok)
            {
                e->accept();
                m_command.inputNumber(n);
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(LinePolygonCommand::tr("Not a valid expression"));
            }
            GUIDIALOGFACTORY->requestCommandOptions(&m_command, true, true);
            setStatus(m_lastStatus);
            break;
        }

        default:
            break;
        }
    }

private:
    /// @brief 当前状态下可用的命令行命令（help 列出）
    QStringList availableCommands() const
    {
        QStringList cmd;
        if (status() == SetCenter || status() == SetPoint)
        {
            cmd += Commands::command("number");
        }
        return cmd;
    }

    LinePolygonCommand& m_command;
    DmVector m_center;              ///< 多边形中心点
    DmVector m_point;               ///< 角点或切点
    Status m_lastStatus = SetCenter; ///< 进入输入边数前的状态
};

/// @brief 以中心、半径、起始角画 number 条边
std::vector<DmLine*> createRegularPolygon(DmEntityContainer* container, DmDocument* doc, const DmVector& center,
                                          const DmVector& corner, int number)
{
    std::vector<DmLine*> ret;
    const double r = center.distanceTo(corner);
    const double angle0 = center.angleTo(corner);
    const double da = TWO_PI / number;
    for (int i = 0; i < number; ++i)
    {
        const DmVector& c0 = center + DmVector::polar(r, angle0 + i * da);
        const DmVector& c1 = center + DmVector::polar(r, angle0 + ((i + 1) % number) * da);
        DmLine* line = new DmLine(container, c0, c1);
        line->setDocument(doc);
        ret.emplace_back(line);
        if (container)
        {
            container->addEntity(line);
        }
    }
    return ret;
}
}  // namespace

std::unique_ptr<BasePlaceTool> LinePolygonCommand::createTool()
{
    return std::make_unique<LinePolygonTool>(*this, document(), view());
}

void LinePolygonCommand::previewPolygon(const DmVector& center, const DmVector& point)
{
    preview().clear();
    createPolygon(preview().entities().getEntityContainer(), center, point);
    preview().draw();
}

void LinePolygonCommand::commitPolygon(const DmVector& center, const DmVector& point)
{
    preview().clear();
    auto lines = createPolygon(nullptr, center, point);
    if (!lines.empty())
    {
        Transaction t(transactionName().toStdString(), document());
        t.start();
        for (auto line : lines)
        {
            document()->getEntityTable()->add(line);
        }
        t.commit();
    }
}

void LinePolygonCommand::showOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, true);
}

void LinePolygonCommand::hideOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, false);
}

QString DrawLinePolygonCenCorCommand::secondPointHint() const
{
    return tr("Specify a corner");
}

void DrawLinePolygonCenCorCommand::inputNumber(int n)
{
    if (n > 0 && n < MAX_CEN_COR_EDGES)
    {
        m_number = n;
    }
    else
    {
        GUIDIALOGFACTORY->commandMessage(tr("Not a valid number. Try 1..9999"));
    }
}

std::vector<DmLine*> DrawLinePolygonCenCorCommand::createPolygon(DmEntityContainer* container,
                                                                 const DmVector& center,
                                                                 const DmVector& corner) const
{
    if (!center.valid || !corner.valid || m_number < MIN_POLYGON_EDGES)
    {
        return {};
    }
    return createRegularPolygon(container, document(), center, corner, m_number);
}

QString DrawLinePolygonCenCorCommand::transactionName() const
{
    return tr("Create line center corner");
}

QString DrawLinePolygonCenTanCommand::secondPointHint() const
{
    return tr("Specify a tangent");
}

void DrawLinePolygonCenTanCommand::inputNumber(int n)
{
    if (n > 0 && n <= MAX_CEN_TAN_SIDES)
    {
        m_number = n;
    }
    else
    {
        GUIDIALOGFACTORY->commandMessage(tr("Not a valid number. Try 1..%1").arg(MAX_CEN_TAN_SIDES));
    }
}

std::vector<DmLine*> DrawLinePolygonCenTanCommand::createPolygon(DmEntityContainer* container,
                                                                 const DmVector& center,
                                                                 const DmVector& tangent) const
{
    if (!center.valid || !tangent.valid || m_number < MIN_POLYGON_EDGES)
    {
        return {};
    }
    // 切点是边的中点：先求出一个角点
    DmVector corner(0, 0);
    const double angle = TWO_PI / m_number / 2.0;
    corner.x = tangent.x + (center.y - tangent.y) * std::tan(angle);
    corner.y = tangent.y + (tangent.x - center.x) * std::tan(angle);
    return createRegularPolygon(container, document(), center, corner, m_number);
}

QString DrawLinePolygonCenTanCommand::transactionName() const
{
    return tr("Create line polygon center tan");
}

namespace
{
const bool g_registeredCenCor = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionDrawLinePolygonCenCor, QStringLiteral("draw.line_polygon_cen_cor"),
    exclusiveCommandFactory<DrawLinePolygonCenCorCommand>());

const bool g_registeredCenTan = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionDrawLinePolygonCenTan, QStringLiteral("draw.line_polygon_cen_tan"),
    exclusiveCommandFactory<DrawLinePolygonCenTanCommand>());
}  // namespace
