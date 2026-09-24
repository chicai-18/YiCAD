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

/// @file ModifyRotateCommand.cpp
/// @brief 旋转命令与旋转工具的实现

#include "ModifyRotateCommand.h"

#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "DmBlockReference.h"
#include "DmDocument.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Math2d.h"
#include "Modification.h"
#include "Transaction.h"

namespace
{
/// @brief 未定义角度时的默认值
constexpr double ROTATE_ANGLE_UNDEFINED = 0.0;

/// @brief 旋转工具：指定旋转中心，再指定或输入角度
class ModifyRotateTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetCenterPoint, ///< 设置旋转中心
        SetAngle        ///< 设置角度
    };

    ModifyRotateTool(ModifyRotateCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetCenterPoint:
            GUIDIALOGFACTORY->updateMouseWidget(ModifyRotateCommand::tr("Specify rotation center"),
                                                ModifyRotateCommand::tr("Back"));
            break;
        case SetAngle:
            GUIDIALOGFACTORY->updateMouseWidget(ModifyRotateCommand::tr("Input angle"), ModifyRotateCommand::tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        DmVector mouse = snapper()->snapPoint(e);
        if (status() == SetAngle && mouse.valid)
        {
            m_command.previewRotate(m_center, Math2d::correctAngle((mouse - m_center).angle()));
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
        {
            onCoordinate(snapper()->snapPoint(e));
        }
        else if (e->button() == Qt::RightButton)
        {
            m_command.clearPreview();
            stepBack();
        }
    }

    void onCoordinate(const DmVector& position) override
    {
        if (!position.valid)
        {
            return;
        }
        switch (status())
        {
        case SetCenterPoint:
            m_center = position;
            view()->moveRelativeZero(m_center);
            setStatus(SetAngle);
            break;

        case SetAngle:
        {
            DmVector pos = position - m_center;
            // 点与中心重合时角度未定义
            const double angle =
                pos.squared() < DM_TOLERANCE2 ? ROTATE_ANGLE_UNDEFINED : Math2d::correctAngle(pos.angle());
            m_command.commitRotate(m_center, angle);
            break;
        }

        default:
            break;
        }
    }

    /// @brief 设置角度时可输入角度（度）；设置中心时不接受文本，文本被当作新命令
    void onCommand(GuiCommandEvent* e) override
    {
        if (status() != SetAngle)
        {
            return;
        }
        bool ok = false;
        double r = Math2d::eval(e->getCommand().toLower(), &ok);
        if (ok)
        {
            m_command.commitRotate(m_center, Math2d::deg2rad(r));
        }
        else
        {
            GUIDIALOGFACTORY->updateMouseWidget(ModifyRotateCommand::tr("Input invalid"),
                                                ModifyRotateCommand::tr("Back"));
        }
        e->accept();
    }

private:
    ModifyRotateCommand& m_command;
    DmVector m_center;
};
}  // namespace

ModifyRotateCommand::ModifyRotateCommand() = default;

ModifyRotateCommand::~ModifyRotateCommand() = default;

bool ModifyRotateCommand::onSelectionReady()
{
    m_preview = std::make_unique<CommandPreview>(document(), view());
    auto tool = std::make_unique<ModifyRotateTool>(*this, document(), view());
    tool->setPreview(m_preview.get());
    activateTool(std::move(tool));
    return true;
}

void ModifyRotateCommand::previewRotate(const DmVector& center, double angle)
{
    m_preview->clear();
    m_preview->entities().addSelectionFromDocument();
    m_preview->entities().getEntityContainer()->rotateAngle(center, angle);
    m_preview->draw();
}

void ModifyRotateCommand::clearPreview()
{
    m_preview->clear();
}

void ModifyRotateCommand::commitRotate(const DmVector& center, double angle)
{
    Transaction t(tr("Rotate").toStdString(), document());
    t.start();
    auto entTable = document()->getEntityTable();
    for (auto e : *entTable)
    {
        if (e->isSelected())
        {
            e->setSelected(false);
            entTable->startModify(e);
            e->rotateAngle(center, angle);
            if (e->getEntityType() == DM::EntityBlockReference)
            {
                static_cast<DmBlockReference*>(e)->update();
            }
        }
    }
    t.commit();
    GUIDIALOGFACTORY->updateSelectionWidget(document()->getEntityTable()->countSelect());
    finish();
}

namespace
{
const bool g_registered = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionModifyRotate, QStringLiteral("modify.rotate"), exclusiveCommandFactory<ModifyRotateCommand>());
}  // namespace
