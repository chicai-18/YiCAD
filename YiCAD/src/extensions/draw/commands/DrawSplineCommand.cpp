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

/// @file DrawSplineCommand.cpp
/// @brief 样条命令与工具的实现

#include "DrawSplineCommand.h"

#include <memory>
#include <vector>

#include <QList>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "DrawCommands.h"
#include "Commands.h"
#include "DmDocument.h"
#include "DmPoint.h"
#include "DmSpline.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Transaction.h"

namespace
{
/// @brief 拟合点样条的阶数
constexpr int DEFAULT_SPLINE_DEGREE = 3;
/// @brief 拟合点样条：可撤销、可闭合的最少点数
constexpr int MIN_POINTS_FOR_CLOSE = 3;
}  // namespace

void SplineCommand::previewSpline(const DmSpline& spline, bool withControlPoints)
{
    preview().clear();
    auto* copy = static_cast<DmSpline*>(spline.clone());
    copy->setDocument(document());
    copy->setParent(preview().entities().getEntityContainer());
    preview().entities().addEntity(copy);
    if (withControlPoints)
    {
        for (const DmVector& vp : copy->getControlPoints())
        {
            preview().entities().addEntity(new DmPoint(nullptr, PointData(vp)));
        }
    }
    preview().draw();
}

void SplineCommand::commitSpline(DmSpline* spline, const QString& transactionName)
{
    Transaction t(transactionName.toStdString(), document());
    t.start();
    document()->getEntityTable()->add(spline);
    t.commit();
}

void SplineCommand::showOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, true);
}

void SplineCommand::hideOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, false);
}

/// @brief 控制点样条工具：逐个指定控制点，右键结束
class DrawSplineTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetStartpoint, ///< 设置起始点
        SetNextPoint   ///< 设置下一个控制点
    };

    DrawSplineTool(DrawSplineCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    ~DrawSplineTool() override = default;

    void undo()
    {
        if (m_history.size() > 1)
        {
            m_history.removeLast();
            m_command.preview().clear();
            if (m_spline)
            {
                setControlPointsKnotsByClose(m_spline, m_spline->isClosed());
                m_command.previewSpline(*m_spline, false);
                if (!m_history.isEmpty())
                {
                    view()->moveRelativeZero(m_history.last());
                }
            }
            m_command.preview().draw();
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(DrawSplineCommand::tr("Cannot undo: Not enough entities defined yet."));
        }
    }

    void setDegree(int deg)
    {
        m_data.setDegree(deg);
        if (m_spline)
        {
            m_spline->setDegree(deg);
            setControlPointsKnotsByClose(m_spline, m_spline->isClosed());
        }
    }

    int getDegree() const { return m_data.getDegree(); }

    void setClosed(bool c)
    {
        m_data.setIsClosed(c);
        if (m_spline && m_spline->isClosed() != c)
        {
            setControlPointsKnotsByClose(m_spline, c);
        }
    }

    bool isClosed() const { return m_data.getIsClosed(); }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetStartpoint:
            GUIDIALOGFACTORY->updateMouseWidget(DrawSplineCommand::tr("Specify first control point"),
                                                DrawSplineCommand::tr("Cancel"));
            break;
        case SetNextPoint:
        {
            QString msg;
            if (m_history.size() >= 3)
            {
                msg += Commands::command("close");
                msg += "/";
            }
            if (m_history.size() >= 2)
            {
                msg += Commands::command("undo");
                GUIDIALOGFACTORY->updateMouseWidget(DrawSplineCommand::tr("Specify next control point or [%1]").arg(msg),
                                                    DrawSplineCommand::tr("Back"));
            }
            else
            {
                GUIDIALOGFACTORY->updateMouseWidget(DrawSplineCommand::tr("Specify next control point"),
                                                    DrawSplineCommand::tr("Back"));
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
        if (status() == SetNextPoint && m_spline)
        {
            std::unique_ptr<DmSpline> tmp(static_cast<DmSpline*>(m_spline->clone()));
            setControlPointsKnotsByClose(tmp.get(), tmp->isClosed(), mouse);
            m_command.previewSpline(*tmp, true);
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
            // 控制点足够时右键提交
            if (status() == SetNextPoint && m_spline
                && m_spline->getNumberOfControlPoints() >= m_spline->getDegree() + 1)
            {
                commit();
            }
            m_command.preview().clear();
            init(status() - 1);
        }
    }

    void onCoordinate(const DmVector& mouse) override
    {
        switch (status())
        {
        case SetStartpoint:
            m_history.clear();
            m_history.append(mouse);
            if (!m_spline)
            {
                m_spline = new DmSpline(nullptr, m_data);
                m_spline->setDocument(document());
                setControlPointsKnotsByClose(m_spline, m_spline->isClosed());
            }
            setStatus(SetNextPoint);
            view()->moveRelativeZero(mouse);
            updateHints();
            break;
        case SetNextPoint:
            view()->moveRelativeZero(mouse);
            m_history.append(mouse);
            if (m_spline)
            {
                setControlPointsKnotsByClose(m_spline, m_spline->isClosed());
                m_command.previewSpline(*m_spline, false);
            }
            updateHints();
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
                // 与原 Action 一致：列出命令后不接受
                GUIDIALOGFACTORY->commandMessage(Commands::msgAvailableCommands());
            }
            break;
        case SetNextPoint:
            if (Commands::checkCommand("undo", c))
            {
                // 与原 Action 一致：撤销后不接受这段文本
                undo();
                updateHints();
            }
            break;
        default:
            break;
        }
    }

private:
    /// @brief 回到某一状态并丢弃采集的点（原 init(status)）；status < 0 时结束命令
    void init(int s)
    {
        if (s < 0)
        {
            command().finish();
            return;
        }
        restart(s);
        m_spline = nullptr;
        m_history.clear();
    }

    /// @brief 提交样条（原 trigger()）
    void commit()
    {
        m_command.preview().clear();
        if (!m_spline)
        {
            return;
        }
        setControlPointsKnotsByClose(m_spline, m_spline->isClosed());
        m_command.commitSpline(m_spline, DrawSplineCommand::tr("Add spline"));
        m_spline = nullptr;
    }

    /// @brief 按采集的控制点（及可选的动态点）设置样条的控制点与节点
    void setControlPointsKnotsByClose(DmSpline* spline, bool isClosed, DmVector dynamicPt = DmVector(false))
    {
        std::vector<DmVector> controlPts(m_history.begin(), m_history.end());
        if (dynamicPt.valid)
        {
            controlPts.emplace_back(dynamicPt);
        }
        DmSpline::setControlPointsKnotsByClose(spline, isClosed, controlPts);
    }

    DrawSplineCommand& m_command;
    SplineData m_data;           ///< 阶数与是否闭合
    DmSpline* m_spline = nullptr; ///< 正在画的样条；提交后归文档（与原 Action 一致，结束时未提交的不释放）
    QList<DmVector> m_history;   ///< 控制点历史记录（用于撤销）
};

/// @brief 拟合点样条工具：逐个指定拟合点，右键结束
class DrawSplinePointsTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetStartPoint, ///< 设置起始点
        SetNextPoint   ///< 设置下一个点
    };

    DrawSplinePointsTool(DrawSplinePointsCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
        reset();
    }

    void undo()
    {
        if (!m_spline)
        {
            GUIDIALOGFACTORY->commandMessage(
                DrawSplinePointsCommand::tr("Cannot undo: Not enough entities defined yet."));
            return;
        }
        if (m_undoBuffer.size() > 1)
        {
            m_undoBuffer.pop_back();
            m_command.preview().clear();
            fitPoints(m_spline.get());
            m_command.previewSpline(*m_spline, false);
            if (!m_undoBuffer.empty())
            {
                view()->moveRelativeZero(m_undoBuffer.back());
            }
            m_command.preview().draw();
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(
                DrawSplinePointsCommand::tr("Cannot undo: Not enough entities defined yet."));
        }
    }

    void setClosed(bool c)
    {
        m_data.setIsClosed(c);
        if (m_spline && m_spline->isClosed() != c)
        {
            m_spline->setClosed(c);
            fitPoints(m_spline.get());
        }
    }

    bool isClosed() const { return m_data.getIsClosed(); }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetStartPoint:
            GUIDIALOGFACTORY->updateMouseWidget(DrawSplinePointsCommand::tr("Specify first control point"),
                                                DrawSplinePointsCommand::tr("Cancel"));
            break;
        case SetNextPoint:
        {
            QString msg = "";
            if (m_undoBuffer.size() >= MIN_POINTS_FOR_CLOSE)
            {
                msg += Commands::command("close");
                msg += "/";
            }
            if (m_undoBuffer.size() > 0)
            {
                msg += Commands::command("undo");
                GUIDIALOGFACTORY->updateMouseWidget(
                    DrawSplinePointsCommand::tr("Specify next control point or [%1]").arg(msg),
                    DrawSplinePointsCommand::tr("Back"));
            }
            else
            {
                GUIDIALOGFACTORY->updateMouseWidget(DrawSplinePointsCommand::tr("Specify next control point"),
                                                    DrawSplinePointsCommand::tr("Back"));
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
        if (status() == SetNextPoint && m_spline)
        {
            std::unique_ptr<DmSpline> tmp(static_cast<DmSpline*>(m_spline->clone()));
            fitPoints(tmp.get(), mouse);
            m_command.previewSpline(*tmp, true);
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
            if (status() == SetNextPoint && m_spline)
            {
                commit();
            }
            init(status() - 1);
        }
    }

    void onCoordinate(const DmVector& mouse) override
    {
        switch (status())
        {
        case SetStartPoint:
            m_undoBuffer.clear();
            m_undoBuffer.emplace_back(mouse);
            if (!m_spline)
            {
                m_spline = std::make_unique<DmSpline>(nullptr, m_data);
                m_spline->setDocument(document());
                // 与原 Action 一致：只加进预览容器，不重绘
                m_command.preview().entities().addEntity(new DmPoint(nullptr, PointData(mouse)));
            }
            setStatus(SetNextPoint);
            view()->moveRelativeZero(mouse);
            updateHints();
            break;
        case SetNextPoint:
            view()->moveRelativeZero(mouse);
            m_undoBuffer.emplace_back(mouse);
            if (m_spline)
            {
                fitPoints(m_spline.get());
                m_command.previewSpline(*m_spline, false);
            }
            updateHints();
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
        case SetStartPoint:
            if (Commands::checkCommand("help", c))
            {
                // 与原 Action 一致：列出命令后不接受
                GUIDIALOGFACTORY->commandMessage(Commands::msgAvailableCommands());
            }
            break;
        case SetNextPoint:
            if (Commands::checkCommand("undo", c))
            {
                // 与原 Action 一致：撤销后不接受这段文本
                undo();
                updateHints();
            }
            break;
        default:
            break;
        }
    }

private:
    /// @brief 丢弃采集的点（原 reset()）
    void reset()
    {
        m_spline.reset();
        m_undoBuffer.clear();
        m_data.setDegree(DEFAULT_SPLINE_DEGREE);
        m_data.setSplineType(ESplineType::eFitPoints);
    }

    /// @brief 回到某一状态并丢弃采集的点（原 init(status)）；status < 0 时结束命令
    void init(int s)
    {
        if (s < 0)
        {
            command().finish();
            return;
        }
        restart(s);
        reset();
    }

    /// @brief 提交样条（原 trigger()；与原 Action 一致，不清除预览）
    void commit()
    {
        if (!m_spline || !m_spline->isValid())
        {
            return;
        }
        m_spline->update();
        m_command.commitSpline(m_spline.release(), DrawSplinePointsCommand::tr("Add spline points"));
        view()->redraw();
        reset();
    }

    /// @brief 按采集的拟合点（及可选的动态点）拟合样条；闭合时追加起点
    void fitPoints(DmSpline* spline, DmVector dynamicPt = DmVector(false))
    {
        std::vector<DmVector> fitPts{m_undoBuffer};
        if (dynamicPt.valid)
        {
            fitPts.emplace_back(dynamicPt);
        }
        if (spline->isClosed())
        {
            fitPts.emplace_back(fitPts.front());
        }
        spline->setFitPts(fitPts);
        spline->fit();
        spline->update();
    }

    DrawSplinePointsCommand& m_command;
    SplineData m_data;                 ///< 阶数、类型与是否闭合
    std::unique_ptr<DmSpline> m_spline; ///< 正在画的样条
    std::vector<DmVector> m_undoBuffer; ///< 拟合点历史记录（用于撤销）
};

std::unique_ptr<BasePlaceTool> DrawSplineCommand::createTool()
{
    return std::make_unique<DrawSplineTool>(*this, document(), view());
}

void DrawSplineCommand::undo()
{
    if (placeTool())
    {
        static_cast<DrawSplineTool*>(placeTool())->undo();
    }
}

void DrawSplineCommand::setClosed(bool c)
{
    if (placeTool())
    {
        static_cast<DrawSplineTool*>(placeTool())->setClosed(c);
    }
}

bool DrawSplineCommand::isClosed() const
{
    return placeTool() && static_cast<DrawSplineTool*>(placeTool())->isClosed();
}

void DrawSplineCommand::setDegree(int deg)
{
    if (placeTool())
    {
        static_cast<DrawSplineTool*>(placeTool())->setDegree(deg);
    }
}

int DrawSplineCommand::getDegree() const
{
    return placeTool() ? static_cast<DrawSplineTool*>(placeTool())->getDegree() : 0;
}

std::unique_ptr<BasePlaceTool> DrawSplinePointsCommand::createTool()
{
    return std::make_unique<DrawSplinePointsTool>(*this, document(), view());
}

void DrawSplinePointsCommand::undo()
{
    if (placeTool())
    {
        static_cast<DrawSplinePointsTool*>(placeTool())->undo();
    }
}

void DrawSplinePointsCommand::setClosed(bool c)
{
    if (placeTool())
    {
        static_cast<DrawSplinePointsTool*>(placeTool())->setClosed(c);
    }
}

bool DrawSplinePointsCommand::isClosed() const
{
    return placeTool() && static_cast<DrawSplinePointsTool*>(placeTool())->isClosed();
}

ExclusiveCommandFactory DrawCommands::spline()
{
    return exclusiveCommandFactory<DrawSplineCommand>();
}

ExclusiveCommandFactory DrawCommands::splinePoints()
{
    return exclusiveCommandFactory<DrawSplinePointsCommand>();
}
