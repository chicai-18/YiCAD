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

/// @file AILLMClassifier.h
/// @brief LLM 意图分类器 —— 通过 DeepSeek API 做 QA / Modeling / Mixed 三分类
///
/// 职责：
///   - 构造极简路由 prompt（~80 tokens），temperature=0，max_tokens=16
///   - 异步调用 DeepSeekProvider::sendMessage()，通过回调返回分类结果
///   - 内部 3s 超时保护，超时/网络错误/解析失败 → classificationFailed 信号
///
/// 使用方式：
///   @code
///     auto* classifier = new AILLMClassifier(this);
///     classifier->classify("draw a circle", recentMsgs,
///         [](IntentType intent, float confidence) {
///             // 处理分类结果
///         });
///   @endcode

#ifndef AILLMCLASSIFIER_H
#define AILLMCLASSIFIER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>
#include <memory>

#include "AIIntentRouter.h"  // IntentType

class DeepSeekProvider;

class AILLMClassifier : public QObject
{
    Q_OBJECT

public:
    explicit AILLMClassifier(QObject* parent = nullptr);

    /// @brief 异步 LLM 意图分类（始终返回 QA / Modeling / Mixed，不应返回 Uncertain）
    /// @param input      用户原始输入
    /// @param recentMsgs 最近 N 条用户消息（用于上下文感知，可为空）
    /// @param callback   完成回调 (IntentType, confidence 0.0~1.0)
    /// @return 请求序列号（用于竞态防护）
    int classify(const QString& input,
                 const QStringList& recentMsgs,
                 std::function<void(IntentType, float)> callback);

    /// @brief LLM API 是否可用（API Key 已配置）
    bool isAvailable() const;

    /// @brief 设置是否启用对话上下文（默认 true）
    void setContextEnabled(bool enabled);

signals:
    /// @brief 分类失败时发射（超时、网络错误、解析失败）
    void classificationFailed(const QString& reason);

private:
    /// @brief 构建分类 prompt（system 消息）
    QString buildRouterPrompt(const QString& input,
                              const QStringList& recentMsgs) const;

    /// @brief 从 LLM 响应中提取意图类型
    static IntentType parseClassification(const QString& llmResponse);

    /// @brief 从 LLM 响应中提取置信度
    static float parseConfidence(const QString& llmResponse);

    std::unique_ptr<DeepSeekProvider> m_provider;
    bool m_contextEnabled = true;
    int m_nextSeq = 0;  ///< 请求序列号，单调递增
};

#endif // AILLMCLASSIFIER_H
