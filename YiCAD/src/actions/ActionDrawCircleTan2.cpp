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


/// @file ActionDrawCircleTan2.cpp
/// @brief 绘制与两个给定圆相切且具有指定半径的圆的交互动作实现

#include <vector>
#include "ActionDrawCircleTan2.h"

#include <QMouseEvent>

#include "DmCircle.h"
#include "DmPoint.h"
#include "DmDocument.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Preview.h"
#include "Transaction.h"

struct ActionDrawCircleTan2::Points
{
    CircleData cData;
    DmVector coord;
    double radius{0.};
    bool valid{false};
    DmVectorSolutions centers;
    std::vector<DmAtomicEntity*> circles;
};

/// @brief 构造函数
ActionDrawCircleTan2::ActionDrawCircleTan2(DmDocument* doc, GuiDocumentView* docView) :
    PreviewActionInterface("Draw circle inscribed", doc, docView), pPoints(new Points{})
{
    actionType = DM::ActionDrawCircleTan2;
}

ActionDrawCircleTan2::~ActionDrawCircleTan2() = default;

void ActionDrawCircleTan2::init(int status)
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

void ActionDrawCircleTan2::finish(bool updateTB)
{
    if (pPoints->circles.size() > 0)
    {
        for (auto p : pPoints->circles)
        {
            if (p)
            {
                p->setHighlighted(false);
            }
        }
        docView->redraw();
        pPoints->circles.clear();
    }
    PreviewActionInterface::finish(updateTB);
}

void ActionDrawCircleTan2::trigger()
{
    PreviewActionInterface::trigger();

    Transaction t(tr("Create CircleTan2").toStdString(), pDocument);
    t.start();
    DmCircle* circle = new DmCircle(nullptr, pPoints->cData);
    circle->setDocument(pDocument);
    pDocument->getEntityTable()->add(circle);
    t.commit();

    for (auto p : pPoints->circles)
    {
        p->setHighlighted(false);
    }
    docView->redraw();

    pPoints->circles.clear();
    setStatus(SetCircle1);
}

void ActionDrawCircleTan2::mouseMoveEvent(QMouseEvent* e)
{
    switch (getStatus())
    {
        case SetCenter:
        {
            pPoints->coord = docView->toGraph(e->x(), e->y());
            if (preparePreview())
            {
                deletePreview();
                auto* pCircle = new DmCircle(preview->getEntityContainer(), pPoints->cData);
                pCircle->setDocument(pDocument);
                preview->addEntity(pCircle);
                for (const auto& center : pPoints->centers)
                {
                    preview->addEntity(new DmPoint(nullptr, PointData(center)));
                }
                drawPreview();
            }
        }
        break;
        default:
            break;
    }
}

void ActionDrawCircleTan2::setRadius(const double& r)
{
    pPoints->cData.setRadius(r);
    if (getStatus() == SetCenter)
    {
        pPoints->centers = DmCircle::createTan2(pPoints->circles, pPoints->cData.getRadius());
    }
}

bool ActionDrawCircleTan2::getCenters()
{
    if (getStatus() != SetCircle2)
    {
        return false;
    }
    pPoints->centers = DmCircle::createTan2(pPoints->circles, pPoints->cData.getRadius());
    pPoints->valid   = (pPoints->centers.size() > 0);
    return pPoints->valid;
}

bool ActionDrawCircleTan2::preparePreview() const
{
    if (pPoints->valid)
    {
        pPoints->cData.setCenter(pPoints->centers.getClosest(pPoints->coord));
    }
    return pPoints->valid;
}

DmEntity* ActionDrawCircleTan2::catchCircle(QMouseEvent* e)
{
    DmEntity* en = catchEntity(e, EntityTypeList{ DM::EntityLine, DM::EntityArc, DM::EntityCircle }, DM::ResolveAll);
    if (!en)
    {
        return nullptr;
    }
    if (!en->isVisible())
    {
        return nullptr;
    }
    for (int i = 0; i < getStatus(); i++)
    {
        if (en->getId() == pPoints->circles[i]->getId())
        {
            return nullptr;  // 不重复选择同一条线
        }
    }
    return en;
}

void ActionDrawCircleTan2::mouseReleaseEvent(QMouseEvent* e)
{
    // Proceed to next status
    if (e->button() == Qt::LeftButton)
    {
        switch (getStatus())
        {
            case SetCircle1:
            case SetCircle2:
            {
                DmEntity* en = catchCircle(e);
                if (!en)
                {
                    return;
                }
                pPoints->circles.resize(getStatus());
                pPoints->circles.push_back(dynamic_cast<DmAtomicEntity*>(en));
                if (getStatus() == SetCircle1 || getCenters())
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

void ActionDrawCircleTan2::showOptions()
{
    ActionInterface::showOptions();
    GUIDIALOGFACTORY->requestOptions(this, true);
}

void ActionDrawCircleTan2::hideOptions()
{
    ActionInterface::hideOptions();
    GUIDIALOGFACTORY->requestOptions(this, false);
}

QStringList ActionDrawCircleTan2::getAvailableCommands()
{
    return {};
}

void ActionDrawCircleTan2::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetCircle1:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify the first line/arc/circle"), tr("Cancel"));
            break;

        case SetCircle2:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify the second line/arc/circle"), tr("Back"));
            break;

        case SetCenter:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select the center of the tangent circle"), tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void ActionDrawCircleTan2::updateMouseCursor()
{
    docView->setMouseCursor(DM::SelectCursor);
}

double ActionDrawCircleTan2::getRadius() const
{
    return pPoints->cData.getRadius();
}
