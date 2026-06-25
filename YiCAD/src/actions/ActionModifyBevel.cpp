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


/// @file ActionModifyBevel.cpp
/// @brief 倒角修改 Action 类的实现

#include "ActionModifyBevel.h"

#include <QAction>
#include <QMouseEvent>

#include "ArcData.h"
#include "DmAtomicEntity.h"
#include "DmArc.h"
#include "DmCircle.h"
#include "DmLine.h"
#include "DmSpline.h"
#include "DmEllipse.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Information.h"
#include "LineData.h"
#include "Math2d.h"
#include "Modification.h"
#include "Preview.h"
#include "Transaction.h"

/// @brief 端点判定距离阈值
constexpr double BEVEL_ENDPOINT_TOLERANCE = 1e-10;

/// @brief 内部点数据结构，保存倒角计算所需的坐标和数据
struct ActionModifyBevel::Points
{
    DmVector coord1;
    DmVector coord2;
    double length1 = 1.0;
    double length2 = 1.0;
    bool trim = true;
};

ActionModifyBevel::ActionModifyBevel(DmDocument* doc, GuiDocumentView* docView)
    : PreviewActionInterface("Bevel Entities", doc, docView)
    , entity1(nullptr)
    , entity2(nullptr)
    , pPoints(new Points{})
    , lastStatus(SetEntity1)
{
    actionType = DM::ActionModifyBevel;
}

ActionModifyBevel::~ActionModifyBevel()
{
    unhighlightEntity();
}

void ActionModifyBevel::unhighlightEntity()
{
    if (prevHighlighted)
    {
        prevHighlighted->setHighlighted(false);
        docView->specifyDocumentModified();
        docView->redraw();
        prevHighlighted = nullptr;
    }
}

void ActionModifyBevel::init(int status)
{
    ActionInterface::init(status);

    snapMode.clear();
    snapMode.restriction = DM::RestrictNothing;
}

ActionModifyBevel::BevelResult ActionModifyBevel::computeBevel(
    const DmVector& ref1, DmAtomicEntity* e1,
    const DmVector& ref2, DmAtomicEntity* e2,
    double l1, double l2) const
{
    BevelResult result;

    if (!e1 || !e2 || l1 < 0.0 || l2 < 0.0)
        return result;

    DmVectorSolutions sol = Information::getIntersection(e1, e2, false);
    if (sol.getNumber() == 0)
        return result;

    DmVector P = sol.getClosest(ref1);
    result.intersection = P;

    // 计算 point1：在 entity1 上从交点向 ref1（entity1 上的点击位置）方向走 l1
    DM::EntityType t1 = e1->getEntityType();
    if (t1 == DM::EntityLine)
    {
        DmVector tangent = e1->getTangentDirection(P);
        DmVector dir = tangent.normalize();
        DmVector candA = P + dir * l1;
        DmVector candB = P - dir * l1;
        result.point1 = candA.distanceTo(ref1) < candB.distanceTo(ref1) ? candA : candB;
    }
    else if (t1 == DM::EntityArc || t1 == DM::EntityCircle)
    {
        DmVector center = e1->getCenter();
        double radius = e1->getRadius();
        double angleP = center.angleTo(P);
        double angularChange = l1 / radius;
        DmVector candCCW = center + DmVector::polar(radius, angleP + angularChange);
        DmVector candCW = center + DmVector::polar(radius, angleP - angularChange);
        result.point1 = candCCW.distanceTo(ref1) < candCW.distanceTo(ref1) ? candCCW : candCW;
    }
    else
    {
        return result;
    }

    // 计算 point2：在 entity2 上从交点向 ref2（entity2 上的点击位置）方向走 l2
    DM::EntityType t2 = e2->getEntityType();
    if (t2 == DM::EntityLine)
    {
        DmVector tangent = e2->getTangentDirection(P);
        DmVector dir = tangent.normalize();
        DmVector candA = P + dir * l2;
        DmVector candB = P - dir * l2;
        result.point2 = candA.distanceTo(ref2) < candB.distanceTo(ref2) ? candA : candB;
    }
    else if (t2 == DM::EntityArc || t2 == DM::EntityCircle)
    {
        DmVector center = e2->getCenter();
        double radius = e2->getRadius();
        double angleP = center.angleTo(P);
        double angularChange = l2 / radius;
        DmVector candCCW = center + DmVector::polar(radius, angleP + angularChange);
        DmVector candCW = center + DmVector::polar(radius, angleP - angularChange);
        result.point2 = candCCW.distanceTo(ref2) < candCW.distanceTo(ref2) ? candCCW : candCW;
    }
    else
    {
        return result;
    }

    result.valid = true;
    return result;
}

void ActionModifyBevel::trigger()
{
    if (!entity1 || entity1->isContainer() || !entity2 || entity2->isContainer())
        return;

    if (entity1 == entity2)
        return;

    auto pe1 = dynamic_cast<DmAtomicEntity*>(entity1);
    auto pe2 = dynamic_cast<DmAtomicEntity*>(entity2);
    if (!pe1 || !pe2)
        return;

    deletePreview();

    double l1 = pPoints->length1;
    double l2 = pPoints->length2;
    bool trim = pPoints->trim;

    BevelResult br = computeBevel(pPoints->coord1, pe1, pPoints->coord2, pe2, l1, l2);
    if (!br.valid)
        return;

    DmVectorSolutions sol2 = Information::getIntersection(pe1, pe2, false);

    Transaction t(tr("Bevel").toStdString(), pDocument);
    t.start();

    auto entTable = pDocument->getEntityTable();

    if (trim)
    {
        DmAtomicEntity* trimmed1 = static_cast<DmAtomicEntity*>(pe1->clone());
        DmAtomicEntity* trimmed2 = static_cast<DmAtomicEntity*>(pe2->clone());
        trimmed1->setParent(nullptr);
        trimmed2->setParent(nullptr);

        // 裁剪 entity1
        DmVector is2 = sol2.getClosest(pPoints->coord2);
        DM::Ending ending1 = pe1->getTrimPoint(pPoints->coord1, is2);
        switch (ending1)
        {
        case DM::EndingStart:
            trimmed1->trimStartpoint(br.point1);
            break;
        case DM::EndingEnd:
            trimmed1->trimEndpoint(br.point1);
            break;
        default:
            break;
        }

        // 裁剪 entity2
        is2 = sol2.getClosest(pPoints->coord1);
        DM::Ending ending2 = pe2->getTrimPoint(pPoints->coord2, is2);
        switch (ending2)
        {
        case DM::EndingStart:
            trimmed2->trimStartpoint(br.point2);
            break;
        case DM::EndingEnd:
            trimmed2->trimEndpoint(br.point2);
            break;
        default:
            break;
        }

        entTable->remove(pe1);
        entTable->remove(pe2);
        entTable->add(trimmed1);
        entTable->add(trimmed2);
    }

    DmLine* line = new DmLine(nullptr, LineData(br.point1, br.point2));
    line->setPen(pe1->getPen());
    line->setLayer(pe1->getLayer());
    entTable->add(line);

    t.commit();

    unhighlightEntity();
    pPoints->coord1 = DmVector(false);
    entity1 = nullptr;
    pPoints->coord2 = DmVector(false);
    entity2 = nullptr;
    setStatus(SetEntity1);

    GUIDIALOGFACTORY->updateSelectionWidget(pDocument->getEntityTable()->countSelect());
}

DmVector ActionModifyBevel::setmousePoint(const DmVector& m_p, DmEntity* e)
{
    DmVector c_p;
    isEndPt = false;

    if (e != nullptr)
    {
        DmVector nearestPt = e->getNearestPointOnEntity(m_p, true, nullptr, nullptr);
        DmVector start = e->getStartpoint();
        DmVector end = e->getEndpoint();
        if (start.distanceTo(nearestPt) < BEVEL_ENDPOINT_TOLERANCE || end.distanceTo(nearestPt) < BEVEL_ENDPOINT_TOLERANCE)
        {
            isEndPt = true;
        }
        else
        {
            c_p = nearestPt;
        }
    }
    return c_p;
}

void ActionModifyBevel::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = docView->toGraph(e->x(), e->y());
    DmEntity* se = catchEntity(e, { DM::EntityLine, DM::EntityArc, DM::EntityCircle, DM::EntityEllipse, DM::EntitySpline }, DM::ResolveAllButTextImage);

    switch (getStatus())
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
                docView->specifyDocumentModified();
                docView->redraw();
                prevHighlighted = entity1;
            }
        }
        pPoints->coord1 = setmousePoint(mouse, entity1);
    }
    break;

    case SetEntity2:
    {
        if (se != prevHighlighted)
        {
            if (prevHighlighted && prevHighlighted != entity1)
            {
                prevHighlighted->setHighlighted(false);
                docView->specifyDocumentModified();
                docView->redraw();
            }
            entity2 = se;
            if (entity2 && entity2 != entity1)
            {
                entity2->setHighlighted(true);
                docView->specifyDocumentModified();
                docView->redraw();
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
        pPoints->coord2 = setmousePoint(mouse, entity2);

        deletePreview();
        if (entity1 && entity2 && entity2 != entity1 && !entity2->isContainer() && !isEndPt &&
            Modification::isCutableEntity(entity1) && Modification::isCutableEntity(entity2))
        {
            auto pe1 = dynamic_cast<DmAtomicEntity*>(entity1);
            auto pe2 = dynamic_cast<DmAtomicEntity*>(entity2);
            if (pe1 && pe2)
            {
                BevelResult br = computeBevel(pPoints->coord1, pe1, pPoints->coord2, pe2, pPoints->length1, pPoints->length2);
                if (br.valid)
                {
                    // 预览倒角线
                    DmLine* previewLine = new DmLine(nullptr, LineData(br.point1, br.point2));
                    preview->addEntity(previewLine);

                    if (pPoints->trim)
                    {
                        DmAtomicEntity* t1 = static_cast<DmAtomicEntity*>(pe1->clone());
                        DmAtomicEntity* t2 = static_cast<DmAtomicEntity*>(pe2->clone());
                        t1->setParent(nullptr);
                        t2->setParent(nullptr);

                        DmVectorSolutions sol2 = Information::getIntersection(pe1, pe2, false);

                        DmVector is2 = sol2.getClosest(pPoints->coord2);
                        DM::Ending e1 = pe1->getTrimPoint(pPoints->coord1, is2);
                        if (e1 == DM::EndingStart)
                            t1->trimStartpoint(br.point1);
                        else if (e1 == DM::EndingEnd)
                            t1->trimEndpoint(br.point1);

                        is2 = sol2.getClosest(pPoints->coord1);
                        DM::Ending e2 = pe2->getTrimPoint(pPoints->coord2, is2);
                        if (e2 == DM::EndingStart)
                            t2->trimStartpoint(br.point2);
                        else if (e2 == DM::EndingEnd)
                            t2->trimEndpoint(br.point2);

                        preview->addEntity(t1);
                        preview->addEntity(t2);
                    }
                }
            }
        }
        drawPreview();
    }
    break;

    default:
        break;
    }
}

void ActionModifyBevel::mouseReleaseEvent(QMouseEvent* e)
{
    DmVector mouse = docView->toGraph(e->x(), e->y());
    DmEntity* se = catchEntity(e, { DM::EntityLine, DM::EntityArc, DM::EntityCircle, DM::EntityEllipse, DM::EntitySpline }, DM::ResolveAll);

    if (e->button() == Qt::LeftButton)
    {
        switch (getStatus())
        {
        case SetEntity1:
        {
            entity1 = se;
            pPoints->coord1 = setmousePoint(mouse, entity1);
            if (entity1 && !entity1->isContainer() && !isEndPt && Modification::isCutableEntity(entity1))
            {
                setStatus(SetEntity2);
            }
        }
        break;

        case SetEntity2:
        {
            entity2 = se;
            pPoints->coord2 = setmousePoint(mouse, entity2);
            if (entity2 && entity2 != entity1 && !entity2->isContainer() && !isEndPt && Modification::isCutableEntity(entity2))
            {
                trigger();
            }
        }
        break;

        default:
            break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        unhighlightEntity();
        deletePreview();
        init(getStatus() - 1);
    }
}

void ActionModifyBevel::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus())
    {
    case SetEntity1:
    case SetEntity2:
        if (checkCommand("length1", c))
        {
            e->accept();
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetLength1);
        }
        else if (checkCommand("length2", c))
        {
            e->accept();
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetLength2);
        }
        else if (checkCommand("trim", c))
        {
            e->accept();
            pPoints->trim = !pPoints->trim;
            GUIDIALOGFACTORY->requestOptions(this, true, true);
        }
        break;

    case SetLength1:
    {
        bool ok = false;
        double l = Math2d::eval(c, &ok);
        if (ok)
        {
            e->accept();
            pPoints->length1 = l;
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        GUIDIALOGFACTORY->requestOptions(this, true, true);
        setStatus(lastStatus);
    }
    break;

    case SetLength2:
    {
        bool ok = false;
        double l = Math2d::eval(c, &ok);
        if (ok)
        {
            e->accept();
            pPoints->length2 = l;
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        GUIDIALOGFACTORY->requestOptions(this, true, true);
        setStatus(lastStatus);
    }
    break;

    default:
        break;
    }
}

void ActionModifyBevel::setLength1(double l1)
{
    pPoints->length1 = l1;
}

double ActionModifyBevel::getLength1() const
{
    return pPoints->length1;
}

void ActionModifyBevel::setLength2(double l2)
{
    pPoints->length2 = l2;
}

double ActionModifyBevel::getLength2() const
{
    return pPoints->length2;
}

void ActionModifyBevel::setTrim(bool t)
{
    pPoints->trim = t;
}

bool ActionModifyBevel::isTrimOn() const
{
    return pPoints->trim;
}

QStringList ActionModifyBevel::getAvailableCommands()
{
    QStringList cmd;

    switch (getStatus())
    {
    case SetEntity1:
    case SetEntity2:
        cmd += command("length1");
        cmd += command("length2");
        cmd += command("trim");
        break;

    default:
        break;
    }

    return cmd;
}

void ActionModifyBevel::showOptions()
{
    ActionInterface::showOptions();

    GUIDIALOGFACTORY->requestOptions(this, true);
}

void ActionModifyBevel::hideOptions()
{
    ActionInterface::hideOptions();

    GUIDIALOGFACTORY->requestOptions(this, false);
}

void ActionModifyBevel::updateMouseButtonHints()
{
    switch (getStatus())
    {
    case SetEntity1:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Specify first entity"), tr("Back"));
        break;

    case SetEntity2:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Specify second entity"), tr("Back"));
        break;

    case SetLength1:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Enter length 1:"), tr("Cancel"));
        break;

    case SetLength2:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Enter length 2:"), tr("Cancel"));
        break;

    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void ActionModifyBevel::updateMouseCursor()
{
    docView->setMouseCursor(DM::SelectCursor);
}
