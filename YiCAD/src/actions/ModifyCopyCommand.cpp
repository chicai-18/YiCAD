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

/// @file ModifyCopyCommand.cpp
/// @brief 复制命令与复制工具的实现

#include "ModifyCopyCommand.h"

#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "DmBlockReference.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "DmSettings.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Math2d.h"
#include "Transaction.h"

namespace
{
/// @brief 捕捉角度阈值（度）
constexpr double SNAP_ANGLE_THRESHOLD = 15.0;
/// @brief 默认复制数量
constexpr int DEFAULT_COPY_COUNT = 1;
/// @brief 最小复制数量
constexpr int MIN_COPY_COUNT = 0;

/// @brief 复制工具：指定参考点，再指定目标点；随时可输入复制数量
class ModifyCopyTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetReferencePoint, ///< 设置参考点
        SetTargetPoint,    ///< 设置目标点
    };

    ModifyCopyTool(ModifyCopyCommand& command, DmDocument* doc, IDocumentView* view)
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
            GUIDIALOGFACTORY->updateMouseWidget(
                ModifyCopyCommand::tr("Specify reference point or input copy number, default copy number is %1")
                    .arg(m_command.copyCount()),
                ModifyCopyCommand::tr("Cancel"));
            break;
        case SetTargetPoint:
            GUIDIALOGFACTORY->updateMouseWidget(
                ModifyCopyCommand::tr("Specify target point or input copy number, default copy number is %1")
                    .arg(m_command.copyCount()),
                ModifyCopyCommand::tr("Back"));
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
                    mouse = snapper()->snapToAngle(mouse, m_referencePoint, SNAP_ANGLE_THRESHOLD);
                }
                m_targetPoint = mouse;
                m_command.previewCopy(m_referencePoint, m_targetPoint, shift);
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
                snapped = snapper()->snapToAngle(snapped, m_referencePoint, SNAP_ANGLE_THRESHOLD);
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
            m_command.commitCopy(m_referencePoint, m_targetPoint);
            break;

        default:
            break;
        }
    }

    /// @brief 任何状态下都可输入复制数量；输入无效时提示，文本总是被接受
    void onCommand(GuiCommandEvent* e) override
    {
        if (!m_command.setCopyCount(e->getCommand().toLower()))
        {
            GUIDIALOGFACTORY->updateMouseWidget(ModifyCopyCommand::tr("Input invalid"), ModifyCopyCommand::tr("Back"));
        }
        e->accept();
    }

private:
    ModifyCopyCommand& m_command;
    DmVector m_referencePoint;
    DmVector m_targetPoint;
};
}  // namespace

ModifyCopyCommand::ModifyCopyCommand() = default;

ModifyCopyCommand::~ModifyCopyCommand() = default;

bool ModifyCopyCommand::onSelectionReady()
{
    DMSETTINGS->beginGroup("/Modify");
    m_copyCount = DMSETTINGS->readNumEntry("/CopyCount", DEFAULT_COPY_COUNT);
    DMSETTINGS->endGroup();

    m_preview = std::make_unique<CommandPreview>(document(), view());
    auto tool = std::make_unique<ModifyCopyTool>(*this, document(), view());
    tool->setPreview(m_preview.get());
    activateTool(std::move(tool));
    return true;
}

void ModifyCopyCommand::onStop()
{
    DMSETTINGS->beginGroup("/Modify");
    DMSETTINGS->writeEntry("/CopyCount", m_copyCount);
    DMSETTINGS->endGroup();
}

bool ModifyCopyCommand::setCopyCount(const QString& input)
{
    bool ok = false;
    double r = Math2d::eval(input, &ok);
    if (ok && (static_cast<int>(r) > MIN_COPY_COUNT))
    {
        m_copyCount = static_cast<int>(r);
        return true;
    }
    return false;
}

std::vector<DmEntity*> ModifyCopyCommand::cloneSelection(const DmVector& offset) const
{
    auto entTable = document()->getEntityTable();
    std::vector<DmEntity*> addedEnts;
    for (int num = 1; num <= m_copyCount; num++)
    {
        for (auto e : *entTable)
        {
            if (e->isSelected())
            {
                auto cloneEnt = e->clone();
                cloneEnt->move(offset * num);
                if (cloneEnt->getEntityType() == DM::EntityBlockReference)
                {
                    static_cast<DmBlockReference*>(cloneEnt)->update();
                }
                addedEnts.emplace_back(cloneEnt);
            }
        }
    }
    return addedEnts;
}

void ModifyCopyCommand::previewCopy(const DmVector& reference, const DmVector& target, bool showGuide)
{
    m_preview->clear();
    for (auto ent : cloneSelection(target - reference))
    {
        m_preview->entities().addEntity(ent);
    }
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

void ModifyCopyCommand::clearPreview()
{
    m_preview->clear();
}

void ModifyCopyCommand::commitCopy(const DmVector& reference, const DmVector& target)
{
    Transaction t(tr("Copy").toStdString(), document());
    t.start();
    auto entTable = document()->getEntityTable();
    for (auto e : cloneSelection(target - reference))
    {
        entTable->add(e);
    }
    t.commit();

    GUIDIALOGFACTORY->updateSelectionWidget(document()->getEntityTable()->countSelect());
    finish();
}

namespace
{
const bool g_registered = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionModifyCopy, QStringLiteral("modify.copy"), exclusiveCommandFactory<ModifyCopyCommand>());
}  // namespace
