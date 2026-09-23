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

/// @file ConversationHistory.h
/// @brief 对话历史 —— 内存中的环形消息队列，带 token 预算管理
///
/// 职责：
///   - 存储 system / user / assistant 消息序列
///   - toMessages() 时做滑动窗口截断（按 token 预算从最早历史丢弃）
///   - 支持 summarizeOldest() 用摘要替代旧消息
///
/// 使用方式：
///   @code
///     ConversationHistory history;
///     history.pushSystem("You are a CAD assistant.");
///     history.pushUser("画一个圆");
///     // ... LLM 处理 ...
///     history.pushAssistant("已创建圆。");
///     auto msgs = history.toMessages("You are a CAD assistant.", 8192);
///     // msgs 包含 system + 历史窗口内的 user/assistant
///   @endcode

#ifndef CONVERSATIONHISTORY_H
#define CONVERSATIONHISTORY_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVector>

/// @brief 一条消息
struct MessageEntry
{
    QString   role;       ///< "system" | "user" | "assistant"
    QString   content;    ///< 消息文本
    QDateTime timestamp;  ///< 记录时间

    MessageEntry() = default;

    MessageEntry(const QString& r, const QString& c)
        : role(r), content(c), timestamp(QDateTime::currentDateTime())
    {}
};

/// @brief 对话历史 —— 内存中的消息队列，带 token 预算管理
///
/// 内部维护一个消息列表，支持滑动窗口截断。
/// 默认保留最近 20 轮对话（一轮 = user + assistant）。
/// toMessages() 生成可直接传给 LLM API 的消息数组，超出 token 预算时自动从最早历史截断。
class ConversationHistory : public QObject
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param parent 父 QObject
    explicit ConversationHistory(QObject* parent = nullptr);

    // ---- 写入 ----

    /// @brief 记录一条 system 消息
    /// @param content 系统指令文本
    ///
    /// system 消息不参与轮次计数，也不被滑动窗口丢弃。
    /// 同一时刻只保留最新一条 system 消息（后来的覆盖前面的）。
    void pushSystem(const QString& content);

    /// @brief 记录一条 user 消息
    /// @param content 用户输入文本
    void pushUser(const QString& content);

    /// @brief 记录一条 assistant 消息
    /// @param content AI 回复文本
    void pushAssistant(const QString& content);

    // ---- 读取 ----

    /// @brief 返回给 LLM 的消息数组（system + 滑动窗口内的历史）
    /// @param systemPrompt 当前系统指令（注入为第一条消息）
    /// @param maxTokens    最大 token 预算（超限时从最早的历史截断）
    /// @return 按时间升序排列的消息数组，可直接序列化为 API 请求的 messages 字段
    QVector<MessageEntry> toMessages(const QString& systemPrompt,
                                     int maxTokens = 8192) const;

    /// @brief 获取所有消息（用于 UI 展示 / 持久化）
    const QVector<MessageEntry>& allMessages() const;

    /// @brief 获取最近 N 条用户消息文本（倒序：最新在前）
    /// @param n 最大返回条数（默认 2）
    /// @return 最近 N 条 role=="user" 的消息内容，按时间降序排列
    ///
    /// 用于 LLM 意图分类时注入对话上下文。
    QStringList recentUserMessages(int n = 2) const;

    /// @brief 用给定的消息列表替换当前全部历史（用于加载旧会话）
    /// @param messages 按时间升序排列的消息列表
    void loadFrom(const QVector<MessageEntry>& messages);

    // ---- 管理 ----

    /// @brief 清空所有消息（保留 system 消息）
    void clear();

    /// @brief 设置最大保留轮数
    /// @param turns 轮数，默认 20
    void setMaxTurns(int turns);

    /// @brief 获取当前最大轮数设置
    int maxTurns() const;

    // ---- 采样/压缩策略 ----

    /// @brief 当历史超限时，用 summarize 替代旧消息
    /// @param summary 摘要文本（由上层 LLM 生成，或 "…" 兜底）
    ///
    /// 将最旧的 user + assistant 对替换为一条 system 消息（角色保持 "system"，
    /// 内容为摘要），以节省 token 预算。
    void summarizeOldest(const QString& summary);

signals:
    /// @brief 历史被截断时发射
    /// @param removedCount 被移除的消息条数
    void historyTrimmed(int removedCount);

private:
    /// @brief 估算一条消息的 token 数
    ///
    /// 简单启发式：中文字符 × 2 + ASCII 字符 ÷ 4
    /// （DeepSeek tokenizer 对中文约 1.5~2 token/字，英文约 0.25 token/字）
    static int estimateTokens(const QString& text);

    /// @brief 按轮次上限截断内部消息列表
    void enforceTurnLimit();

    QVector<MessageEntry> m_messages;  ///< 消息列表（按时间升序）
    QString               m_systemMsg; ///< 最新 system 消息（独立存储，不参与轮次计数）
    int                   m_maxTurns = 20;  ///< 最大保留轮数
};

#endif // CONVERSATIONHISTORY_H
