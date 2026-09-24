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

/// @file ModifyBevelCommand.cpp
/// @brief ModifyBevelCommand 与倒角工具的实现；工具的事件处理从原 ActionModifyBevel 机械改写而来

#include "ModifyBevelCommand.h"

#include <memory>

#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "Commands.h"
#include "DmAtomicEntity.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Information.h"
#include "LineData.h"
#include "Math2d.h"
#include "Modification.h"
#include "Transaction.h"

namespace
{
constexpr double BEVEL_ENDPOINT_TOLERANCE = 1e-10; ///< 端点判定距离阈值

/// @brief 倒角工具：选第一个实体，再选第二个实体后倒角；可连续倒角
class ModifyBevelTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetEntity1, ///< 选择第一个实体
        SetEntity2, ///< 选择第二个实体
        SetLength1, ///< 在命令行中设置长度1
        SetLength2  ///< 在命令行中设置长度2
    };

    ModifyBevelTool(ModifyBevelCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
        clearSnapMode();
    }

    std::optional<DM::CursorType> getCursor() const override { return DM::SelectCursor; }

protected:
    void updateHints() override;
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;
    void onCommand(GuiCommandEvent* e) override;
    /// @brief 原 Action 在析构时取消高亮
    void onFinish() override { unhighlightEntity(); }

private:
    /// @brief 倒角计算结果
    struct BevelResult
    {
        bool valid = false;
        DmVector point1;       ///< entity1 上的倒角点（距交点 length1）
        DmVector point2;       ///< entity2 上的倒角点（距交点 length2）
        DmVector intersection; ///< 两实体交点
    };

    /// @brief 两个实体上的点击位置
    struct Points
    {
        DmVector coord1;
        DmVector coord2;
    };

    /// @brief 原 Action 只拾取实体，不捕捉点：初始化捕捉器之后清空捕捉方式
    void clearSnapMode()
    {
        snapper()->getSnapMode()->clear();
        snapper()->getSnapMode()->restriction = DM::RestrictNothing;
    }

    /// @brief 回到某一状态（原 init(status)）；status < 0 时结束命令
    void init(int s)
    {
        if (s < 0)
        {
            command().finish();
            return;
        }
        restart(s);
        clearSnapMode();
    }

    BevelResult computeBevel(const DmVector& ref1, DmAtomicEntity* e1, const DmVector& ref2, DmAtomicEntity* e2,
                             double l1, double l2) const;
    void trigger();
    DmVector setmousePoint(const DmVector& m_p, DmEntity* e);
    QStringList availableCommands() const;
    void unhighlightEntity();

    ModifyBevelCommand& m_command;
    DmEntity* entity1 = nullptr;         ///< 第一个选中实体
    DmEntity* entity2 = nullptr;         ///< 第二个选中实体
    Points m_points;                     ///< 两个实体上的点击位置
    Status lastStatus = SetEntity1;      ///< 进入长度设置前的上一个状态
    bool isEndPt = false;                ///< 鼠标是否在实体端点附近
    DmEntity* prevHighlighted = nullptr; ///< 上次高亮的实体
};
}  // namespace

void ModifyBevelTool::unhighlightEntity()
{
    if (prevHighlighted)
    {
        prevHighlighted->setHighlighted(false);
        view()->specifyDocumentModified();
        view()->redraw();
        prevHighlighted = nullptr;
    }
}

ModifyBevelTool::BevelResult ModifyBevelTool::computeBevel(
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

void ModifyBevelTool::trigger()
{
    if (!entity1 || entity1->isContainer() || !entity2 || entity2->isContainer())
        return;

    if (entity1 == entity2)
        return;

    auto pe1 = dynamic_cast<DmAtomicEntity*>(entity1);
    auto pe2 = dynamic_cast<DmAtomicEntity*>(entity2);
    if (!pe1 || !pe2)
        return;

    m_command.preview().clear();

    double l1 = m_command.length1();
    double l2 = m_command.length2();
    bool trim = m_command.isTrimOn();

    BevelResult br = computeBevel(m_points.coord1, pe1, m_points.coord2, pe2, l1, l2);
    if (!br.valid)
        return;

    DmVectorSolutions sol2 = Information::getIntersection(pe1, pe2, false);

    Transaction t(ModifyBevelCommand::tr("Bevel").toStdString(), document());
    t.start();

    auto entTable = document()->getEntityTable();

    if (trim)
    {
        DmAtomicEntity* trimmed1 = static_cast<DmAtomicEntity*>(pe1->clone());
        DmAtomicEntity* trimmed2 = static_cast<DmAtomicEntity*>(pe2->clone());
        trimmed1->setParent(nullptr);
        trimmed2->setParent(nullptr);

        // 裁剪 entity1
        DmVector is2 = sol2.getClosest(m_points.coord2);
        DM::Ending ending1 = pe1->getTrimPoint(m_points.coord1, is2);
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
        is2 = sol2.getClosest(m_points.coord1);
        DM::Ending ending2 = pe2->getTrimPoint(m_points.coord2, is2);
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
    m_points.coord1 = DmVector(false);
    entity1 = nullptr;
    m_points.coord2 = DmVector(false);
    entity2 = nullptr;
    setStatus(SetEntity1);

    GUIDIALOGFACTORY->updateSelectionWidget(document()->getEntityTable()->countSelect());
}

DmVector ModifyBevelTool::setmousePoint(const DmVector& m_p, DmEntity* e)
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

void ModifyBevelTool::onMouseMove(QMouseEvent* e)
{
    DmVector mouse = view()->toGraph(e->x(), e->y());
    DmEntity* se = snapper()->catchEntity(e, { DM::EntityLine, DM::EntityArc, DM::EntityCircle, DM::EntityEllipse, DM::EntitySpline }, DM::ResolveAllButTextImage);

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
        m_points.coord1 = setmousePoint(mouse, entity1);
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
        m_points.coord2 = setmousePoint(mouse, entity2);

        m_command.preview().clear();
        if (entity1 && entity2 && entity2 != entity1 && !entity2->isContainer() && !isEndPt &&
            Modification::isCutableEntity(entity1) && Modification::isCutableEntity(entity2))
        {
            auto pe1 = dynamic_cast<DmAtomicEntity*>(entity1);
            auto pe2 = dynamic_cast<DmAtomicEntity*>(entity2);
            if (pe1 && pe2)
            {
                BevelResult br = computeBevel(m_points.coord1, pe1, m_points.coord2, pe2, m_command.length1(), m_command.length2());
                if (br.valid)
                {
                    // 预览倒角线
                    DmLine* previewLine = new DmLine(nullptr, LineData(br.point1, br.point2));
                    m_command.preview().entities().addEntity(previewLine);

                    if (m_command.isTrimOn())
                    {
                        DmAtomicEntity* t1 = static_cast<DmAtomicEntity*>(pe1->clone());
                        DmAtomicEntity* t2 = static_cast<DmAtomicEntity*>(pe2->clone());
                        t1->setParent(nullptr);
                        t2->setParent(nullptr);

                        DmVectorSolutions sol2 = Information::getIntersection(pe1, pe2, false);

                        DmVector is2 = sol2.getClosest(m_points.coord2);
                        DM::Ending e1 = pe1->getTrimPoint(m_points.coord1, is2);
                        if (e1 == DM::EndingStart)
                            t1->trimStartpoint(br.point1);
                        else if (e1 == DM::EndingEnd)
                            t1->trimEndpoint(br.point1);

                        is2 = sol2.getClosest(m_points.coord1);
                        DM::Ending e2 = pe2->getTrimPoint(m_points.coord2, is2);
                        if (e2 == DM::EndingStart)
                            t2->trimStartpoint(br.point2);
                        else if (e2 == DM::EndingEnd)
                            t2->trimEndpoint(br.point2);

                        m_command.preview().entities().addEntity(t1);
                        m_command.preview().entities().addEntity(t2);
                    }
                }
            }
        }
        m_command.preview().draw();
    }
    break;

    default:
        break;
    }
}

void ModifyBevelTool::onMouseRelease(QMouseEvent* e)
{
    DmVector mouse = view()->toGraph(e->x(), e->y());
    DmEntity* se = snapper()->catchEntity(e, { DM::EntityLine, DM::EntityArc, DM::EntityCircle, DM::EntityEllipse, DM::EntitySpline }, DM::ResolveAll);

    if (e->button() == Qt::LeftButton)
    {
        switch (status())
        {
        case SetEntity1:
        {
            entity1 = se;
            m_points.coord1 = setmousePoint(mouse, entity1);
            if (entity1 && !entity1->isContainer() && !isEndPt && Modification::isCutableEntity(entity1))
            {
                setStatus(SetEntity2);
            }
        }
        break;

        case SetEntity2:
        {
            entity2 = se;
            m_points.coord2 = setmousePoint(mouse, entity2);
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
        m_command.preview().clear();
        init(status() - 1);
    }
}

void ModifyBevelTool::onCommand(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (Commands::checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(Commands::msgAvailableCommands() + availableCommands().join(", "));
        return;
    }

    switch (status())
    {
    case SetEntity1:
    case SetEntity2:
        if (Commands::checkCommand("length1", c))
        {
            e->accept();
            m_command.preview().clear();
            lastStatus = (Status)status();
            setStatus(SetLength1);
        }
        else if (Commands::checkCommand("length2", c))
        {
            e->accept();
            m_command.preview().clear();
            lastStatus = (Status)status();
            setStatus(SetLength2);
        }
        else if (Commands::checkCommand("trim", c))
        {
            e->accept();
            m_command.setTrim(!m_command.isTrimOn());
            GUIDIALOGFACTORY->requestCommandOptions(&m_command, true, true);
        }
        break;

    case SetLength1:
    {
        bool ok = false;
        double l = Math2d::eval(c, &ok);
        if (ok)
        {
            e->accept();
            m_command.setLength1(l);
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(ModifyBevelCommand::tr("Not a valid expression"));
        }
        GUIDIALOGFACTORY->requestCommandOptions(&m_command, true, true);
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
            m_command.setLength2(l);
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(ModifyBevelCommand::tr("Not a valid expression"));
        }
        GUIDIALOGFACTORY->requestCommandOptions(&m_command, true, true);
        setStatus(lastStatus);
    }
    break;

    default:
        break;
    }
}

void ModifyBevelTool::updateHints()
{
    switch (status())
    {
    case SetEntity1:
        GUIDIALOGFACTORY->updateMouseWidget(ModifyBevelCommand::tr("Specify first entity"), ModifyBevelCommand::tr("Back"));
        break;

    case SetEntity2:
        GUIDIALOGFACTORY->updateMouseWidget(ModifyBevelCommand::tr("Specify second entity"), ModifyBevelCommand::tr("Back"));
        break;

    case SetLength1:
        GUIDIALOGFACTORY->updateMouseWidget(ModifyBevelCommand::tr("Enter length 1:"), ModifyBevelCommand::tr("Cancel"));
        break;

    case SetLength2:
        GUIDIALOGFACTORY->updateMouseWidget(ModifyBevelCommand::tr("Enter length 2:"), ModifyBevelCommand::tr("Cancel"));
        break;

    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}
QStringList ModifyBevelTool::availableCommands() const
{
    QStringList cmd;
    switch (status())
    {
    case SetEntity1:
    case SetEntity2:
        cmd += Commands::command("length1");
        cmd += Commands::command("length2");
        cmd += Commands::command("trim");
        break;

    default:
        break;
    }
    return cmd;
}

std::unique_ptr<BasePlaceTool> ModifyBevelCommand::createTool()
{
    return std::make_unique<ModifyBevelTool>(*this, document(), view());
}

void ModifyBevelCommand::showOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, true);
}

void ModifyBevelCommand::hideOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, false);
}

namespace
{
const bool g_registered = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionModifyBevel, QStringLiteral("modify.bevel"), exclusiveCommandFactory<ModifyBevelCommand>());
}  // namespace
