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

/// @file ModifyExtension.cpp

#include "ModifyExtension.h"

#include <QCoreApplication>

#include "DmSystem.h"
#include "IExtensionContext.h"
#include "ModifyCommands.h"
#include "UIBevelOptions.h"
#include "UIModifyOffsetOptions.h"
#include "UIRibbonRegistry.h"
#include "UIRoundOptions.h"

namespace
{
/// @brief 构造选项条并交给命令
template <typename Options>
ExclusiveCommandOptionsFactory optionsFactory()
{
    return [](QWidget* parent, IExclusiveCommand* command, bool update) -> QWidget*
    {
        auto* options = new Options(parent);
        options->setCommand(command, update);
        return options;
    };
}

/// @brief 一条修改命令：命令 ID、按钮所在面板（为空表示没有按钮）、按钮文字、图标、工厂、选项条
struct ModifyCommand
{
    const char* id;
    const char* panelId;
    const char* text;
    const char* iconPath;
    ExclusiveCommandFactory factory;
    ExclusiveCommandOptionsFactory optionsFactory = {};
};
}  // namespace

void ModifyExtension::OnRegister(IExtensionContext& ctx)
{
    // 本扩展的翻译包（src/extensions/modify/ts/），必须早于下面的 translate() 调用。
    DMSYSTEM->loadExtensionTranslation(QStringLiteral("modify"));

    using namespace UIRibbonIds;
    // 按钮按原先宿主注册的顺序
    const ModifyCommand commands[] = {
        {"ext.modify.copy", kPanelDraw2dModify, QT_TRANSLATE_NOOP("ModifyExtension", "Copy"),
         ":/ribbon/draw2d/modify_copy.svg", ModifyCommands::copy()},
        {"ext.modify.move", kPanelDraw2dModify, QT_TRANSLATE_NOOP("ModifyExtension", "Move"),
         ":/ribbon/draw2d/modify_move.svg", ModifyCommands::move()},
        {"ext.modify.rotate", kPanelDraw2dModify, QT_TRANSLATE_NOOP("ModifyExtension", "Rotate"),
         ":/ribbon/draw2d/modify_rotate.svg", ModifyCommands::rotate()},
        {"ext.modify.scale", kPanelDraw2dModify, QT_TRANSLATE_NOOP("ModifyExtension", "Scale"),
         ":/ribbon/draw2d/modify_zoom.svg", ModifyCommands::scale()},
        {"ext.modify.mirror", kPanelDraw2dModify, QT_TRANSLATE_NOOP("ModifyExtension", "Mirror"),
         ":/ribbon/draw2d/modify_mirror.svg", ModifyCommands::mirror()},
        {"ext.modify.trim", kPanelDraw2dModify, QT_TRANSLATE_NOOP("ModifyExtension", "Trim"),
         ":/ribbon/draw2d/modify_trim.svg", ModifyCommands::trim()},
        {"ext.modify.extend", kPanelDraw2dModify, QT_TRANSLATE_NOOP("ModifyExtension", "Lengthen"),
         ":/ribbon/draw2d/modify_lengthen.svg", ModifyCommands::extend()},
        {"ext.modify.single_offset", kPanelDraw2dModify, QT_TRANSLATE_NOOP("ModifyExtension", "Offset"),
         ":/ribbon/draw2d/modify_offset.svg", ModifyCommands::singleOffset(), optionsFactory<UIModifyOffsetOptions>()},
        {"ext.modify.bevel", kPanelDraw2dModify, QT_TRANSLATE_NOOP("ModifyExtension", "Bevel"),
         ":/ribbon/draw2d/modify_bevel.svg", ModifyCommands::bevel(), optionsFactory<UIBevelOptions>()},
        {"ext.modify.round", kPanelDraw2dModify, QT_TRANSLATE_NOOP("ModifyExtension", "Fillet"),
         ":/ribbon/draw2d/modify_fillet.svg", ModifyCommands::round(), optionsFactory<UIRoundOptions>()},
        {"ext.modify.cut", kPanelDraw2dModify, QT_TRANSLATE_NOOP("ModifyExtension", "Divide"),
         ":/ribbon/draw2d/modify_divide.svg", ModifyCommands::cut()},
        {"ext.modify.cut_2p", kPanelDraw2dModify, QT_TRANSLATE_NOOP("ModifyExtension", "Divide_2P"),
         ":/ribbon/draw2d/modify_twopoints_break.svg", ModifyCommands::cut2p()},
        {"ext.modify.entity", kPanelDraw2dModify, QT_TRANSLATE_NOOP("ModifyExtension", "Properties"),
         ":/ribbon/draw2d/modify_attributes.svg", ModifyCommands::entity()},
        {"ext.modify.explode", kPanelDraw2dModify, QT_TRANSLATE_NOOP("ModifyExtension", "Explode"),
         ":/ribbon/draw2d/modify_explode.svg", ModifyCommands::explode()},

        // 多段线面板：绘图扩展的多段线、云线按钮之后
        {"ext.modify.polyline_add", kPanelDraw2dPolyline, QT_TRANSLATE_NOOP("ModifyExtension", "Add node"),
         ":/ribbon/draw2d/polyline_add_node.svg", ModifyCommands::polylineAdd()},
        {"ext.modify.polyline_append", kPanelDraw2dPolyline, QT_TRANSLATE_NOOP("ModifyExtension", "Append node"),
         ":/ribbon/draw2d/polyline_append_node.svg", ModifyCommands::polylineAppend()},
        {"ext.modify.polyline_del", kPanelDraw2dPolyline, QT_TRANSLATE_NOOP("ModifyExtension", "Delete node"),
         ":/ribbon/draw2d/polyline_delete_node.svg", ModifyCommands::polylineDel()},

        // 没有按钮：反向、删除（原先经 UIActionHandler::slotModifyDelete，没有调用方）、
        // 复制到图层（宿主图层面板的按钮按 ID 启动）
        {"ext.modify.reverse", nullptr, nullptr, nullptr, ModifyCommands::reverse()},
        {"ext.modify.delete", nullptr, nullptr, nullptr, ModifyCommands::remove()},
        {"ext.modify.copy_to_layer", nullptr, nullptr, nullptr, ModifyCommands::copyToLayer()},
    };

    for (const ModifyCommand& command : commands)
    {
        ctx.registerExclusiveCommand(command.id, command.factory, {.commandOptionsFactory = command.optionsFactory});
        if (command.panelId)
        {
            ctx.ribbon().addAction({
                .panelId = command.panelId,
                .text = QCoreApplication::translate("ModifyExtension", command.text),
                .iconPath = command.iconPath,
                .commandId = command.id,
            });
        }
    }

    // Delete 键与手写板橡皮擦：直接删除选择集的即时命令
    ctx.registerInstantCommand(QStringLiteral("ext.modify.delete_no_select"), ModifyCommands::deleteSelection(), {});
}
