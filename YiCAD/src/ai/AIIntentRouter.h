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

/// @file AIIntentRouter.h
/// @brief AI 意图路由器 —— 手动模式兜底 + 自动模式占位（实际分类由 AILLMClassifier 异步完成）
///
/// 职责：
///   - 手动模式（qa / modeling）同步返回 1.0 置信度，bypass LLM
///   - 自动模式返回占位 Uncertain，由 AIPipeline 调用 AILLMClassifier 异步获取真实意图
///   - 保留 IntentType 枚举和 RouterResult 结构体（与外部兼容）
///
/// 关键词路由代码已完全移除，意图分类改为 LLM 驱动（参见 AILLMClassifier）。
///
/// UI 发送的手动模式 token：
///   - "auto"     → 返回 Uncertain 占位（AIPipeline 触发 LLM 分类）
///   - "qa"       → 强制问答（1.0 置信度）
///   - "modeling" → 强制建模（1.0 置信度）

#ifndef AIINTENTROUTER_H
#define AIINTENTROUTER_H

#include <QObject>
#include <QString>

// ---------------------------------------------------------------------------
// 意图类型
// ---------------------------------------------------------------------------

enum class IntentType
{
    QA,         ///< 问答：询问用法、概念等帮助类问题
    Modeling,   ///< 建模：执行绘制、修改等 CAD 操作
    Mixed,      ///< 混合：既有提问又有操作意图，先回答再确认
    Social,     ///< 社交：问候、感谢、闲聊等非任务性消息
    Uncertain   ///< 不确定：无法判断意图，需追问
};

// ---------------------------------------------------------------------------
// 路由结果
// ---------------------------------------------------------------------------

struct RouterResult
{
    IntentType intent            = IntentType::Uncertain;  ///< 分类结果
    float      confidence        = 0.0f;                   ///< 置信度 [0.0, 1.0]
    QString    reasoning;        ///< 分类理由（人类可读，用于调试/日志）
    QString    suggestedAction;  ///< 建议的下一步动作
    bool       needsConfirmation = false;  ///< 是否需要用户确认后再执行
};

// ---------------------------------------------------------------------------
// AIIntentRouter
// ---------------------------------------------------------------------------

class AIIntentRouter : public QObject
{
    Q_OBJECT

public:
    explicit AIIntentRouter(QObject* parent = nullptr);
    ~AIIntentRouter() override;

    /// @brief 对用户输入进行意图分类
    /// @param input      用户原始文本
    /// @param manualMode 手动模式："auto" / "qa" / "modeling"（来自 UI 切换）
    ///
    /// 手动模式（qa / modeling）：同步返回 1.0 置信度。
    /// 自动模式（auto）：返回占位 Uncertain，真实分类由 AILLMClassifier 异步回调送达。
    RouterResult route(const QString& input,
                       const QString& manualMode = QStringLiteral("auto")) const;

private:
    // ---- 预处理 ----
    static QString preprocess(const QString& raw);
};

#endif // AIINTENTROUTER_H
