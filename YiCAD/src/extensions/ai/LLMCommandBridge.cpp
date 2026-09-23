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

/// @file LLMCommandBridge.cpp
/// @brief LLMCommandBridge 实现 —— 建模链路结构化 JSON 解析与校验

#include "LLMCommandBridge.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>

// ============================================================================
// 内部：intent 字符串 ↔ 枚举 映射表
// ============================================================================

namespace {

struct IntentEntry
{
    const char*   name;    // JSON 协议中的字符串，如 "draw_circle"
    CommandIntent intent;  // 对应枚举值
};

constexpr IntentEntry kIntentTable[] = {
    // ---- 绘制 ----
    { "draw_point",          CommandIntent::DrawPoint       },
    { "draw_line",           CommandIntent::DrawLine        },
    { "draw_circle",         CommandIntent::DrawCircle      },
    { "draw_arc",            CommandIntent::DrawArc         },
    { "draw_rectangle",      CommandIntent::DrawRectangle   },
    { "draw_ellipse",        CommandIntent::DrawEllipse     },
    { "draw_spline",         CommandIntent::DrawSpline      },
    { "draw_polyline",       CommandIntent::DrawPolyline    },
    { "draw_text",           CommandIntent::DrawText        },
    { "draw_compound",       CommandIntent::DrawCompound    },

    // ---- 标注 ----
    { "dimension",           CommandIntent::Dimension       },
    { "dimension_linear",    CommandIntent::DimensionLinear },
    { "dimension_aligned",   CommandIntent::DimensionAligned},
    { "dimension_angular",   CommandIntent::DimensionAngular},
    { "dimension_radial",    CommandIntent::DimensionRadial },
    { "dimension_diametric", CommandIntent::DimensionDiametric},

    // ---- 图块 ----
    { "block_insert",        CommandIntent::BlockInsert     },
    { "block_create",        CommandIntent::BlockCreate     },
};

constexpr int kIntentTableSize = sizeof(kIntentTable) / sizeof(kIntentTable[0]);

// ---- 懒初始化：intent 字符串白名单 ----
QVector<QString> buildKnownIntentStrings()
{
    QVector<QString> v;
    v.reserve(kIntentTableSize);
    for (int i = 0; i < kIntentTableSize; ++i) {
        v.append(QString::fromLatin1(kIntentTable[i].name));
    }
    return v;
}

} // anonymous namespace

// ============================================================================
// 公开函数：intent 字符串 ↔ 枚举
// ============================================================================

CommandIntent intentFromString(const QString& s)
{
    const QByteArray key = s.trimmed().toLatin1();
    for (int i = 0; i < kIntentTableSize; ++i) {
        if (key == kIntentTable[i].name) {
            return kIntentTable[i].intent;
        }
    }
    return CommandIntent::Unknown;
}

QString intentToString(CommandIntent intent)
{
    for (int i = 0; i < kIntentTableSize; ++i) {
        if (kIntentTable[i].intent == intent) {
            return QString::fromLatin1(kIntentTable[i].name);
        }
    }
    return QStringLiteral("unknown");
}

const QVector<QString>& knownIntentStrings()
{
    static const QVector<QString> s_known = buildKnownIntentStrings();
    return s_known;
}

// ============================================================================
// SelectionSpec
// ============================================================================

SelectionSpec SelectionSpec::fromJson(const QJsonObject& obj)
{
    SelectionSpec spec;
    spec.raw = obj;

    if (obj.isEmpty())
        return spec;  // 默认 None

    const QString modeStr = obj.value(QStringLiteral("mode")).toString();

    if (modeStr == QStringLiteral("current_selection")) {
        spec.mode = SelectionMode::CurrentSelection;
    } else if (modeStr == QStringLiteral("last_created")) {
        spec.mode = SelectionMode::LastCreated;
    } else if (modeStr == QStringLiteral("all")) {
        spec.mode = SelectionMode::All;
    } else {
        // 未知 mode 或空 mode → None
        spec.mode = SelectionMode::None;
    }

    return spec;
}

// ============================================================================
// LLMCommandBridge 公开接口
// ============================================================================

ParsedCommand LLMCommandBridge::parse(const QString& llmRawResponse) const
{
    // ---- 1. 提取 JSON ----
    const QString candidate = extractJsonCandidate(llmRawResponse);
    if (candidate.isEmpty()) {
        ParsedCommand fail;
        fail.ok           = false;
        fail.errorMessage = tr(
            "LLMCommandBridge: unable to extract JSON from LLM response. "
            "The model may have returned text without a valid JSON block.");
        fail.rawText      = llmRawResponse;
        return fail;
    }

    // ---- 2. JSON 解析 ----
    QJsonObject obj;
    QString     parseError;
    if (!tryParseJson(candidate, obj, parseError)) {
        ParsedCommand fail;
        fail.ok           = false;
        fail.errorMessage = tr("LLMCommandBridge: JSON parse failed -- ")
                            + parseError;
        fail.rawText      = llmRawResponse;
        return fail;
    }

    // ---- 3 & 4. 字段校验 + 组装 ----
    return buildFromJson(obj, llmRawResponse);
}

// ============================================================================
// 步骤 1：从原始文本提取 JSON 候选片段
// ============================================================================

QString LLMCommandBridge::extractJsonCandidate(const QString& raw)
{
    if (raw.isEmpty())
        return {};

    // 策略 A：```json ... ``` 围栏（Markdown 代码块）
    // 匹配 ```json（可选）开头，直到 ``` 结束
    static const QRegularExpression fenceRe(
        QStringLiteral(R"(```(?:json)?\s*\n(.*?)\n\s*```)"),
        QRegularExpression::DotMatchesEverythingOption
        | QRegularExpression::MultilineOption
    );
    QRegularExpressionMatch m = fenceRe.match(raw);
    if (m.hasMatch()) {
        return m.captured(1).trimmed();
    }

    // 策略 B：查找最外层 { ... } 作为 JSON 候选
    // 从第一个 '{' 开始，到最后一个 '}' 结束
    int firstBrace = raw.indexOf(QLatin1Char('{'));
    int lastBrace  = raw.lastIndexOf(QLatin1Char('}'));
    if (firstBrace >= 0 && lastBrace > firstBrace) {
        return raw.mid(firstBrace, lastBrace - firstBrace + 1).trimmed();
    }

    // 策略 C：如果整个文本看起来像 JSON（以 { 开头且以 } 结尾），直接使用
    const QString trimmed = raw.trimmed();
    if (trimmed.startsWith(QLatin1Char('{')) && trimmed.endsWith(QLatin1Char('}'))) {
        return trimmed;
    }

    return {};
}

// ============================================================================
// 步骤 2：JSON 字符串 → QJsonObject
// ============================================================================

bool LLMCommandBridge::tryParseJson(const QString& candidate,
                                    QJsonObject&   out,
                                    QString&       errorDetail)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(candidate.toUtf8(), &err);

    if (err.error != QJsonParseError::NoError) {
        errorDetail = err.errorString();
        return false;
    }

    if (!doc.isObject()) {
        errorDetail = tr(
            "JSON root is not an object (expected {...}), got an array or scalar.");
        return false;
    }

    out = doc.object();
    return true;
}

// ============================================================================
// 步骤 3 & 4：字段校验 + 组装
// ============================================================================

ParsedCommand LLMCommandBridge::buildFromJson(const QJsonObject& obj,
                                              const QString&     rawText)
{
    ParsedCommand cmd;
    cmd.rawText = rawText;
    cmd.rawJson = obj;

    // ---- 3a. intent 字段 ----
    if (!obj.contains(QStringLiteral("intent"))) {
        cmd.ok           = false;
        cmd.errorMessage = tr(
            "LLMCommandBridge: missing required field 'intent' in command JSON.");
        return cmd;
    }

    const QJsonValue intentVal = obj.value(QStringLiteral("intent"));
    if (!intentVal.isString()) {
        cmd.ok           = false;
        cmd.errorMessage = tr(
            "LLMCommandBridge: field 'intent' must be a string.");
        return cmd;
    }

    const QString intentStr = intentVal.toString().trimmed();
    cmd.intent = intentFromString(intentStr);

    if (cmd.intent == CommandIntent::Unknown) {
        // 未知 intent：仍标记 ok=true（让下游决定如何处理），但记录警告信息
        cmd.ok      = true;
        cmd.message = tr("Unknown intent: %1. The command may not be "
                                     "executable.").arg(intentStr);
        // 未知 intent 不做进一步校验，直接返回
        return cmd;
    }

    // ---- 3a2. steps 字段（仅 draw_compound） ----
    if (cmd.intent == CommandIntent::DrawCompound
        && obj.contains(QStringLiteral("steps")))
    {
        const QJsonValue stepsVal = obj.value(QStringLiteral("steps"));
        if (stepsVal.isArray())
        {
            const QJsonArray arr = stepsVal.toArray();
            for (const auto& elem : arr)
            {
                if (!elem.isObject())
                    continue;

                const QJsonObject stepObj = elem.toObject();
                if (!stepObj.contains(QStringLiteral("intent")))
                    continue;

                const QJsonValue stepIntentVal = stepObj.value(QStringLiteral("intent"));
                if (!stepIntentVal.isString())
                    continue;

                const CommandIntent stepIntent =
                    intentFromString(stepIntentVal.toString().trimmed());

                // 仅接受已知的绘制类 intent（防止递归嵌套 draw_compound）
                if (stepIntent == CommandIntent::Unknown
                    || stepIntent == CommandIntent::DrawCompound)
                    continue;

                CommandStep step;
                step.intent = stepIntent;
                step.params = stepObj.value(QStringLiteral("params")).toObject();
                cmd.steps.append(step);
            }
        }
        // steps 缺失或非数组：不报错，由下游 executeCompound() 处理空 steps
    }

    // ---- 3b. selection 字段 ----
    if (obj.contains(QStringLiteral("selection"))) {
        const QJsonValue selVal = obj.value(QStringLiteral("selection"));
        if (selVal.isObject()) {
            cmd.selection = SelectionSpec::fromJson(selVal.toObject());
        } else if (!selVal.isNull() && !selVal.isUndefined()) {
            // selection 存在但不是对象也不是 null → 告警但容错
            cmd.selection.mode = SelectionMode::None;
        }
    }
    // selection 缺失 → 保持默认 None

    // ---- 3c. params 字段 ----
    if (obj.contains(QStringLiteral("params"))) {
        const QJsonValue pVal = obj.value(QStringLiteral("params"));
        if (pVal.isObject()) {
            cmd.params = pVal.toObject();
        } else if (!pVal.isNull() && !pVal.isUndefined()) {
            // params 存在但不是对象 → 告警但容错
        }
    }
    // params 缺失 → 保持空 {}

    // ---- 3d. missing_inputs 字段 ----
    if (obj.contains(QStringLiteral("missing_inputs"))) {
        const QJsonValue miVal = obj.value(QStringLiteral("missing_inputs"));
        if (miVal.isArray()) {
            const QJsonArray arr = miVal.toArray();
            for (const auto& item : arr) {
                if (item.isString()) {
                    cmd.missingInputs.append(item.toString());
                }
                // 非字符串元素静默跳过
            }
        }
    }

    // ---- 3e. needs_confirmation 字段 ----
    if (obj.contains(QStringLiteral("needs_confirmation"))) {
        const QJsonValue ncVal = obj.value(QStringLiteral("needs_confirmation"));
        if (ncVal.isBool()) {
            cmd.needsConfirmation = ncVal.toBool();
        }
        // 非 bool 类型 → 保持默认 false
    }

    // ---- 3f. message 字段 ----
    if (obj.contains(QStringLiteral("message"))) {
        const QJsonValue msgVal = obj.value(QStringLiteral("message"));
        if (msgVal.isString()) {
            cmd.message = msgVal.toString();
        }
    }

    // ---- 全部通过 ----
    cmd.ok = true;
    return cmd;
}


