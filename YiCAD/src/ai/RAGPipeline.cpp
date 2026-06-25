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

/// @file RAGPipeline.cpp
/// @brief RAGPipeline 实现 —— RAG 问答管线的完整编排

#include "RAGPipeline.h"
#include "ChunkSplitter.h"
#include "ConversationHistory.h"
#include "DeepSeekProvider.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QSet>

// ============================================================================
// 构造 / 析构
// ============================================================================

RAGPipeline::RAGPipeline(QObject* parent)
    : QObject(parent)
    , m_provider(new DeepSeekProvider(this))
    , m_ready(false)
    , m_isRouting(false)
{
    // 转发 provider 信号
    connect(m_provider, &DeepSeekProvider::responseReceived,
            this, &RAGPipeline::onModelResponse);
    connect(m_provider, &DeepSeekProvider::errorOccurred,
            this, &RAGPipeline::onModelError);
}

RAGPipeline::~RAGPipeline() = default;

// ============================================================================
// 公开接口 —— 初始化
// ============================================================================

bool RAGPipeline::initialize(const QString& docsDir, const QString& readmePath)
{
    ChunkSplitter splitter;
    QVector<Chunk> chunks;

    // 1. 加载 docs/ 目录下的所有 .md 文件
    if (!docsDir.isEmpty())
    {
        QVector<Chunk> docChunks = splitter.splitDirectory(docsDir);
        chunks.append(docChunks);
    }

    // 2. 加载 README.md
    if (!readmePath.isEmpty())
    {
        QVector<Chunk> readmeChunks = splitter.splitFile(readmePath, "readme");
        chunks.append(readmeChunks);
    }

    if (chunks.isEmpty())
    {
        m_ready = false;
        return false;
    }

    // 存储全部 chunks（LLM 路由后再筛选）
    m_allChunks = chunks;
    m_ready = true;

    return true;
}

bool RAGPipeline::isReady() const
{
    return m_ready && !m_allChunks.isEmpty();
}

int RAGPipeline::chunkCount() const
{
    return m_allChunks.size();
}

// ============================================================================
// 公开接口 —— 查询
// ============================================================================

void RAGPipeline::query(const QString& question)
{
    if (!isReady())
    {
        emit errorOccurred(tr("RAG pipeline is not initialized. "
                              "Please load knowledge sources first."));
        return;
    }

    if (question.trimmed().isEmpty())
    {
        emit errorOccurred(tr("Question is empty."));
        return;
    }

    m_lastQuestion = question;

    // Phase 1: LLM 文档路由 → 选中文件 → Phase 2 回答
    startRouting(question);
}

// ============================================================================
// 私有槽 —— 模型回复
// ============================================================================

void RAGPipeline::onModelResponse(const QString& responseText)
{
    // Phase 1 路由响应 → 过滤 chunks 并进入 Phase 2
    if (m_isRouting)
    {
        m_isRouting = false;
        onRoutingResponse(responseText);
        return;
    }

    // Phase 2 回答生成 —— 正常处理
    RAGAnswer answer = parseResponse(responseText);

    // 如果模型未返回引用，用 Phase 2 使用的 context 作为 fallback citations
    if (answer.citations.isEmpty() && !m_lastContext.isEmpty())
    {
        for (const Chunk& chunk : m_lastContext)
        {
            answer.citations.append(chunkToCitation(chunk));
        }
    }

    emit answerReady(answer);
}

void RAGPipeline::onModelError(const QString& errorMessage)
{
    // 路由阶段出错 → fallback 到全量 chunks 直接回答
    if (m_isRouting)
    {
        m_isRouting = false;
        // 用全量 chunks 作为上下文进入 Phase 2
        m_lastContext = m_allChunks;
        const QString prompt = buildPrompt(m_pendingQuestion, m_allChunks);
        m_provider->sendMessage(prompt);
        return;
    }

    emit errorOccurred(errorMessage);
}

// ============================================================================
// 私有方法 —— 两阶段 LLM 路由
// ============================================================================

QString RAGPipeline::buildDocumentIndex() const
{
    // 从 m_allChunks 提取每个唯一 docPath 的标题，构建紧凑索引
    QMap<QString, QString> index;  // docPath → title
    for (const Chunk& c : m_allChunks)
    {
        if (!index.contains(c.docPath))
        {
            index[c.docPath] = c.title;
        }
    }

    QString text;
    text += QStringLiteral("Document Index:\n");
    for (auto it = index.constBegin(); it != index.constEnd(); ++it)
    {
        text += QStringLiteral("  %1 | %2\n").arg(it.key()).arg(it.value());
    }

    return text;
}

void RAGPipeline::startRouting(const QString& question)
{
    m_pendingQuestion = question;
    m_isRouting = true;

    const QString indexText = buildDocumentIndex();

    // 路由 system prompt：纯指令，不随 UI 语言切换
    const QString routingSystem = QStringLiteral(
        "You are a document router for a CAD software help system.\n"
        "Given a user question and a document index, select the document files "
        "that are MOST LIKELY to contain the answer.\n\n"
        "Rules:\n"
        "1. Return ONLY a JSON array of file names, nothing else.\n"
        "2. Select 1-3 files maximum. Be conservative — only include files "
        "that you are confident contain relevant information.\n"
        "3. If no file seems relevant, return an empty array [].\n"
        "4. File names MUST exactly match those in the index.\n\n"
        "Example: [\"02-drawing-curves.md\", \"14-command-reference.md\"]");

    const QString routingPrompt = indexText
        + QStringLiteral("\n[User Question]\n%1\n\n"
                         "Return the JSON array of relevant file names:").arg(question);

    // 空历史，temperature=0，max_tokens=512（需为推理模型留足 reasoning 空间）
    m_provider->sendMessage(routingPrompt, QVector<MessageEntry>(),
                            routingSystem, 0.0, 512);
}

void RAGPipeline::onRoutingResponse(const QString& responseText)
{
    // 解析 LLM 返回的 JSON 数组
    QStringList selectedFiles;
    const QString trimmed = responseText.trimmed();

    // 提取 [...] JSON 数组
    int start = trimmed.indexOf('[');
    int end = trimmed.lastIndexOf(']');
    if (start >= 0 && end > start)
    {
        const QString jsonArray = trimmed.mid(start, end - start + 1);
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonArray.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isArray())
        {
            const QJsonArray arr = doc.array();
            for (const QJsonValue& val : arr)
            {
                if (val.isString())
                {
                    selectedFiles.append(val.toString());
                }
            }
        }
    }

    // 过滤 m_allChunks：只保留选中文件中的 chunks
    QVector<Chunk> filteredChunks;
    if (!selectedFiles.isEmpty())
    {
        // 用 set 加速查找
        const QSet<QString> fileSet(selectedFiles.begin(), selectedFiles.end());
        for (const Chunk& c : m_allChunks)
        {
            if (fileSet.contains(c.docPath))
            {
                filteredChunks.append(c);
            }
        }
    }

    // 兜底：路由未选中任何文件或过滤后为空 → 使用全量 chunks
    if (filteredChunks.isEmpty())
    {
        filteredChunks = m_allChunks;
    }

    // 保存用于回答阶段回填引用
    m_lastContext = filteredChunks;

    // Phase 2: 回答生成
    const QString prompt = buildPrompt(m_pendingQuestion, filteredChunks);
    m_provider->sendMessage(prompt);
}

// ============================================================================
// 私有方法 —— Prompt 构建
// ============================================================================
//
// 注意：prompt 内容使用 QStringLiteral 而非 tr()，因为这些字符串是发给 LLM 的
// 指令，不应随 UI 语言切换而变化。

QString RAGPipeline::buildPrompt(const QString& question,
                                 const QVector<Chunk>& contextChunks) const
{
    // 系统指令（嵌在 user message 中，与 DeepSeekProvider 的 system prompt 叠加）
    QString prompt;

    prompt += QStringLiteral(
        "[System Instruction]\n"
        "You are YiCAD's help assistant. You MUST answer based ONLY on the "
        "provided reference materials below.\n"
        "If the materials are insufficient, clearly state: "
        "\"The current documentation does not contain enough information to fully answer this question.\"\n"
        "Keep your answer concise and accurate.\n"
        "At the end of your answer, list the sources you used in this format:\n"
        "  Sources: [1] doc_path (section_title)\n"
        "Do NOT fabricate any information not present in the references.\n\n");

    // 检索上下文
    if (contextChunks.isEmpty())
    {
        prompt += QStringLiteral(
            "[Retrieved Context]\n"
            "(No relevant documentation found for this question.)\n\n");
    }
    else
    {
        prompt += QStringLiteral("[Retrieved Context]\n");
        for (int i = 0; i < contextChunks.size(); ++i)
        {
            const Chunk& c = contextChunks[i];
            prompt += QStringLiteral("--- Reference %1 ---\n"
                                     "Source: %2\n"
                                     "Section: %3\n"
                                     "Content:\n%4\n\n")
                          .arg(i + 1)
                          .arg(c.docPath)
                          .arg(c.section)
                          .arg(c.content);
        }
    }

    // 用户问题
    prompt += QStringLiteral("[User Question]\n%1").arg(question);

    return prompt;
}

// ============================================================================
// 私有方法 —— 响应解析
// ============================================================================

RAGAnswer RAGPipeline::parseResponse(const QString& responseText) const
{
    RAGAnswer answer;

    if (responseText.isEmpty())
    {
        answer.answer       = tr("(Empty response from model)");
        answer.confidence   = 0.0f;
        answer.needFollowup = true;
        return answer;
    }

    // 尝试作为 JSON 解析（模型可能返回 JSON 格式）
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseText.toUtf8(), &parseError);

    if (parseError.error == QJsonParseError::NoError && doc.isObject())
    {
        const QJsonObject root = doc.object();

        // 提取 answer
        if (root.contains("answer"))
        {
            answer.answer = root["answer"].toString();
        }
        else
        {
            answer.answer = responseText;  // fallback
        }

        // 提取 confidence
        if (root.contains("confidence"))
        {
            answer.confidence = static_cast<float>(root["confidence"].toDouble(0.5));
        }

        // 提取 need_followup
        if (root.contains("need_followup"))
        {
            answer.needFollowup = root["need_followup"].toBool(false);
        }

        // 提取 citations
        if (root.contains("citations"))
        {
            const QJsonArray arr = root["citations"].toArray();
            for (const QJsonValue& val : arr)
            {
                if (!val.isObject()) continue;
                const QJsonObject cit = val.toObject();
                Citation c;
                c.docPath = cit["doc_path"].toString();
                c.title   = cit["title"].toString();
                c.chunkId = cit["chunk_id"].toString();
                c.snippet = cit["snippet"].toString();

                // 如果模型没给 snippet，尝试从 m_lastContext 回填
                if (c.snippet.isEmpty() && !c.chunkId.isEmpty())
                {
                    for (const Chunk& chunk : m_lastContext)
                    {
                        if (chunk.chunkId == c.chunkId)
                        {
                            c.snippet = chunk.content.left(120);
                            break;
                        }
                    }
                }

                if (!c.docPath.isEmpty())
                {
                    answer.citations.append(c);
                }
            }
        }
    }
    else
    {
        // 非 JSON：整个响应作为回答
        answer.answer = responseText;

        // 尝试从文本中提取 "Sources:" 行做引用
        int sourcesIdx = responseText.lastIndexOf("Sources:");
        if (sourcesIdx < 0)
        {
            sourcesIdx = responseText.lastIndexOf("sources:");
        }
        if (sourcesIdx >= 0)
        {
            // 从 m_lastContext 回填 citations
            for (const Chunk& chunk : m_lastContext)
            {
                answer.citations.append(chunkToCitation(chunk));
            }
        }
    }

    // 置信度兜底
    if (answer.confidence <= 0.0f)
    {
        answer.confidence = 0.5f;
    }

    return answer;
}

// ============================================================================
// 辅助
// ============================================================================

Citation RAGPipeline::chunkToCitation(const Chunk& chunk)
{
    Citation c;
    c.docPath = chunk.docPath;
    c.title   = chunk.title;
    c.chunkId = chunk.chunkId;
    c.snippet = chunk.content.left(120);
    return c;
}
