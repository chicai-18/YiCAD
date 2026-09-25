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

/// @file DrawCircleCommands.cpp
/// @brief 画圆命令：圆心+半径 ext.draw.circle（原 ActionDrawCircle）、两点 ext.draw.circle_2p
///        （原 ActionDrawCircle2P）、三点 ext.draw.circle_3p（原 ActionDrawCircle3P）
///
/// 三个命令没有选项条，只在本文件里定义。

#include <memory>

#include <QCoreApplication>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CircleData.h"
#include "CommandPreview.h"
#include "DrawCommands.h"
#include "Commands.h"
#include "DmCircle.h"
#include "DmDocument.h"
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
/// @brief 画圆命令的公共部分：预览与提交一个圆
class CircleCommand : public PlaceCommand
{
public:
    /// @brief 预览圆
    /// @param inContainer 圆的父容器是否为预览容器（原圆心+半径的圆没有设父容器）
    void previewCircle(const CircleData& data, bool inContainer = true)
    {
        DmCircle* circle = new DmCircle(inContainer ? preview().entities().getEntityContainer() : nullptr, data);
        circle->setDocument(document());
        preview().clear();
        preview().entities().addEntity(circle);
        preview().draw();
    }

    /// @brief 提交圆
    /// @return 提交的圆，交给文档
    DmCircle* commitCircle(const CircleData& data, const QString& transactionName)
    {
        preview().clear();
        Transaction t(transactionName.toStdString(), document());
        t.start();
        DmCircle* circle = new DmCircle(nullptr, data);
        circle->setDocument(document());
        document()->getEntityTable()->add(circle);
        t.commit();
        return circle;
    }
};

/// @brief 圆心+半径画圆命令
class DrawCircleCommand : public CircleCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawCircleCommand)

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 两点（直径两端）画圆命令
class DrawCircle2PCommand : public CircleCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawCircle2PCommand)

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 三点画圆命令
class DrawCircle3PCommand : public CircleCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawCircle3PCommand)

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 画完一个圆后：正交限制下结束命令，否则回到第一步
template <typename Tool>
void afterCommit(Tool& tool, int firstStatus)
{
    if (tool.snapper()->getSnapMode()->restriction == DM::RestrictOrthogonal)
    {
        tool.finishCommand();
    }
    else
    {
        tool.backToStatus(firstStatus);
    }
}

/// @brief 画圆工具的公共部分：暴露 afterCommit() 需要的两个动作
class CircleToolBase : public BasePlaceTool
{
public:
    using BasePlaceTool::BasePlaceTool;
    void finishCommand() { command().finish(); }
    void backToStatus(int s) { setStatus(s); }

protected:
    /// @brief help：列出命令后不接受（与原 Action 一致）
    void listHelp(GuiCommandEvent* e)
    {
        if (Commands::checkCommand("help", e->getCommand().toLower()))
        {
            GUIDIALOGFACTORY->commandMessage(Commands::msgAvailableCommands());
        }
    }
};

/// @brief 圆心+半径画圆工具
class DrawCircleTool : public CircleToolBase
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetCenter, ///< 设置圆心
        SetRadius  ///< 设置半径
    };

    DrawCircleTool(DrawCircleCommand& command, DmDocument* doc, IDocumentView* view)
        : CircleToolBase(command, doc, view)
        , m_command(command)
    {
    }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetCenter:
            GUIDIALOGFACTORY->updateMouseWidget(DrawCircleCommand::tr("Specify center"),
                                                DrawCircleCommand::tr("Cancel"));
            break;
        case SetRadius:
            GUIDIALOGFACTORY->updateMouseWidget(DrawCircleCommand::tr("Specify point on circle"),
                                                DrawCircleCommand::tr("Back"));
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
            m_data.setCenter(mouse);
            break;
        case SetRadius:
            if (m_data.getCenter().valid)
            {
                m_data.setRadius(m_data.getCenter().distanceTo(mouse));
                m_command.previewCircle(m_data, false);
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
            stepBackAndReset();
        }
    }

    void onCoordinate(const DmVector& mouse) override
    {
        switch (status())
        {
        case SetCenter:
            m_data.setCenter(mouse);
            view()->moveRelativeZero(mouse);
            setStatus(SetRadius);
            break;
        case SetRadius:
            if (m_data.getCenter().valid)
            {
                view()->moveRelativeZero(mouse);
                m_data.setRadius(m_data.getCenter().distanceTo(mouse));
                commit();
            }
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
        if (status() != SetRadius)
        {
            return;
        }
        bool ok = false;
        double r = Math2d::eval(c, &ok);
        if (ok)
        {
            m_data.setRadius(r);
            e->accept();
            commit();
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(DrawCircleCommand::tr("Not a valid expression"));
        }
    }

private:
    /// @brief 原 init(getStatus() - 1)：退回时丢弃圆的数据
    void stepBackAndReset()
    {
        if (status() <= 0)
        {
            command().finish();
            return;
        }
        restart(status() - 1);
        m_data = CircleData{};
    }

    /// @brief 提交后正交限制下结束命令，否则回到第一步（原 trigger()）
    void commit()
    {
        DmCircle* circle = m_command.commitCircle(m_data, DrawCircleCommand::tr("Create Circle"));
        view()->moveRelativeZero(circle->getCenter());
        afterCommit(*this, SetCenter);
        m_data = CircleData{};
    }

    DrawCircleCommand& m_command;
    CircleData m_data; ///< 正在画的圆
};

/// @brief 两点画圆工具
class DrawCircle2PTool : public CircleToolBase
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetPoint1, ///< 设置第一个点
        SetPoint2  ///< 设置第二个点
    };

    DrawCircle2PTool(DrawCircle2PCommand& command, DmDocument* doc, IDocumentView* view)
        : CircleToolBase(command, doc, view)
        , m_command(command)
    {
    }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetPoint1:
            GUIDIALOGFACTORY->updateMouseWidget(DrawCircle2PCommand::tr("Specify first point"),
                                                DrawCircle2PCommand::tr("Cancel"));
            break;
        case SetPoint2:
            GUIDIALOGFACTORY->updateMouseWidget(DrawCircle2PCommand::tr("Specify second point"),
                                                DrawCircle2PCommand::tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        DmVector mouse = snapper()->snapPoint(e);
        m_mouse = mouse;
        switch (status())
        {
        case SetPoint1:
            m_point1 = mouse;
            break;
        case SetPoint2:
            m_point2 = mouse;
            prepareCircle();
            if (m_data.isValid())
            {
                m_command.previewCircle(m_data);
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
        case SetPoint1:
            m_point1 = mouse;
            view()->moveRelativeZero(mouse);
            setStatus(SetPoint2);
            break;
        case SetPoint2:
            m_point2 = mouse;
            view()->moveRelativeZero(mouse);
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
        switch (status())
        {
        case SetPoint1:
            listHelp(e);
            break;
        case SetPoint2:
        {
            // 输入长度：第二点沿第一点指向鼠标的方向；与原 Action 一致，数值不接受
            bool isLength = false;
            double length = c.toDouble(&isLength);
            if (isLength)
            {
                DmVector vec(1.0, 0.0);
                if (m_mouse.valid)
                {
                    vec = (m_mouse - m_point1).normalize();
                }
                DmVector endPt = m_point1 + vec * length;
                m_point2 = endPt;
                view()->moveRelativeZero(endPt);
                commit();
                finishIfOrthogonal();
            }
            break;
        }
        default:
            break;
        }
    }

private:
    void reset()
    {
        m_data = CircleData{};
        m_point1 = {};
        m_point2 = {};
    }

    void prepareCircle()
    {
        m_data = CircleData{};
        if (m_point1.valid && m_point2.valid)
        {
            DmCircle circle(nullptr, m_data);
            if (circle.createFrom2P(m_point1, m_point2))
            {
                m_data = circle.getData();
            }
        }
    }

    /// @brief 提交（原 trigger()）；圆无效时弹出警告
    void commit()
    {
        m_command.preview().clear();
        prepareCircle();
        if (m_data.isValid())
        {
            m_command.commitCircle(m_data, DrawCircle2PCommand::tr("Create Circle2p"));
            afterCommit(*this, SetPoint1);
            reset();
        }
        else
        {
            GUIDIALOGFACTORY->requestWarningDialog(DrawCircle2PCommand::tr("Invalid Circle data."));
        }
    }

    DrawCircle2PCommand& m_command;
    CircleData m_data;  ///< 求出的圆
    DmVector m_point1;  ///< 第一个点
    DmVector m_point2;  ///< 第二个点
    DmVector m_mouse;   ///< 当前鼠标所在点
};

/// @brief 三点画圆工具
class DrawCircle3PTool : public CircleToolBase
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetPoint1, ///< 设置第一个点
        SetPoint2, ///< 设置第二个点
        SetPoint3  ///< 设置第三个点
    };

    DrawCircle3PTool(DrawCircle3PCommand& command, DmDocument* doc, IDocumentView* view)
        : CircleToolBase(command, doc, view)
        , m_command(command)
    {
    }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetPoint1:
            GUIDIALOGFACTORY->updateMouseWidget(DrawCircle3PCommand::tr("Specify first point"),
                                                DrawCircle3PCommand::tr("Cancel"));
            break;
        case SetPoint2:
            GUIDIALOGFACTORY->updateMouseWidget(DrawCircle3PCommand::tr("Specify second point"),
                                                DrawCircle3PCommand::tr("Back"));
            break;
        case SetPoint3:
            GUIDIALOGFACTORY->updateMouseWidget(DrawCircle3PCommand::tr("Specify third point"),
                                                DrawCircle3PCommand::tr("Back"));
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
            break;
        case SetPoint3:
            m_point3 = mouse;
            prepareCircle();
            if (m_data.isValid())
            {
                m_command.previewCircle(m_data);
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

    void onCommand(GuiCommandEvent* e) override { listHelp(e); }

private:
    void reset()
    {
        m_data = CircleData{};
        m_point1 = m_point2 = m_point3 = DmVector{};
    }

    void prepareCircle()
    {
        m_data = CircleData{};
        if (m_point1.valid && m_point2.valid && m_point3.valid)
        {
            DmCircle circle{nullptr, m_data};
            if (circle.createFrom3P(m_point1, m_point2, m_point3))
            {
                m_data = circle.getData();
            }
        }
    }

    /// @brief 提交（原 trigger()）；圆无效时弹出警告
    void commit()
    {
        m_command.preview().clear();
        prepareCircle();
        if (m_data.isValid())
        {
            m_command.commitCircle(m_data, DrawCircle3PCommand::tr("Create Circle3p"));
            snapper()->drawSnapper();
            afterCommit(*this, SetPoint1);
            reset();
        }
        else
        {
            GUIDIALOGFACTORY->requestWarningDialog(DrawCircle3PCommand::tr("Invalid circle data."));
        }
    }

    DrawCircle3PCommand& m_command;
    CircleData m_data;  ///< 求出的圆
    DmVector m_point1;  ///< 第一个点
    DmVector m_point2;  ///< 第二个点
    DmVector m_point3;  ///< 第三个点
};

std::unique_ptr<BasePlaceTool> DrawCircleCommand::createTool()
{
    return std::make_unique<DrawCircleTool>(*this, document(), view());
}

std::unique_ptr<BasePlaceTool> DrawCircle2PCommand::createTool()
{
    return std::make_unique<DrawCircle2PTool>(*this, document(), view());
}

std::unique_ptr<BasePlaceTool> DrawCircle3PCommand::createTool()
{
    return std::make_unique<DrawCircle3PTool>(*this, document(), view());
}

}  // namespace

ExclusiveCommandFactory DrawCommands::circle()
{
    return exclusiveCommandFactory<DrawCircleCommand>();
}

ExclusiveCommandFactory DrawCommands::circle2p()
{
    return exclusiveCommandFactory<DrawCircle2PCommand>();
}

ExclusiveCommandFactory DrawCommands::circle3p()
{
    return exclusiveCommandFactory<DrawCircle3PCommand>();
}
