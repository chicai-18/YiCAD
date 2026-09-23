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

/// @file LLMCommandBridge.h
/// @brief 建模链路结构化 JSON 解析与校验桥梁
///
/// 职责：
///   - 接收 LLM 返回的原始文本
///   - 提取并解析为结构化 JSON（Command Protocol）
///   - 校验必要字段、类型、值域
///   - 对非法 JSON / 缺失字段做兜底错误处理
///   - 标记高风险操作（needs_confirmation）
///   - 仅产出 ParsedCommand，不连接任何执行器
///
/// 使用方式：
///   @code
///     LLMCommandBridge bridge;
///     ParsedCommand cmd = bridge.parse(llmRawResponse);
///     if (!cmd.ok) {
///         // 展示 cmd.errorMessage 给用户
///         return;
///     }
///     if (cmd.needsConfirmation) {
///         // 展示 cmd.message 并要求用户确认
///     }
///     // 将 cmd 交给下游 ContextResolver / Executor
///   @endcode
///
/// 已知限制（当前版本）：
///   - 仅支持 JSON 协议解析，不做真实实体 ID 补全
///   - 不连接网络、不调用 LLM
///   - intent 白名单硬编码在源码中（参见 kKnownIntents）
///   - selection / params 不做深层业务语义校验

#ifndef LLMCOMMANDBRIDGE_H
#define LLMCOMMANDBRIDGE_H

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVector>

// ============================================================================
// 选择说明 —— 描述"操作哪些实体"
// ============================================================================

/// @brief 选择模式
enum class SelectionMode
{
    None,              ///< 无需选择（如新建实体）
    CurrentSelection,  ///< 使用当前画布已选中的实体
    LastCreated,       ///< 使用最近创建的实体
    All,               ///< 操作全部实体（如全选删除）
};

/// @brief 选择说明
struct SelectionSpec
{
    SelectionMode mode      = SelectionMode::None;

    /// 原始 JSON 中 selection 字段内容（透传，供下游 ContextResolver 使用）
    QJsonObject   raw;

    /// @brief 从 JSON 对象反序列化
    static SelectionSpec fromJson(const QJsonObject& obj);
};

// ============================================================================
// 已知命令意图
// ============================================================================

/// @brief 命令意图 —— 枚举与字符串互转
///
/// 新增意图只需在 kKnownIntents 表中添加一行即可，
/// 无需修改解析逻辑。
enum class CommandIntent
{
    Unknown,

    // ---- 绘制 ----
    DrawPoint,
    DrawLine,
    DrawCircle,
    DrawArc,
    DrawRectangle,
    DrawEllipse,
    DrawSpline,
    DrawPolyline,
    DrawText,
    DrawCompound,      ///< 复合绘制（steps 数组中含多个图元步骤）

    // ---- 标注 ----
    Dimension,
    DimensionLinear,
    DimensionAligned,
    DimensionAngular,
    DimensionRadial,
    DimensionDiametric,

    // ---- 图块 ----
    BlockInsert,
    BlockCreate,
};

// ============================================================================
// 复合命令步骤
// ============================================================================

/// @brief 复合命令中的单个图元步骤
///
/// 仅当 ParsedCommand::intent == DrawCompound 时使用。
/// steps 数组中每个元素为一个 CommandStep。
struct CommandStep
{
    CommandIntent intent = CommandIntent::Unknown;  ///< 图元类型
    QJsonObject   params;                            ///< 图元参数（透传）
};

/// @brief 将 intent 字符串转为枚举（如 "draw_circle" → CommandIntent::DrawCircle）
CommandIntent intentFromString(const QString& s);

/// @brief 将 intent 枚举转为字符串（如 CommandIntent::DrawCircle → "draw_circle"）
QString intentToString(CommandIntent intent);

/// @brief intent 白名单集合（包含所有已知 intent 字符串）
const QVector<QString>& knownIntentStrings();

// ============================================================================
// 解析结果
// ============================================================================

/// @brief LLM 命令解析结果 —— LLMCommandBridge::parse() 的返回值
struct ParsedCommand
{
    // ---- 解析状态 ----
    bool    ok           = false;   ///< 整体解析是否成功
    QString errorMessage;           ///< 失败时的错误描述（人类可读）

    // ---- 协议字段 ----
    CommandIntent intent            = CommandIntent::Unknown;  ///< 命令意图
    SelectionSpec selection;                                   ///< 操作对象选择方式
    QJsonObject   params;                                      ///< 命令参数（透传）
    QStringList   missingInputs;                               ///< 缺失的输入项
    bool          needsConfirmation = false;                   ///< 是否需要用户确认
    QString       message;                                     ///< 人类可读的操作描述
    QVector<CommandStep> steps;                                ///< 复合命令的步骤列表（仅 intent == DrawCompound 时有效）

    // ---- 原始数据（调试用） ----
    QString       rawText;          ///< LLM 返回的完整原始文本
    QJsonObject   rawJson;          ///< 解析出的完整 JSON 对象（可能为空）
};

// ============================================================================
// LLMCommandBridge
// ============================================================================

/// @brief 建模链路命令桥 —— 解析、校验、兜底
///
/// 无状态；一个实例可反复调用 parse()。
class LLMCommandBridge : public QObject
{
    Q_OBJECT
public:
    explicit LLMCommandBridge(QObject* parent = nullptr) : QObject(parent) {}
    ~LLMCommandBridge() override = default;

    /// @brief 解析 LLM 返回的原始文本为结构化命令
    /// @param llmRawResponse  LLM 返回的完整文本（可能含 Markdown 包裹的 JSON）
    /// @return ParsedCommand  解析结果，调用方应首先检查 ok 字段
    ///
    /// 解析流程：
    ///   1. 从原始文本中提取 JSON 片段（处理 ```json ... ``` 包裹）
    ///   2. JSON 解析
    ///   3. 逐字段校验
    ParsedCommand parse(const QString& llmRawResponse) const;

private:
    // ---- 步骤 1：提取 JSON ----
    static QString extractJsonCandidate(const QString& raw);

    // ---- 步骤 2：JSON → QJsonObject ----
    static bool tryParseJson(const QString& candidate, QJsonObject& out,
                             QString& errorDetail);

    // ---- 步骤 3：字段校验 + 组装 ----
    static ParsedCommand buildFromJson(const QJsonObject& obj,
                                       const QString& rawText);
};

#endif // LLMCOMMANDBRIDGE_H
