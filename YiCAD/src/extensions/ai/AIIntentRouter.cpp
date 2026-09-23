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

/// @file AIIntentRouter.cpp
/// @brief AIIntentRouter 实现 —— 手动模式兜底 + 自动模式占位
///
/// 关键词路由代码已完全移除。意图分类由 AILLMClassifier 异步完成。

#include "AIIntentRouter.h"

// ============================================================================
// 构造 / 析构
// ============================================================================

AIIntentRouter::AIIntentRouter(QObject* parent)
    : QObject(parent)
{
}

AIIntentRouter::~AIIntentRouter() = default;

// ============================================================================
// 公开接口 —— 路由
// ============================================================================

RouterResult AIIntentRouter::route(const QString& input,
                                   const QString& manualMode) const
{
    // ------ 空输入保护 ------
    if (input.trimmed().isEmpty()) {
        RouterResult r;
        r.intent            = IntentType::Uncertain;
        r.confidence        = 0.0f;
        r.reasoning         = tr("Input is empty.");
        r.suggestedAction   = tr("Please enter your question or modeling command.");
        r.needsConfirmation = false;
        return r;
    }

    // ------ 手动模式兜底（不变） ------
    if (manualMode == QStringLiteral("qa")) {
        RouterResult r;
        r.intent            = IntentType::QA;
        r.confidence        = 1.0f;
        r.reasoning         = tr("Manually set to QA mode.");
        r.suggestedAction   = tr("Will enter QA pipeline (RAG).");
        r.needsConfirmation = false;
        return r;
    }

    if (manualMode == QStringLiteral("modeling")) {
        RouterResult r;
        r.intent            = IntentType::Modeling;
        r.confidence        = 1.0f;
        r.reasoning         = tr("Manually set to Modeling mode.");
        r.suggestedAction   = tr("Will enter modeling execution pipeline.");
        r.needsConfirmation = false;
        return r;
    }

    // ------ 自动模式：返回占位结果，由 AIPipeline 异步触发 LLM 分类 ------
    RouterResult r;
    r.intent            = IntentType::Uncertain;
    r.confidence        = 0.0f;
    r.reasoning         = tr("Awaiting LLM classification...");
    r.suggestedAction   = QString();
    r.needsConfirmation = false;
    return r;
}

// ============================================================================
// 私有方法 —— 预处理
// ============================================================================

QString AIIntentRouter::preprocess(const QString& raw)
{
    QString s = raw.trimmed().simplified();

    // 全角转半角
    s.replace(QChar(0xFF1F), QChar('?'));   // ？ → ?
    s.replace(QChar(0x3000), QChar(' '));   // 全角空格 → 半角空格

    return s;
}
