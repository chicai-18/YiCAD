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

/// @file AILLMClassifier.cpp
/// @brief AILLMClassifier 实现 —— LLM 意图分类器

#include "AILLMClassifier.h"
#include "ConversationHistory.h"
#include "DeepSeekProvider.h"
#include "LLMSettingsService.h"

#include <QPointer>
#include <QTimer>

// ---------------------------------------------------------------------------
// 分类 prompt 模板
// ---------------------------------------------------------------------------
// 使用 QStringLiteral 而非 tr()：这是发给 LLM 的指令，不随 UI 语言切换。

static const QString kClassifierSystemPrompt = QStringLiteral(
    "You are a CAD command classifier. Analyze the user's input and classify it "
    "as exactly one of: QA, MODELING, MIXED, SOCIAL.\n\n"
    "QA: The user is asking a question, seeking help, or wants to learn about a "
    "feature. Examples: \"how to draw a circle\", \"what is trim\", \"explain layers\"\n\n"
    "MODELING: The user wants to create, modify, or manipulate CAD entities. "
    "Examples: \"draw a circle\", \"move this line right\", \"delete the rectangle\"\n\n"
    "MIXED: The user wants both — perform an operation AND ask a question. "
    "Examples: \"delete this line and tell me how to draw an arc\"\n\n"
    "SOCIAL: The user is greeting, thanking, making small talk, or sending a "
    "non-task message. Examples: \"hello\", \"hi\", \"good morning\", \"thanks\", "
    "\"how are you\", \"what can you do\"\n\n"
    "Reply with exactly ONE word (QA, MODELING, MIXED, or SOCIAL) followed by a confidence "
    "score (0.0 to 1.0) on the next line.\n"
    "Example:\n"
    "MODELING\n"
    "0.95");

// ---------------------------------------------------------------------------
// 构造
// ---------------------------------------------------------------------------

AILLMClassifier::AILLMClassifier(QObject* parent)
    : QObject(parent)
    , m_provider(new DeepSeekProvider(this))
{
}

// ---------------------------------------------------------------------------
// 公开接口
// ---------------------------------------------------------------------------

bool AILLMClassifier::isAvailable() const
{
    LLMSettingsService* svc = LLMSettingsService::instance();
    return svc && svc->isInitialized() && svc->hasApiKey();
}

void AILLMClassifier::setContextEnabled(bool enabled)
{
    m_contextEnabled = enabled;
}

int AILLMClassifier::classify(const QString& input,
                               const QStringList& recentMsgs,
                               std::function<void(IntentType, float)> callback)
{
    const int seq = m_nextSeq++;

    // 检查 LLM 可用性
    if (!isAvailable())
    {
        emit classificationFailed(tr("LLM API is not available (API Key not configured)."));
        if (callback)
            callback(IntentType::Uncertain, 0.0f);
        return seq;
    }

    // 构建分类 prompt
    const QString sysPrompt = buildRouterPrompt(input, recentMsgs);

    // 用 QPointer 保护回调中的 this
    QPointer<AILLMClassifier> guard(this);

    // 用 shared_ptr 管理连接句柄，实现 Qt 5 兼容的一次性连接
    auto connResp = std::make_shared<QMetaObject::Connection>();
    auto connErr  = std::make_shared<QMetaObject::Connection>();
    auto fired    = std::make_shared<bool>(false);  // 防止重复回调

    // 连接 responseReceived
    *connResp = connect(
        m_provider.get(), &DeepSeekProvider::responseReceived,
        this,
        [this, guard, seq, callback, connResp, connErr, fired](const QString& responseText) {
            if (!guard || *fired) return;
            *fired = true;
            disconnect(*connResp);
            disconnect(*connErr);

            IntentType intent = parseClassification(responseText);
            float conf = parseConfidence(responseText);

            if (intent == IntentType::Uncertain)
            {
                emit classificationFailed(
                    tr("Failed to parse LLM classification response: %1").arg(responseText.left(200)));
            }

            if (callback)
                callback(intent, conf);
        });

    // 连接 errorOccurred
    *connErr = connect(
        m_provider.get(), &DeepSeekProvider::errorOccurred,
        this,
        [this, guard, seq, callback, connResp, connErr, fired](const QString& error) {
            if (!guard || *fired) return;
            *fired = true;
            disconnect(*connResp);
            disconnect(*connErr);
            emit classificationFailed(error);
            if (callback)
                callback(IntentType::Uncertain, 0.0f);
        });

    // 3s 超时保护
    QTimer::singleShot(3000, this, [this, guard, seq, connResp, connErr, fired, callback]() {
        if (!guard || *fired) return;
        *fired = true;
        disconnect(*connResp);
        disconnect(*connErr);
        emit classificationFailed(tr("LLM classification timed out after 3 seconds."));
        if (callback)
            callback(IntentType::Uncertain, 0.0f);
    });

    // 发起 LLM 调用：空历史，传入 system prompt，temperature=0
    // max_tokens=128：为推理模型（deepseek-v4-pro 等）留足 reasoning + answer 空间
    m_provider->sendMessage(input, QVector<MessageEntry>(),
                            sysPrompt, 0.0, 128);

    return seq;
}

// ---------------------------------------------------------------------------
// Prompt 构建
// ---------------------------------------------------------------------------

QString AILLMClassifier::buildRouterPrompt(const QString& input,
                                            const QStringList& recentMsgs) const
{
    QString prompt = kClassifierSystemPrompt;

    // 注入最近对话上下文（如果启用且有历史）
    if (m_contextEnabled && !recentMsgs.isEmpty())
    {
        prompt += QStringLiteral("\n\nRecent conversation context (for reference only):\n");
        // recentMsgs 是倒序的（最新在前），这里正序展示
        for (int i = recentMsgs.size() - 1; i >= 0; --i)
        {
            prompt += QStringLiteral("- %1\n").arg(recentMsgs[i]);
        }
    }

    return prompt;
}

// ---------------------------------------------------------------------------
// 响应解析
// ---------------------------------------------------------------------------

IntentType AILLMClassifier::parseClassification(const QString& response)
{
    const QString upper = response.trimmed().toUpper();

    // 检查第一行（可能包含 LLM 的额外文本）
    if (upper.startsWith(QStringLiteral("MODEL")))
        return IntentType::Modeling;
    if (upper.startsWith(QStringLiteral("QA")))
        return IntentType::QA;
    if (upper.startsWith(QStringLiteral("MIX")))
        return IntentType::Mixed;
    if (upper.startsWith(QStringLiteral("SOC")))
        return IntentType::Social;

    // 如果第一行只有单词
    const QString firstLine = upper.split(QStringLiteral("\n")).first().trimmed();
    if (firstLine == QStringLiteral("MODELING"))
        return IntentType::Modeling;
    if (firstLine == QStringLiteral("QA"))
        return IntentType::QA;
    if (firstLine == QStringLiteral("MIXED"))
        return IntentType::Mixed;
    if (firstLine == QStringLiteral("SOCIAL"))
        return IntentType::Social;

    return IntentType::Uncertain;
}

float AILLMClassifier::parseConfidence(const QString& response)
{
    // 从响应的第二行提取置信度
    const QStringList lines = response.trimmed().split(QStringLiteral("\n"));
    if (lines.size() >= 2)
    {
        bool ok = false;
        float conf = lines[1].trimmed().toFloat(&ok);
        if (ok)
            return qBound(0.0f, conf, 1.0f);
    }

    return 0.5f;  // 解析失败，中性置信度
}
