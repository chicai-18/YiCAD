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

/// @file ModifyMoveCommand.cpp
/// @brief 移动命令与移动工具的实现

#include "ModifyMoveCommand.h"

#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "ModifyCommands.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Modification.h"
#include "Selection.h"

namespace
{
/// @brief 移动操作步进角度（用于 Shift 约束）
constexpr double MOVE_SNAP_ANGLE = 15.0;

/// @brief 移动工具：指定参考点，再指定目标点
class ModifyMoveTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetReferencePoint, ///< 设置参考点
        SetTargetPoint,    ///< 设置目标点
    };

    ModifyMoveTool(ModifyMoveCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetReferencePoint:
            GUIDIALOGFACTORY->updateMouseWidget(ModifyMoveCommand::tr("Specify reference point"),
                                                ModifyMoveCommand::tr("Cancel"));
            break;
        case SetTargetPoint:
            GUIDIALOGFACTORY->updateMouseWidget(ModifyMoveCommand::tr("Specify target point"),
                                                ModifyMoveCommand::tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        DmVector mouse = snapper()->snapPoint(e);
        switch (status())
        {
        case SetReferencePoint:
            m_referencePoint = mouse;
            break;

        case SetTargetPoint:
            if (m_referencePoint.valid)
            {
                const bool shift = e->modifiers() & Qt::ShiftModifier;
                if (shift)
                {
                    mouse = snapper()->snapToAngle(mouse, m_referencePoint, MOVE_SNAP_ANGLE);
                }
                m_targetPoint = mouse;
                m_command.previewMove(m_referencePoint, m_targetPoint, shift);
            }
            break;

        default:
            break;
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
        {
            DmVector snapped = snapper()->snapPoint(e);
            if ((e->modifiers() & Qt::ShiftModifier) && status() == SetTargetPoint)
            {
                snapped = snapper()->snapToAngle(snapped, m_referencePoint, MOVE_SNAP_ANGLE);
            }
            onCoordinate(snapped);
        }
        else if (e->button() == Qt::RightButton)
        {
            m_command.clearPreview();
            stepBack();
        }
    }

    void onCoordinate(const DmVector& pos) override
    {
        switch (status())
        {
        case SetReferencePoint:
            m_referencePoint = pos;
            view()->moveRelativeZero(m_referencePoint);
            setStatus(SetTargetPoint);
            break;

        case SetTargetPoint:
            m_targetPoint = pos;
            view()->moveRelativeZero(m_targetPoint);
            m_command.commitMove(m_referencePoint, m_targetPoint);
            break;

        default:
            break;
        }
    }

private:
    ModifyMoveCommand& m_command;
    DmVector m_referencePoint;
    DmVector m_targetPoint;
};
}  // namespace

ModifyMoveCommand::ModifyMoveCommand() = default;

ModifyMoveCommand::~ModifyMoveCommand() = default;

bool ModifyMoveCommand::onSelectionReady()
{
    m_preview = std::make_unique<CommandPreview>(document(), view());
    auto tool = std::make_unique<ModifyMoveTool>(*this, document(), view());
    tool->setPreview(m_preview.get());
    activateTool(std::move(tool));
    return true;
}

void ModifyMoveCommand::previewMove(const DmVector& reference, const DmVector& target, bool showGuide)
{
    m_preview->clear();
    m_preview->entities().addSelectionFromDocument();
    m_preview->entities().move(target - reference);

    if (showGuide)
    {
        DmLine* line = new DmLine(nullptr, reference, target);
        m_preview->entities().addEntity(line);
        line->setSelected(true);
        line->setLayerToActive();
        line->setPenToActive();
    }
    m_preview->draw();
}

void ModifyMoveCommand::clearPreview()
{
    m_preview->clear();
}

void ModifyMoveCommand::commitMove(const DmVector& reference, const DmVector& target)
{
    std::vector<DmEntity*> ents;
    for (auto e : *document()->getEntityTable())
    {
        if (e->isSelected())
        {
            ents.push_back(e);
        }
    }
    Modification m(document());
    m.move(ents, target - reference);
    // 移动之后取消选中
    Selection(document()).selectAll(false);

    GUIDIALOGFACTORY->updateSelectionWidget(document()->getEntityTable()->countSelect());
    finish();
}

ExclusiveCommandFactory ModifyCommands::move()
{
    return exclusiveCommandFactory<ModifyMoveCommand>();
}
