/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file BlockExtension.cpp

#include "BlockExtension.h"

#include <memory>

#include <QCoreApplication>

#include "BaseExclusiveCommand.h"
#include "BlockEditTool.h"
#include "BlockFileCommands.h"
#include "BlockInsertCommand.h"
#include "BlocksCreateCommand.h"
#include "BlocksEditCommand.h"
#include "CommandRegistry.h"
#include "DefineAttributesCommand.h"
#include "DmAttributeDefinition.h"
#include "DmBlockReference.h"
#include "DmDocument.h"
#include "DmSystem.h"
#include "ExclusiveCommandBus.h"
#include "IDocumentView.h"
#include "IExtensionContext.h"
#include "UIDialogRunner.h"
#include "UIDlgDefineAttribute.h"
#include "UIDlgInsert.h"
#include "UIInsertOptions.h"
#include "UIRibbonRegistry.h"
#include "UIView.h"

namespace
{
/// @brief 撤销/重做使文档回到块编辑状态、视图却没有编辑模式时，恢复块编辑模式
///        （原在 UIActionHandler::slotCmdStateChanged 里直接构造 BlockEditTool）
void reenterBlockEdit(const CommandContext& ctx)
{
    if (!ctx.document || !ctx.view)
    {
        return;
    }
    auto* view = qobject_cast<UIView*>(ctx.view->asQObject());
    ExclusiveCommandBus* bus = view ? view->commandBus() : nullptr;
    DmBlock* editingBlock = ctx.document->getEditingBlock();
    if (!bus || !editingBlock || bus->editMode())
    {
        return;
    }
    auto mode = std::make_unique<BlockEditTool>(*view);
    BlockEditTool* blockEdit = mode.get();
    bus->enterEditMode(std::move(mode));
    blockEdit->reenter(editingBlock);
}

/// @brief 块参照与属性定义的属性对话框（原 UIDialogFactory::requestModifyEntityDialog 的对应分支）；
///        上下文的 entity 为要修改的实体
void editBlockProperties(const CommandContext& ctx)
{
    if (!ctx.entity)
    {
        return;
    }
    QWidget* parent = BaseExclusiveCommand::dialogParentOf(ctx.view);
    switch (ctx.entity->getEntityType())
    {
    case DM::EntityBlockReference:
    {
        auto* insert = static_cast<DmBlockReference*>(ctx.entity);
        UIDlgInsert dlg(parent);
        dlg.setInsert(*insert);
        if (UIDialogRunner::exec(dlg) == QDialog::Accepted)
        {
            dlg.updateInsert();
            insert->update();
        }
        break;
    }
    case DM::EntityAttributeDefinition:
    {
        UIDlgDefineAttribute dlg(parent);
        dlg.setAttributeDefinition(*static_cast<DmAttributeDefinition*>(ctx.entity), false);
        if (UIDialogRunner::exec(dlg) == QDialog::Accepted)
        {
            dlg.updateAttributeDefinition();
        }
        break;
    }
    default:
        break;
    }
}

/// @brief "绘图/块"面板里的一个按钮
struct BlockButton
{
    const char* id;
    const char* text;
    const char* iconPath;
};
}  // namespace

void BlockExtension::OnRegister(IExtensionContext& ctx)
{
    // 本扩展的翻译包（src/extensions/block/ts/），必须早于下面的 translate() 调用。
    DMSYSTEM->loadExtensionTranslation(QStringLiteral("block"));

    // 上下文有效到 OnShutdown 返回，命令在那之后才注销（IExtension.h 的契约）
    IExtensionContext* context = &ctx;
    auto text = [](const char* source) { return QCoreApplication::translate("BlockExtension", source); };

    // ---- 交互命令 ----
    ctx.registerExclusiveCommand(QStringLiteral("ext.block.create"), exclusiveCommandFactory<BlocksCreateCommand>(),
                                 {.description = text(QT_TRANSLATE_NOOP("BlockExtension", "Create Block"))});
    ctx.registerExclusiveCommand(
        QStringLiteral("ext.block.insert"),
        [](const CommandContext& c) -> std::unique_ptr<IExclusiveCommand>
        {
            // 原 ActionBlockInsertPrepare 是排他的（isExclusive）：连同块编辑模式一起结束全部命令，
            // 被否决（在块编辑的保存提示里取消）时不启动。交互命令本身只替换当前命令
            auto* view = c.view ? qobject_cast<UIView*>(c.view->asQObject()) : nullptr;
            if (view && !view->prepareInstantCommand(InstantInterrupt::EndAll))
            {
                return nullptr;
            }
            return std::make_unique<BlockInsertCommand>();
        },
        {.description = text(QT_TRANSLATE_NOOP("BlockExtension", "Insert the active block")),
         .commandOptionsFactory = [](QWidget* parent, IExclusiveCommand* command, bool update) -> QWidget*
         {
             auto* options = new UIInsertOptions(parent);
             options->setCommand(command, update);
             return options;
         }});
    ctx.registerExclusiveCommand(QStringLiteral("ext.block.edit"), &BlocksEditCommand::create,
                                 {.description = text(QT_TRANSLATE_NOOP("BlockExtension", "Edit Block"))});
    ctx.registerExclusiveCommand(QStringLiteral("ext.block.define_attributes"),
                                 exclusiveCommandFactory<DefineAttributesCommand>(),
                                 {.description = text(QT_TRANSLATE_NOOP("BlockExtension", "Define attributes"))});

    // ---- 即时命令 ----
    ctx.registerInstantCommand(
        QStringLiteral("ext.block.delete"),
        [context](const CommandContext& c)
        {
            if (c.document)
            {
                BlockFileCommands::deleteBlocks(c.document, context->mainWindow());
            }
        },
        {.description = text(QT_TRANSLATE_NOOP("BlockExtension", "Delete Block"))});
    // 原 ActionBlocksSaveAs 是排他的（isExclusive）：先结束全部命令
    ctx.registerInstantCommand(
        QStringLiteral("ext.block.save_as"),
        [context](const CommandContext& c)
        {
            if (c.document)
            {
                BlockFileCommands::showSaveAs(c.document, context->mainWindow());
            }
        },
        {.description = text(QT_TRANSLATE_NOOP("BlockExtension", "save the block to a file")),
         .instantInterrupt = InstantInterrupt::EndAll});
    ctx.registerInstantCommand(
        QStringLiteral("ext.block.save"),
        [context](const CommandContext& c)
        {
            if (c.document)
            {
                BlockFileCommands::saveActiveBlock(c.document, context->mainWindow());
            }
        },
        {});
    ctx.registerInstantCommand(
        QStringLiteral("ext.block.import"),
        [context](const CommandContext& c)
        {
            if (c.document)
            {
                BlockFileCommands::importBlocks(c.document, context->mainWindow());
            }
        },
        {.description = text(QT_TRANSLATE_NOOP("BlockExtension", "Import Block"))});
    // 宿主的撤销/重做钩子：不打断任何命令
    ctx.registerInstantCommand(QStringLiteral("ext.block.reenter_edit"), reenterBlockEdit,
                               {.instantInterrupt = InstantInterrupt::KeepAll});
    // 属性编辑（"修改实体属性"与选择层双击）：块参照、属性定义的属性对话框，不打断正在运行的命令
    ctx.registerInstantCommand(QStringLiteral("ext.block.properties"), editBlockProperties,
                               {.instantInterrupt = InstantInterrupt::KeepAll});
    ctx.registerPropertyEditor(DM::EntityBlockReference, QStringLiteral("ext.block.properties"));
    ctx.registerPropertyEditor(DM::EntityAttributeDefinition, QStringLiteral("ext.block.properties"));

    // ---- 按钮：宿主占位的"绘图/块"面板，顺序与迁移前一致 ----
    const BlockButton buttons[] = {
        {"ext.block.create", QT_TRANSLATE_NOOP("BlockExtension", "Create Block"), ":/ribbon/block/block_create.svg"},
        {"ext.block.insert", QT_TRANSLATE_NOOP("BlockExtension", "Insert the active block"),
         ":/ribbon/block/block_insert.svg"},
        {"ext.block.save_as", QT_TRANSLATE_NOOP("BlockExtension", "save the block to a file"),
         ":/ribbon/block/block_save.svg"},
        {"ext.block.define_attributes", QT_TRANSLATE_NOOP("BlockExtension", "Define attributes"),
         ":/ribbon/block/define_attribute.svg"},
        {"ext.block.delete", QT_TRANSLATE_NOOP("BlockExtension", "Delete Block"), ":/ribbon/block/block_delete.svg"},
        {"ext.block.edit", QT_TRANSLATE_NOOP("BlockExtension", "Edit Block"), ":/ribbon/block/block_edit.svg"},
        {"ext.block.import", QT_TRANSLATE_NOOP("BlockExtension", "Import Block"), ":/ribbon/file/import_block.svg"},
    };
    for (const BlockButton& button : buttons)
    {
        ctx.ribbon().addAction({
            .panelId = UIRibbonIds::kPanelDraw2dBlock,
            .text = text(button.text),
            .iconPath = button.iconPath,
            .commandId = button.id,
        });
    }
}
