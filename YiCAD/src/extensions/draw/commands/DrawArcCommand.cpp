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

/// @file DrawArcCommand.cpp
/// @brief 圆心圆弧命令、三点圆弧命令及其工具的实现

#include "DrawArcCommand.h"

#include <cmath>
#include <memory>

#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CircleData.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "DrawCommands.h"
#include "Commands.h"
#include "DmArc.h"
#include "DmCircle.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "EntityTable.h"
#include "ExclusiveCommandBus.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Math2d.h"
#include "Transaction.h"

namespace
{
/// @brief 指定起始角时预览的默认包角（60 度）
constexpr double DEFAULT_ARC_ANGLE = M_PI / 3.0;
}  // namespace

/// @brief 圆心圆弧工具：圆心、半径、起始角、包角
class DrawArcTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetCenter, ///< 设置圆心
        SetRadius, ///< 设置半径
        SetAngle1, ///< 设置起始角度
        ArcAngle   ///< 设置圆弧角度
    };

    DrawArcTool(DrawArcCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    bool isClockwise() const { return m_arc->isClockwise(); }

    void setClockwise(bool clockwise)
    {
        bool oldClockwise = isClockwise();
        m_arc->setClockwise(clockwise);
        if (oldClockwise != clockwise && status() == ArcAngle)
        {
            m_arc->switchStartEndAngle();
        }
    }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetCenter:
            GUIDIALOGFACTORY->updateMouseWidget(DrawArcCommand::tr("Specify center"), DrawArcCommand::tr("Cancel"));
            break;
        case SetRadius:
            GUIDIALOGFACTORY->updateMouseWidget(DrawArcCommand::tr("Specify radius"), DrawArcCommand::tr("Back"));
            break;
        case SetAngle1:
            GUIDIALOGFACTORY->updateMouseWidget(DrawArcCommand::tr("Specify start angle:"),
                                                DrawArcCommand::tr("Back"));
            break;
        case ArcAngle:
            GUIDIALOGFACTORY->updateMouseWidget(DrawArcCommand::tr("Specify arc angle"), DrawArcCommand::tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        DmVector mouse = snapper()->snapPoint(e);
        switch (status())
        {
        case SetCenter:
            m_arc->setCenter(mouse);
            break;
        case SetRadius:
            if (m_arc->getCenter().valid)
            {
                m_arc->setRadius(m_arc->getCenter().distanceTo(mouse));
                m_command.previewCircle(m_arc->getCenter(), m_arc->getRadius());
            }
            break;
        case SetAngle1:
            setStartAngle(mouse);
            m_arc->setEndAngle(Math2d::correctAngle(m_arc->getStartAngle() + DEFAULT_ARC_ANGLE));
            m_command.previewArc(m_arc->getData());
            break;
        case ArcAngle:
            setEndAngle(mouse);
            m_command.previewArc(m_arc->getData());
            break;
        default:
            break;
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
            // 原 init(getStatus() - 1)：退回时圆弧复位（连同方向）
            if (status() <= 0)
            {
                command().finish();
                return;
            }
            restart(status() - 1);
            reset();
        }
    }

    void onCoordinate(const DmVector& mouse) override
    {
        switch (status())
        {
        case SetCenter:
            m_arc->setCenter(mouse);
            view()->moveRelativeZero(mouse);
            setStatus(SetRadius);
            break;
        case SetRadius:
            if (m_arc->getCenter().valid)
            {
                m_arc->setRadius(m_arc->getCenter().distanceTo(mouse));
            }
            setStatus(SetAngle1);
            break;
        case SetAngle1:
            setStartAngle(mouse);
            setStatus(ArcAngle);
            break;
        case ArcAngle:
            setEndAngle(mouse);
            commit();
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
            GUIDIALOGFACTORY->commandMessage(Commands::msgAvailableCommands());
            return;
        }

        bool ok = false;
        switch (status())
        {
        case SetRadius:
        {
            double r = Math2d::eval(c, &ok);
            if (ok)
            {
                m_arc->setRadius(r);
                setStatus(SetAngle1);
                e->accept();
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(DrawArcCommand::tr("Not a valid expression"));
            }
            break;
        }
        case SetAngle1:
        {
            double a = Math2d::eval(c, &ok);
            if (ok)
            {
                double aR = Math2d::deg2rad(a);
                if (m_arc->isClockwise())
                {
                    m_arc->setStartAngle(Math2d::correctAngle(M_PI - aR));
                }
                else
                {
                    m_arc->setStartAngle(aR);
                }
                e->accept();
                setStatus(ArcAngle);
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(DrawArcCommand::tr("Not a valid expression"));
            }
            break;
        }
        case ArcAngle:
        {
            double a = Math2d::eval(c, &ok);
            if (ok)
            {
                // 输入的是包角
                m_arc->setEndAngle(Math2d::correctAngle(m_arc->getStartAngle() + Math2d::deg2rad(a)));
                e->accept();
                commit();
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(DrawArcCommand::tr("Not a valid expression"));
            }
            break;
        }
        default:
            break;
        }
    }

private:
    /// @brief 圆弧复位（原 reset()）
    void reset() { m_arc = std::make_unique<DmArc>(); }

    /// @brief 提交圆弧：正交限制下结束命令，否则回到第一步（原 trigger()）
    void commit()
    {
        m_command.commitArc(m_arc->getData());
        if (snapper()->getSnapMode()->restriction == DM::RestrictOrthogonal)
        {
            command().finish();
        }
        else
        {
            setStatus(SetCenter);
        }
        reset();
    }

    void setStartAngle(const DmVector& mouse)
    {
        double angle = m_arc->getCenter().angleTo(mouse);
        m_arc->setStartAngle(m_arc->isClockwise() ? Math2d::correctAngle(-angle + M_PI) : angle);
    }

    void setEndAngle(const DmVector& mouse)
    {
        double angle = m_arc->getCenter().angleTo(mouse);
        m_arc->setEndAngle(m_arc->isClockwise() ? Math2d::correctAngle(-angle + M_PI) : angle);
    }

    DrawArcCommand& m_command;
    std::unique_ptr<DmArc> m_arc = std::make_unique<DmArc>(); ///< 正在画的圆弧，法向随方向可能为负
};

DrawArcCommand::DrawArcCommand() = default;

DrawArcCommand::~DrawArcCommand() = default;

std::unique_ptr<BasePlaceTool> DrawArcCommand::createTool()
{
    return std::make_unique<DrawArcTool>(*this, document(), view());
}

DrawArcTool* DrawArcCommand::tool() const
{
    return static_cast<DrawArcTool*>(placeTool());
}

bool DrawArcCommand::isClockwise() const
{
    return tool() && tool()->isClockwise();
}

void DrawArcCommand::setClockwise(bool clockwise)
{
    if (tool())
    {
        tool()->setClockwise(clockwise);
    }
}

void DrawArcCommand::previewCircle(const DmVector& center, double radius)
{
    preview().clear();
    auto c = new DmCircle(preview().entities().getEntityContainer(), {center, radius});
    c->setDocument(document());
    preview().entities().addEntity(c);
    preview().draw();
}

void DrawArcCommand::previewArc(const ArcData& data)
{
    preview().clear();
    auto a = new DmArc(preview().entities().getEntityContainer(), data);
    a->setDocument(document());
    preview().entities().addEntity(a);
    preview().draw();
}

void DrawArcCommand::commitArc(const ArcData& data)
{
    preview().clear();
    Transaction t(tr("Create Arc").toStdString(), document());
    t.start();
    DmArc* arc = new DmArc(nullptr, data);
    arc->setDocument(document());
    if (arc->isClockwise())
    {
        arc->setClockwise(!arc->isClockwise());
    }
    document()->getEntityTable()->add(arc);
    t.commit();
    view()->moveRelativeZero(arc->getCenter());
}

void DrawArcCommand::showOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, true);
}

void DrawArcCommand::hideOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, false);
}

/// @brief 三点圆弧命令 ext.draw.arc_3p，取代原 ActionDrawArc3P；交互由 DrawArc3PTool 驱动
class DrawArc3PCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawArc3PCommand)

public:
    /// @brief 预览第一、二点间的直线
    void previewLine(const DmVector& p1, const DmVector& p2)
    {
        DmLine* line = new DmLine{preview().entities().getEntityContainer(), p1, p2};
        line->setDocument(document());
        preview().clear();
        preview().entities().addEntity(line);
        preview().draw();
    }

    /// @brief 预览圆弧
    void previewArc(const ArcData& data)
    {
        DmArc* arc = new DmArc(preview().entities().getEntityContainer(), data);
        arc->setDocument(document());
        preview().clear();
        preview().entities().addEntity(arc);
        preview().draw();
    }

    /// @brief 提交圆弧，相对零点移到终点
    void commitArc(const ArcData& data)
    {
        Transaction t(tr("Create Arc").toStdString(), document());
        t.start();
        DmArc* arc = new DmArc(nullptr, data);
        arc->setDocument(document());
        document()->getEntityTable()->add(arc);
        t.commit();
        view()->moveRelativeZero(arc->getEndpoint());
    }

    /// @brief 切换为圆心圆弧 ext.draw.arc：总线先请本命令让位（Replaced），再启动它
    ///        （原 Action 在 finish() 之后 setCurrentAction(new ActionDrawArc)）
    void switchToCenterArc()
    {
        std::unique_ptr<IExclusiveCommand> next = CommandRegistry::instance().createCommand(
            QStringLiteral("ext.draw.arc"), CommandContext{document(), view(), selection()});
        if (next && bus())
        {
            bus()->start(std::move(next));
        }
    }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

namespace
{
/// @brief 三点圆弧工具：起点、第二点、终点；命令行输入任何文字切换为圆心圆弧
class DrawArc3PTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetPoint1, ///< 设置第一个点
        SetPoint2, ///< 设置第二个点
        SetPoint3  ///< 设置第三个点
    };

    DrawArc3PTool(DrawArc3PCommand& command, DmDocument* doc, IDocumentView* view)
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
            GUIDIALOGFACTORY->updateMouseWidget(DrawArc3PCommand::tr("Specify startpoint or [center]"),
                                                DrawArc3PCommand::tr("Cancel"));
            break;
        case SetPoint2:
            GUIDIALOGFACTORY->updateMouseWidget(DrawArc3PCommand::tr("Specify second point"),
                                                DrawArc3PCommand::tr("Back"));
            break;
        case SetPoint3:
            GUIDIALOGFACTORY->updateMouseWidget(DrawArc3PCommand::tr("Specify endpoint"),
                                                DrawArc3PCommand::tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        DmVector mouse = snapper()->snapPoint(e);
        switch (status())
        {
        case SetPoint1:
            m_point1 = mouse;
            break;
        case SetPoint2:
            m_point2 = mouse;
            if (m_point1.valid)
            {
                m_command.previewLine(m_point1, m_point2);
            }
            break;
        case SetPoint3:
            m_point3 = mouse;
            prepareArc();
            if (m_data.isValid())
            {
                m_command.previewArc(m_data);
            }
            break;
        default:
            break;
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
            view()->moveRelativeZero(mouse);
            setStatus(SetPoint2);
            break;
        case SetPoint2:
            m_point2 = mouse;
            view()->moveRelativeZero(mouse);
            setStatus(SetPoint3);
            break;
        case SetPoint3:
            m_point3 = mouse;
            commit();
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
            GUIDIALOGFACTORY->commandMessage(Commands::msgAvailableCommands());
            return;
        }
        // 与原 Action 一致（既有缺陷）：Commands::checkCommand 对 "center" 一律返回真，任何
        // 文字都切换为圆心圆弧；文字不被接受，随后还会被当作新命令解析
        if (Commands::checkCommand("center", c))
        {
            m_command.switchToCenterArc();
        }
    }

private:
    /// @brief 由三点求圆弧，求不出时数据无效
    void prepareArc()
    {
        m_data = {};
        if (m_point1.valid && m_point2.valid && m_point3.valid)
        {
            DmArc arc(nullptr, m_data);
            if (arc.createFrom3P(m_point1, m_point2, m_point3))
            {
                m_data = arc.getData();
            }
        }
    }

    /// @brief 提交圆弧（原 trigger()）
    void commit()
    {
        m_command.preview().clear();
        prepareArc();
        if (m_data.isValid())
        {
            m_command.commitArc(m_data);
            if (snapper()->getSnapMode()->restriction == DM::RestrictOrthogonal)
            {
                command().finish();
            }
            else
            {
                setStatus(SetPoint1);
            }
            m_data = {};
            m_point1 = m_point2 = m_point3 = DmVector{};
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(DrawArc3PCommand::tr("Invalid arc data."));
        }
    }

    DrawArc3PCommand& m_command;
    ArcData m_data;    ///< 求出的圆弧
    DmVector m_point1; ///< 起点
    DmVector m_point2; ///< 第二点
    DmVector m_point3; ///< 终点
};
}  // namespace

std::unique_ptr<BasePlaceTool> DrawArc3PCommand::createTool()
{
    return std::make_unique<DrawArc3PTool>(*this, document(), view());
}

ExclusiveCommandFactory DrawCommands::arc()
{
    return exclusiveCommandFactory<DrawArcCommand>();
}

ExclusiveCommandFactory DrawCommands::arc3p()
{
    return exclusiveCommandFactory<DrawArc3PCommand>();
}
