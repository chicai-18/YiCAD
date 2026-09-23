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

#include <QAction>
#include <QCoreApplication>
#include <QIcon>

#include "AIAssistant.h"
#include "DmDocument.h"
#include "GuiDocumentView.h"
#include "IExtensionContext.h"
#include "LLMSettingsPage.h"
#include "LLMSettingsService.h"
#include "SARibbonBar.h"
#include "SARibbonButtonGroupWidget.h"

AIExtension::AIExtension() = default;
AIExtension::~AIExtension() = default;

void AIExtension::OnRegister(IExtensionContext& ctx)
{
    m_ctx = &ctx;
    m_mainWindow = ctx.mainWindow();

    m_assistant = std::make_unique<AIAssistant>(m_mainWindow, m_mainWindow);

    // 从 Main.cpp 搬过来（阶段4第二阶段）：init() 自带重复调用防护，
    // 不必赶在这里之前完成——ai/ 里真正用到它的路径都是首次点击 AI
    // 按钮才惰性触发。
    LLMSettingsService::instance()->init(
        QCoreApplication::organizationName(), QCoreApplication::applicationName());

    // 复刻原 ApplicationWindow.cpp 里 AI 按钮的文本/图标/objectName，
    // 只是把常驻 rightButtonGroup() 的放置方式和点击回调搬到这里。
    auto* action = new QAction(QCoreApplication::translate("ApplicationWindow", "AI Assistant"),
                                &ctx.ribbon());
    action->setIcon(QIcon(":/ribbon/tabbar/ai.svg"));
    action->setObjectName("ai-assistant");
    QObject::connect(action, &QAction::triggered, &ctx.ribbon(),
                      [this]()
                      {
                          DmDocument* doc = m_ctx->currentDocument();
                          GuiDocumentView* docView = m_ctx->currentDocumentView();
                          m_assistant->show(doc, docView);
                      });
    ctx.ribbon().rightButtonGroup()->addAction(action);

    ctx.registerSettingsPage(
        QStringLiteral("ext.ai.llm_settings"),
        QCoreApplication::translate("ApplicationWindow", "AI Settings"), QString(),
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
