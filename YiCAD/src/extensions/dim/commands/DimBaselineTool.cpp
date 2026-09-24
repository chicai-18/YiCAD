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

/// @file DimBaselineTool.cpp
/// @brief DimBaselineCommand 与工具（从原 ActionDimBaseline 机械改写）

#include <memory>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QStringList>
#include "DmDimAligned.h"
#include "DmDimAngular.h"
#include "DmDimLinear.h"
#include "DmDimension.h"
#include "GeometryMethods.h"
#include "Tools.h"

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
/// @brief 基线标注工具：选一个对齐或线性标注，再逐个指定新标注的位置；右键结束
class DimBaselineTool : public DimensionTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SelectDim, ///< 选择原标注
        SetPos     ///< 设置定位点
    };

    DimBaselineTool(DimBaselineCommand& command, DmDocument* doc, IDocumentView* view)
        : DimensionTool(command, doc, view)
        , m_command(command)
    {
    }

protected:
    void updateHints() override;
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;
    void onCoordinate(const DmVector& coord) override;

private:
    void reset();
    void trigger();
    DmDimLinear* createDimForAligned(DmVector pos);
    DmDimLinear* createDimForLinear(DmVector pos);
    DmDimAngular* createDimForAngular(DmVector pos);

    DimBaselineCommand& m_command;
    DmDimension* selectedDim = nullptr; ///< 选择的原标注
    DmEntity* newEnt = nullptr;         ///< 新添加的标注
    int addNum = 1;                     ///< 第几次添加
};
}  // namespace

void DimBaselineTool::reset()
{
    selectedDim = nullptr;
    newEnt = nullptr;
    addNum = 1;
}

/// @brief 触发创建基线标注
void DimBaselineTool::trigger()
{
    m_command.preview().clear();

    if (!newEnt)
    {
        return;
    }

    Transaction t(DimBaselineCommand::tr("Add dimension baseline").toStdString(), document());
    t.start();
    document()->getEntityTable()->add(newEnt);
    t.commit();

    DmVector rz = view()->getRelativeZero();
    addNum++;
}

/// @brief 鼠标移动事件处理
void DimBaselineTool::onMouseMove(QMouseEvent* e)
{
    switch (status())
    {
    case SelectDim:
    {
        snapper()->deleteSnapper();
    }
    break;

    case SetPos:
    {
        m_command.preview().clear();
        DmVector pos = snapper()->snapPoint(e);

        DmDimAligned* dimAligned = dynamic_cast<DmDimAligned*>(selectedDim);

        if (dimAligned)
        {
            DmDimLinear* dim = createDimForAligned(pos);
            dim->setParent(m_command.preview().entities().getEntityContainer());
            m_command.preview().entities().addEntity(dim);
        }

        DmDimLinear* dimLinear = dynamic_cast<DmDimLinear*>(selectedDim);

        if (dimLinear)
        {
            DmDimLinear* dim = createDimForLinear(pos);
            dim->setParent(m_command.preview().entities().getEntityContainer());
            m_command.preview().entities().addEntity(dim);
        }

        m_command.preview().draw();
    }
    break;

    default:
        break;
    }
}

/// @brief 鼠标释放事件处理
void DimBaselineTool::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        switch (status())
        {
        case SelectDim:
        {
            DmEntity* catchEnt = snapper()->catchEntity(e);

            if (catchEnt == nullptr)
            {
                GUIDIALOGFACTORY->commandMessage(DimBaselineCommand::tr("No Entity found."));
            }
            else
            {
                DM::EntityType type = catchEnt->getEntityType();

                if (type != DM::EntityDimAligned
                    && type != DM::EntityDimAngular
                    && type != DM::EntityDimLinear)
                {
                    GUIDIALOGFACTORY->commandMessage(
                        DimBaselineCommand::tr("Entity must be a aligned dimension, angular dimension or linear dimension."));
                }
                else
                {
                    selectedDim = static_cast<DmDimension*>(catchEnt);
                    setStatus(SetPos);
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
        command().finish();
    }
}

/// @brief 坐标事件处理
void DimBaselineTool::onCoordinate(const DmVector& coord)
{
    DmVector pos = coord;

    if (status() == SetPos)
    {
        newEnt = nullptr;

        DmDimAligned* dimAligned = dynamic_cast<DmDimAligned*>(selectedDim);

        if (dimAligned)
        {
            newEnt = createDimForAligned(pos);
        }

        DmDimLinear* dimLinear = dynamic_cast<DmDimLinear*>(selectedDim);

        if (dimLinear)
        {
            newEnt = createDimForLinear(pos);
        }

        trigger();
    }
}

/// @brief 更新鼠标按钮提示
void DimBaselineTool::updateHints()
{
    switch (status())
    {
    case SelectDim:
        GUIDIALOGFACTORY->updateMouseWidget(
            DimBaselineCommand::tr("Specify origin dimension"), DimBaselineCommand::tr("Cancel"));
        break;

    case SetPos:
        GUIDIALOGFACTORY->updateMouseWidget(
            DimBaselineCommand::tr("Specify the point to define dimension"), DimBaselineCommand::tr("Cancel"));
        break;

    default:
        break;
    }
}

/// @brief 为选择的对齐标注创建基线标注
DmDimLinear* DimBaselineTool::createDimForAligned(DmVector pos)
{
    constexpr double DISTANCE_MULTIPLIER = 2.0;
    constexpr double DIR_LINE_LENGTH = 100.0;

    DmDimAligned* dimAligned = dynamic_cast<DmDimAligned*>(selectedDim);

    if (dimAligned == nullptr)
    {
        return nullptr;
    }

    DmDimensionData data = dimAligned->getData();
    DmDimAlignedData edata = dimAligned->getEData();
    DmVector textCenter = data.textCenter;
    DmVector textFoot = GeometryMethods::getPerpendicularFoot(
        data.definitionPoint,
        data.definitionPoint + (edata.extensionPoint1 - edata.extensionPoint2),
        textCenter);

    double offsetDist = textCenter.distanceTo(textFoot) * DISTANCE_MULTIPLIER;
    offsetDist *= addNum;

    DmVector textDir = (edata.extensionPoint1 - edata.extensionPoint2).normalize();
    DmVector textVDir = DmVector(textDir).rotate(M_PI_2);
    DmVector offsetDir(true);

    if (textVDir.dotP(pos - textFoot) < 0)
    {
        offsetDir = textVDir;
    }
    else
    {
        offsetDir = -textVDir;
    }

    // 设置新标注信息
    DmVector definitionFoot = GeometryMethods::getPerpendicularFoot(
        pos, pos + offsetDir, data.definitionPoint);
    data.definitionPoint = definitionFoot + offsetDir * offsetDist;

    DmDimLinearData edata_new;
    edata_new.extensionPoint1 = edata.extensionPoint1;
    edata_new.extensionPoint2 = pos;
    data.angle = textDir.angle();

    DmDimLinear* dim = new DmDimLinear(nullptr, data, edata_new);
    dim->setDocument(document());
    dim->update();

    return dim;
}

/// @brief 为选择的线性标注创建基线标注
DmDimLinear* DimBaselineTool::createDimForLinear(DmVector pos)
{
    constexpr double DISTANCE_MULTIPLIER = 2.0;
    constexpr double DIR_LINE_LENGTH = 100.0;

    DmDimLinear* dimLinear = dynamic_cast<DmDimLinear*>(selectedDim);

    if (dimLinear == nullptr)
    {
        return nullptr;
    }

    DmDimensionData data = dimLinear->getData();
    DmDimLinearData edata = dimLinear->getEData();
    DmVector textCenter = data.textCenter;
    DmVector textFoot = GeometryMethods::getPerpendicularFoot(
        data.definitionPoint,
        data.definitionPoint + DmVector(data.angle) * DIR_LINE_LENGTH,
        textCenter);

    double offsetDist = textCenter.distanceTo(textFoot) * DISTANCE_MULTIPLIER;
    offsetDist *= addNum;

    DmVector textDir(data.angle);
    DmVector textVDir = DmVector(textDir).rotate(M_PI_2);
    DmVector offsetDir(true);

    if (textVDir.dotP(pos - textFoot) < 0)
    {
        offsetDir = textVDir;
    }
    else
    {
        offsetDir = -textVDir;
    }

    // 设置新标注信息
    DmVector definitionFoot = GeometryMethods::getPerpendicularFoot(
        pos, pos + offsetDir, data.definitionPoint);
    data.definitionPoint = definitionFoot + offsetDir * offsetDist;

    DmDimLinearData edata_new;
    edata_new.extensionPoint1 = edata.extensionPoint1;
    edata_new.extensionPoint2 = pos;
    data.angle = textDir.angle();

    DmDimLinear* dim = new DmDimLinear(nullptr, data, edata_new);
    dim->setDocument(document());
    dim->update();

    return dim;
}

/// @brief 为选择的角度标注创建基线标注（暂未实现）
DmDimAngular* DimBaselineTool::createDimForAngular(DmVector pos)
{
    DmDimAngular* dimAngular = dynamic_cast<DmDimAngular*>(selectedDim);

    if (dimAngular == nullptr)
    {
        return nullptr;
    }

    // TODO : 角度标注对应的基线标注是"三点角度标注"，是一种新标注类型，暂不处理

    return nullptr;
}

std::unique_ptr<BasePlaceTool> DimBaselineCommand::createTool()
{
    return std::make_unique<DimBaselineTool>(*this, document(), view());
}
