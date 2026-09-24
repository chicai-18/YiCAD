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

/// @file InfoAngleAreaCommands.cpp
/// @brief 查询角度命令 info.angle（原 ActionInfoAngle）与查询面积命令 info.area
///        （原 ActionInfoArea）
///
/// 两个命令没有选项条，只在本文件里定义；工具的事件处理从原 Action 机械改写而来。

#include <cmath>
#include <memory>

#include <QCoreApplication>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "DmUnits.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "InfoArea.h"
#include "Information.h"
#include "PlaceCommand.h"

namespace
{
constexpr double TWO_PI = 2.0 * M_PI; ///< 2π常量
constexpr double ANGLE_ZERO = 0.0;    ///< 角度零值

/// @brief 查询角度命令；交互由 InfoAngleTool 驱动
class InfoAngleCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(InfoAngleCommand)

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 查询面积命令；交互由 InfoAreaTool 驱动
class InfoAreaCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(InfoAreaCommand)

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 查询角度工具：选两条直线（或多段线），在命令行输出夹角
class InfoAngleTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetEntity1, ///< 选择第一条线
        SetEntity2  ///< 选择第二条线
    };

    InfoAngleTool(InfoAngleCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    std::optional<DM::CursorType> getCursor() const override { return DM::SelectCursor; }

protected:
    void updateHints() override;
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;
    /// @brief 原 Action 在析构时取消高亮
    void onFinish() override { unhighlightEntity(); }

private:
    /// @brief 两条线上的点与交点
    struct Points
    {
        DmVector point1;
        DmVector point2;
        DmVector intersection;
    };

    /// @brief 回到某一状态（原 init(status)）；status < 0 时结束命令
    void init(int s)
    {
        if (s < 0)
        {
            command().finish();
            return;
        }
        restart(s);
    }

    void trigger();
    void unhighlightEntity();

    InfoAngleCommand& m_command;
    DmEntity* entity1 = nullptr;         ///< 第一条线
    DmEntity* entity2 = nullptr;         ///< 第二条线
    DmEntity* prevHighlighted = nullptr; ///< 上次高亮的实体
    Points m_points;
};

/// @brief 查询面积工具：逐点指定多边形，回到已有的点时闭合并输出周长与面积
class InfoAreaTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetFirstPoint, ///< 设置多边形第一个点
        SetNextPoint   ///< 设置多边形下一个点
    };

    InfoAreaTool(InfoAreaCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
        , ia(std::make_unique<InfoArea>())
    {
    }

protected:
    void updateHints() override;
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;
    void onCoordinate(const DmVector& coord) override;

private:
    /// @brief 回到某一状态（原 init(status)）；回到第一步时清空多边形；status < 0 时结束命令
    void init(int s)
    {
        if (s < 0)
        {
            command().finish();
            return;
        }
        restart(s);
        if (s == SetFirstPoint)
        {
            m_command.preview().clear();
            ia->reset();
        }
    }

    /// @brief 输出结果并回到第一步（原 trigger()）
    void trigger()
    {
        display();
        init(SetFirstPoint);
    }

    void display();

    InfoAreaCommand& m_command;
    std::unique_ptr<InfoArea> ia; ///< 面积信息对象
};

void InfoAngleTool::unhighlightEntity()
{
    if (prevHighlighted)
    {
        prevHighlighted->setHighlighted(false);
        view()->specifyDocumentModified();
        view()->redraw();
        prevHighlighted = nullptr;
    }
}

/// @brief 触发角度计算和显示
void InfoAngleTool::trigger()
{
    if (entity1 && entity2)
    {
        const DmVectorSolutions& sol = Information::getIntersection(entity1, entity2, false);

        if (sol.hasValid())
        {
            m_points.intersection = sol.get(0);

            if (m_points.intersection.valid && m_points.point1.valid && m_points.point2.valid)
            {
                double startAngle = m_points.intersection.angleTo(m_points.point1);
                double endAngle = m_points.intersection.angleTo(m_points.point2);
                double angle = remainder(endAngle - startAngle, TWO_PI);

                QString str = DmUnits::formatAngle(angle, document()->getAngleFormat(), document()->getAnglePrecision());

                if (angle < ANGLE_ZERO)
                {
                    str += " or ";
                    str += DmUnits::formatAngle(angle + TWO_PI, document()->getAngleFormat(), document()->getAnglePrecision());
                }
                GUIDIALOGFACTORY->commandMessage(InfoAngleCommand::tr("Angle: %1").arg(str));
            }
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(InfoAngleCommand::tr("Lines are parallel"));
        }
    }
}

/// @brief 鼠标移动事件处理，实现实体高亮
/// @param e 鼠标事件
void InfoAngleTool::onMouseMove(QMouseEvent* e)
{
    DmEntity* se = snapper()->catchEntity(e, { DM::EntityLine, DM::EntityPolyline }, DM::ResolveAllButTextImage);
    switch (status())
    {
        case SetEntity1:
        {
            if (se != prevHighlighted)
            {
                unhighlightEntity();
                entity1 = se;
                if (entity1)
                {
                    entity1->setHighlighted(true);
                    view()->specifyDocumentModified();
                    view()->redraw();
                    prevHighlighted = entity1;
                }
            }
        }
        break;

        case SetEntity2:
        {
            if (se != prevHighlighted)
            {
                if (prevHighlighted && prevHighlighted != entity1)
                {
                    prevHighlighted->setHighlighted(false);
                    view()->specifyDocumentModified();
                    view()->redraw();
                }
                entity2 = se;
                if (entity2 && entity2 != entity1)
                {
                    entity2->setHighlighted(true);
                    view()->specifyDocumentModified();
                    view()->redraw();
                    prevHighlighted = entity2;
                }
                else
                {
                    prevHighlighted = entity1;
                }
            }
            else
            {
                entity2 = se;
            }
        }
        break;

        default:
            break;
    }
}

/// @brief 鼠标释放事件处理
/// @param e 鼠标事件
void InfoAngleTool::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        DmVector mouse{view()->toGraphX(e->x()), view()->toGraphY(e->y())};

        switch (status())
        {
            case SetEntity1:
                entity1 = snapper()->catchEntity(e, DM::ResolveAll);
                if (entity1 && (entity1->getEntityType() == DM::EntityLine || entity1->getEntityType() == DM::EntityPolyline))
                {
                    m_points.point1 = entity1->getNearestPointOnEntity(mouse);
                    setStatus(SetEntity2);
                }
                break;

            case SetEntity2:
                entity2 = snapper()->catchEntity(e, DM::ResolveAll);
                if (entity2 && (entity2->getEntityType() == DM::EntityLine || entity2->getEntityType() == DM::EntityPolyline))
                {
                    m_points.point2 = entity2->getNearestPointOnEntity(mouse);
                    setStatus(SetEntity1);
                    trigger();
                }
                break;

            default:
                break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        unhighlightEntity();
        m_command.preview().clear();
        init(status() - 1);
    }
}

/// @brief 更新鼠标按钮提示
void InfoAngleTool::updateHints()
{
    switch (status())
    {
        case SetEntity1:
            GUIDIALOGFACTORY->updateMouseWidget(InfoAngleCommand::tr("Specify first line"), InfoAngleCommand::tr("Cancel"));
            break;
        case SetEntity2:
            GUIDIALOGFACTORY->updateMouseWidget(InfoAngleCommand::tr("Specify second line"), InfoAngleCommand::tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void InfoAreaTool::display()
{
    m_command.preview().clear();
    if (ia->size() < 1)
    {
        return;
    }
    switch (ia->size())
    {
        case 2:
            m_command.preview().entities().addEntity(new DmLine(nullptr, ia->at(0), ia->at(1)));
            break;
        default:
            for (int i = 0; i < ia->size(); i++)
            {
                m_command.preview().entities().addEntity(new DmLine(nullptr, ia->at(i), ia->at((i + 1) % ia->size())));
            }
            const QString linear = DmUnits::formatLinear(ia->getCircumference(), document()->getUnit(), document()->getLinearFormat(), document()->getLinearPrecision());
            GUIDIALOGFACTORY->commandMessage(InfoAreaCommand::tr("Circumference: %1").arg(linear));
            GUIDIALOGFACTORY->commandMessage(InfoAreaCommand::tr("Area: %1 %2^2").arg(ia->getArea()).arg(DmUnits::unitToString(document()->getUnit())));
            break;
    }
    m_command.preview().draw();
}

/// @brief 鼠标移动事件处理
/// @param e 鼠标事件
void InfoAreaTool::onMouseMove(QMouseEvent* e)
{
    DmVector mouse = snapper()->snapPoint(e);
    if (status() == SetNextPoint)
    {
        ia->push_back(mouse);
        display();
        ia->pop_back();
    }
}

/// @brief 鼠标释放事件处理
/// @param e 鼠标事件
void InfoAreaTool::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        onCoordinate(snapper()->snapPoint(e));
    }
    else if (e->button() == Qt::RightButton)
    {
        init(status() - 1);
    }
}

/// @brief 坐标输入：回到已有的点时闭合并输出结果，否则加入多边形
/// @param coord 坐标
void InfoAreaTool::onCoordinate(const DmVector& coord)
{
    DmVector mouse = coord;
    if (ia->duplicated(mouse))
    {
        ia->push_back(mouse);
        GUIDIALOGFACTORY->commandMessage(InfoAreaCommand::tr("Closing Point: %1/%2").arg(mouse.x).arg(mouse.y));
        trigger();
        return;
    }
    view()->moveRelativeZero(mouse);

    ia->push_back(mouse);
    GUIDIALOGFACTORY->commandMessage(InfoAreaCommand::tr("Point: %1/%2").arg(mouse.x).arg(mouse.y));
    switch (status())
    {
        case SetFirstPoint:
            setStatus(SetNextPoint);
            break;
        case SetNextPoint:
            display();
            break;

        default:
            break;
    }
}

/// @brief 更新鼠标按钮提示
void InfoAreaTool::updateHints()
{
    switch (status())
    {
        case SetFirstPoint:
            GUIDIALOGFACTORY->updateMouseWidget(InfoAreaCommand::tr("Specify first point of polygon"), InfoAreaCommand::tr("Cancel"));
            break;
        case SetNextPoint:
            GUIDIALOGFACTORY->updateMouseWidget(InfoAreaCommand::tr("Specify next point of polygon"), InfoAreaCommand::tr("Cancel"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

std::unique_ptr<BasePlaceTool> InfoAngleCommand::createTool()
{
    return std::make_unique<InfoAngleTool>(*this, document(), view());
}

std::unique_ptr<BasePlaceTool> InfoAreaCommand::createTool()
{
    return std::make_unique<InfoAreaTool>(*this, document(), view());
}

const bool g_registeredAngle = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionInfoAngle, QStringLiteral("info.angle"), exclusiveCommandFactory<InfoAngleCommand>());

const bool g_registeredArea = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionInfoArea, QStringLiteral("info.area"), exclusiveCommandFactory<InfoAreaCommand>());
}  // namespace
