/****************************************************************************
**
 * Draw circle by foci and a point on circle

Copyright (C) 2012 Dongxu Li (dongxuli2011@gmail.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/


/// @file ActionDrawCircleTan3.cpp
/// @brief 绘制三个给定圆的公切圆（阿波罗尼奥斯问题）的交互动作实现

#include "ActionDrawCircleTan3.h"

#include <QMouseEvent>

#include "DmCircle.h"
#include "DmLine.h"
#include "DmPoint.h"
#include "DmDocument.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Information.h"
#include "Preview.h"
#include "Quadratic.h"
#include "Transaction.h"

struct ActionDrawCircleTan3::Points
{
    std::vector<DmAtomicEntity*> circles;
    std::shared_ptr<CircleData> cData{std::make_shared<CircleData>()};
    DmVector coord;
    bool valid{false};
    // keep a list of centers found
    std::vector<std::shared_ptr<CircleData> > candidates;
    DmVectorSolutions centers;
};

ActionDrawCircleTan3::~ActionDrawCircleTan3() = default;

/// @brief 构造函数
ActionDrawCircleTan3::ActionDrawCircleTan3(DmDocument* doc, GuiDocumentView* docView) :
    PreviewActionInterface("Draw circle inscribed", doc, docView), pPoints(new Points{})
{
    actionType = DM::ActionDrawCircleTan3;
}

void ActionDrawCircleTan3::init(int status)
{
    PreviewActionInterface::init(status);
    if (status >= 0)
    {
        PreviewActionInterface::suspend();
    }

    if (status == SetCircle1)
    {
        pPoints->circles.clear();
    }
}

void ActionDrawCircleTan3::finish(bool updateTB)
{
    if (!pPoints->circles.empty())
    {
        for (DmAtomicEntity* const pc : pPoints->circles)
        {
            if (pc)
            {
                pc->setHighlighted(false);
            }
        }

        docView->redraw();
        pPoints->circles.clear();
    }
    PreviewActionInterface::finish(updateTB);
}

void ActionDrawCircleTan3::trigger()
{
    PreviewActionInterface::trigger();

    Transaction t(tr("Create Circletan3").toStdString(), pDocument);
    t.start();
    DmCircle* circle = new DmCircle(nullptr, *pPoints->cData);
    circle->setDocument(pDocument);
    pDocument->getEntityTable()->add(circle);
    t.commit();

    for (DmAtomicEntity* const pc : pPoints->circles)
    {
        if (pc)
        {
            pc->setHighlighted(false);
        }
    }
    docView->redraw();

    pPoints->circles.clear();
    setStatus(SetCircle1);
}

void ActionDrawCircleTan3::mouseMoveEvent(QMouseEvent* e)
{
    switch (getStatus())
    {
        case SetCenter:
        {
            pPoints->coord = docView->toGraph(e->x(), e->y());
            deletePreview();
            if (preparePreview())
            {
                DmCircle* pCircle = new DmCircle(preview->getEntityContainer(), *pPoints->cData);
                pCircle->setDocument(PreviewActionInterface::pDocument);
                preview->addEntity(pCircle);
                for (auto& c : pPoints->candidates)
                {
                    preview->addEntity(new DmPoint(nullptr, PointData(c->getCenter())));
                }
                drawPreview();
            }
        }
        break;
        default:
            break;
    }
}

bool ActionDrawCircleTan3::getData()
{
    if (getStatus() != SetCircle3)
    {
        return false;
    }
    // find the nearest circle
    size_t i                = 0;
    size_t const countLines = std::count_if(pPoints->circles.begin(), pPoints->circles.end(),
                                            [](DmAtomicEntity* e) -> bool { return e->getEntityType() == DM::EntityLine; });

    for (; i < pPoints->circles.size(); ++i)
    {
        if (pPoints->circles[i]->getEntityType() == DM::EntityLine)
        {
            break;
        }
    }
    pPoints->candidates.clear();
    size_t i1 = (i + 1) % 3;
    size_t i2 = (i + 2) % 3;
    if (i < pPoints->circles.size() && pPoints->circles[i]->getEntityType() == DM::EntityLine)
    {
        // one or more lines

        Quadratic lc0(pPoints->circles[i], pPoints->circles[i1], false);
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
                        lc1           = Quadratic(pPoints->circles[i], pPoints->circles[i1], k & 1u);
                        Quadratic lc2 = Quadratic(pPoints->circles[i], pPoints->circles[i2], k & 2u);
                        sol.push_back(Quadratic::getIntersection(lc1, lc2));
                    }
                }
                break;
            case 2:
                // 2 lines, one circle
                {
                    if (pPoints->circles[i2]->getEntityType() == DM::EntityLine)
                    {
                        std::swap(i1, i2);
                    }
                    // i2 is circle

                    for (unsigned k = 0; k < 4; ++k)
                    {
                        // loop through all mirroring cases
                        lc1           = Quadratic(pPoints->circles[i2], pPoints->circles[i], k & 1u);
                        Quadratic lc2 = Quadratic(pPoints->circles[i2], pPoints->circles[i1], k & 2u);
                        sol.push_back(Quadratic::getIntersection(lc1, lc2));
                    }
                }
                break;
            case 3:
                // 3 lines
                {
                    lc0       = pPoints->circles[i]->getQuadratic();
                    lc1       = pPoints->circles[i1]->getQuadratic();
                    auto lc2  = pPoints->circles[i2]->getQuadratic();
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

                    DmLine* line0 = static_cast<DmLine*>(pPoints->circles[i]);
                    DmLine* line1 = static_cast<DmLine*>(pPoints->circles[i1]);
                    DmLine* line2 = static_cast<DmLine*>(pPoints->circles[i2]);
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
            if (pPoints->circles[(i + j) % 3]->getEntityType() == DM::EntityCircle)
            {
                pPoints->circles[i]->getNearestPointOnEntity(pPoints->circles[(i + j) % 3]->getCenter(), false, &d);
                if (d < DM_TOLERANCE)
                {
                    Quadratic lc2(pPoints->circles[i], pPoints->circles[(i + j) % 3], true);
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
            pPoints->circles[i]->getNearestPointOnEntity(v, false, &d);
            auto data = std::make_shared<CircleData>(v, d);
            if (pPoints->circles[(i + 1) % 3]->isTangent(*data) == false)
            {
                continue;
            }
            if (pPoints->circles[(i + 2) % 3]->isTangent(*data) == false)
            {
                continue;
            }
            pPoints->candidates.push_back(data);
        }
    }
    else
    {
        DmCircle c{nullptr, *pPoints->cData};
        auto solutions = c.createTan3(pPoints->circles);
        pPoints->candidates.clear();
        for (const DmCircle& s : solutions)
        {
            pPoints->candidates.push_back(std::make_shared<CircleData>(s.getData()));
        }
    }
    pPoints->valid = (pPoints->candidates.size() > 0);
    return pPoints->valid;
}

bool ActionDrawCircleTan3::preparePreview()
{
    if (getStatus() != SetCenter || pPoints->valid == false)
    {
        pPoints->valid = false;
        return false;
    }
    // find the nearest circle
    size_t index = pPoints->candidates.size();
    double dist  = DM_MAXDOUBLE * DM_MAXDOUBLE;
    for (size_t i = 0; i < pPoints->candidates.size(); ++i)
    {
        preview->addEntity(new DmPoint(nullptr, PointData(pPoints->candidates.at(i)->getCenter())));
        double d;
        DmCircle(nullptr, *pPoints->candidates.at(i)).getNearestPointOnEntity(pPoints->coord, false, &d);
        double dCenter = pPoints->coord.distanceTo(pPoints->candidates.at(i)->getCenter());
        d              = std::min(d, dCenter);
        if (d < dist)
        {
            dist  = d;
            index = i;
        }
    }
    if (index < pPoints->candidates.size())
    {
        pPoints->cData = pPoints->candidates.at(index);
        pPoints->valid = true;
    }
    else
    {
        pPoints->valid = false;
    }
    return pPoints->valid;
}

DmEntity* ActionDrawCircleTan3::catchCircle(QMouseEvent* e)
{
    DmEntity* ret = nullptr;
    DmEntity* en  = catchEntity(e, EntityTypeList{ DM::EntityArc, DM::EntityCircle, DM::EntityLine, DM::EntityPoint }, DM::ResolveAll);
    if (!en)
    {
        return ret;
    }
    if (!en->isVisible())
    {
        return ret;
    }
    for (int i = 0; i < getStatus(); ++i)
    {
        if (en->getId() == pPoints->circles[i]->getId())
        {
            return ret;  // 不重复选择同一条线
        }
    }
    return en;
}

void ActionDrawCircleTan3::mouseReleaseEvent(QMouseEvent* e)
{
    // Proceed to next status
    if (e->button() == Qt::LeftButton)
    {
        switch (getStatus())
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
                pPoints->circles.resize(getStatus());
                for (const DmAtomicEntity* const pc : pPoints->circles)
                {
                    if (pc == en)
                    {
                        continue;
                    }
                }
                pPoints->circles.push_back(static_cast<DmAtomicEntity*>(en));
                if (getStatus() <= SetCircle2 || (getStatus() == SetCircle3 && getData()))
                {
                    pPoints->circles.at(pPoints->circles.size() - 1)->setHighlighted(true);
                    docView->redraw();
                    setStatus(getStatus() + 1);
                }
            }
            break;
            case SetCenter:
                pPoints->coord = docView->toGraph(e->x(), e->y());
                if (preparePreview())
                {
                    trigger();
                }
                break;

            default:
                break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        // Return to last status:
        if (getStatus() > 0)
        {
            pPoints->circles[getStatus() - 1]->setHighlighted(false);
            pPoints->circles.pop_back();
            docView->redraw();
            deletePreview();
        }
        init(getStatus() - 1);
    }
}

QStringList ActionDrawCircleTan3::getAvailableCommands()
{
    QStringList cmd;
    return cmd;
}

void ActionDrawCircleTan3::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetCircle1:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify the first line/arc/circle"), tr("Cancel"));
            break;

        case SetCircle2:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify the second line/arc/circle"), tr("Back"));
            break;
        case SetCircle3:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify the third line/arc/circle"), tr("Back"));
            break;

        case SetCenter:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select the center of the tangent circle"), tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void ActionDrawCircleTan3::updateMouseCursor()
{
    docView->setMouseCursor(DM::SelectCursor);
}
