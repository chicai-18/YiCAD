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

/// @file DrawPolylineCommand.cpp
/// @brief 画多段线命令与画多段线工具的实现

#include "DrawPolylineCommand.h"

#include <cmath>
#include <vector>

#include <QList>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "Commands.h"
#include "DmArc.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "DmPolyline.h"
#include "DmSolid.h"
#include "EntityTable.h"
#include "GeometryMethods.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Math2d.h"
#include "Transaction.h"

namespace
{
/// @brief 包角（度）换算凸度时的除数：bulge = tan(angle * π / 720)
constexpr double ANGLE_TO_BULGE_DIVISOR = 720.0;
/// @brief 小于该凸度按直线处理
constexpr double SMALL_BULGE_THRESHOLD = 1E-5;
}  // namespace

/// @brief 画多段线工具：指定起点，再逐点添加线段
class DrawPolylineTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetStartpoint, ///< 设置起始点
        SetNextPoint   ///< 设置下一个点
    };

    DrawPolylineTool(DrawPolylineCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    /// @brief 闭合多段线
    void close()
    {
        if (m_polyline && (m_polyline->getDataConstRef().getVertexCount() >= 2))
        {
            m_polyline->setClosed(true);
            m_polyline->getDataRef().appendBulge(0.0);
            m_polyline->getDataRef().appendLineWeight(m_command.getStartWeight(), m_command.getEndWeight());
            m_polyline->update();
            m_command.updatePolyline(m_polyline);
            finishPolyline();
            setStatus(SetStartpoint);
            view()->moveRelativeZero(m_start);
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(
                DrawPolylineCommand::tr("Cannot close sequence of lines: Not enough entities defined yet."));
        }
    }

    /// @brief 撤销上一点
    void undo()
    {
        if (m_history.size() > 1)
        {
            m_history.removeLast();
            m_bulgeHistory.removeLast();
            m_command.preview().clear();
            m_point = m_history.last();

            if (m_history.size() == 1)
            {
                view()->moveRelativeZero(m_history.front());
                m_command.removePolyline(m_polyline);
                m_polyline = nullptr;
                view()->redraw();
            }
            if (m_polyline)
            {
                auto& dataRef = m_polyline->getDataRef();
                std::vector<DmVector> vertexs = dataRef.getVertexs();
                vertexs.erase(vertexs.end() - 1);
                dataRef.setVertexs(vertexs);

                std::vector<double> bulges = dataRef.getBulges();
                bulges.erase(bulges.end() - 1);
                dataRef.setBulges(bulges);

                std::vector<double> weights = dataRef.getLineWeights();
                weights.erase(weights.end() - 2, weights.end() - 1);
                dataRef.setLineWeights(weights);

                m_polyline->update();
                m_command.updatePolyline(m_polyline);
                view()->moveRelativeZero(m_polyline->getEndpoint());
                view()->redraw();
            }
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(DrawPolylineCommand::tr("Cannot undo: Not enough entities defined yet."));
        }
    }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetStartpoint:
            GUIDIALOGFACTORY->updateMouseWidget(DrawPolylineCommand::tr("Specify first point"),
                                                DrawPolylineCommand::tr("Cancel"));
            break;
        case SetNextPoint:
        {
            QString msg = "";
            if (m_history.size() >= 3)
            {
                msg += Commands::command("close");
                msg += "/";
            }
            if (m_history.size() >= 2)
            {
                msg += Commands::command("undo");
            }

            if (m_history.size() >= 2)
            {
                GUIDIALOGFACTORY->updateMouseWidget(DrawPolylineCommand::tr("Specify next point or [%1]").arg(msg),
                                                    DrawPolylineCommand::tr("Back"));
            }
            else
            {
                GUIDIALOGFACTORY->updateMouseWidget(DrawPolylineCommand::tr("Specify next point"),
                                                    DrawPolylineCommand::tr("Back"));
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
        double bulge = solveBulge(mouse);
        if ((status() == SetNextPoint) && m_point.valid)
        {
            if ((std::fabs(bulge) < DM_TOLERANCE) || (m_command.getMode() == DrawPolylineCommand::Line))
            {
                m_command.previewLineSegment(m_point, mouse);
            }
            else
            {
                m_command.previewArcSegment(m_arcData);
            }
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
            if (status() == SetNextPoint)
            {
                finishPolyline();
            }
            m_command.preview().clear();
            snapper()->deleteSnapper();
            // 原 init(getStatus() - 1)：退回时丢弃采集的点
            if (status() <= 0)
            {
                command().finish();
                return;
            }
            reset();
            restart(status() - 1);
        }
    }

    void onCoordinate(const DmVector& mouse) override
    {
        double bulge = solveBulge(mouse);
        switch (status())
        {
        case SetStartpoint:
            m_point = mouse;
            m_history.clear();
            m_history.append(mouse);
            m_bulgeHistory.clear();
            m_bulgeHistory.append(0.0);
            m_start = m_point;
            setStatus(SetNextPoint);
            view()->moveRelativeZero(mouse);
            updateHints();
            break;

        case SetNextPoint:
            if (bulge == 0.0)
            {
                addNextPoint(mouse, bulge);
            }
            else
            {
                DmArc arc(nullptr, m_arcData);
                addNextPoint(arc.getEndpoint(), arc.getBulge());
            }
            break;

        default:
            break;
        }
    }

    void onCommand(GuiCommandEvent* e) override
    {
        QString c = e->getCommand().toLower();

        switch (status())
        {
        case SetStartpoint:
            if (Commands::checkCommand("help", c))
            {
                // 与原 ActionDrawPolyline 一致：列出命令后不接受
                GUIDIALOGFACTORY->commandMessage(Commands::msgAvailableCommands()
                                                 + availableCommands().join(", "));
                return;
            }
            break;

        case SetNextPoint:
        {
            // 输入长度：沿上一点指向鼠标的方向添加直线段
            bool ok = false;
            double length = Math2d::eval(c, &ok);
            if (ok)
            {
                DmVector vec(1.0, 0.0);
                if (m_mouse.valid)
                {
                    vec = (m_mouse - m_point).normalize();
                }
                addNextPoint(m_point + vec * length, 0.0);
                e->accept();
            }
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
            break;
        }

        default:
            break;
        }
    }

private:
    /// @brief 丢弃采集的点（原 reset()）
    void reset()
    {
        m_polyline = nullptr;
        m_start = {};
        m_history.clear();
        m_bulgeHistory.clear();
    }

    /// @brief 结束当前多段线：相对零点移到它的终点（原 trigger()）
    void finishPolyline()
    {
        m_command.preview().clear();
        if (!m_polyline)
        {
            return;
        }
        snapper()->deleteSnapper();
        view()->moveRelativeZero(m_polyline->getEndpoint());
        snapper()->drawSnapper();
        m_polyline = nullptr;
    }

    /// @brief 按当前模式求下一段的凸度，圆弧段的数据记在 m_arcData
    double solveBulge(const DmVector& mouse)
    {
        double b = 0.0;
        bool suc = false;
        double direction = 0.0;
        DmVector arcCenter(true);
        DmVector arcNormal(true);
        double arcRadius = 0.0;
        double arcStartAngle = 0.0;
        double arcEndAngle = 0.0;

        switch (m_command.getMode())
        {
        case DrawPolylineCommand::Tangential:
            if (m_polyline)
            {
                direction = lastPartDirection();
                suc = GeometryMethods::createArcInfoTangentialFree(m_point, DmVector(direction), mouse, arcCenter,
                                                                   arcNormal, arcRadius, arcStartAngle, arcEndAngle);
                if (suc)
                {
                    m_arcData = ArcData(arcCenter, arcNormal, arcRadius, arcStartAngle, arcEndAngle);
                    b = DmArc(nullptr, m_arcData).getBulge();
                }
                break;
            }
            [[fallthrough]];
        case DrawPolylineCommand::TanRad:
            if (m_polyline)
            {
                direction = lastPartDirection();
                suc = GeometryMethods::createArcInfoTangentialLockRadius(
                    m_point, DmVector(direction), mouse, m_command.getRadius(), arcCenter, arcNormal, arcRadius,
                    arcStartAngle, arcEndAngle);
                if (suc)
                {
                    m_arcData = ArcData(arcCenter, arcNormal, arcRadius, arcStartAngle, arcEndAngle);
                    b = DmArc(nullptr, m_arcData).getBulge();
                }
            }
            break;
        case DrawPolylineCommand::Ang:
        {
            if (m_command.isCCW())
            {
                b = std::tan(m_command.getAngle() * M_PI / ANGLE_TO_BULGE_DIVISOR);
            }
            else
            {
                b = std::tan(-m_command.getAngle() * M_PI / ANGLE_TO_BULGE_DIVISOR);
            }
            if (std::fabs(b) > SMALL_BULGE_THRESHOLD)
            {
                DmVector center(true);
                double radius = 0.0;
                double startAng = 0.0;
                double endAng = 0.0;
                DmVector normal(true);
                GeometryMethods::getArcInfo(m_point, mouse, b, center, radius, startAng, endAng, normal);
                DmArc arcCalc(nullptr, ArcData(center, normal, radius, startAng, endAng));
                m_arcData = arcCalc.getData();
            }
            else
            {
                b = 0.0;
            }
            break;
        }
        default:
            break;
        }
        return b;
    }

    /// @brief 添加下一点：第一段落下时多段线加入文档，此后就地修改
    void addNextPoint(const DmVector& mouse, double bulge)
    {
        view()->moveRelativeZero(mouse);
        m_point = mouse;
        m_history.append(mouse);
        m_bulgeHistory.append(bulge);
        if (!m_polyline)
        {
            m_polyline = new DmPolyline(nullptr, PolylineData());
            m_polyline->setDocument(document());
            m_polyline->getDataRef().setVertexs({m_start});
        }

        m_polyline->insertVertex(m_polyline->getDataConstRef().getVertexCount(), mouse, bulge,
                                 m_command.getStartWeight(), m_command.getEndWeight());
        m_polyline->update();
        if (m_polyline->getDataConstRef().getBulgesCount() == 1)
        {
            m_command.addPolyline(m_polyline);
        }
        else
        {
            m_command.updatePolyline(m_polyline);
        }
        m_command.preview().clear();
        snapper()->deleteSnapper();
        view()->redraw();
        snapper()->drawSnapper();
        updateHints();
    }

    /// @brief 最后一段在终点处的切线方向
    double lastPartDirection() const
    {
        double direction = 0.0;
        const auto& dataRef = m_polyline->getDataConstRef();
        double lastBulge = dataRef.getBulgeAt(dataRef.getBulgesCount() - 1);
        DmVector lastVertex = dataRef.getVertexAt(dataRef.getVertexCount() - 1);
        DmVector lastSecVertex = dataRef.getVertexAt(dataRef.getVertexCount() - 2);
        if (lastBulge == 0.0)
        {
            direction = lastSecVertex.angleTo(lastVertex);
        }
        else
        {
            DmVector center(true);
            double radius = 0.0;
            double startAng = 0.0;
            double endAng = 0.0;
            DmVector normal(true);
            GeometryMethods::getArcInfo(lastSecVertex, lastVertex, lastBulge, center, radius, startAng, endAng,
                                        normal);
            DmArc lastArc(nullptr, ArcData(center, normal, radius, startAng, endAng));
            direction = Math2d::correctAngle(lastArc.getDirection2() + M_PI);
        }
        return direction;
    }

    /// @brief 当前状态下可用的命令行命令（help 列出）
    QStringList availableCommands() const
    {
        QStringList cmd;
        if (status() == SetNextPoint)
        {
            if (m_history.size() >= 2)
            {
                cmd += Commands::command("undo");
            }
            if (m_history.size() >= 3)
            {
                cmd += Commands::command("close");
            }
        }
        return cmd;
    }

    DrawPolylineCommand& m_command;
    ArcData m_arcData;               ///< 圆弧段数据
    DmPolyline* m_polyline = nullptr; ///< 正在绘制的多段线（加入文档后归文档所有）
    DmVector m_point;                ///< 上一个点
    DmVector m_start;                ///< 起始点，用于闭合
    DmVector m_mouse;                ///< 当前鼠标位置
    QList<DmVector> m_history;       ///< 点历史记录，用于撤销
    QList<double> m_bulgeHistory;    ///< 凸度历史记录，用于撤销
};

DrawPolylineCommand::DrawPolylineCommand() = default;

DrawPolylineCommand::~DrawPolylineCommand() = default;

std::unique_ptr<BasePlaceTool> DrawPolylineCommand::createTool()
{
    return std::make_unique<DrawPolylineTool>(*this, document(), view());
}

DrawPolylineTool* DrawPolylineCommand::tool() const
{
    return static_cast<DrawPolylineTool*>(placeTool());
}

void DrawPolylineCommand::close()
{
    if (tool())
    {
        tool()->close();
    }
}

void DrawPolylineCommand::undo()
{
    if (tool())
    {
        tool()->undo();
    }
}

void DrawPolylineCommand::previewLineSegment(const DmVector& from, const DmVector& to)
{
    preview().clear();
    Preview& entities = preview().entities();
    if ((m_startWeight == 0.0) && (m_endWeight == 0.0))
    {
        DmLine* line = new DmLine(entities.getEntityContainer(), from, to);
        line->setDocument(document());
        entities.addEntity(line);
    }
    else
    {
        constexpr double HALF = 0.5;
        auto normalize = GeometryMethods::getPerpendicularNormalizeVector(from, to);
        auto pt1 = from + normalize * m_startWeight * HALF;
        auto pt2 = from - normalize * m_startWeight * HALF;
        auto pt3 = to + normalize * m_endWeight * HALF;
        auto pt4 = to - normalize * m_endWeight * HALF;
        std::vector<DmVector> vertexs = {pt1, pt2, pt3, pt4};
        DmSolid* entity = new DmSolid(entities.getEntityContainer(), SolidData(vertexs));
        entity->setDocument(document());
        entities.addEntity(entity);
    }
    preview().draw();
}

void DrawPolylineCommand::previewArcSegment(const ArcData& arcData)
{
    preview().clear();
    DmArc arc(nullptr, arcData);
    std::vector<DmEntity*> ents;
    DmPolyline::getEntitiesByInfo(arc.getStartpoint(), arc.getEndpoint(), arc.getBulge(), m_startWeight,
                                  m_endWeight, ents);
    for (auto ent : ents)
    {
        ent->setParent(nullptr);
        ent->setDocument(document());
        preview().entities().addEntity(ent);
    }
    preview().draw();
}

void DrawPolylineCommand::addPolyline(DmPolyline* polyline)
{
    // 事务名沿用原 ActionDrawPolyline 的"Add cloud line"
    Transaction t(tr("Add cloud line").toStdString(), document());
    t.start();
    document()->getEntityTable()->add(polyline);
    t.commit();
}

void DrawPolylineCommand::updatePolyline(DmPolyline* polyline)
{
    document()->specifyModifiedEntity(polyline);
}

void DrawPolylineCommand::removePolyline(DmPolyline* polyline)
{
    document()->getEntityTable()->remove_direct(polyline);
}

void DrawPolylineCommand::showOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, true);
}

void DrawPolylineCommand::hideOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, false);
}

namespace
{
const bool g_registered = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionDrawPolyline, QStringLiteral("draw.polyline"), exclusiveCommandFactory<DrawPolylineCommand>());
}  // namespace
