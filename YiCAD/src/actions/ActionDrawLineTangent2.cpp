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


/// @file ActionDrawLineTangent2.cpp
/// @brief 两圆/椭圆公切线绘制交互动作的实现

#include "ActionDrawLineTangent2.h"

#include <QAction>
#include <QMouseEvent>

#include "Debug.h"
#include "DmLine.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Preview.h"
#include "Math2d.h"
#include "DmEllipse.h"
#include "Transaction.h"
#include "GeUtility.h"

namespace
{
    constexpr double DISTANCE_TOLERANCE = 1.0e-6; ///< 圆心距离容差阈值
}

/// @brief 构造函数
/// @param [in] doc 文档指针
/// @param [in] docView 文档视图指针
ActionDrawLineTangent2::ActionDrawLineTangent2(DmDocument* doc,
                                               GuiDocumentView* docView)
    : PreviewActionInterface("Draw Tangents 2", doc, docView)
    , m_circle1(nullptr)
    , m_circle2(nullptr)
    , m_valid(false)
{
    actionType = DM::ActionDrawLineTangent2;
    setStatus(SetCircle1);
}

ActionDrawLineTangent2::~ActionDrawLineTangent2() = default;

/// @brief 完成动作, 清除高亮和预览
/// @param [in] updateTB 是否更新工具栏状态
void ActionDrawLineTangent2::finish(bool updateTB)
{
    if (m_circle1)
    {
        m_circle1->setHighlighted(false);
        m_circle2->setHighlighted(false);
        docView->specifyDocumentModified();
        docView->redraw();
    }
    PreviewActionInterface::finish(updateTB);
}

/// @brief 执行绘制动作, 将切线添加到文档
void ActionDrawLineTangent2::trigger()
{
    PreviewActionInterface::trigger();

    DmEntity* newEntity = new DmLine(nullptr, *m_lineData);
    newEntity->setDocument(pDocument);
    Transaction t(tr("Create line tangent").toStdString(), pDocument);
    t.start();
    pDocument->getEntityTable()->add(newEntity);
    t.commit();

    clearHighlighted();
    setStatus(SetCircle1);
    m_tangent.reset();
}

/// @brief 清除所有高亮选中的实体
void ActionDrawLineTangent2::clearHighlighted()
{
    for (DmEntity** p : { &m_circle1, &m_circle2 })
    {
        if (*p)
        {
            (*p)->setHighlighted(false);
            *p = nullptr;
        }
    }
    docView->specifyDocumentModified();
    docView->redraw();
}

/// @brief 处理鼠标移动事件, 实时预览切线
/// @param [in] e 鼠标事件指针
void ActionDrawLineTangent2::mouseMoveEvent(QMouseEvent* e)
{
    e->accept();
    if (getStatus() != SetCircle2)
    {
        return;
    }
    DmEntity* en = catchEntity(e,
        EntityTypeList{
            DM::EntityArc,
            DM::EntityCircle,
            DM::EntityEllipse
        },
        DM::ResolveAll);
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
    docView->specifyDocumentModified();
    docView->redraw();
    DmVector mouse(docView->toGraphX(e->x()),
                   docView->toGraphY(e->y()));
    m_tangent.reset(createTangent2(mouse, m_circle1, m_circle2));
    if (!m_tangent.get())
    {
        m_valid = false;
        return;
    }
    m_valid = true;
    m_lineData.reset(new LineData(m_tangent->getData()));

    deletePreview();
    auto l = new DmLine(preview->getEntityContainer(), *m_lineData);
    l->setDocument(pDocument);
    preview->addEntity(l);
    drawPreview();
}

/// @brief 处理鼠标释放事件, 选择圆或确认切线
/// @param [in] e 鼠标事件指针
void ActionDrawLineTangent2::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::RightButton)
    {
        deletePreview();
        init(getStatus() - 1);
        if (getStatus() >= 0)
        {
            clearHighlighted();
        }
        return;
    }
    switch (getStatus())
    {
    case SetCircle1:
    {
        m_circle1 = catchEntity(e,
            EntityTypeList{
                DM::EntityArc,
                DM::EntityCircle,
                DM::EntityEllipse
            },
            DM::ResolveAll);
        if (!m_circle1)
        {
            return;
        }
        m_circle1->setHighlighted(true);
        docView->specifyDocumentModified();
        docView->redraw();
        setStatus(getStatus() + 1);
        break;
    }

    case SetCircle2:
    {
        if (m_valid)
        {
            trigger();
        }
        break;
    }

    default:
    {
        break;
    }
    }
}

/// @brief 更新鼠标按钮提示文本
void ActionDrawLineTangent2::updateMouseButtonHints()
{
    switch (getStatus())
    {
    case SetCircle1:
        GUIDIALOGFACTORY->updateMouseWidget(
            tr("Select first circle or ellipse"),
            tr("Cancel"));
        break;
    case SetCircle2:
        GUIDIALOGFACTORY->updateMouseWidget(
            tr("Select second circle or ellipse"),
            tr("Back"));
        break;
    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

/// @brief 更新鼠标光标样式
void ActionDrawLineTangent2::updateMouseCursor()
{
    docView->setMouseCursor(DM::SelectCursor);
}

/// @brief 根据给定参考点和两个圆/椭圆计算并创建公切线
///
/// 该函数计算两个圆/椭圆之间的公切线(包含外公切线和内公切线),
/// 并返回距离参考点 coord 最近的那条切线。
///
/// @param [in] coord 参考点坐标(通常为鼠标位置)
/// @param [in] circle1Entity 第一个圆/椭圆实体
/// @param [in] circle2Entity 第二个圆/椭圆实体
/// @return 最近公切线的 DmLine 对象, 调用者负责释放; 无解时返回 nullptr
/// TODO: 该函数约190行, 超过100行限制, 建议拆分为多个子函数
DmLine* ActionDrawLineTangent2::createTangent2(const DmVector& coord,
                                               DmEntity* circle1Entity,
                                               DmEntity* circle2Entity)
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
