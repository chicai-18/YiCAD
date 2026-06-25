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

/// @file RAGPipeline.h
/// @brief RAG 问答管线 —— 两阶段 LLM 路由 + 回答
///
/// 职责：
///   - 持有 ChunkSplitter + DeepSeekProvider
///   - initialize() 加载知识源并切分为 chunks
///   - query() 执行两阶段流程：
///       1. LLM 文档路由：根据问题 + 文档索引选择相关文件
///       2. LLM 回答生成：仅加载选中文件的 chunks，拼 prompt → 调模型 → 解析引用
///
/// 使用方式：
///   @code
///     auto* rag = new RAGPipeline(this);
///     connect(rag, &RAGPipeline::answerReady,
///             this, &MyClass::onRagAnswer);
///     connect(rag, &RAGPipeline::errorOccurred,
///             this, &MyClass::onRagError);
///     rag->initialize("path/to/docs", "path/to/README.md");
///     rag->query("修剪命令怎么用");
///   @endcode
///
/// 说明：
///   - 首版不支持多轮对话，每次 query() 独立
///   - 路由失败自动 fallback 到全量 chunks

#ifndef RAGPIPELINE_H
#define RAGPIPELINE_H

#include "RAGTypes.h"

#include <QObject>
#include <QString>
#include <QVector>
#include <memory>

class DeepSeekProvider;

class RAGPipeline : public QObject
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父对象指针
    explicit RAGPipeline(QObject* parent = nullptr);

    /// @brief 析构函数
    ~RAGPipeline() override;

    /// @brief 初始化管线：加载知识源、切分、建索引
    /// @param docsDir   docs/ 目录的绝对路径
    /// @param readmePath README.md 的绝对路径
    /// @return 成功返回 true
    ///
    /// 可以多次调用以重建索引（如文档更新后）。
    bool initialize(const QString& docsDir, const QString& readmePath);

    /// @brief 是否已完成初始化并可查询
    bool isReady() const;

    /// @brief 发起一次 RAG 问答
    /// @param question 用户原始问题
    ///
    /// 结果通过 answerReady / errorOccurred 信号异步返回。
    /// 如果未初始化或检索器为空，会直接发出 errorOccurred。
    void query(const QString& question);

    /// @brief 当前索引中的 chunk 总数（调试用）
    int chunkCount() const;

signals:
    /// @brief RAG 问答成功完成
    /// @param answer 包含主回答、引用列表和置信度
    void answerReady(const RAGAnswer& answer);

    /// @brief 管线执行过程中发生错误
    /// @param errorMessage 错误描述
    void errorOccurred(const QString& errorMessage);

private slots:
    /// @brief 接收 DeepSeekProvider 的模型回复
    void onModelResponse(const QString& responseText);

    /// @brief 接收 DeepSeekProvider 的错误
    void onModelError(const QString& errorMessage);

private:
    /// @brief 构建发送给模型的系统提示词 + 上下文 + 用户问题
    QString buildPrompt(const QString& question,
                        const QVector<Chunk>& contextChunks) const;

    /// @brief 解析模型回复，提取 answer + citations
    RAGAnswer parseResponse(const QString& responseText) const;

    /// @brief 从 chunk 构建 Citation
    static Citation chunkToCitation(const Chunk& chunk);

    // ---- 两阶段 LLM 路由 ----

    /// @brief 构建文档索引文本（文件名 → 标题），供路由 LLM 使用
    QString buildDocumentIndex() const;

    /// @brief 发起 Phase 1 —— LLM 文档路由请求
    void startRouting(const QString& question);

    /// @brief 处理 Phase 1 路由响应，过滤 chunks 并进入 Phase 2 回答
    void onRoutingResponse(const QString& responseText);

    DeepSeekProvider*  m_provider;       ///< 模型调用（不持有）
    QVector<Chunk>     m_allChunks;      ///< 全部知识库 chunk
    QString            m_lastQuestion;   ///< 最近一次问题
    QVector<Chunk>     m_lastContext;    ///< 最近一次使用的上下文（用于回填引用）
    QString            m_pendingQuestion;///< 路由阶段暂存的问题（跨 Phase 1→2）
    bool               m_ready;          ///< 是否已初始化
    bool               m_isRouting;      ///< 当前是否处于路由阶段（区分 Phase 1 / Phase 2 响应）
};

#endif // RAGPIPELINE_H
