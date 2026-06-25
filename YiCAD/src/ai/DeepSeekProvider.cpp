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

/// @file DeepSeekProvider.cpp
/// @brief DeepSeekProvider 实现 —— 最小可用文本请求

#include "DeepSeekProvider.h"
#include "ConversationHistory.h"
#include "LLMSettingsService.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QTimer>

// ---------------------------------------------------------------------------
// 构造 / 析构
// ---------------------------------------------------------------------------

DeepSeekProvider::DeepSeekProvider(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_silenceTimer(new QTimer(this))
{
    // 静默超时定时器：单次触发，每次 readyRead 重置
    m_silenceTimer->setSingleShot(true);
    connect(m_silenceTimer, &QTimer::timeout,
            this, &DeepSeekProvider::onSilenceTimeout);
}

DeepSeekProvider::~DeepSeekProvider() = default;

// ---------------------------------------------------------------------------
// 公共接口
// ---------------------------------------------------------------------------

void DeepSeekProvider::sendMessage(const QString& userMessage)
{
    // 委托到三参数重载，传空历史
    sendMessage(userMessage, QVector<MessageEntry>());
}

void DeepSeekProvider::sendMessage(const QString& userMessage,
                                   const QVector<MessageEntry>& historyMessages)
{
    // 委托到五参数重载，使用全局默认设置
    sendMessage(userMessage, historyMessages, QString(), -1.0, 0);
}

void DeepSeekProvider::sendMessage(const QString& userMessage,
                                   const QVector<MessageEntry>& historyMessages,
                                   const QString& systemPrompt,
                                   double temperature,
                                   int maxTokens)
{
    // ---- 1. 读取配置 ----
    LLMSettingsService* svc = LLMSettingsService::instance();
    if (!svc || !svc->isInitialized())
    {
        emit errorOccurred(tr("LLM configuration service is not initialized."));
        return;
    }

    // ---- 2. 校验 API Key ----
    if (!svc->hasApiKey())
    {
        emit errorOccurred(tr("API Key not configured. Please set it in AI settings."));
        return;
    }

    const QString baseUrl     = svc->baseUrl();
    const QString model       = svc->model();
    const QString apiKey      = svc->apiKey();
    const int     timeoutSecs = svc->timeoutSecs();
    const double  effectiveTemperature = (temperature < 0.0) ? svc->temperature() : temperature;

    // ---- 3. 构建完整消息数组：历史 + 当前用户输入 ----
    QVector<MessageEntry> allMessages = historyMessages;
    allMessages.append(MessageEntry(QStringLiteral("user"), userMessage));

    // ---- 4. 构建请求 ----
    const QUrl url(baseUrl + "/chat/completions");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/json");
    request.setRawHeader("Authorization",
                         "Bearer " + apiKey.toUtf8());
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(timeoutSecs * 1000);  // Qt 5.15+
#endif

    // 如果传入了 systemPrompt 或 maxTokens > 0，使用完整参数覆盖的 buildRequestBody
    const bool hasParamOverride = !systemPrompt.isEmpty() || maxTokens > 0;
    const QByteArray body = hasParamOverride
        ? buildRequestBody(model, allMessages, systemPrompt, effectiveTemperature, maxTokens)
        : buildRequestBody(model, allMessages, effectiveTemperature);

    // ---- 5. 发起 POST（流式模式） ----
    qDebug().noquote() << "[AI] >>> POST" << url.toString();
    qDebug().noquote() << "[AI] >>> Body:" << QString::fromUtf8(body);
    QNetworkReply* reply = m_networkManager->post(request, body);

    // 初始化流式状态
    m_streamBuffer.clear();
    m_accumulatedContent.clear();
    m_rawContent.clear();
    m_reasoningBuffer.clear();
    m_currentReply = reply;

    // 按 reply 连接流式信号（不使用 manager 级 finished）
    connect(reply, &QNetworkReply::readyRead,
            this, &DeepSeekProvider::onReadyRead);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });

    // 启动静默超时检测
    m_silenceTimer->setInterval(timeoutSecs * 1000);
    m_silenceTimer->start();
}

// ---------------------------------------------------------------------------
// 槽：网络回复完成
// ---------------------------------------------------------------------------

void DeepSeekProvider::onReplyFinished(QNetworkReply* reply)
{
    if (!reply)
    {
        return;
    }

    // 防御性检查：只处理当前活跃的流式 reply
    if (reply != m_currentReply)
    {
        reply->deleteLater();
        return;
    }

    // 停止静默超时检测
    m_silenceTimer->stop();
    m_currentReply.clear();

    const int     statusCode  = reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body      = reply->readAll();
    qDebug().noquote() << "[AI] <<< Response (HTTP" << statusCode << "):"
                        << QString::fromUtf8(body);
    const QString     errorStr = reply->errorString();

    // ---- 情况 1：正常完成 ----
    if (reply->error() == QNetworkReply::NoError)
    {
        // 流结束已由 processSseChunk 处理（finish_reason / [DONE]），
        // 此处仅做兜底：若流式解析未产出任何内容，尝试非流式回退解析
        if (m_accumulatedContent.isEmpty() && m_reasoningBuffer.isEmpty() && !body.isEmpty())
        {
            QString content;
            if (parseResponseContent(body, content))
            {
                emit responseReceived(content);
            }
        }
        reply->deleteLater();
        return;
    }

    // ---- 情况 2：操作被取消（超时 abort） ----
    if (reply->error() == QNetworkReply::OperationCanceledError)
    {
        if (!m_rawContent.isEmpty())
        {
            // 已有部分内容 — 返回已累积内容 + 截断警告
            emit responseReceived(m_rawContent.trimmed()
                                  + tr("\n\n[⚠ Request cancelled before completion]"));
        }
        else if (!m_reasoningBuffer.isEmpty())
        {
            emit responseReceived(m_reasoningBuffer
                                  + tr("\n\n[⚠ Request cancelled before completion]"));
        }
        else
        {
            // 完全无数据 — 静默超时
            emit errorOccurred(
                tr("Request timed out (no data received within the timeout period). "
                   "Please try simplifying your request or increasing the timeout setting."));
        }
        reply->deleteLater();
        return;
    }

    // ---- 情况 3：网络/HTTP 错误（保持现有逻辑） ----
    if (statusCode > 0)
    {
        // 尝试解析 DeepSeek 错误 JSON 格式
        QString deepseekError;
        if (parseErrorResponse(body, deepseekError))
        {
            emit errorOccurred(tr("DeepSeek API error (HTTP %1): %2")
                                .arg(statusCode)
                                .arg(deepseekError));
        }
        else
        {
            // 返回非 JSON 错误（如 HTML 页面）
            QString preview = QString::fromUtf8(body.left(300));
            emit errorOccurred(tr("HTTP %1: %2")
                                .arg(statusCode)
                                .arg(preview.isEmpty() ? errorStr : preview));
        }
    }
    else
    {
        // 纯网络错误：无法连接、DNS 等
        emit errorOccurred(tr("Network request failed: %1").arg(errorStr));
    }

    reply->deleteLater();
}

// ---------------------------------------------------------------------------
// 请求体构建
// ---------------------------------------------------------------------------

QByteArray DeepSeekProvider::buildRequestBody(const QString& model,
                                              const QString& userMessage,
                                              double temperature) const
{
    QJsonObject systemMsg;
    systemMsg["role"]    = QStringLiteral("system");
    systemMsg["content"] = QStringLiteral("You are a helpful CAD assistant.");

    QJsonObject userMsg;
    userMsg["role"]    = QStringLiteral("user");
    userMsg["content"] = userMessage;

    QJsonArray messages;
    messages.append(systemMsg);
    messages.append(userMsg);

    QJsonObject root;
    root["model"]       = model;
    root["messages"]    = messages;
    root["temperature"] = temperature;
    root["stream"]      = true;

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact);
}

QByteArray DeepSeekProvider::buildRequestBody(const QString& model,
                                              const QVector<MessageEntry>& messages,
                                              double temperature) const
{
    QJsonArray messagesJson;
    for (const auto& msg : messages)
    {
        QJsonObject msgObj;
        msgObj["role"]    = msg.role;
        msgObj["content"] = msg.content;
        messagesJson.append(msgObj);
    }

    QJsonObject root;
    root["model"]       = model;
    root["messages"]    = messagesJson;
    root["temperature"] = temperature;
    root["stream"]      = true;

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact);
}

QByteArray DeepSeekProvider::buildRequestBody(const QString& model,
                                              const QVector<MessageEntry>& messages,
                                              const QString& systemPrompt,
                                              double temperature,
                                              int maxTokens) const
{
    QJsonArray messagesJson;

    // 注入 system prompt（如果提供）
    if (!systemPrompt.isEmpty())
    {
        QJsonObject sysMsg;
        sysMsg["role"]    = QStringLiteral("system");
        sysMsg["content"] = systemPrompt;
        messagesJson.append(sysMsg);
    }

    for (const auto& msg : messages)
    {
        QJsonObject msgObj;
        msgObj["role"]    = msg.role;
        msgObj["content"] = msg.content;
        messagesJson.append(msgObj);
    }

    QJsonObject root;
    root["model"]       = model;
    root["messages"]    = messagesJson;
    root["temperature"] = temperature;
    root["stream"]      = true;

    if (maxTokens > 0)
    {
        root["max_tokens"] = maxTokens;
    }

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact);
}

// ---------------------------------------------------------------------------
// 响应解析
// ---------------------------------------------------------------------------

bool DeepSeekProvider::parseResponseContent(const QByteArray& responseData,
                                            QString& outText)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        return false;
    }

    const QJsonObject root = doc.object();

    // DeepSeek / OpenAI 格式: { "choices": [ { "message": { "content": "..." } } ] }
    const QJsonArray choices = root["choices"].toArray();
    if (choices.isEmpty())
    {
        return false;
    }

    const QJsonObject firstChoice = choices[0].toObject();
    const QJsonObject message     = firstChoice["message"].toObject();

    // 1. 优先读取 content（标准模型）
    if (message.contains("content"))
    {
        const QJsonValue contentVal = message["content"];
        if (contentVal.isString())
        {
            const QString text = contentVal.toString().trimmed();
            if (!text.isEmpty())
            {
                outText = text;
                return true;
            }
        }
    }

    // 2. content 为空 → 回退到 reasoning_content（推理模型如 deepseek-v4-pro）
    if (message.contains("reasoning_content"))
    {
        const QJsonValue reasoningVal = message["reasoning_content"];
        if (reasoningVal.isString())
        {
            const QString text = reasoningVal.toString().trimmed();
            if (!text.isEmpty())
            {
                outText = text;
                return true;
            }
        }
    }

    return false;
}

bool DeepSeekProvider::parseErrorResponse(const QByteArray& responseData,
                                          QString& outError)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        return false;
    }

    const QJsonObject root = doc.object();

    // DeepSeek / OpenAI 错误格式: { "error": { "message": "..." } }
    if (!root.contains("error"))
    {
        return false;
    }

    const QJsonObject errorObj = root["error"].toObject();
    outError = errorObj["message"].toString(
        tr("(no message)"));

    // 附加 type / code 信息
    const QString type = errorObj["type"].toString();
    const QString code = errorObj["code"].toString();
    if (!type.isEmpty())
    {
        outError += tr(" [%1]").arg(type);
    }
    if (!code.isEmpty() && code != type)
    {
        outError += tr(" code=%1").arg(code);
    }

    return true;
}

// ---------------------------------------------------------------------------
// SSE 流式解析
// ---------------------------------------------------------------------------

bool DeepSeekProvider::parseSseDelta(const QByteArray& json,
                                     QString& outContent,
                                     QString* outReasoning)
{
    // 解析格式: {"choices":[{"delta":{"content":"...","reasoning_content":"..."},"finish_reason":null}]}
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        return false;
    }

    const QJsonObject root = doc.object();
    const QJsonArray choices = root["choices"].toArray();
    if (choices.isEmpty())
    {
        return false;
    }

    const QJsonObject firstChoice = choices[0].toObject();
    const QJsonObject delta = firstChoice["delta"].toObject();

    // 1. 提取 content 增量
    if (delta.contains("content"))
    {
        const QJsonValue contentVal = delta["content"];
        if (contentVal.isString())
        {
            outContent = contentVal.toString();
        }
    }

    // 2. 提取 reasoning_content 增量（推理模型思维链）
    if (outReasoning && delta.contains("reasoning_content"))
    {
        const QJsonValue reasoningVal = delta["reasoning_content"];
        if (reasoningVal.isString())
        {
            *outReasoning = reasoningVal.toString();
        }
    }

    return true;
}

void DeepSeekProvider::onReadyRead()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply || reply != m_currentReply)
    {
        return;
    }

    // 重置静默超时
    m_silenceTimer->start();

    // 读取新数据追加到行缓冲
    QByteArray newData = reply->readAll();
    m_streamBuffer.append(newData);

    // 标准化换行符：\r\n 和 \r 统一转为 \n
    m_streamBuffer.replace("\r\n", "\n");
    m_streamBuffer.replace('\r', '\n');

    // 按 "\n\n" 分割 SSE 帧，最后一段为不完整尾部
    while (true)
    {
        const int idx = m_streamBuffer.indexOf("\n\n");
        if (idx < 0)
        {
            break;
        }

        const QByteArray frame = m_streamBuffer.left(idx);
        m_streamBuffer.remove(0, idx + 2);  // 移除已处理的帧 + 分隔符

        if (!frame.isEmpty())
        {
            processSseChunk(frame);
        }
    }
}

void DeepSeekProvider::onSilenceTimeout()
{
    if (m_currentReply && m_currentReply->isRunning())
    {
        m_currentReply->abort();
        // abort 触发 finished → onReplyFinished 统一处理取消逻辑
    }
}

void DeepSeekProvider::extractCompleteJson()
{
    // 括号深度扫描：从 m_accumulatedContent 中提取完整 JSON 对象
    // 每当检测到一个 { } 平衡闭合的顶层 JSON 对象，解析并发射 commandReady，
    // 然后从 buffer 前端移除，继续扫描。

    int depth = 0;
    int objStart = -1;
    bool inString = false;
    bool escapeNext = false;

    const int len = m_accumulatedContent.length();

    for (int i = 0; i < len; ++i)
    {
        const QChar ch = m_accumulatedContent.at(i);

        if (escapeNext)
        {
            escapeNext = false;
            continue;
        }

        if (ch == QLatin1Char('\\') && inString)
        {
            escapeNext = true;
            continue;
        }

        if (ch == QLatin1Char('"'))
        {
            inString = !inString;
            continue;
        }

        if (inString)
        {
            continue;
        }

        if (ch == QLatin1Char('{'))
        {
            if (depth == 0)
            {
                objStart = i;
            }
            ++depth;
        }
        else if (ch == QLatin1Char('}'))
        {
            --depth;
            if (depth == 0 && objStart >= 0)
            {
                // 找到一个完整 JSON 对象
                const int objLen = i - objStart + 1;
                const QString jsonStr = m_accumulatedContent.mid(objStart, objLen);

                // 验证是否为合法 JSON
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
                if (parseError.error == QJsonParseError::NoError && doc.isObject())
                {
                    // 发射该指令，从 buffer 中移除
                    qDebug().noquote() << "[AI] <<< Complete JSON command:" << jsonStr;
                    emit commandReady(jsonStr);
                    m_accumulatedContent.remove(objStart, objLen);

                    // 递归继续扫描（buffer 已变，重新开始）
                    extractCompleteJson();
                    return;
                }

                // JSON 不合法（可能是字符串中的假括号匹配），重置，继续扫描
                objStart = -1;
            }
            else if (depth < 0)
            {
                // 多余的 }，重置深度（容错）
                depth = 0;
                objStart = -1;
            }
        }
    }
}

void DeepSeekProvider::processSseChunk(const QByteArray& chunk)
{
    // 去掉前缀空白，按 "\n" 分割为行
    const QByteArray trimmed = chunk.trimmed();
    if (trimmed.isEmpty())
    {
        return;
    }

    const QList<QByteArray> lines = trimmed.split('\n');
    for (const QByteArray& line : lines)
    {
        const QByteArray trimmedLine = line.trimmed();

        // SSE 注释行（以 ":" 开头）
        if (trimmedLine.startsWith(':') && !trimmedLine.startsWith("data:"))
        {
            continue;
        }

        // "data: [DONE]" — 流结束信号
        if (trimmedLine == "data: [DONE]")
        {
            // 先尝试提取残留内容中的完整 JSON 指令
            extractCompleteJson();

            // 发射完整累积内容（供只监听 responseReceived 的消费者，如分类器）
            const QString finalContent = m_rawContent.trimmed();
            if (!finalContent.isEmpty())
            {
                emit responseReceived(finalContent);
            }
            else if (!m_reasoningBuffer.trimmed().isEmpty())
            {
                // 推理模型（deepseek-v4-pro）可能在 content 为空的情况下
                // 把所有 token 花在了 reasoning_content 上（如 max_tokens 太小）。
                // 此时回退到 reasoning buffer，避免消费者（RAGPipeline 路由等）永久挂起。
                emit responseReceived(m_reasoningBuffer.trimmed());
            }
            else
            {
                // 完全空响应：发射空串标记流结束，避免消费者永久等待
                emit responseReceived(QString());
            }
            return;
        }

        // 非 "data:" 前缀的行 → 跳过
        if (!trimmedLine.startsWith("data:"))
        {
            continue;
        }

        // 提取 payload：跳过 "data:"（5 个字符）
        QByteArray payload = trimmedLine.mid(5);

        // SSE 规范允许 "data:" 后跟一个可选空格
        if (payload.startsWith(' '))
        {
            payload = payload.mid(1);
        }

        if (payload.isEmpty())
        {
            continue;
        }

        // 解析 JSON
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError)
        {
            // 单帧损坏，跳过
            continue;
        }

        // 提取 delta 内容
        QString deltaContent;
        QString deltaReasoning;
        if (!parseSseDelta(payload, deltaContent, &deltaReasoning))
        {
            continue;
        }

        if (!deltaContent.isEmpty())
        {
            qDebug().noquote() << "[AI] <<< SSE delta content:" << deltaContent;
            m_accumulatedContent += deltaContent;
            m_rawContent += deltaContent;
            // 每收到新内容就尝试提取完整 JSON 指令
            extractCompleteJson();
        }
        if (!deltaReasoning.isEmpty())
        {
            qDebug().noquote() << "[AI] <<< SSE delta reasoning:" << deltaReasoning;
            m_reasoningBuffer += deltaReasoning;
        }

        // 检查 finish_reason
        const QJsonObject root = doc.object();
        const QJsonArray choices = root["choices"].toArray();
        if (!choices.isEmpty())
        {
            const QJsonObject firstChoice = choices[0].toObject();
            const QString finishReason = firstChoice["finish_reason"].toString();

            if (finishReason == QLatin1String("stop") || finishReason == QLatin1String("length"))
            {
                // 提取残留内容中的完整 JSON 指令（[DONE] 随后做最终清理）
                extractCompleteJson();
            }
        }
    }
}
