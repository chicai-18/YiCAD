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

/// @file DrawEllipseAxisCommand.cpp
/// @brief 轴端点画椭圆命令 draw.ellipse_axis 与椭圆弧命令 draw.ellipse_arc_axis，
///        取代原 ActionDrawEllipseAxis：中心、长轴端点、短轴，椭圆弧再加起止角

#include <cmath>
#include <memory>

#include <QCoreApplication>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "Commands.h"
#include "DmDocument.h"
#include "DmEllipse.h"
#include "DmLine.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Math2d.h"
#include "PlaceCommand.h"
#include "Transaction.h"

namespace
{
constexpr double ELLIPSE_DEFAULT_RATIO = 0.5;      ///< 默认短轴/长轴比
constexpr double ELLIPSE_DEFAULT_ANGLE = 0.0;      ///< 默认角度（弧度）
constexpr double ELLIPSE_FULL_CIRCLE = 2.0 * M_PI; ///< 完整椭圆的角度范围
constexpr double ELLIPSE_MAJOR_LENGTH = 1.0;       ///< 命令模式下的默认长轴长度
constexpr double ELLIPSE_Z_COORD = 0.0;            ///< Z轴坐标分量
constexpr double ELLIPSE_ANGLE_PREVIEW = 1.0;      ///< 角度预览增量
constexpr double ELLIPSE_LINE_HALF = 2.0;          ///< 线长除半因子

/// @brief 轴端点画椭圆（弧）命令；交互由 DrawEllipseAxisTool 驱动
class DrawEllipseAxisCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawEllipseAxisCommand)

public:
    /// @param isArc true 画椭圆弧，false 画完整椭圆
    explicit DrawEllipseAxisCommand(bool isArc)
        : m_isArc(isArc)
    {
    }

    bool isArc() const { return m_isArc; }

    /// @brief 提交椭圆；短轴比大于 1 时交换长短轴
    void commitEllipse(const EllipseData& data, bool switchAxes)
    {
        preview().clear();
        DmEllipse* ellipse = new DmEllipse(nullptr, data);
        ellipse->setDocument(document());
        if (switchAxes)
        {
            ellipse->switchMajorMinor();
        }
        Transaction t(tr("Add ellipse").toStdString(), document());
        t.start();
        document()->getEntityTable()->add(ellipse);
        t.commit();
    }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;

private:
    bool m_isArc = false;
};

/// @brief 轴端点画椭圆（弧）工具
class DrawEllipseAxisTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetCenter, ///< 设置中心点
        SetMajor,  ///< 设置长轴端点
        SetMinor,  ///< 设置短轴比
        SetAngle1, ///< 设置起始角度
        SetAngle2  ///< 设置结束角度
    };

    DrawEllipseAxisTool(DrawEllipseAxisCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
        m_points.isArc = command.isArc();
        m_points.endAngle = command.isArc() ? ELLIPSE_FULL_CIRCLE : ELLIPSE_DEFAULT_ANGLE;
        // 原 init(0)：各项数据回到默认
        resetFrom(SetCenter);
    }

protected:
    void updateHints() override;
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;
    void onCoordinate(const DmVector& coord) override;
    void onCommand(GuiCommandEvent* e) override;

private:
    /// @brief 正在画的椭圆
    struct Points
    {
        DmVector center;                          ///< 椭圆中心点
        DmVector m_vMajorP;                       ///< 长轴端点向量
        double ratio{ELLIPSE_DEFAULT_RATIO};      ///< 短轴/长轴比
        double startAngle{ELLIPSE_DEFAULT_ANGLE}; ///< 起始角度
        double endAngle{ELLIPSE_DEFAULT_ANGLE};   ///< 结束角度
        bool isArc{false};                        ///< 为 true 创建椭圆弧
        DmVector mouse;                           ///< 鼠标当前位置
    };

    /// @brief 把 status 及之后各步的数据回到默认（原 init(status) 的后半部分）
    void resetFrom(int s)
    {
        if (s == SetCenter)
        {
            m_points.center = {};
        }
        if (s <= SetMajor)
        {
            m_points.m_vMajorP = {};
        }
        if (s <= SetMinor)
        {
            m_points.ratio = ELLIPSE_DEFAULT_RATIO;
        }
        if (s <= SetAngle1)
        {
            m_points.startAngle = ELLIPSE_DEFAULT_ANGLE;
        }
        if (s <= SetAngle2)
        {
            m_points.endAngle = ELLIPSE_DEFAULT_ANGLE;
        }
    }

    /// @brief 回到某一状态（原 init(status)）：status < 0 时结束命令
    void init(int s)
    {
        if (s < 0)
        {
            command().finish();
            return;
        }
        restart(s);
        resetFrom(s);
    }

    /// @brief 提交：正交限制下结束命令，否则回到第一步（原 trigger()）
    void commit()
    {
        EllipseData ed(m_points.center, m_points.m_vMajorP,
                       DmVector(ELLIPSE_Z_COORD, ELLIPSE_Z_COORD, ELLIPSE_MAJOR_LENGTH), m_points.ratio,
                       !m_points.isArc, m_points.startAngle, m_points.endAngle);
        m_command.commitEllipse(ed, m_points.ratio > 1.0);
        snapper()->drawSnapper();
        if (snapper()->getSnapMode()->restriction == DM::RestrictOrthogonal)
        {
            command().finish();
        }
        else
        {
            setStatus(SetCenter);
        }
    }

    DrawEllipseAxisCommand& m_command;
    Points m_points;
};

void DrawEllipseAxisTool::onMouseMove(QMouseEvent* e)
{
    DmVector mouse = snapper()->snapPoint(e);
    m_points.mouse = mouse;

    switch (status())
    {
    case SetMajor:
        if (m_points.center.valid)
        {
            m_command.preview().clear();
            EllipseData ed(m_points.center, mouse - m_points.center,
                           DmVector(ELLIPSE_Z_COORD, ELLIPSE_Z_COORD, ELLIPSE_MAJOR_LENGTH),
                           ELLIPSE_DEFAULT_RATIO, !m_points.isArc,
                           ELLIPSE_DEFAULT_ANGLE,
                           m_points.isArc ? ELLIPSE_FULL_CIRCLE : ELLIPSE_DEFAULT_ANGLE);
            auto ellipsePtr = new DmEllipse(m_command.preview().entities().getEntityContainer(), ed);
            ellipsePtr->setDocument(document());
            m_command.preview().entities().addEntity(ellipsePtr);
            m_command.preview().draw();
        }
        break;

    case SetMinor:
        if (m_points.center.valid && m_points.m_vMajorP.valid)
        {
            m_command.preview().clear();
            DmLine line{m_points.center - m_points.m_vMajorP, m_points.center + m_points.m_vMajorP};
            double d = line.getDistanceToPoint(mouse);
            m_points.ratio = d / (line.getLength() / ELLIPSE_LINE_HALF);
            EllipseData ed(m_points.center, m_points.m_vMajorP,
                           DmVector(ELLIPSE_Z_COORD, ELLIPSE_Z_COORD, ELLIPSE_MAJOR_LENGTH),
                           m_points.ratio, !m_points.isArc,
                           ELLIPSE_DEFAULT_ANGLE,
                           m_points.isArc ? ELLIPSE_FULL_CIRCLE : ELLIPSE_DEFAULT_ANGLE);
            auto ellipsePtr = new DmEllipse(m_command.preview().entities().getEntityContainer(), ed);
            ellipsePtr->setDocument(document());
            m_command.preview().entities().addEntity(ellipsePtr);
            m_command.preview().draw();
        }
        break;

    case SetAngle1:
        if (m_points.center.valid && m_points.m_vMajorP.valid)
        {
            m_command.preview().clear();

            DmVector m = mouse;
            m.rotate(m_points.center, -m_points.m_vMajorP.angle());
            DmVector v = m - m_points.center;
            v.y /= m_points.ratio;
            m_points.startAngle = v.angle();

            m_command.preview().entities().addEntity(new DmLine(nullptr, m_points.center, mouse));
            EllipseData ed(m_points.center, m_points.m_vMajorP,
                           DmVector(ELLIPSE_Z_COORD, ELLIPSE_Z_COORD, ELLIPSE_MAJOR_LENGTH),
                           m_points.ratio, false, m_points.startAngle,
                           m_points.startAngle + ELLIPSE_ANGLE_PREVIEW);
            auto ellipsePtr = new DmEllipse(m_command.preview().entities().getEntityContainer(), ed);
            ellipsePtr->setDocument(document());
            m_command.preview().entities().addEntity(ellipsePtr);
            m_command.preview().draw();
        }
        break;

    case SetAngle2:
        if (m_points.center.valid && m_points.m_vMajorP.valid)
        {
            m_command.preview().clear();

            DmVector m = mouse;
            m.rotate(m_points.center, -m_points.m_vMajorP.angle());
            DmVector v = m - m_points.center;
            v.y /= m_points.ratio;
            m_points.endAngle = v.angle();

            m_command.preview().entities().addEntity(new DmLine(nullptr, m_points.center, mouse));
            EllipseData ed(m_points.center, m_points.m_vMajorP,
                           DmVector(ELLIPSE_Z_COORD, ELLIPSE_Z_COORD, ELLIPSE_MAJOR_LENGTH),
                           m_points.ratio, false, m_points.startAngle, m_points.endAngle);
            auto ellipsePtr = new DmEllipse(m_command.preview().entities().getEntityContainer(), ed);
            ellipsePtr->setDocument(document());
            m_command.preview().entities().addEntity(ellipsePtr);
            m_command.preview().draw();
        }

    default:
        break;
    }
}

void DrawEllipseAxisTool::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        onCoordinate(snapper()->snapPoint(e));
    }
    else if (e->button() == Qt::RightButton)
    {
        m_command.preview().clear();
        init(status() - 1);
    }
}

void DrawEllipseAxisTool::onCoordinate(const DmVector& coord){

    DmVector const& mouse = coord;

    switch (status())
    {
    case SetCenter:
        m_points.center = mouse;
        view()->moveRelativeZero(mouse);
        setStatus(SetMajor);
        break;

    case SetMajor:
        m_points.m_vMajorP = mouse - m_points.center;
        setStatus(SetMinor);
        break;

    case SetMinor:
        {
            DmLine line{m_points.center + m_points.m_vMajorP, m_points.center - m_points.m_vMajorP};
            double d = line.getDistanceToPoint(mouse);
            m_points.ratio = d / (line.getLength() / ELLIPSE_LINE_HALF);
            if (!m_points.isArc)
            {
                commit();
                finishIfOrthogonal();
            }
            else
            {
                setStatus(SetAngle1);
            }
        }
        break;

    case SetAngle1:
        {
            DmVector m = mouse;
            m.rotate(m_points.center, -m_points.m_vMajorP.angle());
            DmVector v = m - m_points.center;
            v.y /= m_points.ratio;
            m_points.startAngle = v.angle();
            setStatus(SetAngle2);
        }
        break;

    case SetAngle2:
        {
            DmVector m = mouse;
            m.rotate(m_points.center, -m_points.m_vMajorP.angle());
            DmVector v = m - m_points.center;
            v.y /= m_points.ratio;
            m_points.endAngle = v.angle();
            commit();
            // 与原 Action 一致：椭圆弧用鼠标指定终止角后结束命令
            command().finish();
        }
        break;

    default:
        break;
    }
}

void DrawEllipseAxisTool::onCommand(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();
    bool ok = false;
    double m = Math2d::eval(c, &ok);

    if (Commands::checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(Commands::msgAvailableCommands());
        return;
    }

    switch (status())
    {
    case SetMajor:
        {
            if (ok)
            {
                e->accept();
                DmVector vec(ELLIPSE_MAJOR_LENGTH, ELLIPSE_DEFAULT_ANGLE);
                if (m_points.mouse)
                {
                    vec = (m_points.mouse - m_points.center).normalize();
                }
                m_points.m_vMajorP = -vec * m;
                setStatus(SetMinor);

                // 重绘预览
                m_command.preview().clear();
                EllipseData ed(m_points.center, m_points.m_vMajorP,
                               DmVector(ELLIPSE_Z_COORD, ELLIPSE_Z_COORD, ELLIPSE_MAJOR_LENGTH),
                               ELLIPSE_DEFAULT_RATIO, !m_points.isArc,
                               ELLIPSE_DEFAULT_ANGLE,
                               m_points.isArc ? ELLIPSE_FULL_CIRCLE : ELLIPSE_DEFAULT_ANGLE);
                m_command.preview().entities().addEntity(new DmEllipse(nullptr, ed));
                m_command.preview().draw();
            }
        }
        break;

    case SetMinor:
        {
            if (ok)
            {
                e->accept();
                m_points.ratio = m / m_points.m_vMajorP.magnitude();
                if (!m_points.isArc)
                {
                    commit();
                }
                else
                {
                    setStatus(SetAngle1);
                }
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(DrawEllipseAxisCommand::tr("Not a valid expression"));
            }
        }
        break;

    case SetAngle1:
        {
            bool angleOk = false;
            double a = Math2d::eval(c, &angleOk);
            if (angleOk)
            {
                e->accept();
                m_points.startAngle = Math2d::deg2rad(a);
                setStatus(SetAngle2);
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(DrawEllipseAxisCommand::tr("Not a valid expression"));
            }
        }
        break;

    case SetAngle2:
        {
            bool angleOk = false;
            double a = Math2d::eval(c, &angleOk);
            if (angleOk)
            {
                e->accept();
                m_points.endAngle = Math2d::deg2rad(a);
                commit();
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(DrawEllipseAxisCommand::tr("Not a valid expression"));
            }
        }
        break;

    default:
        break;
    }
}

void DrawEllipseAxisTool::updateHints()
{
    switch (status())
    {
    case SetCenter:
        GUIDIALOGFACTORY->updateMouseWidget(DrawEllipseAxisCommand::tr("Specify ellipse center"), DrawEllipseAxisCommand::tr("Cancel"));
        break;

    case SetMajor:
        GUIDIALOGFACTORY->updateMouseWidget(DrawEllipseAxisCommand::tr("Specify endpoint of major axis"), DrawEllipseAxisCommand::tr("Back"));
        break;

    case SetMinor:
        GUIDIALOGFACTORY->updateMouseWidget(DrawEllipseAxisCommand::tr("Specify endpoint or length of minor axis:"), DrawEllipseAxisCommand::tr("Back"));
        break;

    case SetAngle1:
        GUIDIALOGFACTORY->updateMouseWidget(DrawEllipseAxisCommand::tr("Specify start angle"), DrawEllipseAxisCommand::tr("Back"));
        break;

    case SetAngle2:
        GUIDIALOGFACTORY->updateMouseWidget(DrawEllipseAxisCommand::tr("Specify end angle"), DrawEllipseAxisCommand::tr("Back"));
        break;

    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}
std::unique_ptr<BasePlaceTool> DrawEllipseAxisCommand::createTool()
{
    return std::make_unique<DrawEllipseAxisTool>(*this, document(), view());
}

const bool g_registeredAxis = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionDrawEllipseAxis, QStringLiteral("draw.ellipse_axis"),
    [](const CommandContext&) -> std::unique_ptr<IExclusiveCommand>
    { return std::make_unique<DrawEllipseAxisCommand>(false); });

const bool g_registeredArcAxis = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionDrawEllipseArcAxis, QStringLiteral("draw.ellipse_arc_axis"),
    [](const CommandContext&) -> std::unique_ptr<IExclusiveCommand>
    { return std::make_unique<DrawEllipseAxisCommand>(true); });
}  // namespace
