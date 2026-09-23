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

/// @file DeepSeekProvider.h
/// @brief DeepSeek API 调用提供者 —— 最小可用文本请求
///
/// 职责：
///   - 从 LLMSettingsService 读取 BaseUrl / Model / API Key / Timeout
///   - 发起一次 /chat/completions 文本请求
///   - 返回 AI 回复文本或错误信息
///
/// 使用方式：
///   @code
///     auto* provider = new DeepSeekProvider(this);
///     connect(provider, &DeepSeekProvider::responseReceived,
///             dialog, &AIDialog::appendMessage);
///     connect(provider, &DeepSeekProvider::errorOccurred,
///             this,   &MyClass::handleError);
///     provider->sendMessage("你好");
///   @endcode
///
/// 错误覆盖：
///   - 未配置 Key
///   - 网络失败
///   - 返回格式异常

#ifndef DEEPSEEKPROVIDER_H
#define DEEPSEEKPROVIDER_H

#include <QObject>
#include <QPointer>
#include <QString>
#include <QVector>

struct MessageEntry;

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class DeepSeekProvider : public QObject
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父对象指针
    explicit DeepSeekProvider(QObject* parent = nullptr);

    /// @brief 析构函数
    ~DeepSeekProvider() override;

    /// @brief 发送一条用户消息到 DeepSeek API（无历史上下文）
    /// @param [in] userMessage 用户输入文本
    ///
    /// 调用前应确保 LLMSettingsService 已初始化且 API Key 已配置。
    /// 结果通过 responseReceived / errorOccurred 信号异步返回。
    void sendMessage(const QString& userMessage);

    /// @brief 发送一条用户消息，附带完整对话历史
    /// @param [in] userMessage     用户输入文本
    /// @param [in] historyMessages 历史消息数组（按时间升序，不含当前 userMessage）
    ///
    /// 此重载将 historyMessages + userMessage 合并为完整 messages 数组
    /// 发送给 API，使 LLM 能感知前文。
    void sendMessage(const QString& userMessage,
                     const QVector<MessageEntry>& historyMessages);

    /// @brief 发送消息，附带完整参数覆盖（用于分类器等需要精确控制参数的场景）
    /// @param [in] userMessage     用户输入文本
    /// @param [in] historyMessages 历史消息数组（可为空）
    /// @param [in] systemPrompt    覆盖 system 消息（为空则使用默认 CAD assistant prompt）
    /// @param [in] temperature     覆盖 temperature（< 0 表示使用全局默认）
    /// @param [in] maxTokens       覆盖 max_tokens（0 表示不限制）
    void sendMessage(const QString& userMessage,
                     const QVector<MessageEntry>& historyMessages,
                     const QString& systemPrompt,
                     double temperature,
                     int maxTokens);

signals:
    /// @brief 成功收到 AI 回复（流结束后一次性发射，作为兜底）
    /// @param [in] responseText AI 返回的文本内容
    void responseReceived(const QString& responseText);

    /// @brief 流式接收过程中检测到完整 JSON 指令（边收边发射）
    /// @param [in] commandJson 完整的 JSON 命令字符串（单个 {…} 对象）
    ///
    /// 连接到该信号可实现增量执行：每个指令到达即执行，无需等待全流结束。
    void commandReady(const QString& commandJson);

    /// @brief 请求过程中发生错误
    /// @param [in] errorMessage 人类可读的错误描述
    void errorOccurred(const QString& errorMessage);

private slots:
    /// @brief 处理网络回复完成事件
    /// @param [in] reply 完成的网络回复对象
    void onReplyFinished(QNetworkReply* reply);

    /// @brief 处理流式响应的增量数据到达
    void onReadyRead();

    /// @brief 静默超时回调，abort 当前流式请求
    void onSilenceTimeout();

private:
    /// @brief 构建 Chat Completions 请求体 JSON（单条消息，无历史）
    /// @param [in] model       模型名称
    /// @param [in] userMessage 用户消息文本
    /// @param [in] temperature 温度参数
    /// @return JSON 字节数组
    QByteArray buildRequestBody(const QString& model,
                                const QString& userMessage,
                                double temperature) const;

    /// @brief 构建 Chat Completions 请求体 JSON（带完整历史消息）
    /// @param [in] model       模型名称
    /// @param [in] messages    历史消息数组 + 当前 user 消息
    /// @param [in] temperature 温度参数
    /// @return JSON 字节数组
    QByteArray buildRequestBody(const QString& model,
                                const QVector<MessageEntry>& messages,
                                double temperature) const;

    /// @brief 构建 Chat Completions 请求体 JSON（完整参数覆盖）
    /// @param [in] model        模型名称
    /// @param [in] messages     历史消息数组 + 当前 user 消息
    /// @param [in] systemPrompt system 消息内容（插入为第一条消息）
    /// @param [in] temperature  温度参数
    /// @param [in] maxTokens    max_tokens（0 表示不限制）
    /// @return JSON 字节数组
    QByteArray buildRequestBody(const QString& model,
                                const QVector<MessageEntry>& messages,
                                const QString& systemPrompt,
                                double temperature,
                                int maxTokens) const;

    /// @brief 从 DeepSeek 响应 JSON 中提取 assistant 文本
    /// @param [in] responseData 响应体 JSON
    /// @param [out] outText     提取的文本（成功时）
    /// @return true 解析成功
    static bool parseResponseContent(const QByteArray& responseData,
                                     QString& outText);

    /// @brief 从 DeepSeek 错误响应 JSON 中提取错误信息
    /// @param [in] responseData 响应体 JSON
    /// @param [out] outError    提取的错误描述（成功时）
    /// @return true 识别为错误响应格式
    static bool parseErrorResponse(const QByteArray& responseData,
                                   QString& outError);

    /// @brief 从 SSE delta JSON 中提取 content 和 reasoning_content 增量
    /// @param [in]  json         delta 对象的 JSON 字节数组
    /// @param [out] outContent   提取的 content 增量
    /// @param [out] outReasoning 提取的 reasoning_content 增量（可为 nullptr）
    /// @return true JSON 解析成功
    static bool parseSseDelta(const QByteArray& json,
                              QString& outContent,
                              QString* outReasoning);

    /// @brief 解析单个 SSE 帧（以 \n\n 分隔的完整数据块）
    /// @param [in] chunk SSE 帧字节数组（不含帧分隔符）
    void processSseChunk(const QByteArray& chunk);

    /// @brief 从累积内容中提取完整 JSON 对象（括号深度扫描）
    ///
    /// 扫描 m_accumulatedContent，检测第一个 { } 平衡闭合的 JSON 子串，
    /// 从 buffer 中移除并发射 commandReady。
    /// 字符串内的括号通过反斜杠转义和引号状态机正确处理。
    void extractCompleteJson();

private:
    QNetworkAccessManager* m_networkManager;  ///< Qt 网络管理器
    QByteArray m_streamBuffer;                ///< SSE 行缓冲（跨 readyRead 累积不完整帧）
    QString m_accumulatedContent;             ///< 累积的 model content 增量文本（会被 extractCompleteJson 消费）
    QString m_rawContent;                     ///< 完整累积文本（不被消费，[DONE] 时完整发射）
    QString m_reasoningBuffer;                ///< 累积的 reasoning_content 增量文本
    QTimer* m_silenceTimer;                   ///< 静默超时定时器
    QPointer<QNetworkReply> m_currentReply;   ///< 当前流式 reply（QPointer 自动置 null）
};

#endif // DEEPSEEKPROVIDER_H
