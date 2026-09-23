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

/// @file AIPipeline.cpp
/// @brief AIPipeline 实现 —— AI 总调度器

#include "AIPipeline.h"

#include "AILLMClassifier.h"
#include "AIIntentRouter.h"
#include "ContextResolver.h"
#include "ConversationHistory.h"
#include "DeepSeekProvider.h"
#include "DirectEntityExecutor.h"
#include "LLMCommandBridge.h"
#include "RAGPipeline.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>

// ============================================================================
// 构造 / 析构
// ============================================================================

AIPipeline::AIPipeline(const QString& docsDir,
                       const QString& readmePath,
                       DmDocument* doc,
                       GuiDocumentView* docView,
                       QObject* parent)
    : QObject(parent)
    , m_router(new AIIntentRouter(this))
    , m_llmClassifier(new AILLMClassifier(this))
    , m_ragPipeline(new RAGPipeline(this))
    , m_modelingProvider(new DeepSeekProvider(this))
    , m_bridge()
    , m_contextResolver(doc ? new ContextResolver(doc, this) : nullptr)
    , m_drawExecutor(doc ? new DirectEntityExecutor(doc, this) : nullptr)
    , m_doc(doc)
{
    // ---- 1. 初始化 LLM 分类器 ----
    m_routerReady = m_llmClassifier->isAvailable();
    if (!m_routerReady)
    {
        emit errorOccurred(tr("Warning"),
                           tr("LLM classifier is not available (API Key not configured). "
                              "Auto mode will be limited."));
    }

    // 将分类器内部错误转发到用户可见的错误信号
    connect(m_llmClassifier.get(), SIGNAL(classificationFailed(QString)),
            this, SLOT(onClassificationFailed(QString)));

    // ---- 2. 初始化 RAG（加载知识库） ----
    m_ragReady = m_ragPipeline->initialize(docsDir, readmePath);
    if (!m_ragReady)
    {
        emit errorOccurred(tr("Warning"),
                           tr("RAG knowledge base is empty. "
                              "QA functionality will be limited. "
                              "Check docs/ directory."));
    }

    // ---- 3. 建模链路就绪检查 ----
    m_modelingReady = (doc != nullptr) && (docView != nullptr) && (m_contextResolver != nullptr);
    if (!m_modelingReady)
    {
        emit errorOccurred(tr("Warning"),
                           tr("No document open. Modeling commands are unavailable."));
    }

    // ---- 4. 连接 QA 链路信号 ----
    connect(m_ragPipeline.get(), &RAGPipeline::answerReady,
            this, &AIPipeline::onRAGAnswer);
    connect(m_ragPipeline.get(), &RAGPipeline::errorOccurred,
            this, &AIPipeline::onRAGError);

    // ---- 5. 连接 Modeling 链路信号 ----
    connect(m_modelingProvider.get(), &DeepSeekProvider::responseReceived,
            this, &AIPipeline::onModelingProviderResponse);
    connect(m_modelingProvider.get(), &DeepSeekProvider::errorOccurred,
            this, &AIPipeline::onModelingProviderError);

    // ---- 6. 连接流式增量指令信号 ----
    connect(m_modelingProvider.get(), &DeepSeekProvider::commandReady,
            this, &AIPipeline::onCommandReady);
}

AIPipeline::~AIPipeline() = default;

// ============================================================================
// 公开接口
// ============================================================================

bool AIPipeline::isReady() const
{
    return m_routerReady;
}

void AIPipeline::handleUserInput(const QString& text, const QString& mode)
{
    m_lastUserText = text;
    m_lastMode     = mode;

    // ---- 空输入保护 ----
    if (text.trimmed().isEmpty())
    {
        emit errorOccurred(tr("Error"), tr("Input is empty."));
        return;
    }

    // ---- 0. 记录用户消息到对话历史 ----
    m_history.pushUser(text);

    // ---- 1. 手动模式：同步路由（不变） ----
    if (mode == QStringLiteral("qa") || mode == QStringLiteral("modeling"))
    {
        const RouterResult route = m_router->route(text, mode);
        dispatchByIntent(route, text);
        return;
    }

    // ---- 2. 自动模式：异步 LLM 路由 ----
    // LLM 分类器不可用 → 重新检查（用户可能在运行时配置了 API Key）
    if (!m_routerReady)
    {
        m_routerReady = m_llmClassifier->isAvailable();
    }
    if (!m_routerReady)
    {
        emit responseReady(tr("AI"),
                           tr("AI features require an API key. Please configure it in AI Settings."));
        return;
    }

    emit responseReady(tr("System"), tr("Analyzing intent..."));

    // 递增请求序列号
    const int seq = ++m_classifySeq;

    // 提取最近 N 条用户消息作为上下文
    const QStringList recentMsgs = m_history.recentUserMessages(2);

    QPointer<AIPipeline> guard(this);
    m_llmClassifier->classify(
        text, recentMsgs,
        [this, guard, seq, text](IntentType intent, float confidence) {
            // 生命周期保护：如果 AIPipeline 已被析构，直接返回
            if (!guard) return;
            // 序列号保护：忽略过时的回调
            if (seq != m_classifySeq) return;

            // 构造 RouterResult
            RouterResult route;
            route.intent     = intent;
            route.confidence = confidence;
            dispatchByIntent(route, text);
        });
}

// ============================================================================
// 意图分发
// ============================================================================

void AIPipeline::dispatchByIntent(const RouterResult& route,
                                   const QString& text)
{
    switch (route.intent)
    {
    case IntentType::QA:
        m_lastResolvedIntent = IntentType::QA;
        handleUserInput_QA(route, text);
        break;

    case IntentType::Modeling:
        m_lastResolvedIntent = IntentType::Modeling;
        handleUserInput_Modeling(route, text);
        break;

    case IntentType::Mixed:
        emit responseReady(tr("System"),
                           tr("Detected mixed intent. Providing help answer first."));
        m_lastResolvedIntent = IntentType::QA;
        handleUserInput_QA(route, text);
        break;

    case IntentType::Social:
        m_lastResolvedIntent = IntentType::Social;
        emit responseReady(tr("AI"),
                           tr("Hello! I'm YiCAD AI Assistant. I can help you with:\n"
                              "• Q&A: Ask me anything about YiCAD features and usage\n"
                              "• Modeling: Describe what you want to draw or modify, "
                              "and I'll execute it on the canvas\n\n"
                              "Switch modes via the dropdown menu (Q&A / Modeling / Auto), "
                              "or just type your request in Auto mode."));
        break;

    case IntentType::Uncertain:
    default:
        // 上一轮建模缺少参数 → 用户可能在补充参数，续接到建模链路
        if (m_lastHadMissingInputs)
        {
            m_lastHadMissingInputs = false;
            m_lastResolvedIntent = IntentType::Modeling;
            handleUserInput_Modeling(route, text);
            break;
        }
        // LLM 也无法确定意图 → 提示用户
        emit responseReady(tr("AI"),
                           tr("Unable to parse your command."));
        break;
    }
}

// ============================================================================
// 模式映射
// ============================================================================

QString AIPipeline::modeToRouterToken(int comboIndex)
{
    switch (comboIndex)
    {
    case 0:  return QStringLiteral("qa");       // Q&A
    case 1:  return QStringLiteral("modeling"); // Modeling
    case 2:
    default: return QStringLiteral("auto");      // Auto
    }
}

// ============================================================================
// QA 链路 —— 分发与回调
// ============================================================================

void AIPipeline::handleUserInput_QA(const RouterResult& route, const QString& text)
{
    if (!m_ragReady)
    {
        emit errorOccurred(tr("Error"),
                           tr("RAG pipeline is not initialized. "
                              "QA mode is unavailable."));
        return;
    }

    emit responseReady(tr("System"),
                       tr("Intent: Q&A (confidence: %1%). Searching knowledge base...")
                           .arg(static_cast<int>(route.confidence * 100)));

    m_ragPipeline->query(text);
}

void AIPipeline::onClassificationFailed(const QString& reason)
{
    emit errorOccurred(tr("Error"), reason);
}

void AIPipeline::onRAGAnswer(const RAGAnswer& answer)
{
    // 记录 assistant 回复到对话历史
    m_history.pushAssistant(answer.answer);

    // 直接展示 LLM 回答（LLM 已在正文中用 Sources: 格式内联了引用）
    emit responseReady(tr("AI"), answer.answer);
}

void AIPipeline::onRAGError(const QString& error)
{
    emit errorOccurred(tr("RAG Error"), error);
}

// ============================================================================
// Modeling 链路 —— 分发与回调
// ============================================================================

void AIPipeline::handleUserInput_Modeling(const RouterResult& route, const QString& text)
{
    if (!m_modelingReady)
    {
        emit errorOccurred(tr("Error"),
                           tr("Modeling mode requires an open document. "
                              "Please open or create a drawing first."));
        return;
    }

    emit responseReady(tr("System"),
                       tr("Intent: Modeling (%1%). Generating command...")
                           .arg(static_cast<int>(route.confidence * 100)));

    // 注入建模系统指令到对话历史
    m_history.pushSystem(buildModelingSystemPrompt());

    // 发送给 LLM：传入历史消息数组（含 system + 前文）
    m_streamingReceived = false;
    const QString sysPrompt = buildModelingSystemPrompt();
    m_modelingProvider->sendMessage(text, m_history.toMessages(sysPrompt));
}

void AIPipeline::onCommandReady(const QString& commandJson)
{
    // 流式增量执行：每收到一个完整 JSON 指令就立即解析并执行
    m_streamingReceived = true;

    // 1. 解析 JSON
    const ParsedCommand cmd = m_bridge.parse(commandJson);

    if (!cmd.ok)
    {
        // 解析失败，记录警告但不阻断（后续指令可能正常）
        emit errorOccurred(tr("Stream Parse"),
                           tr("Failed to parse streaming command: %1\nJSON: %2")
                               .arg(cmd.errorMessage)
                               .arg(commandJson.left(200)));
        return;
    }

    // 2. 高风险操作提示（不阻断执行）
    if (cmd.needsConfirmation)
    {
        emit responseReady(tr("System"),
                           tr("⚠ High-risk operation: %1").arg(cmd.message));
    }

    // 3. 缺失输入 → 跳过执行，提示用户并提供上下文给下一轮
    if (!cmd.missingInputs.isEmpty())
    {
        QString missingList = cmd.missingInputs.join(QStringLiteral("、"));
        QString prompt = tr("The following parameters are required: %1\n"
                             "Please provide their values directly in the dialog.")
                             .arg(missingList);

        // 关键：将参数请求写入对话历史，使下一轮 LLM 调用能获取上下文
        m_history.pushAssistant(prompt);
        m_lastHadMissingInputs = true;

        emit responseReady(tr("AI"), prompt);
        return;
    }

    // 4. 直接执行（不包 Transaction，每个指令独立提交）
    const QString execResult = executeCommand(cmd);
    emit responseReady(tr("AI"), execResult);
}

void AIPipeline::onModelingProviderResponse(const QString& responseText)
{
    // 若已通过 commandReady 收到流式指令，跳过 responseReceived 的全文解析
    // （全文是多个 JSON 对象的拼接，m_bridge.parse 只能解析第一个，其余已通过 commandReady 处理）
    if (m_streamingReceived)
    {
        m_streamingReceived = false;
        return;
    }

    // 1. 解析 LLM 返回的 JSON
    const ParsedCommand cmd = m_bridge.parse(responseText);

    if (!cmd.ok)
    {
        // 解析失败：可能是 LLM 返回了纯文本（非建模指令的友好回复）
        // 检查响应是否看起来像纯文本而非 JSON
        const QString trimmed = responseText.trimmed();
        if (!trimmed.startsWith(QStringLiteral("{")))
        {
            // 这是纯文本回复，直接展示给用户
            m_history.pushAssistant(trimmed);
            emit responseReady(tr("AI"), trimmed);
            return;
        }

        // 确实是 JSON 解析错误
        emit errorOccurred(tr("Parse Error"),
                           tr("Failed to parse modeling command from LLM response:\n%1\n\n"
                              "Raw response:\n%2")
                               .arg(cmd.errorMessage)
                               .arg(responseText.left(500)));
        return;
    }

    // 2. 高风险操作提示（首版仅警告，不阻断执行）
    if (cmd.needsConfirmation)
    {
        emit responseReady(tr("System"),
                           tr("⚠ High-risk operation: %1").arg(cmd.message));
    }

    // 3. 检查是否有缺失输入 → 用文本提示替代画布拾取
    if (!cmd.missingInputs.isEmpty())
    {
        QString missingList = cmd.missingInputs.join(QStringLiteral("、"));
        QString prompt = tr("The following parameters are required: %1\n"
                             "Please provide their values directly in the dialog.")
                             .arg(missingList);

        // 关键：将参数请求写入对话历史，使下一轮 LLM 调用能获取上下文
        m_history.pushAssistant(prompt);
        m_lastHadMissingInputs = true;

        emit responseReady(tr("AI"), prompt);
        return;
    }

    // 4. 直接执行
    executeParsedCommand(cmd);

    // 5. 记录 assistant 回复到对话历史
    //    （executeParsedCommand 内部已 emit responseReady，此处仅记账）
}

void AIPipeline::onModelingProviderError(const QString& error)
{
    emit errorOccurred(tr("Modeling Error"), error);
}

void AIPipeline::executeParsedCommand(const ParsedCommand& cmd)
{
    // 1. 解析 selection 中的实体引用
    if (m_contextResolver)
    {
        ResolvedSelection sel = m_contextResolver->resolve(cmd.selection);
        if (!sel.ok && cmd.selection.mode != SelectionMode::None)
        {
            emit errorOccurred(tr("Context Error"),
                               tr("Failed to resolve selection: %1").arg(sel.errorMessage));
            return;
        }
        if (!sel.explanation.isEmpty())
        {
            emit responseReady(tr("System"), sel.explanation);
        }
    }

    // 2. 执行命令
    const QString execResult = executeCommand(cmd);
    emit responseReady(tr("AI"), execResult);
}

// ============================================================================
// Modeling prompt 构建
// ============================================================================

QString AIPipeline::buildModelingSystemPrompt()
{
    // 注意：prompt 内容使用 QStringLiteral 而非 tr()，
    // 因为这些是发给 LLM 的指令，不应随 UI 语言切换而变化。
    //
    // 此方法只返回 system 指令部分。
    // 用户输入由 handleUserInput_Modeling() 通过对话历史的 user 消息传递。

    QString prompt;

    prompt += QStringLiteral(
        "You are YiCAD's CAD modeling command parser.\n"
        "Convert natural language CAD commands into one or more JSON objects.\n"
        "Output each entity as a SEPARATE JSON object, concatenated directly "
        "(no commas between objects, no markdown fences, no explanations).\n"
        "Each object will be executed immediately as it arrives — entities appear "
        "one by one on the canvas.\n\n"

        "IMPORTANT: If the user's input is NOT a modeling command (e.g., greetings like "
        "\"hello\", \"hi\", \"你好\", questions, or casual chat), respond with plain text "
        "explaining that you are in Modeling mode and can only execute drawing commands. "
        "Do NOT generate JSON for non-modeling inputs.\n\n"

        "The JSON must have these fields:\n"
        "- intent: (string) one of: draw_point, draw_line, draw_circle, "
        "draw_rectangle, draw_ellipse, draw_arc, draw_text\n"
        "- selection: (object) with \"mode\" field. mode values:\n"
        "    \"none\" (for creating new entities),\n"
        "    \"current_selection\" (use currently selected entities),\n"
        "    \"last_created\" (use the most recently created entity),\n"
        "    \"all\" (operate on all entities)\n"
        "- params: (object) geometric parameters. Use these keys:\n"
        "    center: [x, y], radius: number (for draw_circle)\n"
        "    start: [x, y], end: [x, y] (for draw_line)\n"
        "    corner1: [x, y], corner2: [x, y] (for draw_rectangle)\n"
        "    position: [x, y] (for draw_point)\n"
        "    center: [x, y], majorRadius: number, minorRadius: number (for draw_ellipse)\n"
        "    center: [x, y], radius: number, start_angle: number, end_angle: number (angles in degrees, for draw_arc)\n"
        "    position: [x, y], height: number, text: string (for draw_text)\n"
        "    angle: number (optional, rotation in degrees, for draw_text)\n"
        "    halign: \"left\" | \"center\" | \"right\" (optional, default left, for draw_text)\n"
        "    valign: \"top\" | \"middle\" | \"bottom\" | \"baseline\" (optional, default baseline, for draw_text)\n"
        "- missing_inputs: (array of strings) inputs that need more information\n"
        "- needs_confirmation: (boolean) reserved for future use\n"
        "- message: (string) human-readable description of the operation\n\n"

        "CRITICAL RULES:\n"
        "1. NEVER invent entity IDs. Use selection modes instead.\n"
        "2. When entity selection is needed, set selection.mode to "
        "\"current_selection\" and tell user in message.\n"
        "3. If coordinates or points are unspecified, list them in missing_inputs "
        "AND clearly state what is missing in the message field.\n"
        "4. Use only the intent values listed above.\n"
        "5. For complex objects composed of multiple primitives (tables, doors, "
        "stairs, gears), output ONE JSON object per entity. Do NOT use draw_compound "
        "or steps arrays — each entity is its own top-level JSON object.\n"
        "6. MULTI-TURN: If the conversation history shows you previously asked "
        "for missing parameters (missing_inputs was non-empty), and the user's "
        "latest message provides those values, combine them with the original "
        "request and output the COMPLETE command with all parameters filled in. "
        "Look at the previous user message for the original intent and entity "
        "description.\n\n"

        "Example for \"draw a table\":\n"
        "{\"intent\":\"draw_rectangle\",\"message\":\"Drawing table top\",\"selection\":{\"mode\":\"none\"},\"params\":{\"corner1\":[0,0],\"corner2\":[120,60]},"
        "\"missing_inputs\":[],\"needs_confirmation\":false}\n"
        "{\"intent\":\"draw_line\",\"message\":\"Drawing leg 1\",\"selection\":{\"mode\":\"none\"},\"params\":{\"start\":[10,60],\"end\":[10,100]},"
        "\"missing_inputs\":[],\"needs_confirmation\":false}\n"
        "{\"intent\":\"draw_line\",\"message\":\"Drawing leg 2\",\"selection\":{\"mode\":\"none\"},\"params\":{\"start\":[110,60],\"end\":[110,100]},"
        "\"missing_inputs\":[],\"needs_confirmation\":false}\n\n"

        "For modeling commands, output the JSON objects only, no other text. "
        "For non-modeling inputs, output plain text explanation.");

    return prompt;
}

// ============================================================================
// 命令执行分发
// ============================================================================

QString AIPipeline::executeCommand(const ParsedCommand& cmd)
{
    // 记录 assistant 回复到对话历史
    m_history.pushAssistant(cmd.message);

    // 根据 intent 类型分发到不同的执行器
    // 绘制类 intent → DirectEntityExecutor
    // 修改类 intent → 暂不支持，返回提示信息

    const auto intent = cmd.intent;

    // ---- 绘制类 ----
    switch (intent)
    {
    case CommandIntent::DrawPoint:
    case CommandIntent::DrawLine:
    case CommandIntent::DrawCircle:
    case CommandIntent::DrawRectangle:
    case CommandIntent::DrawArc:
    case CommandIntent::DrawEllipse:
    case CommandIntent::DrawText:
    {
        if (!m_drawExecutor)
        {
            return tr("Drawing executor is not available (no document open).");
        }

        const ExecutorResult result = m_drawExecutor->execute(cmd);
        if (!result.success)
        {
            return tr("Drawing failed: %1").arg(result.errorMessage);
        }

        // 构造成功消息
        QString msg = tr("Drawing completed: %1").arg(cmd.message);
        if (!result.createdEntities.isEmpty())
        {
            msg += QStringLiteral("\n");
            msg += tr("Created %1 entity(s):").arg(result.createdEntities.size());
            for (const auto& e : result.createdEntities)
            {
                msg += QStringLiteral("\n  • %1 [%2]")
                           .arg(e.entityType)
                           .arg(QString::fromStdString(e.entityId.asString()));
            }
        }
        return msg;
    }

    case CommandIntent::DrawCompound:
    {
        if (!m_drawExecutor)
        {
            return tr("Drawing executor is not available (no document open).");
        }

        const ExecutorResult result = m_drawExecutor->executeCompound(cmd);
        if (!result.success)
        {
            return tr("Compound drawing failed: %1").arg(result.errorMessage);
        }

        QString msg = tr("Compound drawing completed: %1").arg(cmd.message);
        if (!result.createdEntities.isEmpty())
        {
            msg += QStringLiteral("\n");
            msg += tr("Created %1 entity(s):").arg(result.createdEntities.size());
            for (const auto& e : result.createdEntities)
            {
                msg += QStringLiteral("\n  • %1 [%2]")
                           .arg(e.entityType)
                           .arg(QString::fromStdString(e.entityId.asString()));
            }
        }
        return msg;
    }

    // ---- 暂不支持的意图 ----
    default:
        return tr("Command intent '%1' is not yet supported by the executors.")
            .arg(intentToString(intent));
    }
}
