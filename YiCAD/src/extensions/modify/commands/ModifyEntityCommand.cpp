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

/// @file ModifyEntityCommand.cpp
/// @brief 修改实体属性命令 ext.modify.entity，取代原 ActionModifyEntity：单击实体，运行实体所在
///        扩展登记的属性编辑命令（CommandRegistry::registerPropertyEditor）——属性对话框，或多行
///        文字的属性面板（ext.text.modify_mtext）

#include <memory>

#include <QCoreApplication>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandRegistry.h"
#include "ModifyCommands.h"
#include "DmEntity.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "PlaceCommand.h"

namespace
{
/// @brief 修改实体属性命令；交互由 ModifyEntityTool 驱动
class ModifyEntityCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(ModifyEntityCommand)

public:
    /// @brief 修改实体：运行这类实体登记的属性编辑命令；没有登记时什么也不做
    void modify(DmEntity* entity)
    {
        const CommandRegistry& registry = CommandRegistry::instance();
        const QString editor = registry.propertyEditor(entity->getEntityType());
        switch (registry.kind(editor))
        {
        case CommandKind::Instant:
            // 属性对话框是模态的，本命令留着，可以接着点下一个实体（与原先一致）
            registry.runInstant(editor, CommandContext{document(), view(), nullptr, entity});
            break;
        case CommandKind::Exclusive:
            // 属性面板（多行文字）不是模态对话框，由它接替本命令；面板结束后回到空闲态
            // （原先把属性编辑的旧 Action 叠在本命令之上，结束后回到本命令）
            replaceWith(editor, entity);
            break;
        default:
            break;
        }
    }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 修改实体属性工具：只有一步
class ModifyEntityTool : public BasePlaceTool
{
public:
    ModifyEntityTool(ModifyEntityCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    std::optional<DM::CursorType> getCursor() const override { return DM::SelectCursor; }

protected:
    void updateHints() override
    {
        GUIDIALOGFACTORY->updateMouseWidget(ModifyEntityCommand::tr("Click on entity to modify"),
                                            ModifyEntityCommand::tr("Cancel"));
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::RightButton)
        {
            stepBack();
            return;
        }
        DmEntity* entity = snapper()->catchEntity(e);
        if (entity)
        {
            entity->setSelected(true);
            view()->emitSelectedChanged();
            m_command.modify(entity);
        }
    }

private:
    ModifyEntityCommand& m_command;
};

std::unique_ptr<BasePlaceTool> ModifyEntityCommand::createTool()
{
    return std::make_unique<ModifyEntityTool>(*this, document(), view());
}

}  // namespace

ExclusiveCommandFactory ModifyCommands::entity()
{
    return exclusiveCommandFactory<ModifyEntityCommand>();
}
