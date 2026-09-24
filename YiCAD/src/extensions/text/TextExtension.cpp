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

/// @file TextExtension.cpp

#include "TextExtension.h"

#include <memory>

#include <QCoreApplication>

#include "CommandRegistry.h"
#include "DmDocument.h"
#include "DmLayer.h"
#include "DmMText.h"
#include "DmSystem.h"
#include "DrawMTextCommand.h"
#include "DrawTextCommand.h"
#include "EntityTable.h"
#include "ExclusiveCommandBus.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "IExtensionContext.h"
#include "ModifyMTextCommand.h"
#include "UIRibbonRegistry.h"
#include "UITextOptions.h"
#include "UIView.h"

namespace
{
/// @brief 上下文里要作用的多行文字；不是多行文字时返回空
DmMText* mtextOf(const CommandContext& ctx)
{
    return ctx.entity && ctx.entity->getEntityType() == DM::EntityMText ? static_cast<DmMText*>(ctx.entity)
                                                                          : nullptr;
}

/// @brief 选择变化的监听者（原 ActionSelectedChanged 的多行文字部分）：空闲态选中的实体都在
///        同一个图层上、且第一个是多行文字时，显示它的属性面板
void onSelectionChanged(const CommandContext& ctx)
{
    if (!ctx.document || !ctx.view)
    {
        return;
    }
    auto* view = qobject_cast<UIView*>(ctx.view->asQObject());
    ExclusiveCommandBus* bus = view ? view->commandBus() : nullptr;
    // 有命令在运行时不打断它（原先把属性面板叠在命令之上）
    if (!bus || bus->hasActiveCommand() || bus->isInCallback())
    {
        return;
    }

    DmLayer* firstLayer = nullptr;
    DmEntity* first = nullptr;
    for (DmEntity* entity : *ctx.document->getEntityTable())
    {
        if (!entity->isSelected())
        {
            continue;
        }
        if (!first)
        {
            first = entity;
            firstLayer = entity->getLayer();
        }
        else if (entity->getLayer() != firstLayer)
        {
            // 选中的实体不在同一个图层上
            return;
        }
    }
    if (!first || first->getEntityType() != DM::EntityMText)
    {
        return;
    }
    if (std::unique_ptr<IExclusiveCommand> command = CommandRegistry::instance().createCommand(
            QStringLiteral("ext.text.modify_mtext"), CommandContext{ctx.document, ctx.view, nullptr, first}))
    {
        view->startCommand(std::move(command));
    }
}

/// @brief 文字样式对话框（原 ActionTextStyle）
void openTextStyle(const CommandContext& ctx)
{
    if (ctx.document)
    {
        GUIDIALOGFACTORY->requestTextStyleDialog(ctx.document->getTextStyleTable(), ctx.document);
    }
}
}  // namespace

void TextExtension::OnRegister(IExtensionContext& ctx)
{
    // 本扩展的翻译包（src/extensions/text/ts/），必须早于下面的 translate() 调用。
    DMSYSTEM->loadExtensionTranslation(QStringLiteral("text"));

    auto text = [](const char* source) { return QCoreApplication::translate("TextExtension", source); };
    const QString drawText = text(QT_TRANSLATE_NOOP("TextExtension", "Single line text"));
    const QString drawMText = text(QT_TRANSLATE_NOOP("TextExtension", "Multiline text"));
    const QString textStyle = text(QT_TRANSLATE_NOOP("TextExtension", "Text style"));

    // 别名取原 keyconfig.xml"默认""拼音简写"两组的并集；dhwz 两组里都有，原先按单行文字解析
    ctx.registerExclusiveCommand(
        QStringLiteral("ext.text.draw"), exclusiveCommandFactory<DrawTextCommand>(),
        {.description = drawText,
         .aliases = {"text", "txt", "d", "dhwz"},
         .commandOptionsFactory = [](QWidget* parent, IExclusiveCommand* command, bool update) -> QWidget*
         {
             auto* options = new UITextOptions(parent);
             options->setCommand(command, update);
             return options;
         }});
    ctx.registerExclusiveCommand(QStringLiteral("ext.text.mtext"), exclusiveCommandFactory<DrawMTextCommand>(),
                                 {.description = drawMText, .aliases = {"mtext", "mtxt"}});
    ctx.registerInstantCommand(QStringLiteral("ext.text.style"), openTextStyle, {.description = textStyle});

    // 双击多行文字就地编辑；上下文的 entity/point 为双击的文字与位置
    ctx.registerExclusiveCommand(QStringLiteral("ext.text.edit_mtext"),
                                 [](const CommandContext& c) -> std::unique_ptr<IExclusiveCommand>
                                 {
                                     DmMText* mtext = mtextOf(c);
                                     return mtext ? std::make_unique<DrawMTextCommand>(mtext, c.point) : nullptr;
                                 },
                                 {});
    ctx.registerEntityEditor(DM::EntityMText, QStringLiteral("ext.text.edit_mtext"));

    // 多行文字属性面板；上下文的 entity 为要修改的文字
    ctx.registerExclusiveCommand(QStringLiteral("ext.text.modify_mtext"),
                                 [](const CommandContext& c) -> std::unique_ptr<IExclusiveCommand>
                                 {
                                     DmMText* mtext = mtextOf(c);
                                     return mtext ? std::make_unique<ModifyMTextCommand>(mtext) : nullptr;
                                 },
                                 {});
    // 宿主在选择变化时运行；不打断任何命令
    ctx.registerInstantCommand(QStringLiteral("ext.text.selection_changed"), onSelectionChanged,
                               {.instantInterrupt = InstantInterrupt::KeepAll});

    // 按钮：宿主占位的"绘图/文字"面板，顺序与迁移前一致
    ctx.ribbon().addAction({.panelId = UIRibbonIds::kPanelDraw2dText,
                            .text = drawText,
                            .iconPath = QStringLiteral(":/ribbon/draw2d/text.svg"),
                            .commandId = QStringLiteral("ext.text.draw")});
    ctx.ribbon().addAction({.panelId = UIRibbonIds::kPanelDraw2dText,
                            .text = drawMText,
                            .iconPath = QStringLiteral(":/ribbon/draw2d/mtext.svg"),
                            .commandId = QStringLiteral("ext.text.mtext")});
    ctx.ribbon().addAction({.panelId = UIRibbonIds::kPanelDraw2dText,
                            .text = textStyle,
                            .iconPath = QStringLiteral(":/ribbon/draw2d/text_style.svg"),
                            .commandId = QStringLiteral("ext.text.style")});
}
