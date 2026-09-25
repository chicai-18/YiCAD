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

/// @file ModifyMirrorCommand.cpp
/// @brief 镜像命令与镜像工具的实现

#include "ModifyMirrorCommand.h"

#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "ModifyCommands.h"
#include "DmBlockReference.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "DmSettings.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Transaction.h"

namespace
{
/// @brief 镜像操作步进角度（用于 Shift 约束）
constexpr double MIRROR_SNAP_ANGLE = 15.0;

/// @brief 镜像工具：指定镜像线第一点，再指定第二点；随时可输入 Y/N
class ModifyMirrorTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetAxisPoint1, ///< 设置镜像轴第一点
        SetAxisPoint2, ///< 设置镜像轴第二点
    };

    ModifyMirrorTool(ModifyMirrorCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

protected:
    void updateHints() override
    {
        const QString copyStr = m_command.copies() ? ModifyMirrorCommand::tr("copy")
                                                   : ModifyMirrorCommand::tr("delete origin");
        switch (status())
        {
        case SetAxisPoint1:
            GUIDIALOGFACTORY->updateMouseWidget(
                ModifyMirrorCommand::tr("Specify first point of mirror line, or type Y to copy, type N to delete origin, the default is [%1]")
                    .arg(copyStr),
                ModifyMirrorCommand::tr("Cancel"));
            break;
        case SetAxisPoint2:
            GUIDIALOGFACTORY->updateMouseWidget(
                ModifyMirrorCommand::tr("Specify second point of mirror line, or type Y to copy, type N to delete origin, the default is [%1]")
                    .arg(copyStr),
                ModifyMirrorCommand::tr("Back"));
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
        case SetAxisPoint1:
            m_axisPoint1 = mouse;
            break;

        case SetAxisPoint2:
            if (m_axisPoint1.valid)
            {
                if (e->modifiers() & Qt::ShiftModifier)
                {
                    mouse = snapper()->snapToAngle(mouse, m_axisPoint1, MIRROR_SNAP_ANGLE);
                }
                m_axisPoint2 = mouse;
                m_command.previewMirror(m_axisPoint1, m_axisPoint2);
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
            if ((e->modifiers() & Qt::ShiftModifier) && status() == SetAxisPoint2)
            {
                snapped = snapper()->snapToAngle(snapped, m_axisPoint1, MIRROR_SNAP_ANGLE);
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
        case SetAxisPoint1:
            m_axisPoint1 = pos;
            setStatus(SetAxisPoint2);
            view()->moveRelativeZero(pos);
            break;

        case SetAxisPoint2:
            m_axisPoint2 = pos;
            view()->moveRelativeZero(pos);
            m_command.commitMirror(m_axisPoint1, m_axisPoint2);
            break;

        default:
            break;
        }
    }

    /// @brief 任何状态下都可输入 Y/N；其它输入提示无效，文本总是被接受
    void onCommand(GuiCommandEvent* e) override
    {
        if (!m_command.setCopyMode(e->getCommand().toLower()))
        {
            GUIDIALOGFACTORY->updateMouseWidget(ModifyMirrorCommand::tr("Input invalid"),
                                                ModifyMirrorCommand::tr("Back"));
        }
        e->accept();
    }

private:
    ModifyMirrorCommand& m_command;
    DmVector m_axisPoint1;
    DmVector m_axisPoint2;
};
}  // namespace

ModifyMirrorCommand::ModifyMirrorCommand() = default;

ModifyMirrorCommand::~ModifyMirrorCommand() = default;

bool ModifyMirrorCommand::onSelectionReady()
{
    DMSETTINGS->beginGroup("/Modify");
    m_copy = static_cast<bool>(DMSETTINGS->readNumEntry("/MirrorCopy", 1));
    DMSETTINGS->endGroup();

    m_preview = std::make_unique<CommandPreview>(document(), view());
    auto tool = std::make_unique<ModifyMirrorTool>(*this, document(), view());
    tool->setPreview(m_preview.get());
    activateTool(std::move(tool));
    return true;
}

void ModifyMirrorCommand::onStop()
{
    DMSETTINGS->beginGroup("/Modify");
    DMSETTINGS->writeEntry("/MirrorCopy", static_cast<int>(m_copy));
    DMSETTINGS->endGroup();
}

bool ModifyMirrorCommand::setCopyMode(const QString& input)
{
    if (input == "y")
    {
        m_copy = true;
        return true;
    }
    if (input == "n")
    {
        m_copy = false;
        return true;
    }
    return false;
}

void ModifyMirrorCommand::previewMirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    m_preview->clear();
    m_preview->entities().addSelectionFromDocument();
    m_preview->entities().getEntityContainer()->mirror(axisPoint1, axisPoint2);
    m_preview->entities().addEntity(new DmLine(nullptr, axisPoint1, axisPoint2));
    m_preview->draw();
}

void ModifyMirrorCommand::clearPreview()
{
    m_preview->clear();
}

void ModifyMirrorCommand::commitMirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    Transaction t(tr("mirror").toStdString(), document());
    t.start();
    auto entTable = document()->getEntityTable();

    std::vector<DmEntity*> addEnts;
    for (auto e : *entTable)
    {
        if (e->isSelected())
        {
            e->setSelected(false);
            DmEntity* theEnt = nullptr;

            if (m_copy)
            {
                // 复制
                theEnt = e->clone();
            }
            else
            {
                // 删除原始
                theEnt = e;
                entTable->startModify(theEnt);
            }

            theEnt->mirror(axisPoint1, axisPoint2);
            if (theEnt->getEntityType() == DM::EntityBlockReference)
            {
                static_cast<DmBlockReference*>(theEnt)->update();
            }
            if (m_copy)
            {
                addEnts.emplace_back(theEnt);
            }
        }
    }

    // 实体添加到表中
    for (auto ent : addEnts)
    {
        entTable->add(ent);
    }
    t.commit();

    GUIDIALOGFACTORY->updateSelectionWidget(document()->getEntityTable()->countSelect());
    finish();
}

ExclusiveCommandFactory ModifyCommands::mirror()
{
    return exclusiveCommandFactory<ModifyMirrorCommand>();
}
