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


/// @file ActionModifyRound.cpp
/// @brief 圆角修改操作——处理用户鼠标事件以实现圆角功能

#include <QAction>
#include "ActionModifyRound.h"

#include <QMouseEvent>

#include "ArcData.h"
#include "Debug.h"
#include "DmArc.h"
#include "DmAtomicEntity.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Information.h"
#include "Math2d.h"
#include "Modification.h"
#include "Preview.h"
#include "Transaction.h"

/// @brief 判断点是否在实体端点的距离阈值
constexpr double ROUND_ENDPOINT_TOLERANCE = 1e-10;

struct ActionModifyRound::Points
{
    DmVector coord1;
    DmVector coord2;
    double radius = 1.0;
    bool trim = true;
};

ActionModifyRound::ActionModifyRound(DmDocument* doc, GuiDocumentView* docView)
    : PreviewActionInterface("Round Entities", doc, docView)
    , entity1(nullptr)
    , entity2(nullptr)
    , pPoints(new Points())
    , lastStatus(SetEntity1)
{
    actionType = DM::ActionModifyRound;
}

ActionModifyRound::~ActionModifyRound()
{
    unhighlightEntity();
}

void ActionModifyRound::unhighlightEntity()
{
    if (prevHighlighted)
    {
        prevHighlighted->setHighlighted(false);
        docView->specifyDocumentModified();
        docView->redraw();
        prevHighlighted = nullptr;
    }
}

void ActionModifyRound::init(int status)
{
    ActionInterface::init(status);

    snapMode.clear();
    snapMode.restriction = DM::RestrictNothing;
}

ActionModifyRound::FilletResult ActionModifyRound::computeFillet(
    const DmVector& ref1, DmAtomicEntity* e1,
    const DmVector& ref2, DmAtomicEntity* e2,
    double radius) const
{
    FilletResult result;

    if (!e1 || !e2 || radius <= 0.0)
        return result;

    // 克隆实体并偏移以寻找圆心
    DmAtomicEntity* par1 = static_cast<DmAtomicEntity*>(e1->clone());
    DmAtomicEntity* par2 = static_cast<DmAtomicEntity*>(e2->clone());
    if (!par1 || !par2)
    {
        delete par1;
        delete par2;
        return result;
    }

    par1->setParent(nullptr);
    par2->setParent(nullptr);
    par1->offset(ref1, radius);
    par2->offset(ref2, radius);

    DmVectorSolutions sol = Information::getIntersection(par1, par2, false);

    delete par1;
    delete par2;

    if (sol.getNumber() == 0)
        return result;

    result.center = sol.getClosest(ref1);
    result.tangent1 = e1->getNearestPointOnEntity(result.center, false);
    result.tangent2 = e2->getNearestPointOnEntity(result.center, false);

    result.startAngle = result.center.angleTo(result.tangent1);
    result.endAngle = result.center.angleTo(result.tangent2);

    // 确保 startAngle < endAngle（CCW 劣弧）
    if (result.startAngle > result.endAngle)
        std::swap(result.startAngle, result.endAngle);

    // 若CCW弧跨度超过180°，取另一方向弧（始终取劣弧）
    if (result.endAngle - result.startAngle > M_PI)
        std::swap(result.startAngle, result.endAngle);

    result.valid = true;
    return result;
}

void ActionModifyRound::trigger()
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

    double r = pPoints->radius;
    bool trim = pPoints->trim;

    FilletResult fr = computeFillet(pPoints->coord2, pe1, pPoints->coord1, pe2, r);
    if (!fr.valid)
        return;

    // 查找原始实体交点，用于确定裁剪方向
    DmVectorSolutions sol2 = Information::getIntersection(pe1, pe2, false);

    Transaction t(tr("Round").toStdString(), pDocument);
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
        DM::Ending ending1 = trimmed1->getTrimPoint(pPoints->coord1, is2);
        switch (ending1)
        {
        case DM::EndingStart:
            trimmed1->trimStartpoint(fr.tangent1);
            break;
        case DM::EndingEnd:
            trimmed1->trimEndpoint(fr.tangent1);
            break;
        default:
            break;
        }

        // 裁剪 entity2
        is2 = sol2.getClosest(pPoints->coord1);
        DM::Ending ending2 = trimmed2->getTrimPoint(pPoints->coord2, is2);
        switch (ending2)
        {
        case DM::EndingStart:
            trimmed2->trimStartpoint(fr.tangent2);
            break;
        case DM::EndingEnd:
            trimmed2->trimEndpoint(fr.tangent2);
            break;
        default:
            break;
        }

        entTable->remove(pe1);
        entTable->remove(pe2);
        entTable->add(trimmed1);
        entTable->add(trimmed2);
    }

    DmArc* arc = new DmArc(nullptr, ArcData(fr.center, DmVector(0.0, 0.0, 1.0), r, fr.startAngle, fr.endAngle));
    arc->setPen(pe1->getPen());
    arc->setLayer(pe1->getLayer());
    entTable->add(arc);

    t.commit();

    unhighlightEntity();
    pPoints->coord1 = DmVector(false);
    entity1 = nullptr;
    pPoints->coord2 = DmVector(false);
    entity2 = nullptr;
    setStatus(SetEntity1);

    GUIDIALOGFACTORY->updateSelectionWidget(pDocument->getEntityTable()->countSelect());
}

DmVector ActionModifyRound::setmousePoint(const DmVector& m_p, DmEntity* e)
{
    DmVector c_p;
    isEndPt = false;

    if (e != nullptr)
    {
        DmVector nearestPt = e->getNearestPointOnEntity(m_p, true, nullptr, nullptr);
        DmVector start = e->getStartpoint();
        DmVector end = e->getEndpoint();
        if (start.distanceTo(nearestPt) < ROUND_ENDPOINT_TOLERANCE || end.distanceTo(nearestPt) < ROUND_ENDPOINT_TOLERANCE)
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

void ActionModifyRound::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = docView->toGraph(e->x(), e->y());
    DmEntity* se = catchEntity(e, { DM::EntityLine, DM::EntityArc, DM::EntityCircle, DM::EntityEllipse,  DM::EntitySpline }, DM::ResolveAllButTextImage);
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
            if (entity1 && entity2 && entity2 != entity1 && !entity2->isContainer() && !isEndPt && Modification::isCutableEntity(entity1) && Modification::isCutableEntity(entity2))
            {
                auto pe1 = dynamic_cast<DmAtomicEntity*>(entity1);
                auto pe2 = dynamic_cast<DmAtomicEntity*>(entity2);
                if (pe1 && pe2)
                {
                    FilletResult fr = computeFillet(pPoints->coord2, pe1, pPoints->coord1, pe2, pPoints->radius);
                    if (fr.valid)
                    {
                        // 预览圆角弧
                        DmArc* previewArc = new DmArc(nullptr, ArcData(fr.center, DmVector(0.0, 0.0, 1.0), pPoints->radius, fr.startAngle, fr.endAngle));
                        preview->addEntity(previewArc);

                        if (pPoints->trim)
                        {
                            DmAtomicEntity* t1 = static_cast<DmAtomicEntity*>(pe1->clone());
                            DmAtomicEntity* t2 = static_cast<DmAtomicEntity*>(pe2->clone());
                            t1->setParent(nullptr);
                            t2->setParent(nullptr);

                            DmVectorSolutions sol2 = Information::getIntersection(pe1, pe2, false);

                            DmVector is2 = sol2.getClosest(pPoints->coord2);
                            DM::Ending e1 = t1->getTrimPoint(pPoints->coord1, is2);
                            if (e1 == DM::EndingStart)
                                t1->trimStartpoint(fr.tangent1);
                            else if (e1 == DM::EndingEnd)
                                t1->trimEndpoint(fr.tangent1);

                            is2 = sol2.getClosest(pPoints->coord1);
                            DM::Ending e2 = t2->getTrimPoint(pPoints->coord2, is2);
                            if (e2 == DM::EndingStart)
                                t2->trimStartpoint(fr.tangent2);
                            else if (e2 == DM::EndingEnd)
                                t2->trimEndpoint(fr.tangent2);

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

void ActionModifyRound::mouseReleaseEvent(QMouseEvent* e)
{
    DmVector mouse = docView->toGraph(e->x(), e->y());
    DmEntity* se = catchEntity(e, { DM::EntityLine, DM::EntityArc, DM::EntityCircle, DM::EntityEllipse,  DM::EntitySpline }, DM::ResolveAll);
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

void ActionModifyRound::commandEvent(GuiCommandEvent* e)
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
            if (checkCommand("radius", c))
            {
                e->accept();
                deletePreview();
                lastStatus = (Status)getStatus();
                setStatus(SetRadius);
            }
            else if (checkCommand("trim", c))
            {
                e->accept();
                deletePreview();
                lastStatus = (Status)getStatus();
                setStatus(SetTrim);
                pPoints->trim = !pPoints->trim;
                GUIDIALOGFACTORY->requestOptions(this, true, true);
            }
            else
            {
                // 未识别的命令，忽略
            }
            break;

        case SetRadius:
        {
            bool ok = false;
            double r = Math2d::eval(c, &ok);
            if (ok)
            {
                e->accept();
                pPoints->radius = r;
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

QStringList ActionModifyRound::getAvailableCommands()
{
    QStringList cmd;
    switch (getStatus())
    {
        case SetEntity1:
        case SetEntity2:
            cmd += command("radius");
            cmd += command("trim");
            break;
        default:
            break;
    }
    return cmd;
}

void ActionModifyRound::setRadius(double r)
{
    pPoints->radius = r;
}

double ActionModifyRound::getRadius() const
{
    return pPoints->radius;
}

void ActionModifyRound::setTrim(bool t)
{
    pPoints->trim = t;
}

bool ActionModifyRound::isTrimOn() const
{
    return pPoints->trim;
}

void ActionModifyRound::showOptions()
{
    ActionInterface::showOptions();

    GUIDIALOGFACTORY->requestOptions(this, true);
}

void ActionModifyRound::hideOptions()
{
    ActionInterface::hideOptions();

    GUIDIALOGFACTORY->requestOptions(this, false);
}

void ActionModifyRound::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetEntity1:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify first entity"), tr("Back"));
            break;
        case SetEntity2:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify second entity"), tr("Back"));
            break;
        case SetRadius:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Enter radius:"), tr("Cancel"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void ActionModifyRound::updateMouseCursor()
{
    docView->setMouseCursor(DM::SelectCursor);
}
