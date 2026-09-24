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

/// @file InfoDistCommand.cpp
/// @brief 查询距离命令 info.dist，取代原 ActionInfoDist：两点间的距离、坐标差与极坐标

#include <memory>

#include <QCoreApplication>
#include <QMouseEvent>
#include <QStringList>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "DmUnits.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "PlaceCommand.h"

namespace
{
/// @brief 查询距离命令；交互由 InfoDistTool 驱动
class InfoDistCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(InfoDistCommand)

public:
    /// @brief 在命令行输出两点间的距离
    void report(const DmVector& p1, const DmVector& p2)
    {
        DmDocument* doc = document();
        const DmVector dV = p2 - p1;
        QStringList dists;
        for (double a : {dV.magnitude(), dV.x, dV.y})
        {
            dists << DmUnits::formatLinear(a, doc->getUnit(), doc->getLinearFormat(), doc->getLinearPrecision());
        }
        const QString angle = DmUnits::formatAngle(dV.angle(), doc->getAngleFormat(), doc->getAnglePrecision());
        GUIDIALOGFACTORY->commandMessage(tr("Distance: %1 Cartesian: (%2 , %3), Polar: (%4<%5)")
                                             .arg(dists[0])
                                             .arg(dists[1])
                                             .arg(dists[2])
                                             .arg(dists[0])
                                             .arg(angle));
    }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 查询距离工具：第一点，第二点
class InfoDistTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetPoint1, ///< 设置第一个点
        SetPoint2  ///< 设置第二个点
    };

    InfoDistTool(InfoDistCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetPoint1:
            GUIDIALOGFACTORY->updateMouseWidget(InfoDistCommand::tr("Specify first point of distance"),
                                                InfoDistCommand::tr("Cancel"));
            break;
        case SetPoint2:
            GUIDIALOGFACTORY->updateMouseWidget(InfoDistCommand::tr("Specify second point of distance"),
                                                InfoDistCommand::tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        DmVector mouse = snapper()->snapPoint(e);
        if (status() == SetPoint2 && m_point1.valid)
        {
            m_point2 = mouse;
            CommandPreview& preview = m_command.preview();
            preview.clear();
            preview.entities().addEntity(new DmLine(nullptr, m_point1, m_point2));
            preview.draw();
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
        case SetPoint1:
            m_point1 = mouse;
            view()->moveRelativeZero(m_point1);
            setStatus(SetPoint2);
            break;
        case SetPoint2:
            if (m_point1.valid)
            {
                m_point2 = mouse;
                m_command.preview().clear();
                view()->moveRelativeZero(m_point2);
                if (m_point1.valid && m_point2.valid)
                {
                    m_command.report(m_point1, m_point2);
                }
                setStatus(SetPoint1);
            }
            break;
        default:
            break;
        }
    }

private:
    InfoDistCommand& m_command;
    DmVector m_point1; ///< 第一个点
    DmVector m_point2; ///< 第二个点
};

std::unique_ptr<BasePlaceTool> InfoDistCommand::createTool()
{
    return std::make_unique<InfoDistTool>(*this, document(), view());
}

const bool g_registered = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionInfoDist, QStringLiteral("info.dist"), exclusiveCommandFactory<InfoDistCommand>());
}  // namespace
