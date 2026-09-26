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

/// @file OptionsExtension.cpp

#include "OptionsExtension.h"

#include <QColor>
#include <QCoreApplication>

#include "CommandRegistry.h"
#include "DmSettings.h"
#include "DmSystem.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "IDocumentManager.h"
#include "IDocumentView.h"
#include "IExtensionContext.h"
#include "UIDialogRunner.h"
#include "UIDlgOptionsDrawing.h"
#include "UIDlgOptionsGeneral.h"
#include "UIRibbonRegistry.h"

namespace
{
/// @brief 系统设置：弹出对话框，再把设置里的颜色应用到全部打开的视图
void openGeneralOptions(IExtensionContext& ctx)
{
    IDocumentManager* documents = ctx.documentManager();
    {
        UIDlgOptionsGeneral dlg(ctx.mainWindow());
        if (documents)
        {
            dlg.setDocuments(documents->documents());
        }
        UIDialogRunner::exec(dlg);
    }

    DMSETTINGS->beginGroup("Colors");
    const QColor background(DMSETTINGS->readEntry("/background", Colors::BACKGROUND));
    const QColor gridColor(DMSETTINGS->readEntry("/grid", Colors::GRID));
    const QColor metaGridColor(DMSETTINGS->readEntry("/meta_grid", Colors::META_GRID));
    const QColor selectedColor(DMSETTINGS->readEntry("/select", Colors::SELECT));
    const QColor highlightColor(DMSETTINGS->readEntry("/highlight", Colors::HIGHLIGHT));
    DMSETTINGS->endGroup();

    if (!documents)
    {
        return;
    }
    for (GuiDocumentView* view : documents->documentViews())
    {
        view->setBackground(background);
        view->setGridColor(gridColor);
        view->setMetaGridColor(metaGridColor);
        view->setSelectedColor(selectedColor);
        view->setHighlightColor(highlightColor);
        view->redraw();
    }
}

/// @brief 图纸设置：弹出当前图纸的设置对话框，复位坐标显示
void openDrawingOptions(IExtensionContext& ctx, const CommandContext& cmd)
{
    if (!cmd.document)
    {
        return;
    }
    {
        UIDlgOptionsDrawing dlg(ctx.mainWindow());
        dlg.setDocument(cmd.document);
        UIDialogRunner::exec(dlg);
    }
    GUIDIALOGFACTORY->updateCoordinateWidget(DmVector(0.0, 0.0), DmVector(0.0, 0.0), true);
    if (cmd.view)
    {
        cmd.view->redraw();
    }
}
}  // namespace

void OptionsExtension::OnRegister(IExtensionContext& ctx)
{
    // 本扩展的翻译包（src/extensions/options/ts/），必须早于下面的 translate() 调用。
    DMSYSTEM->loadExtensionTranslation(QStringLiteral("options"));

    // 上下文有效到 OnShutdown 返回，命令在那之后才注销（IExtension.h 的契约）
    IExtensionContext* context = &ctx;
    const QString general = QCoreApplication::translate("OptionsExtension", "System Setting");
    ctx.registerInstantCommand(QStringLiteral("ext.options.general"),
                               [context](const CommandContext&) { openGeneralOptions(*context); },
                               {.description = general});
    ctx.ribbon().addAction({
        .panelId = UIRibbonIds::kPanelOptionsSettings,
        .text = general,
        .iconPath = QStringLiteral(":/ribbon/options/settings.svg"),
        .commandId = QStringLiteral("ext.options.general"),
    });

    const QString drawing = QCoreApplication::translate("OptionsExtension", "Draw Setting");
    ctx.registerInstantCommand(QStringLiteral("ext.options.drawing"),
                               [context](const CommandContext& cmd) { openDrawingOptions(*context, cmd); },
                               {.description = drawing});
    ctx.ribbon().addAction({
        .panelId = UIRibbonIds::kPanelOptionsSettings,
        .text = drawing,
        .iconPath = QStringLiteral(":/ribbon/options/draw_settings.svg"),
        .commandId = QStringLiteral("ext.options.drawing"),
    });
}
