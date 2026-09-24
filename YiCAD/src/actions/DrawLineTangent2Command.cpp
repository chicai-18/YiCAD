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

/// @file DrawLineTangent2Command.cpp
/// @brief 两圆公切线命令 draw.line_tangent2，取代原 ActionDrawLineTangent2：
///        选两个圆、圆弧或椭圆，画最靠近鼠标的公切线

#include <cmath>
#include <memory>
#include <vector>

#include <QCoreApplication>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "DmDocument.h"
#include "DmEllipse.h"
#include "DmLine.h"
#include "EntityTable.h"
#include "GeUtility.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Math2d.h"
#include "PlaceCommand.h"
#include "Transaction.h"

/// @brief 两圆公切线命令；交互由 DrawLineTangent2Tool 驱动
class DrawLineTangent2Command : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawLineTangent2Command)

public:
    /// @brief 预览切线
    void previewTangent(const LineData& data)
    {
        preview().clear();
        auto l = new DmLine(preview().entities().getEntityContainer(), data);
        l->setDocument(document());
        preview().entities().addEntity(l);
        preview().draw();
    }

    /// @brief 提交切线
    void commitTangent(const LineData& data)
    {
        preview().clear();
        DmEntity* newEntity = new DmLine(nullptr, data);
        newEntity->setDocument(document());
        Transaction t(tr("Create line tangent").toStdString(), document());
        t.start();
        document()->getEntityTable()->add(newEntity);
        t.commit();
    }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

namespace
{
/// @brief 圆心距离容差阈值
constexpr double DISTANCE_TOLERANCE = 1.0e-6;

/// @brief 两个圆、圆弧或椭圆的公切线中最靠近 coord 的一条；没有时返回空
/// @details 从原 ActionDrawLineTangent2::createTangent2 原样搬来
DmLine* createTangent2(const DmVector& coord, DmEntity* circle1Entity, DmEntity* circle2Entity)
{
    DmLine* ret = nullptr;
    DmVector circleCenter1 = {};
    DmVector circleCenter2 = {};
    double circleRadius1 = 0.0;
    double circleRadius2 = 0.0;

    // 检查给定实体是否有效
    if (!(circle1Entity && circle2Entity))
    {
        return nullptr;
    }
    if (!(GeUtility::isEntityArc(circle1Entity)
          && GeUtility::isEntityArc(circle2Entity)))
    {
        return nullptr;
    }

    std::vector<DmLine*> poss;
    LineData data;

    if (circle1Entity->getEntityType() == DM::EntityEllipse)
    {
        // 将椭圆移到第二个位置, 统一处理
        std::swap(circle1Entity, circle2Entity);
    }
    circleCenter1 = circle1Entity->getCenter();
    circleRadius1 = circle1Entity->getRadius();
    circleCenter2 = circle2Entity->getCenter();
    circleRadius2 = circle2Entity->getRadius();
    if (circle2Entity->getEntityType() != DM::EntityEllipse)
    {
        // 无椭圆情况: 两个都是圆
        // 创建所有可能的公切线

        double startAngle = circleCenter1.angleTo(circleCenter2);
        double dist1 = circleCenter1.distanceTo(circleCenter2);

        if (dist1 > DISTANCE_TOLERANCE)
        {
            // 外公切线
            double dist2 = circleRadius2 - circleRadius1;
            if (dist1 > dist2)
            {
                double endAngle = asin(dist2 / dist1);
                double angt1 = startAngle + endAngle + M_PI_2;
                double angt2 = startAngle - endAngle - M_PI_2;
                DmVector offs1 = DmVector::polar(circleRadius1, angt1);
                DmVector offs2 = DmVector::polar(circleRadius2, angt1);

                poss.push_back(new DmLine{
                    circleCenter1 + offs1,
                    circleCenter2 + offs2
                });

                offs1.setPolar(circleRadius1, angt2);
                offs2.setPolar(circleRadius2, angt2);

                poss.push_back(new DmLine{
                    circleCenter1 + offs1,
                    circleCenter2 + offs2
                });
            }

            // 内公切线
            double dist3 = circleRadius2 + circleRadius1;
            if (dist1 > dist3)
            {
                double angle3 = asin(dist3 / dist1);
                double angt3 = startAngle + angle3 + M_PI_2;
                double angt4 = startAngle - angle3 - M_PI_2;
                DmVector offs1 = {};
                DmVector offs2 = {};

                offs1.setPolar(circleRadius1, angt3);
                offs2.setPolar(circleRadius2, angt3);

                poss.push_back(new DmLine{
                    circleCenter1 - offs1,
                    circleCenter2 + offs2
                });

                offs1.setPolar(circleRadius1, angt4);
                offs2.setPolar(circleRadius2, angt4);

                poss.push_back(new DmLine{
                    circleCenter1 - offs1,
                    circleCenter2 + offs2
                });
            }
        }
    }
    else
    {
        // circle2Entity 是椭圆
        std::unique_ptr<DmEllipse> e2(
            static_cast<DmEllipse*>(circle2Entity->clone()));
        DmVector m0(circle1Entity->getCenter());
        e2->move(-m0); // 将 circle1Entity 中心移到原点

        double a = 0.0;
        double b = 0.0;
        double a0 = 0.0;

        if (circle1Entity->getEntityType() != DM::EntityEllipse)
        {
            a = fabs(circle1Entity->getRadius());
            b = a;
            if (fabs(a) < DM_TOLERANCE)
            {
                return nullptr;
            }
        }
        else
        {
            DmEllipse* e1 = static_cast<DmEllipse*>(circle1Entity);
            a0 = e1->getAngle();
            e2->rotate(-a0); // e1 长轴沿 x 轴方向
            a = e1->getMajorRadius();
            b = e1->getRatio() * a;
            if (fabs(a) < DM_TOLERANCE || fabs(b) < DM_TOLERANCE)
            {
                return nullptr;
            }
        }
        DmVector factor1(1.0 / a, 1.0 / b);
        // 将 circle1Entity 缩放到单位圆
        e2->scale(DmVector(0.0, 0.0), factor1);
        factor1.set(a, b);
        double a2 = e2->getAngle();
        e2->rotate(-a2);
        a = e2->getMajorP().x;
        b = a * e2->getRatio();
        DmVector v(e2->getCenter());

        std::vector<double> m(0, 0.0);
        m.push_back(1.0 / (a * a));              // ma000
        m.push_back(1.0 / (b * b));              // ma000(第二项)
        m.push_back(v.y * v.y - 1.0);            // ma100
        m.push_back(v.x * v.y);                  // ma101
        m.push_back(v.x * v.x - 1.0);            // ma111
        m.push_back(2.0 * a * b * v.y);          // mb10
        m.push_back(2.0 * a * b * v.x);          // mb11
        m.push_back(a * a * b * b);              // mc1

        // 求解四次方程组, 获取公切线解集
        auto vs0 = Math2d::simultaneousQuadraticSolver(m);
        if (vs0.getNumber() < 1)
        {
            return nullptr;
        }
        for (DmVector vpec : vs0)
        {
            DmVector vpe2(e2->getCenter()
                + DmVector(vpec.y / e2->getRatio(),
                           vpec.x * e2->getRatio()));
            vpec.x *= -1.0;
            DmVector vpe1(vpe2 - vpec
                * (DmVector::dotP(vpec, vpe2) / vpec.squared()));

            DmLine* l = new DmLine{ vpe1, vpe2 };
            l->rotate(a2);
            l->scale(factor1);
            l->rotate(a0);
            l->move(m0);
            poss.push_back(l);
        }
    }

    // 寻找距离参考点最近的切线
    if (poss.size() < 1)
    {
        return nullptr;
    }
    double minDist = DM_MAXDOUBLE;
    double dist = 0.0;
    int idx = -1;
    for (size_t i = 0; i < poss.size(); ++i)
    {
        if (poss[i])
        {
            poss[i]->getNearestPointOnEntity(coord, false, &dist);
            if (dist < minDist)
            {
                minDist = dist;
                idx = static_cast<int>(i);
            }
        }
    }
    if (idx != -1)
    {
        LineData resultData = poss[idx]->getData();
        for (auto p : poss)
        {
            if (p)
            {
                delete p;
            }
        }
        ret = new DmLine(nullptr, resultData);
    }
    else
    {
        ret = nullptr;
    }

    return ret;
}

/// @brief 两圆公切线工具：选第一个圆，再选第二个圆
class DrawLineTangent2Tool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetCircle1, ///< 选择第一个圆或椭圆
        SetCircle2  ///< 选择第二个圆或椭圆
    };

    DrawLineTangent2Tool(DrawLineTangent2Command& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    std::optional<DM::CursorType> getCursor() const override { return DM::SelectCursor; }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetCircle1:
            GUIDIALOGFACTORY->updateMouseWidget(DrawLineTangent2Command::tr("Select first circle or ellipse"),
                                                DrawLineTangent2Command::tr("Cancel"));
            break;
        case SetCircle2:
            GUIDIALOGFACTORY->updateMouseWidget(DrawLineTangent2Command::tr("Select second circle or ellipse"),
                                                DrawLineTangent2Command::tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        if (status() != SetCircle2)
        {
            return;
        }

        DmEntity* en = snapper()->catchEntity(
            e, EntityTypeList{DM::EntityArc, DM::EntityCircle, DM::EntityEllipse}, DM::ResolveAll);
        if (!en || en == m_circle1)
        {
            return;
        }
        if (m_circle2)
        {
            m_circle2->setHighlighted(false);
        }
        m_circle2 = en;
        m_circle2->setHighlighted(true);
        view()->specifyDocumentModified();
        view()->redraw();

        DmVector mouse(view()->toGraphX(e->x()), view()->toGraphY(e->y()));
        std::unique_ptr<DmLine> tangent(createTangent2(mouse, m_circle1, m_circle2));
        if (!tangent)
        {
            m_valid = false;
            return;
        }
        m_valid = true;
        m_lineData = tangent->getData();
        m_command.previewTangent(m_lineData);
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::RightButton)
        {
            m_command.preview().clear();
            if (status() <= 0)
            {
                command().finish();
                return;
            }
            restart(status() - 1);
            clearHighlighted();
            return;
        }

        switch (status())
        {
        case SetCircle1:
            m_circle1 = snapper()->catchEntity(
                e, EntityTypeList{DM::EntityArc, DM::EntityCircle, DM::EntityEllipse}, DM::ResolveAll);
            if (!m_circle1)
            {
                return;
            }
            m_circle1->setHighlighted(true);
            view()->specifyDocumentModified();
            view()->redraw();
            setStatus(status() + 1);
            break;

        case SetCircle2:
            // 与原 Action 一致：m_valid 提交后不复位
            if (m_valid)
            {
                m_command.commitTangent(m_lineData);
                clearHighlighted();
                setStatus(SetCircle1);
            }
            break;

        default:
            break;
        }
    }

    void onFinish() override
    {
        if (m_circle1)
        {
            // 原 Action 在还没选中第二个圆时结束会解引用空指针，这里跳过
            m_circle1->setHighlighted(false);
            if (m_circle2)
            {
                m_circle2->setHighlighted(false);
            }
            view()->specifyDocumentModified();
            view()->redraw();
        }
    }

private:
    /// @brief 取消两个圆的高亮
    void clearHighlighted()
    {
        for (DmEntity** p : {&m_circle1, &m_circle2})
        {
            if (*p)
            {
                (*p)->setHighlighted(false);
                *p = nullptr;
            }
        }
        view()->specifyDocumentModified();
        view()->redraw();
    }

    DrawLineTangent2Command& m_command;
    LineData m_lineData;          ///< 最近一次求出的切线
    DmEntity* m_circle1 = nullptr; ///< 第一个被选中的实体
    DmEntity* m_circle2 = nullptr; ///< 第二个被选中的实体
    bool m_valid = false;         ///< 最近一次是否求出了切线
};
}  // namespace

std::unique_ptr<BasePlaceTool> DrawLineTangent2Command::createTool()
{
    return std::make_unique<DrawLineTangent2Tool>(*this, document(), view());
}

namespace
{
const bool g_registered = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionDrawLineTangent2, QStringLiteral("draw.line_tangent2"),
    exclusiveCommandFactory<DrawLineTangent2Command>());
}  // namespace
