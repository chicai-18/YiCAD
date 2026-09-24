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

/// @file DimRadialTool.cpp
/// @brief DimRadialCommand 与工具（从原 ActionDimRadial 机械改写）

#include <memory>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QStringList>
#include "DmArc.h"
#include "DmCircle.h"
#include "DmDimRadial.h"
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
/// @brief 半径标注工具：选圆或圆弧，再指定标注线位置（命令行可输入角度）；可连续标注
class DimRadialTool : public DimensionTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetEntity, ///< 选择实体
        SetPos,    ///< 选择位置
        SetText    ///< 在命令行中设置文本标签
    };

    DimRadialTool(DimRadialCommand& command, DmDocument* doc, IDocumentView* view)
        : DimensionTool(command, doc, view)
        , m_command(command)
    {
        edata = std::make_unique<DmDimRadialData>(DmVector{}, 0.0);
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

    DimRadialCommand& m_command;
    DmEntity* entity = nullptr;                          ///< 已选中的实体（圆弧/圆）
    std::unique_ptr<DmVector> pos = std::make_unique<DmVector>(); ///< 拾取圆后鼠标移动时的位置
    std::unique_ptr<DmDimRadialData> edata;              ///< 新标注数据
    Status lastStatus = SetEntity;                       ///< 进入文字输入前的状态
};
}  // namespace

void DimRadialTool::reset()
{
    resetDimension();

    edata.reset(new DmDimRadialData{{}, 0.0});
    entity     = nullptr;
    *pos       = {};
    lastStatus = SetEntity;
    //GUIDIALOGFACTORY->requestCommandOptions(&m_command, true, true);
}

void DimRadialTool::trigger()
{
    m_command.preview().clear();

    preparePreview();
    if (entity)
    {
        DmDimRadial* newEntity =
                    new DmDimRadial(nullptr, *data, *edata);
        newEntity->setDocument(document());
        newEntity->update();
        Transaction t(DimRadialCommand::tr("Add dimension radial").toStdString(), document());
        t.start();
        document()->getEntityTable()->add(newEntity);
        t.commit();

        DmVector rz = view()->getRelativeZero();
        view()->moveRelativeZero(rz);
        snapper()->finish();
    }
}

void DimRadialTool::preparePreview()
{
    if (entity)
    {
        double dist = data->definitionPoint.distanceTo(*pos);   //圆心到鼠标距离
        double angle  = data->definitionPoint.angleTo(*pos);
        double radius = 0.0;
        if (entity->getEntityType() == DM::EntityArc)
        {
            radius = ((DmArc*)entity)->getRadius();
        }
        else if (entity->getEntityType() == DM::EntityCircle)
        {
            radius = ((DmCircle*)entity)->getRadius();
        }

        edata->endPoint.setPolar(radius, angle);
        edata->endPoint += data->definitionPoint;
        edata->leader =  dist - radius;
        //edata->isInside = dist < radius;
    }
}

void DimRadialTool::onMouseMove(QMouseEvent* e)
{
    switch (status())
    {
        case SetPos:
            if (entity)
            {
                *pos = snapper()->snapPoint(e);

                preparePreview();

                DmDimRadial* d = new DmDimRadial(
                            m_command.preview().entities().getEntityContainer(), *data, *edata);
                d->setDocument(document());
                d->update();

                m_command.preview().clear();
                m_command.preview().entities().addEntity(d);
                d->update();
                m_command.preview().draw();
            }
            break;

        default:
            break;
    }
}

void DimRadialTool::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        switch (status())
        {
            case SetEntity: {
                DmEntity* en = snapper()->catchEntity(e, DM::ResolveAll);
                if (en)
                {
                    if (en->getEntityType() == DM::EntityArc || en->getEntityType() == DM::EntityCircle)
                    {
                        entity = en;
                        if (entity->getEntityType() == DM::EntityArc)
                        {
                            data->definitionPoint = static_cast<DmArc*>(entity)->getCenter();
                        }
                        else if (entity->getEntityType() == DM::EntityCircle)
                        {
                            data->definitionPoint = static_cast<DmCircle*>(entity)->getCenter();
                        }
                        view()->moveRelativeZero(data->definitionPoint);
                        setStatus(SetPos);
                    }
                    else
                    {
                        GUIDIALOGFACTORY->commandMessage(
                                    DimRadialCommand::tr("Not a circle or arc entity"));
                    }
                }
            }
            break;

            case SetPos: {
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

void DimRadialTool::onCoordinate(const DmVector& coord)
{
    switch (status())
    {
        case SetPos:
            *pos = coord;
            trigger();
            reset();
            setStatus(SetEntity);
            break;

        default:
            break;
    }
}

void DimRadialTool::onCommand(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (Commands::checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(Commands::msgAvailableCommands() + availableCommands().join(", "));
        return;
    }

    // setting new text label:
    if (status() == SetText)
    {
        setText(c);
        //GUIDIALOGFACTORY->requestCommandOptions(&m_command, true, true);
        view()->enableCoordinateInput();
        setStatus(lastStatus);
        return;
    }

    // command: text
    if (Commands::checkCommand("text", c))
    {
        lastStatus = (Status)status();
        view()->disableCoordinateInput();
        setStatus(SetText);
    }

    // setting angle
    if (status() == SetPos)
    {
        bool ok;
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
            GUIDIALOGFACTORY->commandMessage(DimRadialCommand::tr("Not a valid expression"));
        }
        return;
    }
}

QStringList DimRadialTool::availableCommands() const
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

void DimRadialTool::updateHints()
{
    switch (status())
    {
        case SetEntity:
            GUIDIALOGFACTORY->updateMouseWidget(DimRadialCommand::tr("Select arc or circle entity"), DimRadialCommand::tr("Cancel"));
            break;
        case SetPos:
            GUIDIALOGFACTORY->updateMouseWidget(DimRadialCommand::tr("Specify dimension line position or enter angle:"), DimRadialCommand::tr("Cancel"));
            break;
        case SetText:
            GUIDIALOGFACTORY->updateMouseWidget(DimRadialCommand::tr("Enter dimension text:"), "");
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

std::unique_ptr<BasePlaceTool> DimRadialCommand::createTool()
{
    return std::make_unique<DimRadialTool>(*this, document(), view());
}
