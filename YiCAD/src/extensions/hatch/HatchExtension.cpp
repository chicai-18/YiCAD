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

/// @file HatchExtension.cpp

#include "HatchExtension.h"

#include <QCoreApplication>

#include "CommandRegistry.h"
#include "DmSystem.h"
#include "DrawHatchCommand.h"
#include "IExtensionContext.h"
#include "UIRibbonRegistry.h"

void HatchExtension::OnRegister(IExtensionContext& ctx)
{
    // 本扩展的翻译包（src/extensions/hatch/ts/），必须早于下面的 translate() 调用。
    DMSYSTEM->loadExtensionTranslation(QStringLiteral("hatch"));

    const QString text = QCoreApplication::translate("HatchExtension", "Hatch");
    // 别名取原 keyconfig.xml"拼音简写"组的 tc（"默认"组没有别名）
    ctx.registerExclusiveCommand(QStringLiteral("ext.hatch.draw"), exclusiveCommandFactory<DrawHatchCommand>(),
                                 {.description = text, .aliases = {"tc"}});
    ctx.ribbon().addAction({
        .panelId = UIRibbonIds::kPanelDraw2dOther,
        .text = text,
        .iconPath = QStringLiteral(":/ribbon/draw2d/hatch.svg"),
        .commandId = QStringLiteral("ext.hatch.draw"),
    });
}
