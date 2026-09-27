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

/// @file DrawCircleTan3Command.cpp
/// @brief 三切圆命令 ext.draw.circle_tan3，取代原 ActionDrawCircleTan3：选三条直线、圆弧、
///        圆或点，在求出的公切圆里选最靠近鼠标的一个

#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>

#include <QCoreApplication>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CircleData.h"
#include "CommandPreview.h"
#include "DrawCommands.h"
#include "DmAtomicEntity.h"
#include "DmCircle.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "DmPoint.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "HighlightSet.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Information.h"
#include "PlaceCommand.h"
#include "Quadratic.h"
#include "Transaction.h"

namespace
{
/// @brief 三切圆命令；交互由 DrawCircleTan3Tool 驱动
class DrawCircleTan3Command : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawCircleTan3Command)

public:
    /// @brief 提交圆
    void commitCircle(const CircleData& data)
    {
        preview().clear();
        Transaction t(tr("Create Circletan3").toStdString(), document());
        t.start();
        DmCircle* circle = new DmCircle(nullptr, data);
        circle->setDocument(document());
        document()->getEntityTable()->add(circle);
        t.commit();
    }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 三切圆工具：选三个实体，再选圆心
class DrawCircleTan3Tool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetCircle1, ///< 选择第一个圆/线
        SetCircle2, ///< 选择第二个圆/线
        SetCircle3, ///< 选择第三个圆/线
        SetCenter   ///< 选择最近的公切圆圆心
    };

    DrawCircleTan3Tool(DrawCircleTan3Command& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
        // 原 init(0)：选实体不捕捉
        snapper()->suspend();
    }

    std::optional<DM::CursorType> getCursor() const override { return DM::SelectCursor; }

protected:
    void updateHints() override;
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;

private:
    /// @brief 采集的实体与求出的候选圆
    struct Points
    {
        std::vector<DmAtomicEntity*> circles;
        std::shared_ptr<CircleData> cData{std::make_shared<CircleData>()};
        DmVector coord;
        bool valid{false};
        std::vector<std::shared_ptr<CircleData>> candidates; ///< 求出的候选圆
        DmVectorSolutions centers;
    };

    /// @brief 回到某一状态（原 init(status)）：status < 0 时结束命令
    void init(int s)
    {
        if (s < 0)
        {
            command().finish();
            return;
        }
        restart(s);
        snapper()->suspend();
        if (s == SetCircle1)
        {
            m_points.circles.clear();
        }
    }

    /// @brief 提交后取消高亮，回到第一步（原 trigger()）
    void commit()
    {
        m_command.commitCircle(*m_points.cData);
        command().highlight()->clear();
        m_points.circles.clear();
        setStatus(SetCircle1);
    }

    bool getData();
    bool preparePreview();
    DmEntity* catchCircle(QMouseEvent* e);

    DrawCircleTan3Command& m_command;
    Points m_points;
};

void DrawCircleTan3Tool::onMouseMove(QMouseEvent* e)
{
    switch (status())
    {
        case SetCenter:
        {
            m_points.coord = view()->toGraph(e->pos().x(), e->pos().y());
            m_command.preview().clear();
            if (preparePreview())
            {
                DmCircle* pCircle = new DmCircle(m_command.preview().entities().getEntityContainer(), *m_points.cData);
                pCircle->setDocument(document());
                m_command.preview().entities().addEntity(pCircle);
                for (auto& c : m_points.candidates)
                {
                    m_command.preview().entities().addEntity(new DmPoint(nullptr, PointData(c->getCenter())));
                }
                m_command.preview().draw();
            }
        }
        break;
        default:
            break;
    }
}

bool DrawCircleTan3Tool::getData()
{
    if (status() != SetCircle3)
    {
        return false;
    }
    // find the nearest circle
    size_t i                = 0;
    size_t const countLines = std::count_if(m_points.circles.begin(), m_points.circles.end(),
                                            [](DmAtomicEntity* e) -> bool { return e->getEntityType() == DM::EntityLine; });

    for (; i < m_points.circles.size(); ++i)
    {
        if (m_points.circles[i]->getEntityType() == DM::EntityLine)
        {
            break;
        }
    }
    m_points.candidates.clear();
    size_t i1 = (i + 1) % 3;
    size_t i2 = (i + 2) % 3;
    if (i < m_points.circles.size() && m_points.circles[i]->getEntityType() == DM::EntityLine)
    {
        // one or more lines

        Quadratic lc0(m_points.circles[i], m_points.circles[i1], false);
        Quadratic lc1;
        DmVectorSolutions sol;
        // detect degenerate case two circles with the same radius
        switch (countLines)
        {
            default:
            case 0:
                // this should not happen
                assert(false);
            case 1:
                // 1 line, two circles
                {
                    for (unsigned k = 0; k < 4; ++k)
                    {
                        // loop through all mirroring cases
                        lc1           = Quadratic(m_points.circles[i], m_points.circles[i1], k & 1u);
                        Quadratic lc2 = Quadratic(m_points.circles[i], m_points.circles[i2], k & 2u);
                        sol.push_back(Quadratic::getIntersection(lc1, lc2));
                    }
                }
                break;
            case 2:
                // 2 lines, one circle
                {
                    if (m_points.circles[i2]->getEntityType() == DM::EntityLine)
                    {
                        std::swap(i1, i2);
                    }
                    // i2 is circle

                    for (unsigned k = 0; k < 4; ++k)
                    {
                        // loop through all mirroring cases
                        lc1           = Quadratic(m_points.circles[i2], m_points.circles[i], k & 1u);
                        Quadratic lc2 = Quadratic(m_points.circles[i2], m_points.circles[i1], k & 2u);
                        sol.push_back(Quadratic::getIntersection(lc1, lc2));
                    }
                }
                break;
            case 3:
                // 3 lines
                {
                    lc0       = m_points.circles[i]->getQuadratic();
                    lc1       = m_points.circles[i1]->getQuadratic();
                    auto lc2  = m_points.circles[i2]->getQuadratic();
                    auto sol1 = Quadratic::getIntersection(lc0, lc1);
                    if (sol1.size() < 1)
                    {
                        std::swap(lc0, lc2);
                        std::swap(i, i2);
                    }
                    sol1 = Quadratic::getIntersection(lc0, lc2);
                    if (sol1.size() < 1)
                    {
                        std::swap(lc0, lc1);
                        std::swap(i, i1);
                    }

                    DmLine* line0 = static_cast<DmLine*>(m_points.circles[i]);
                    DmLine* line1 = static_cast<DmLine*>(m_points.circles[i1]);
                    DmLine* line2 = static_cast<DmLine*>(m_points.circles[i2]);
                    lc0           = line0->getQuadratic();
                    lc1           = line1->getQuadratic();
                    lc2           = line2->getQuadratic();
                    // intersection 0, 1
                    sol1 = Quadratic::getIntersection(lc0, lc1);
                    if (!sol1.size())
                    {
                        return false;
                    }
                    DmVector const v1 = sol1.at(0);
                    double startAngle     = 0.5 * (line0->getStartAngle() + line1->getStartAngle());

                    // intersection 0, 2
                    sol1 = Quadratic::getIntersection(lc0, lc2);
                    double endAngle;
                    if (sol1.size() < 1)
                    {
                        return false;
                    }
                    endAngle             = 0.5 * (line0->getStartAngle() + line2->getStartAngle());
                    DmVector const& v2 = sol1.at(0);
                    // two bisector lines per intersection
                    for (unsigned j = 0; j < 2; ++j)
                    {
                        DmLine l1{v1, v1 + DmVector{startAngle}};
                        for (unsigned j1 = 0; j1 < 2; ++j1)
                        {
                            DmLine l2{v2, v2 + DmVector{endAngle}};
                            sol.push_back(Information::getIntersectionLineLine(&l1, &l2));
                            endAngle += M_PI_2;
                        }
                        startAngle += M_PI_2;
                    }
                }
        }

        double d;

        // line passes circle center, need a second parabola as the image of the
        // line
        for (int j = 1; j <= 2; j++)
        {
            if (m_points.circles[(i + j) % 3]->getEntityType() == DM::EntityCircle)
            {
                m_points.circles[i]->getNearestPointOnEntity(m_points.circles[(i + j) % 3]->getCenter(), false, &d);
                if (d < DM_TOLERANCE)
                {
                    Quadratic lc2(m_points.circles[i], m_points.circles[(i + j) % 3], true);
                    sol.push_back(Quadratic::getIntersection(lc2, lc1));
                }
            }
        }

        // clean up duplicate and invalid
        DmVectorSolutions sol1;
        for (const DmVector& vp : sol)
        {
            if (vp.magnitude() > DM_MAXDOUBLE)
            {
                continue;
            }
            if (sol1.size() && sol1.getClosestDistance(vp) < DM_TOLERANCE)
            {
                continue;
            }
            sol1.push_back(vp);
        }

        for (auto const& v : sol1)
        {
            m_points.circles[i]->getNearestPointOnEntity(v, false, &d);
            auto data = std::make_shared<CircleData>(v, d);
            if (m_points.circles[(i + 1) % 3]->isTangent(*data) == false)
            {
                continue;
            }
            if (m_points.circles[(i + 2) % 3]->isTangent(*data) == false)
            {
                continue;
            }
            m_points.candidates.push_back(data);
        }
    }
    else
    {
        DmCircle c{nullptr, *m_points.cData};
        auto solutions = c.createTan3(m_points.circles);
        m_points.candidates.clear();
        for (const DmCircle& s : solutions)
        {
            m_points.candidates.push_back(std::make_shared<CircleData>(s.getData()));
        }
    }
    m_points.valid = (m_points.candidates.size() > 0);
    return m_points.valid;
}

bool DrawCircleTan3Tool::preparePreview()
{
    if (status() != SetCenter || m_points.valid == false)
    {
        m_points.valid = false;
        return false;
    }
    // find the nearest circle
    size_t index = m_points.candidates.size();
    double dist  = DM_MAXDOUBLE * DM_MAXDOUBLE;
    for (size_t i = 0; i < m_points.candidates.size(); ++i)
    {
        m_command.preview().entities().addEntity(new DmPoint(nullptr, PointData(m_points.candidates.at(i)->getCenter())));
        double d;
        DmCircle(nullptr, *m_points.candidates.at(i)).getNearestPointOnEntity(m_points.coord, false, &d);
        double dCenter = m_points.coord.distanceTo(m_points.candidates.at(i)->getCenter());
        d              = std::min(d, dCenter);
        if (d < dist)
        {
            dist  = d;
            index = i;
        }
    }
    if (index < m_points.candidates.size())
    {
        m_points.cData = m_points.candidates.at(index);
        m_points.valid = true;
    }
    else
    {
        m_points.valid = false;
    }
    return m_points.valid;
}

DmEntity* DrawCircleTan3Tool::catchCircle(QMouseEvent* e)
{
    DmEntity* ret = nullptr;
    DmEntity* en  = snapper()->catchEntity(e, EntityTypeList{ DM::EntityArc, DM::EntityCircle, DM::EntityLine, DM::EntityPoint }, DM::ResolveAll);
    if (!en)
    {
        return ret;
    }
    if (!en->isVisible())
    {
        return ret;
    }
    for (int i = 0; i < status(); ++i)
    {
        if (en->getId() == m_points.circles[i]->getId())
        {
            return ret;  // 不重复选择同一条线
        }
    }
    return en;
}

void DrawCircleTan3Tool::onMouseRelease(QMouseEvent* e)
{
    // Proceed to next status
    if (e->button() == Qt::LeftButton)
    {
        switch (status())
        {
            case SetCircle1:
            case SetCircle2:
            case SetCircle3:
            {
                DmEntity* en = catchCircle(e);
                if (!en)
                {
                    return;
                }
                m_points.circles.resize(status());
                for (const DmAtomicEntity* const pc : m_points.circles)
                {
                    if (pc == en)
                    {
                        continue;
                    }
                }
                m_points.circles.push_back(static_cast<DmAtomicEntity*>(en));
                if (status() <= SetCircle2 || (status() == SetCircle3 && getData()))
                {
                    command().highlight()->add(m_points.circles.back());
                    setStatus(status() + 1);
                }
            }
            break;
            case SetCenter:
                m_points.coord = view()->toGraph(e->pos().x(), e->pos().y());
                if (preparePreview())
                {
                    commit();
                }
                break;

            default:
                break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        // Return to last status:
        if (status() > 0)
        {
            command().highlight()->remove(m_points.circles[status() - 1]);
            m_points.circles.pop_back();
            m_command.preview().clear();
        }
        init(status() - 1);
    }
}

void DrawCircleTan3Tool::updateHints()
{
    switch (status())
    {
        case SetCircle1:
            GUIDIALOGFACTORY->updateMouseWidget(DrawCircleTan3Command::tr("Specify the first line/arc/circle"), DrawCircleTan3Command::tr("Cancel"));
            break;

        case SetCircle2:
            GUIDIALOGFACTORY->updateMouseWidget(DrawCircleTan3Command::tr("Specify the second line/arc/circle"), DrawCircleTan3Command::tr("Back"));
            break;
        case SetCircle3:
            GUIDIALOGFACTORY->updateMouseWidget(DrawCircleTan3Command::tr("Specify the third line/arc/circle"), DrawCircleTan3Command::tr("Back"));
            break;

        case SetCenter:
            GUIDIALOGFACTORY->updateMouseWidget(DrawCircleTan3Command::tr("Select the center of the tangent circle"), DrawCircleTan3Command::tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}
std::unique_ptr<BasePlaceTool> DrawCircleTan3Command::createTool()
{
    return std::make_unique<DrawCircleTan3Tool>(*this, document(), view());
}

}  // namespace

ExclusiveCommandFactory DrawCommands::circleTan3()
{
    return exclusiveCommandFactory<DrawCircleTan3Command>();
}
