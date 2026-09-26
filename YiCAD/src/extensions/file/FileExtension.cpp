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

/// @file FileExtension.cpp

#include "FileExtension.h"

#include <functional>

#include <QCoreApplication>

#include "CommandRegistry.h"
#include "DmSystem.h"
#include "IDocumentManager.h"
#include "IExtensionContext.h"
#include "UIRibbonRegistry.h"

namespace
{
constexpr const char* YCD_FORMAT_TYPE = "ycd"; ///< 打开、保存图纸时的格式

/// @brief 一条文件命令：命令 ID、按钮文字、图标、所在面板、是否要求打开的图纸、执行体
struct FileCommand
{
    const char* id;
    const char* text;
    const char* iconPath;
    const char* panelId;
    bool needsDocument;
    std::function<void(IDocumentManager&)> run;
};
}  // namespace

void FileExtension::OnRegister(IExtensionContext& ctx)
{
    // 本扩展的翻译包（src/extensions/file/ts/），必须早于下面的 translate() 调用。
    DMSYSTEM->loadExtensionTranslation(QStringLiteral("file"));

    using namespace UIRibbonIds;
    const FileCommand commands[] = {
        {"ext.file.new", QT_TRANSLATE_NOOP("FileExtension", "new"), ":/ribbon/file/new.svg", kPanelFileFile, false,
         [](IDocumentManager& documents) { documents.newDocument(); }},
        {"ext.file.open", QT_TRANSLATE_NOOP("FileExtension", "open"), ":/ribbon/file/open.svg", kPanelFileFile, false,
         [](IDocumentManager& documents)
         {
             DMSYSTEM->setCurrentFormatType(YCD_FORMAT_TYPE);
             documents.openDocument();
         }},
        {"ext.file.save", QT_TRANSLATE_NOOP("FileExtension", "save"), ":/ribbon/file/save.svg", kPanelFileFile, true,
         [](IDocumentManager& documents)
         {
             DMSYSTEM->setCurrentFormatType(YCD_FORMAT_TYPE);
             documents.saveDocument();
         }},
        {"ext.file.save_as", QT_TRANSLATE_NOOP("FileExtension", "save as"), ":/ribbon/file/save_as.svg",
         kPanelFileFile, true,
         [](IDocumentManager& documents)
         {
             DMSYSTEM->setCurrentFormatType(YCD_FORMAT_TYPE);
             documents.saveDocumentAs();
         }},
        {"ext.file.export_image", QT_TRANSLATE_NOOP("FileExtension", "Export Image"),
         ":/ribbon/file/export_image.svg", kPanelFileExport, false,
         [](IDocumentManager& documents) { documents.exportImage(); }},
    };

    // 上下文有效到 OnShutdown 返回，命令在那之后才注销（IExtension.h 的契约）
    IExtensionContext* context = &ctx;
    const UIRibbonEnableFn documentOpen = UIRibbonCondition::requireAll(UIRibbonRequires::DocumentOpen);
    for (const FileCommand& command : commands)
    {
        const QString text = QCoreApplication::translate("FileExtension", command.text);
        // 原 Action 是排他的（isExclusive）：先结束全部命令，被否决（如块编辑中取消保存）时不执行
        ctx.registerInstantCommand(
            command.id,
            [context, run = command.run](const CommandContext&)
            {
                if (IDocumentManager* documents = context->documentManager())
                {
                    run(*documents);
                }
            },
            {.description = text, .instantInterrupt = InstantInterrupt::EndAll});
        ctx.ribbon().addAction({
            .panelId = command.panelId,
            .text = text,
            .iconPath = command.iconPath,
            .commandId = command.id,
            .enableFn = command.needsDocument ? documentOpen : UIRibbonEnableFn{},
        });
    }
}
