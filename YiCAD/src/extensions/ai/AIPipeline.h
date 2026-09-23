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

/// @file AIPipeline.h
/// @brief AI 总调度器 —— 串联 LLM 分类、路由、RAG、建模桥接、执行器
///
/// 职责：
///   - 持有并管理 AILLMClassifier / AIIntentRouter / RAGPipeline / LLMCommandBridge / Executors
///   - 接收用户输入，LLM 异步分类 → 分发到 QA 或 Modeling 链路
///   - 手动模式 (qa / modeling) 同步分发，Bypass LLM
///   - QA 链路：RAGPipeline 检索 + 模型回答 + 引用
///   - Modeling 链路：LLM 建模 prompt → JSON 解析 → ContextResolver → Executor
///   - LLM 不确定时提示用户手动选择模式
///   - 统一通过 responseReady / errorOccurred 信号向 UI 层回报
///
/// 使用方式：
///   @code
///     auto* pipeline = new AIPipeline(docsDir, readmePath, doc, docView, this);
///     connect(pipeline, &AIPipeline::responseReady,
///             dialog, &AIDialog::appendMessage);
///     pipeline->handleUserInput("draw a circle", "auto");
///   @endcode

#ifndef AIPIPELINE_H
#define AIPIPELINE_H

#include <QObject>
#include <QString>
#include <memory>
#include <optional>

#include "AIIntentRouter.h"
class RAGPipeline;
class DeepSeekProvider;
class AILLMClassifier;
class ContextResolver;
class DirectEntityExecutor;
class DmDocument;
class GuiDocumentView;

#include "RAGTypes.h"
struct RouterResult;
struct ParsedCommand;

#include "LLMCommandBridge.h"
#include "ConversationHistory.h"

class AIPipeline : public QObject
{
    Q_OBJECT

public:
    /// @brief 构造函数 —— 初始化所有子模块（不含关键词 JSON 路径，分类由 LLM 完成）
    /// @param docsDir    docs/ 目录路径（用于 RAG 知识库）
    /// @param readmePath README.md 路径（用于 RAG 知识库）
    /// @param doc        当前文档指针（用于建模执行器，可为 nullptr）
    /// @param docView     当前文档视图指针（用于建模执行器 UI 刷新，可为 nullptr）
    /// @param parent      父 QObject
    explicit AIPipeline(const QString& docsDir,
                        const QString& readmePath,
                        DmDocument* doc,
                        GuiDocumentView* docView,
                        QObject* parent = nullptr);

    /// @brief 析构函数
    ~AIPipeline() override;

    /// @brief 整体是否就绪（LLM 分类器可用）
    bool isReady() const;

    /// @brief 获取对话历史引用（供 AIAssistant 持久化读写）
    ConversationHistory& history() { return m_history; }

    /// @brief 处理一条用户输入（从 AIDialog 的 sendRequested 信号驱动）
    /// @param text 用户原始输入文本
    /// @param mode 路由模式 token："qa" / "modeling" / "auto"
    ///
    /// 自动模式：AIIntentRouter::route() → 占位 Uncertain → AILLMClassifier 异步分类 → dispatchByIntent
    /// 手动模式：AIIntentRouter::route() 同步返回 1.0 置信度 → dispatchByIntent
    /// LLM 不可用/Uncertain → 提示用户手动选择模式
    void handleUserInput(const QString& text, const QString& mode);

signals:
    /// @brief 管线产出回复（QA 回答 / 建模执行结果 / 路由提示）
    /// @param sender 发送者标识（"AI" / "System"）
    /// @param text   回复文本
    void responseReady(const QString& sender, const QString& text);

    /// @brief 管线执行中发生错误
    /// @param sender 发送者标识（"Error"）
    /// @param text   错误描述
    void errorOccurred(const QString& sender, const QString& text);

private slots:
    // ---- 分类器回调 ----
    void onClassificationFailed(const QString& reason);

    // ---- QA 链路回调 ----
    void onRAGAnswer(const RAGAnswer& answer);
    void onRAGError(const QString& error);

    // ---- Modeling 链路回调 ----
    void onModelingProviderResponse(const QString& responseText);
    void onModelingProviderError(const QString& error);

    // ---- 流式增量指令回调 ----
    /// @brief 接收 DeepSeekProvider 流式提取的单个完整 JSON 指令
    /// @param commandJson 完整的 JSON 命令字符串（单个 {…} 对象）
    ///
    /// 每收到一个完整指令即解析执行，不等流结束。
    void onCommandReady(const QString& commandJson);

private:
    // ---- 意图分发 ----
    /// @brief 根据路由结果分发到 QA / Modeling / Mixed / Uncertain 链路
    void dispatchByIntent(const RouterResult& route, const QString& text);


    // ---- 链路内部分发 ----
    /// @brief QA 链路：展示路由信息并调用 RAGPipeline
    void handleUserInput_QA(const RouterResult& route, const QString& text);

    /// @brief Modeling 链路：构建 prompt 并调用 LLM
    void handleUserInput_Modeling(const RouterResult& route, const QString& text);

    // ---- 模式映射 ----
    /// @brief 将 AIDialog 的模式索引转为 Router token
    static QString modeToRouterToken(int comboIndex);

    // ---- Modeling prompt 构建 ----
    /// @brief 构建发送给 LLM 的建模系统指令（不含用户输入）
    static QString buildModelingSystemPrompt();

    // ---- 执行分发 ----
    /// @brief 根据 ParsedCommand::intent 分发到 DirectEntityExecutor
    /// @return 人类可读的执行结果描述
    QString executeCommand(const ParsedCommand& cmd);

    /// @brief 执行已解析命令（context resolve + dispatch）
    void executeParsedCommand(const ParsedCommand& cmd);

    // ---- 对话记忆 ----
    ConversationHistory                   m_history;         ///< 对话历史（短期记忆 + token 预算管理）

    // ---- 子模块 ----
    std::unique_ptr<AIIntentRouter>       m_router;          ///< 意图路由器（手动模式兜底）
    std::unique_ptr<AILLMClassifier>      m_llmClassifier;   ///< LLM 意图分类器
    std::unique_ptr<RAGPipeline>          m_ragPipeline;     ///< RAG 问答管线（内含自己的 DeepSeekProvider）
    std::unique_ptr<DeepSeekProvider>     m_modelingProvider;///< 建模链路专用 LLM Provider
    LLMCommandBridge                      m_bridge;          ///< JSON 命令桥（无状态）
    std::unique_ptr<ContextResolver>      m_contextResolver; ///< 上下文解析器
    std::unique_ptr<DirectEntityExecutor> m_drawExecutor;    ///< 直接绘图执行器
    // ---- 缓存的文档指针 ----
    DmDocument*       m_doc = nullptr;

    // ---- 状态 ----
    bool m_routerReady  = false;  ///< LLM 分类器是否可用
    bool m_ragReady     = false;  ///< RAG 知识库是否已索引
    bool m_modelingReady = false; ///< 建模链路是否可用（doc + docView 均非空）
    bool m_streamingReceived = false; ///< 本次请求是否已通过 commandReady 收到流式指令
    QString m_lastUserText;       ///< 最近一次用户输入（调试用）
    QString m_lastMode;           ///< 最近一次模式（调试用）
    IntentType m_lastResolvedIntent = IntentType::QA; ///< 最近一次成功分发的意图
    bool m_lastHadMissingInputs = false; ///< 上一轮建模是否缺少参数（用于自动模式续接）

    // ---- LLM 分类竞态保护 ----
    int m_classifySeq = 0;  ///< LLM 分类请求序列号（防止乱序回调）

};

#endif // AIPIPELINE_H
