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

/// @file ConversationHistory.cpp
/// @brief ConversationHistory 实现

#include "ConversationHistory.h"

#include <algorithm>

// ============================================================================
// 构造
// ============================================================================

ConversationHistory::ConversationHistory(QObject* parent)
    : QObject(parent)
{
}

// ============================================================================
// 写入
// ============================================================================

void ConversationHistory::pushSystem(const QString& content)
{
    m_systemMsg = content;
}

void ConversationHistory::pushUser(const QString& content)
{
    m_messages.append(MessageEntry(QStringLiteral("user"), content));
    enforceTurnLimit();
}

void ConversationHistory::pushAssistant(const QString& content)
{
    m_messages.append(MessageEntry(QStringLiteral("assistant"), content));
    enforceTurnLimit();
}

// ============================================================================
// 读取
// ============================================================================

QVector<MessageEntry> ConversationHistory::toMessages(const QString& systemPrompt,
                                                       int maxTokens) const
{
    QVector<MessageEntry> result;

    // 1. system 消息：优先使用传入的 systemPrompt，否则使用内部存储的
    const QString sys = systemPrompt.isEmpty() ? m_systemMsg : systemPrompt;
    if (!sys.isEmpty())
    {
        result.append(MessageEntry(QStringLiteral("system"), sys));
    }

    // 2. 计算 system 消息的 token 数
    int usedTokens = estimateTokens(sys);

    // 3. 从最早的历史开始，反向遍历确定截断点
    //    策略：保留尽可能多的最近消息，丢弃最早的消息
    int totalHistoryTokens = 0;
    for (int i = m_messages.size() - 1; i >= 0; --i)
    {
        const int t = estimateTokens(m_messages[i].content);
        if (usedTokens + totalHistoryTokens + t > maxTokens)
        {
            // 截断点：从 i+1 开始保留
            const int startIdx = i + 1;
            const int removedCount = startIdx;
            result.reserve(result.size() + (m_messages.size() - startIdx));
            for (int j = startIdx; j < m_messages.size(); ++j)
            {
                result.append(m_messages[j]);
            }

            if (removedCount > 0)
            {
                // 注意：这里无法发射信号，因为方法是 const。
                // 截断信息由调用方根据返回值自行判断。
            }
            return result;
        }
        totalHistoryTokens += t;
    }

    // 4. 全部保留
    result.reserve(result.size() + m_messages.size());
    for (const auto& msg : m_messages)
    {
        result.append(msg);
    }
    return result;
}

const QVector<MessageEntry>& ConversationHistory::allMessages() const
{
    return m_messages;
}

QStringList ConversationHistory::recentUserMessages(int n) const
{
    QStringList result;
    result.reserve(n);

    // 从最新到最旧遍历 m_messages，收集 role=="user" 的消息
    for (int i = m_messages.size() - 1; i >= 0 && result.size() < n; --i)
    {
        if (m_messages[i].role == QStringLiteral("user"))
        {
            result.append(m_messages[i].content);
        }
    }

    return result;
}

void ConversationHistory::loadFrom(const QVector<MessageEntry>& messages)
{
    m_messages.clear();
    // 分离 system 消息与其他消息
    for (const auto& msg : messages)
    {
        if (msg.role == QStringLiteral("system"))
        {
            m_systemMsg = msg.content;
        }
        else
        {
            m_messages.append(msg);
        }
    }
    enforceTurnLimit();
}

// ============================================================================
// 管理
// ============================================================================

void ConversationHistory::clear()
{
    m_messages.clear();
    // m_systemMsg 不清空 —— 系统指令通常不变
}

void ConversationHistory::setMaxTurns(int turns)
{
    if (turns < 1)
        turns = 1;
    m_maxTurns = turns;
    enforceTurnLimit();
}

int ConversationHistory::maxTurns() const
{
    return m_maxTurns;
}

// ============================================================================
// 压缩策略
// ============================================================================

void ConversationHistory::summarizeOldest(const QString& summary)
{
    // 找到最旧的一个 user + assistant 对
    if (m_messages.size() < 2)
        return;

    // 查找第一对连续的 user → assistant
    for (int i = 0; i < m_messages.size() - 1; ++i)
    {
        if (m_messages[i].role == QStringLiteral("user") &&
            m_messages[i + 1].role == QStringLiteral("assistant"))
        {
            // 移除这一对，插入摘要（作为 system 消息）
            // 注意：摘要消息的角色保持 "system"，不会与用户的 system 指令混淆
            MessageEntry summaryMsg(QStringLiteral("system"),
                                    QStringLiteral("[Summary of earlier conversation]\n%1").arg(summary));
            m_messages.remove(i, 2);
            m_messages.insert(i, summaryMsg);
            emit historyTrimmed(1);  // 1 对被合并
            return;
        }
    }
}

// ============================================================================
// Token 估算
// ============================================================================

int ConversationHistory::estimateTokens(const QString& text)
{
    int tokens = 0;
    for (const QChar& ch : text)
    {
        const ushort uc = ch.unicode();
        if (uc > 0x7F)
        {
            // CJK / fullwidth / 一般非 ASCII：按 2 token 估算
            tokens += 2;
        }
        else
        {
            // ASCII：约 4 字符 = 1 token
            // 用整数计数避免浮点，累积满 4 加 1
        }
    }

    // ASCII 字符单独统计
    int asciiCount = 0;
    for (const QChar& ch : text)
    {
        if (ch.unicode() <= 0x7F)
        {
            ++asciiCount;
        }
    }
    tokens += (asciiCount + 3) / 4;  // 向上取整

    return tokens;
}

// ============================================================================
// 轮次截断
// ============================================================================

void ConversationHistory::enforceTurnLimit()
{
    // 统计完整轮次数（一个 user 算一轮）
    int turns = 0;
    for (const auto& msg : m_messages)
    {
        if (msg.role == QStringLiteral("user"))
        {
            ++turns;
        }
    }

    if (turns <= m_maxTurns)
        return;

    // 从开头丢弃多余的轮次
    int turnsToDrop = turns - m_maxTurns;
    int dropCount = 0;
    int removedMsgs = 0;

    for (int i = 0; i < m_messages.size() && dropCount < turnsToDrop; ++i)
    {
        if (m_messages[i].role == QStringLiteral("user"))
        {
            ++dropCount;
        }
        ++removedMsgs;
    }

    if (removedMsgs > 0)
    {
        m_messages.remove(0, removedMsgs);
        emit historyTrimmed(removedMsgs);
    }
}
