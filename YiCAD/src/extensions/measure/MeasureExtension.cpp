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

/// @file MeasureExtension.cpp

#include "MeasureExtension.h"

#include <QCoreApplication>

#include "DmSystem.h"
#include "IExtensionContext.h"
#include "MeasureCommands.h"
#include "UIRibbonRegistry.h"

namespace
{
/// @brief 一条查询命令：命令 ID、按钮文字、图标与工厂
struct MeasureCommand
{
    const char* id;
    const char* text;
    const char* iconPath;
    ExclusiveCommandFactory factory;
};
}  // namespace

void MeasureExtension::OnRegister(IExtensionContext& ctx)
{
    // 本扩展的翻译包（src/extensions/measure/ts/），必须早于下面的 translate() 调用。
    DMSYSTEM->loadExtensionTranslation(QStringLiteral("measure"));

    // 按钮按原先宿主注册的顺序
    const MeasureCommand commands[] = {
        {"ext.measure.dist", QT_TRANSLATE_NOOP("MeasureExtension", "Distance Point to Point"),
         ":/ribbon/draw2d/info_dist_pt_pt.svg", MeasureCommands::dist()},
        {"ext.measure.angle", QT_TRANSLATE_NOOP("MeasureExtension", "Angle between two lines"),
         ":/ribbon/draw2d/info_angle_2lines.svg", MeasureCommands::angle()},
        {"ext.measure.total_length", QT_TRANSLATE_NOOP("MeasureExtension", "Total length of selected entities"),
         ":/ribbon/draw2d/info_length_entity.svg", MeasureCommands::totalLength()},
        {"ext.measure.area", QT_TRANSLATE_NOOP("MeasureExtension", "Polygonal Area"),
         ":/ribbon/draw2d/info_area_polygon.svg", MeasureCommands::area()},
    };

    for (const MeasureCommand& command : commands)
    {
        ctx.registerExclusiveCommand(command.id, command.factory, {});
        ctx.ribbon().addAction({
            .panelId = UIRibbonIds::kPanelDraw2dMeasure,
            .text = QCoreApplication::translate("MeasureExtension", command.text),
            .iconPath = command.iconPath,
            .commandId = command.id,
        });
    }

    // 选中实体信息：即时命令，没有按钮（命令行 info/ck）
    ctx.registerInstantCommand(QStringLiteral("ext.measure.selected"), MeasureCommands::selected(), {});
}
