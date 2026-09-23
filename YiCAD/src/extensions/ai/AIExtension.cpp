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

/// @file AIExtension.cpp

#include "AIExtension.h"

#include <QCoreApplication>

#include "AIAssistant.h"
#include "DmDocument.h"
#include "DmSystem.h"
#include "GuiDocumentView.h"
#include "IExtensionContext.h"
#include "LLMSettingsPage.h"
#include "LLMSettingsService.h"
#include "UIRibbonRegistry.h"

AIExtension::AIExtension() = default;
AIExtension::~AIExtension() = default;

void AIExtension::OnRegister(IExtensionContext& ctx)
{
    m_ctx = &ctx;
    m_mainWindow = ctx.mainWindow();

    // 本扩展的翻译包（src/extensions/ai/ts/ → <exe 目录>/resources/qm/ai_*.qm），
    // 必须早于下面任何 tr()/translate() 调用。
    DMSYSTEM->loadExtensionTranslation(QStringLiteral("ai"));

    m_assistant = std::make_unique<AIAssistant>(m_mainWindow, m_mainWindow);

    // 从 Main.cpp 搬过来（阶段4第二阶段）：init() 自带重复调用防护，
    // 不必赶在这里之前完成——本扩展里真正用到它的路径都是首次点击 AI
    // 按钮才惰性触发。
    LLMSettingsService::instance()->init(
        QCoreApplication::organizationName(), QCoreApplication::applicationName());

    // AI 按钮放在 Ribbon 栏右侧常驻按钮组（不属于任何 Ribbon 标签页）。
    ctx.ribbon().addAction(UIRibbonActionDef{
        .id = QStringLiteral("ext.ai.assistant"),
        .panelId = UIRibbonIds::kPanelRightButtons,
        .text = QCoreApplication::translate("AIExtension", "AI Assistant"),
        .iconPath = QStringLiteral(":/extensions/ai/ai.svg"),
        .trigger =
            [this]()
            {
                DmDocument* doc = m_ctx->currentDocument();
                GuiDocumentView* docView = m_ctx->currentDocumentView();
                m_assistant->show(doc, docView);
            },
        .objectName = QStringLiteral("ai-assistant"),
    });

    ctx.registerSettingsPage(
        QStringLiteral("ext.ai.llm_settings"),
        QCoreApplication::translate("AIExtension", "AI Settings"), QString(),
        [this]()
        {
            LLMSettingsPage dlg(m_mainWindow);
            dlg.exec();
        });
}

void AIExtension::OnShutdown()
{
    LLMSettingsService::shutdown();
    m_assistant.reset();
    m_ctx = nullptr;
    m_mainWindow = nullptr;
}
