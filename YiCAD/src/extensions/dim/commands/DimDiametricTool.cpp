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

/// @file DimDiametricTool.cpp
/// @brief DimDiametricCommand 与工具（从原 ActionDimDiametric 机械改写）

#include <memory>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QStringList>
#include <cmath>
#include "DmArc.h"
#include "DmCircle.h"
#include "DmDimDiametric.h"
#include "DmLine.h"
#include "Math2d.h"

#include "CommandPreview.h"
#include "Commands.h"
#include "DimCommands.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Preview.h"
#include "Transaction.h"

namespace
{
/// @brief 直径标注工具：选圆或圆弧，再指定标注线位置（命令行可输入角度）；可连续标注
class DimDiametricTool : public DimensionTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetEntity, ///< 选择实体（圆/圆弧）
        SetPos,    ///< 选择标注线位置
        SetText    ///< 在命令行中设置文本标签
    };

    DimDiametricTool(DimDiametricCommand& command, DmDocument* doc, IDocumentView* view)
        : DimensionTool(command, doc, view)
        , m_command(command)
    {
        edata = std::make_unique<DmDimDiametricData>(DmVector{false}, 0.0);
    }

protected:
    void updateHints() override;
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;
    void onCoordinate(const DmVector& coord) override;
    void onCommand(GuiCommandEvent* e) override;

private:
    void reset();
    void trigger();
    void preparePreview();
    QStringList availableCommands() const;

    DimDiametricCommand& m_command;
    DmEntity* entity = nullptr;                          ///< 选择的实体（圆/圆弧）
    std::unique_ptr<DmVector> pos = std::make_unique<DmVector>(); ///< 拾取圆后鼠标移动时的位置
    std::unique_ptr<DmDimDiametricData> edata;           ///< 直径标注数据
    Status lastStatus = SetEntity;                       ///< 进入文本输入前的上一状态
};
}  // namespace

void DimDiametricTool::reset()
{
    resetDimension();

    edata.reset(new DmDimDiametricData(DmVector{false}, 0.0));
    entity = nullptr;
    *pos = {};
}

/// @brief 触发创建直径标注
void DimDiametricTool::trigger()
{
    m_command.preview().clear();

    preparePreview();

    if (entity)
    {
        DmDimDiametric* newEntity = new DmDimDiametric(nullptr, *data, *edata);
        newEntity->setDocument(document());
        newEntity->update();

        Transaction t(DimDiametricCommand::tr("Add dimension diametric").toStdString(), document());
        t.start();
        document()->getEntityTable()->add(newEntity);
        t.commit();

        DmVector rz = view()->getRelativeZero();
        view()->moveRelativeZero(rz);
        snapper()->finish();
    }
}

/// @brief 准备预览图形，根据选择的圆/圆弧计算标注位置
void DimDiametricTool::preparePreview()
{
    if (entity)
    {
        double radius{0.};
        DmVector center{false};

        if (entity->getEntityType() == DM::EntityArc)
        {
            DmArc* p = static_cast<DmArc*>(entity);
            radius = p->getRadius();
            center = p->getCenter();
        }
        else if (entity->getEntityType() == DM::EntityCircle)
        {
            DmCircle* p = static_cast<DmCircle*>(entity);
            radius = p->getRadius();
            center = p->getCenter();
        }

        double angle = center.angleTo(*pos);

        data->definitionPoint.setPolar(radius, angle + M_PI);
        data->definitionPoint += center;

        edata->endPoint.setPolar(radius, angle);
        edata->endPoint += center;

        // 圆心到鼠标距离减去半径 = 引线长度
        double dist = center.distanceTo(*pos);
        edata->leader = dist - radius;
    }
}

/// @brief 鼠标移动事件处理
void DimDiametricTool::onMouseMove(QMouseEvent* e)
{
    switch (status())
    {
    case SetPos:
    {
        if (entity)
        {
            *pos = snapper()->snapPoint(e);

            preparePreview();

            DmDimDiametric* d =
                new DmDimDiametric(m_command.preview().entities().getEntityContainer(), *data, *edata);
            d->setDocument(document());
            d->update();

            m_command.preview().clear();
            m_command.preview().entities().addEntity(d);
            m_command.preview().draw();
        }
    }
    break;

    default:
        break;
    }
}

/// @brief 鼠标释放事件处理
void DimDiametricTool::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        switch (status())
        {
        case SetEntity:
        {
            DmEntity* en = snapper()->catchEntity(e, DM::ResolveAll);

            if (en)
            {
                if (en->getEntityType() == DM::EntityArc
                    || en->getEntityType() == DM::EntityCircle)
                {
                    entity = en;

                    DmVector center;

                    if (entity->getEntityType() == DM::EntityArc)
                    {
                        center = static_cast<DmArc*>(entity)->getCenter();
                    }
                    else
                    {
                        center = static_cast<DmCircle*>(entity)->getCenter();
                    }

                    view()->moveRelativeZero(center);
                    setStatus(SetPos);
                }
                else
                {
                    GUIDIALOGFACTORY->commandMessage(
                        DimDiametricCommand::tr("Not a circle or arc entity"));
                }
            }
        }
        break;

        case SetPos:
        {
            onCoordinate(snapper()->snapPoint(e));
        }
        break;

        default:
            break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        m_command.preview().clear();
        init(status() - 1);
    }
}

/// @brief 坐标事件处理
void DimDiametricTool::onCoordinate(const DmVector& coord)
{
    switch (status())
    {
    case SetPos:
    {
        *pos = coord;
        trigger();
        finishIfOrthogonal();
        reset();
        setStatus(SetEntity);
    }
    break;

    default:
        break;
    }
}

/// @brief 命令事件处理
void DimDiametricTool::onCommand(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (Commands::checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(
            Commands::msgAvailableCommands() + availableCommands().join(", "));
        return;
    }

    // 设置新的文本标签
    if (status() == SetText)
    {
        setText(c);
        view()->enableCoordinateInput();
        setStatus(lastStatus);
        return;
    }

    // 命令: text
    if (Commands::checkCommand("text", c))
    {
        lastStatus = static_cast<Status>(status());
        view()->disableCoordinateInput();
        setStatus(SetText);
    }

    // 设置角度
    if (status() == SetPos)
    {
        bool ok = false;
        double a = Math2d::eval(c, &ok);

        if (ok)
        {
            pos->setPolar(1.0, Math2d::deg2rad(a));
            *pos += data->definitionPoint;
            trigger();
            reset();
            setStatus(SetEntity);
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(DimDiametricCommand::tr("Not a valid expression"));
        }

        return;
    }
}

/// @brief 获取可用命令列表
QStringList DimDiametricTool::availableCommands() const
{
    QStringList cmd;

    switch (status())
    {
    case SetEntity:
    case SetPos:
        cmd += Commands::command("text");
        break;

    default:
        break;
    }

    return cmd;
}

/// @brief 更新鼠标按钮提示
void DimDiametricTool::updateHints()
{
    switch (status())
    {
    case SetEntity:
        GUIDIALOGFACTORY->updateMouseWidget(
            DimDiametricCommand::tr("Select arc or circle entity"), DimDiametricCommand::tr("Cancel"));
        break;

    case SetPos:
        GUIDIALOGFACTORY->updateMouseWidget(
            DimDiametricCommand::tr("Specify dimension line location"), DimDiametricCommand::tr("Cancel"));
        break;

    case SetText:
        GUIDIALOGFACTORY->updateMouseWidget(
            DimDiametricCommand::tr("Enter dimension text:"), "");
        break;

    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

std::unique_ptr<BasePlaceTool> DimDiametricCommand::createTool()
{
    return std::make_unique<DimDiametricTool>(*this, document(), view());
}
