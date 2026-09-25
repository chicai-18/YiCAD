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

/// @file ModifyRoundCommand.cpp
/// @brief ModifyRoundCommand 与圆角工具的实现；工具的事件处理从原 ActionModifyRound 机械改写而来

#include "ModifyRoundCommand.h"

#include <memory>

#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "ModifyCommands.h"
#include "Commands.h"
#include "ArcData.h"
#include "DmArc.h"
#include "DmAtomicEntity.h"
#include "DmDocument.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Information.h"
#include "Math2d.h"
#include "Modification.h"
#include "Transaction.h"

namespace
{
constexpr double ROUND_ENDPOINT_TOLERANCE = 1e-10; ///< 判断点是否在实体端点的距离阈值

/// @brief 圆角工具：选第一个实体，再选第二个实体后加圆角；可连续操作
class ModifyRoundTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetEntity1, ///< 选择第一个实体
        SetEntity2, ///< 选择第二个实体
        SetRadius,  ///< 在命令行中设置半径
        SetTrim     ///< 在命令行中设置裁剪标志（原 Action 进入后没有处理，见 onCommand）
    };

    ModifyRoundTool(ModifyRoundCommand& command, DmDocument* doc, IDocumentView* view)
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
    /// @brief 圆角计算结果
    struct FilletResult
    {
        bool valid = false;      ///< 计算是否成功
        DmVector center;         ///< 圆角圆心
        DmVector tangent1;       ///< entity1 上的切点
        DmVector tangent2;       ///< entity2 上的切点
        double startAngle = 0.0; ///< 圆弧起始角度
        double endAngle = 0.0;   ///< 圆弧终止角度
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

    FilletResult computeFillet(const DmVector& ref1, DmAtomicEntity* e1, const DmVector& ref2, DmAtomicEntity* e2,
                               double radius) const;
    void trigger();
    DmVector setmousePoint(const DmVector& m_p, DmEntity* e);
    QStringList availableCommands() const;
    void unhighlightEntity();

    ModifyRoundCommand& m_command;
    DmEntity* entity1 = nullptr;         ///< 第一个选中实体
    DmEntity* entity2 = nullptr;         ///< 第二个选中实体
    Points m_points;                     ///< 两个实体上的点击位置
    Status lastStatus = SetEntity1;      ///< 进入半径设置前的上一个状态
    bool isEndPt = false;                ///< 鼠标是否在实体端点附近
    DmEntity* prevHighlighted = nullptr; ///< 上次高亮的实体
};
}  // namespace

void ModifyRoundTool::unhighlightEntity()
{
    if (prevHighlighted)
    {
        prevHighlighted->setHighlighted(false);
        view()->specifyDocumentModified();
        view()->redraw();
        prevHighlighted = nullptr;
    }
}

ModifyRoundTool::FilletResult ModifyRoundTool::computeFillet(
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

void ModifyRoundTool::trigger()
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

    double r = m_command.radius();
    bool trim = m_command.isTrimOn();

    FilletResult fr = computeFillet(m_points.coord2, pe1, m_points.coord1, pe2, r);
    if (!fr.valid)
        return;

    // 查找原始实体交点，用于确定裁剪方向
    DmVectorSolutions sol2 = Information::getIntersection(pe1, pe2, false);

    Transaction t(ModifyRoundCommand::tr("Round").toStdString(), document());
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
        DM::Ending ending1 = trimmed1->getTrimPoint(m_points.coord1, is2);
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
        is2 = sol2.getClosest(m_points.coord1);
        DM::Ending ending2 = trimmed2->getTrimPoint(m_points.coord2, is2);
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
    m_points.coord1 = DmVector(false);
    entity1 = nullptr;
    m_points.coord2 = DmVector(false);
    entity2 = nullptr;
    setStatus(SetEntity1);

    GUIDIALOGFACTORY->updateSelectionWidget(document()->getEntityTable()->countSelect());
}

DmVector ModifyRoundTool::setmousePoint(const DmVector& m_p, DmEntity* e)
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

void ModifyRoundTool::onMouseMove(QMouseEvent* e)
{
    DmVector mouse = view()->toGraph(e->pos().x(), e->pos().y());
    DmEntity* se = snapper()->catchEntity(e, { DM::EntityLine, DM::EntityArc, DM::EntityCircle, DM::EntityEllipse,  DM::EntitySpline }, DM::ResolveAllButTextImage);
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
            if (entity1 && entity2 && entity2 != entity1 && !entity2->isContainer() && !isEndPt && Modification::isCutableEntity(entity1) && Modification::isCutableEntity(entity2))
            {
                auto pe1 = dynamic_cast<DmAtomicEntity*>(entity1);
                auto pe2 = dynamic_cast<DmAtomicEntity*>(entity2);
                if (pe1 && pe2)
                {
                    FilletResult fr = computeFillet(m_points.coord2, pe1, m_points.coord1, pe2, m_command.radius());
                    if (fr.valid)
                    {
                        // 预览圆角弧
                        DmArc* previewArc = new DmArc(nullptr, ArcData(fr.center, DmVector(0.0, 0.0, 1.0), m_command.radius(), fr.startAngle, fr.endAngle));
                        m_command.preview().entities().addEntity(previewArc);

                        if (m_command.isTrimOn())
                        {
                            DmAtomicEntity* t1 = static_cast<DmAtomicEntity*>(pe1->clone());
                            DmAtomicEntity* t2 = static_cast<DmAtomicEntity*>(pe2->clone());
                            t1->setParent(nullptr);
                            t2->setParent(nullptr);

                            DmVectorSolutions sol2 = Information::getIntersection(pe1, pe2, false);

                            DmVector is2 = sol2.getClosest(m_points.coord2);
                            DM::Ending e1 = t1->getTrimPoint(m_points.coord1, is2);
                            if (e1 == DM::EndingStart)
                                t1->trimStartpoint(fr.tangent1);
                            else if (e1 == DM::EndingEnd)
                                t1->trimEndpoint(fr.tangent1);

                            is2 = sol2.getClosest(m_points.coord1);
                            DM::Ending e2 = t2->getTrimPoint(m_points.coord2, is2);
                            if (e2 == DM::EndingStart)
                                t2->trimStartpoint(fr.tangent2);
                            else if (e2 == DM::EndingEnd)
                                t2->trimEndpoint(fr.tangent2);

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

void ModifyRoundTool::onMouseRelease(QMouseEvent* e)
{
    DmVector mouse = view()->toGraph(e->pos().x(), e->pos().y());
    DmEntity* se = snapper()->catchEntity(e, { DM::EntityLine, DM::EntityArc, DM::EntityCircle, DM::EntityEllipse,  DM::EntitySpline }, DM::ResolveAll);
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

void ModifyRoundTool::onCommand(GuiCommandEvent* e)
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
            if (Commands::checkCommand("radius", c))
            {
                e->accept();
                m_command.preview().clear();
                lastStatus = (Status)status();
                setStatus(SetRadius);
            }
            else if (Commands::checkCommand("trim", c))
            {
                // 到不了这里：Commands::checkCommand 对 help/close/undo 以外的关键字都返回 true，
                // 上面的 "radius" 接住了所有文本。保留原 Action 的写法：切到 SetTrim 后没有任何处理，
                // 只能右键退回
                e->accept();
                m_command.preview().clear();
                lastStatus = (Status)status();
                setStatus(SetTrim);
                m_command.setTrim(!m_command.isTrimOn());
                GUIDIALOGFACTORY->requestCommandOptions(&m_command, true, true);
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
                m_command.setRadius(r);
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(ModifyRoundCommand::tr("Not a valid expression"));
            }
            GUIDIALOGFACTORY->requestCommandOptions(&m_command, true, true);
            setStatus(lastStatus);
        }
            break;
        default:
            break;
    }
}

void ModifyRoundTool::updateHints()
{
    switch (status())
    {
        case SetEntity1:
            GUIDIALOGFACTORY->updateMouseWidget(ModifyRoundCommand::tr("Specify first entity"), ModifyRoundCommand::tr("Back"));
            break;
        case SetEntity2:
            GUIDIALOGFACTORY->updateMouseWidget(ModifyRoundCommand::tr("Specify second entity"), ModifyRoundCommand::tr("Back"));
            break;
        case SetRadius:
            GUIDIALOGFACTORY->updateMouseWidget(ModifyRoundCommand::tr("Enter radius:"), ModifyRoundCommand::tr("Cancel"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}
QStringList ModifyRoundTool::availableCommands() const
{
    QStringList cmd;
    switch (status())
    {
    case SetEntity1:
    case SetEntity2:
        cmd += Commands::command("radius");
        cmd += Commands::command("trim");
        break;

    default:
        break;
    }
    return cmd;
}

std::unique_ptr<BasePlaceTool> ModifyRoundCommand::createTool()
{
    return std::make_unique<ModifyRoundTool>(*this, document(), view());
}

void ModifyRoundCommand::showOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, true);
}

void ModifyRoundCommand::hideOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, false);
}

ExclusiveCommandFactory ModifyCommands::round()
{
    return exclusiveCommandFactory<ModifyRoundCommand>();
}
