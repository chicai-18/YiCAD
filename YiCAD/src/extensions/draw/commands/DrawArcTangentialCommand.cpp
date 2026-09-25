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

/// @file DrawArcTangentialCommand.cpp
/// @brief 相切圆弧命令与工具的实现

#include "DrawArcTangentialCommand.h"

#include <cmath>

#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "DrawCommands.h"
#include "Commands.h"
#include "DmArc.h"
#include "DmAtomicEntity.h"
#include "DmDocument.h"
#include "EntityTable.h"
#include "GeometryMethods.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Math2d.h"
#include "Transaction.h"

/// @brief 相切圆弧工具：选基实体（点在哪端就从哪端开始），再指定终点
class DrawArcTangentialTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetBaseEntity, ///< 设置基实体
        SetEndAngle    ///< 设置终止角度
    };

    DrawArcTangentialTool(DrawArcTangentialCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    std::optional<DM::CursorType> getCursor() const override { return DM::SelectCursor; }

    /// @brief 按当前参数重算并重画预览（选项条改参数时也调用）
    void updatePreview()
    {
        if (status() == SetEndAngle)
        {
            prepareArc();
            if (m_command.arc().getData().isValid())
            {
                m_command.previewArc(m_command.arc().getData());
            }
        }
    }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetBaseEntity:
            GUIDIALOGFACTORY->updateMouseWidget(DrawArcTangentialCommand::tr("Specify base entity"),
                                                DrawArcTangentialCommand::tr("Cancel"));
            break;
        case SetEndAngle:
            GUIDIALOGFACTORY->updateMouseWidget(DrawArcTangentialCommand::tr("Specify end point"),
                                                DrawArcTangentialCommand::tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        if (status() == SetEndAngle)
        {
            m_point = snapper()->snapPoint(e);
            updatePreview();
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
        {
            switch (status())
            {
            case SetBaseEntity:
            {
                DmVector coord = view()->toGraph(e->pos().x(), e->pos().y());
                DmEntity* entity = snapper()->catchEntity(coord, DM::ResolveAll);
                if (!entity)
                {
                    break;
                }
                if (entity->getEntityType() == DM::EntityArc || entity->getEntityType() == DM::EntityLine
                    || entity->getEntityType() == DM::EntityPolyline)
                {
                    if (!entity->isContainer())
                    {
                        m_baseEntity = static_cast<DmAtomicEntity*>(entity);
                        m_isStartPoint =
                            m_baseEntity->getStartpoint().distanceTo(coord) < m_baseEntity->getEndpoint().distanceTo(coord);
                        setStatus(SetEndAngle);
                        updateHints();
                    }
                }
                else
                {
                    GUIDIALOGFACTORY->commandMessage(
                        DrawArcTangentialCommand::tr("This type does not support tangent arcs!"));
                }
                break;
            }
            case SetEndAngle:
                onCoordinate(snapper()->snapPoint(e));
                break;
            default:
                break;
            }
        }
        else if (e->button() == Qt::RightButton)
        {
            m_command.preview().clear();
            init(status() - 1);
        }
    }

    void onCoordinate(const DmVector& coord) override
    {
        if (status() == SetEndAngle)
        {
            m_point = coord;
            commit();
        }
    }

    void onCommand(GuiCommandEvent* e) override
    {
        if (Commands::checkCommand("help", e->getCommand().toLower()))
        {
            // 与原 Action 一致：列出命令后不接受
            GUIDIALOGFACTORY->commandMessage(Commands::msgAvailableCommands());
        }
    }

private:
    /// @brief 回到某一状态并丢弃基实体（原 init(status)）；status < 0 时结束命令
    void init(int s)
    {
        if (s < 0)
        {
            command().finish();
            return;
        }
        restart(s);
        m_baseEntity = nullptr;
        m_isStartPoint = false;
        m_point = {};
    }

    /// @brief 按锁定参数求圆弧，写进命令的圆弧，并刷新选项条的显示
    void prepareArc()
    {
        if (!(m_baseEntity && m_point.valid))
        {
            return;
        }
        DmVector startPoint;
        double direction = 0.0;
        if (m_isStartPoint)
        {
            startPoint = m_baseEntity->getStartpoint();
            direction = Math2d::correctAngle(m_baseEntity->getDirection1() + M_PI);
        }
        else
        {
            startPoint = m_baseEntity->getEndpoint();
            direction = Math2d::correctAngle(m_baseEntity->getDirection2() + M_PI);
        }

        bool res = false;
        double radius = 0.0;
        double startAngle = 0.0;
        double endAngle = 0.0;
        DmVector normal(true);
        DmVector center(true);
        const bool lockAngle = m_command.isLockAngle();
        const bool lockRadius = m_command.isLockRadius();
        if (!lockAngle && !lockRadius)
        {
            res = GeometryMethods::createArcInfoTangentialFree(startPoint, DmVector(direction), m_point, center,
                                                               normal, radius, startAngle, endAngle);
        }
        else if (lockAngle && !lockRadius)
        {
            res = GeometryMethods::createArcInfoTangentialLockAngle(startPoint, DmVector(direction), m_point,
                                                                    m_command.lockAngle(), center, normal, radius,
                                                                    startAngle, endAngle);
        }
        else if (!lockAngle && lockRadius)
        {
            res = GeometryMethods::createArcInfoTangentialLockRadius(startPoint, DmVector(direction), m_point,
                                                                     m_command.lockRadius(), center, normal, radius,
                                                                     startAngle, endAngle);
        }
        else
        {
            res = GeometryMethods::createArcInfoTangentialLockRadiusAngle(
                startPoint, DmVector(direction), m_point, m_command.lockRadius(), m_command.lockAngle(), center,
                normal, radius, startAngle, endAngle);
        }

        if (res)
        {
            DmArc arc(nullptr, ArcData(center, normal, radius, startAngle, endAngle));
            m_command.arc().setData(arc.getData());
            m_command.updateOptions(radius, lockRadius, Math2d::rad2deg(arc.getEndAngleNormal()), lockAngle);
        }
        else
        {
            // 设为无效
            m_command.arc().setRadius(0.0);
        }
    }

    /// @brief 提交圆弧后回到第一步（原 trigger()）
    void commit()
    {
        m_command.preview().clear();
        if (!(m_point.valid && m_baseEntity))
        {
            return;
        }
        prepareArc();
        if (!m_command.arc().getData().isValid())
        {
            GUIDIALOGFACTORY->commandMessage(DrawArcTangentialCommand::tr("Invalid input!"));
        }
        else
        {
            m_command.commitArc(m_command.arc().getData());
        }
        init(SetBaseEntity);
    }

    DrawArcTangentialCommand& m_command;
    DmAtomicEntity* m_baseEntity = nullptr; ///< 基实体
    bool m_isStartPoint = false;            ///< 是否点在基实体的起点一端
    DmVector m_point;                       ///< 决定终止角度的点
};

DrawArcTangentialCommand::DrawArcTangentialCommand()
    : m_arc(std::make_unique<DmArc>())
{
}

DrawArcTangentialCommand::~DrawArcTangentialCommand() = default;

std::unique_ptr<BasePlaceTool> DrawArcTangentialCommand::createTool()
{
    return std::make_unique<DrawArcTangentialTool>(*this, document(), view());
}

DrawArcTangentialTool* DrawArcTangentialCommand::tool() const
{
    return static_cast<DrawArcTangentialTool*>(placeTool());
}

double DrawArcTangentialCommand::getRadius() const
{
    return m_arc->getRadius();
}

double DrawArcTangentialCommand::getAngle() const
{
    return m_arc->getAngleLength();
}

void DrawArcTangentialCommand::updatePreview()
{
    if (tool())
    {
        tool()->updatePreview();
    }
}

void DrawArcTangentialCommand::updateOptions(double radius, bool lockRadius, double angle, bool lockAngle) const
{
    if (m_optionsUpdater)
    {
        m_optionsUpdater(radius, lockRadius, angle, lockAngle);
    }
}

void DrawArcTangentialCommand::previewArc(const ArcData& data)
{
    DmArc* arc = new DmArc(preview().entities().getEntityContainer(), data);
    arc->setDocument(document());
    preview().clear();
    preview().entities().addEntity(arc);
    preview().draw();
}

void DrawArcTangentialCommand::commitArc(const ArcData& data)
{
    Transaction t(tr("Create ArcTangential").toStdString(), document());
    t.start();
    DmArc* arc = new DmArc(nullptr, data);
    arc->setDocument(document());
    document()->getEntityTable()->add(arc);
    t.commit();
    view()->moveRelativeZero(arc->getCenter());
}

void DrawArcTangentialCommand::showOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, true);
}

void DrawArcTangentialCommand::hideOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, false);
}

ExclusiveCommandFactory DrawCommands::arcTangential()
{
    return exclusiveCommandFactory<DrawArcTangentialCommand>();
}
