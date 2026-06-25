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

/// @file AIAssistant.cpp
/// @brief AI 助手控制器实现

#include "AIAssistant.h"

#include "AIDialog.h"
#include "AIPipeline.h"
#include "LLMSettingsPage.h"
#include "DmSystem.h"

AIAssistant::AIAssistant(QWidget* parentWindow, QObject* parent)
    : QObject(parent)
    , m_parentWindow(parentWindow)
{
}

AIAssistant::~AIAssistant()
{
}

void AIAssistant::show(DmDocument* doc, GuiDocumentView* docView)
{
    ensureCreated(doc, docView);
    m_dialog->show();
    m_dialog->raise();
    m_dialog->activateWindow();
}

void AIAssistant::ensureCreated(DmDocument* doc, GuiDocumentView* docView)
{
    if (m_dialog)
        return;

    m_dialog = new AIDialog(m_parentWindow);

    const QString appDir = DMSYSTEM->getAppDir();
    const QString docsDir = appDir + "/ai";
    const QString readmePath = appDir + "/ai/README.md";

    m_pipeline = new AIPipeline(docsDir, readmePath,
                                doc, docView, this);

    // ---- 2. 信号连接 ----
    connect(m_dialog, &AIDialog::sendRequested,
            this, &AIAssistant::onSendRequested);

    connect(m_pipeline, &AIPipeline::responseReady,
            m_dialog, &AIDialog::appendMessage);
    connect(m_pipeline, &AIPipeline::responseReady,
            this, &AIAssistant::onPipelineResponse);

    connect(m_pipeline, &AIPipeline::errorOccurred,
            m_dialog, &AIDialog::appendMessage);
    connect(m_pipeline, &AIPipeline::errorOccurred,
            this, &AIAssistant::onPipelineResponse);

    connect(m_dialog, &AIDialog::configRequested,
            this, &AIAssistant::onConfigRequested);

    connect(m_dialog, &AIDialog::newSessionRequested,
            this, &AIAssistant::onNewSessionRequested);
}

void AIAssistant::onSendRequested(const QString& text, const QString& /*mode*/)
{
    // AI 处理期间禁用发送按钮
    m_dialog->setSendEnabled(false);

    const int idx = m_dialog->modeIndex();
    const QString token = (idx == 0) ? QStringLiteral("qa")
                        : (idx == 1) ? QStringLiteral("modeling")
                        : QStringLiteral("auto");
    m_pipeline->handleUserInput(text, token);
}

void AIAssistant::onConfigRequested()
{
    LLMSettingsPage dlg(m_dialog);
    dlg.exec();
}

void AIAssistant::onNewSessionRequested()
{
    // 清空 UI 对话显示
    m_dialog->clearChatView();

    // 清空 LLM 对话历史，后续请求不再携带之前的上下文
    m_pipeline->history().clear();
}

void AIAssistant::onPipelineResponse(const QString& sender, const QString& /*text*/)
{
    // AI 最终回复或错误时恢复发送按钮（System 消息是中间状态）
    if (sender == QStringLiteral("AI") || sender != QStringLiteral("System"))
    {
        m_dialog->setSendEnabled(true);
    }
}


