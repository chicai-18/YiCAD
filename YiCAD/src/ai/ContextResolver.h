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

/// @file ContextResolver.h
/// @brief 上下文解析器 —— 将 LLM 输出中的自然语言引用解析为真实实体 ID
///
/// 职责：
///   - 接收 ParsedCommand::selection（SelectionSpec），解析出 DmId[] 列表
///   - 支持三类主要上下文来源：
///       1. 当前选择集    (SelectionMode::CurrentSelection)
///       2. 最近创建对象  (SelectionMode::LastCreated)
///       3. 上一轮执行结果 (TurnRecord::createdEntityIds / modifiedEntityIds)
///   - 支持从 raw JSON 中提取 entity_type / description 等提示做类型过滤
///   - 每次解析产出可解释的 explanation 文本（人类可读）
///   - 不编造 ID：所有 ID 均来自文档 EntityTable 或 TurnRecord
///
/// 最小连续对话支持：
///   - 每轮建模执行后调用 recordTurn() 记录本轮创建的实体
///   - 下轮 LLM 输出 selection.mode == "last_created" 时，
///     自动从最近一轮 TurnRecord 中提取对应实体
///   - 若 LLM 在 raw 中附带 entity_type 提示（如 "circle"），则按类型过滤
///
/// 使用方式：
///   @code
///     // 构造
///     ContextResolver resolver(doc);
///
///     // 每轮 LLM 解析后
///     ParsedCommand cmd = bridge.parse(llmResponse);
///     ResolvedSelection sel = resolver.resolve(cmd.selection);
///     if (!sel.ok) {
///         // 展示 sel.errorMessage
///         return;
///     }
///     // 使用 sel.entities 中的 DmId
///     // 展示 sel.explanation 给用户
///
///     // 执行完成后记录本轮
///     TurnRecord turn;
///     turn.userInput = userText;
///     turn.intent    = cmd.intent;
///     turn.createdEntityIds = { ... };
///     resolver.recordTurn(turn);
///   @endcode
///
/// 已知限制（当前版本）：
///   - 仅支持单层对话历史（只查最近一轮 TurnRecord）
///   - entity_type 映射基于 DM::EntityType 枚举的硬编码表
///   - 不支持跨轮引用（如"上上轮创建的线"）
///   - 不支持自然语言描述匹配（如"最长的线"）
///   - 所有选择模式均在此解析

#ifndef CONTEXTRESOLVER_H
#define CONTEXTRESOLVER_H

#include "LLMCommandBridge.h"  // SelectionSpec, SelectionMode, CommandIntent

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVector>
#include "DmId.h"

class DmDocument;
class DmEntity;

// ============================================================================
// 解析出的实体引用
// ============================================================================

/// @brief 单个已解析的实体引用
///
/// 记录「哪个实体」、「是什么类型」、「从何而来」。
struct ResolvedEntityRef
{
    DmId    entityId;       ///< 解析出的真实实体 ID（isValid() 为 true）
    QString entityType;     ///< 实体类型可读名（如 "DmLine"、"DmCircle"）
    QString howResolved;    ///< 解析来源标记："selected" / "last_created" / "previous_turn"

    /// @brief 调试用：简要描述该引用
    QString describe() const;
};

// ============================================================================
// 解析结果
// ============================================================================

/// @brief ContextResolver::resolve() 的返回值
struct ResolvedSelection
{
    bool    ok = false;             ///< 解析是否成功
    QString errorMessage;           ///< 失败时的错误描述（人类可读）
    QVector<ResolvedEntityRef> entities;  ///< 解析出的实体引用列表
    QString explanation;            ///< 人类可读的解析说明

    /// @brief 便捷方法：获取所有实体 ID
    QVector<DmId> entityIds() const;
};

// ============================================================================
// 对话轮次记录
// ============================================================================

/// @brief 单轮对话记录
///
/// 每轮建模执行完成后由上层调用 recordTurn() 写入。
struct TurnRecord
{
    QString       userInput;          ///< 用户本轮原始输入
    CommandIntent intent;             ///< 本轮执行的命令意图
    QVector<DmId> createdEntityIds;   ///< 本轮创建的实体 ID 列表
    QVector<DmId> modifiedEntityIds;  ///< 本轮修改的实体 ID 列表
};

// ============================================================================
// ContextResolver
// ============================================================================

/// @brief 上下文解析器
///
/// 无状态（除对话历史外）；一个实例可反复调用 resolve()。
///
/// 典型调用序列：
///   1. 构造时传入 DmDocument*
///   2. 每轮调用 resolve(spec) 解析实体引用
///   3. 执行完成后调用 recordTurn() 记录本轮结果
///   4. 用户"新建"或"重置对话"时调用 clearHistory()
class ContextResolver : public QObject
{
    Q_OBJECT
public:
    /// @brief 构造函数
    /// @param doc 目标文档（非空，用于查询 EntityTable）
    /// @param parent 父 QObject（可选）
    explicit ContextResolver(DmDocument* doc, QObject* parent = nullptr);

    ~ContextResolver() = default;

    // ---- 核心解析 ----

    /// @brief 解析 SelectionSpec，将自然语言引用映射为真实 DmId 列表
    /// @param spec  LLM 输出的选择说明（含 mode 和 raw 提示）
    /// @return ResolvedSelection  调用方应首先检查 ok 字段
    ///
    /// 解析优先级：
    ///   1. CurrentSelection → 从文档 EntityTable 遍历 isSelected() 实体
    ///   2. LastCreated       → 优先查 TurnRecord，其次查文档所有实体(回退)
    ///   3. All               → 所有可见实体
    ///   4. None              → 空列表，ok=true
    ///
    /// 若 spec.raw 中包含 "entity_type" 字段，则在结果上做类型过滤。
    ResolvedSelection resolve(const SelectionSpec& spec) const;

    // ---- 对话历史 ----

    /// @brief 记录一轮对话的执行结果（供后续 LastCreated 解析）
    /// @param turn  本轮对话记录
    ///
    /// 历史只保留最近若干轮（当前版本：最近 1 轮）。
    void recordTurn(const TurnRecord& turn);

    /// @brief 获取对话历史（只读）
    const QVector<TurnRecord>& history() const;

    /// @brief 清空对话历史（如用户新建文档或手动重置）
    void clearHistory();

private:
    // ---- 各 mode 的实现 ----

    /// @brief 解析 CurrentSelection 模式
    ResolvedSelection resolveCurrentSelection(const QJsonObject& raw) const;

    /// @brief 解析 LastCreated 模式
    ResolvedSelection resolveLastCreated(const QJsonObject& raw) const;

    /// @brief 解析 All 模式
    ResolvedSelection resolveAll(const QJsonObject& raw) const;

    // ---- 类型过滤 ----

    /// @brief 从 raw JSON 中提取 entity_type 提示（如 "line" → DM::EntityLine）
    /// @return 如果 raw 中没有 entity_type 或无法识别，返回 EntityUnknown（不过滤）
    static int entityTypeFromHint(const QJsonObject& raw);

    /// @brief 将 DM::EntityType 枚举值转为人类可读字符串（如 EntityLine → "DmLine"）
    static QString entityTypeName(int entityType);

    /// @brief 判断一个实体的 DM::EntityType 是否匹配给定的类型提示枚举值
    static bool entityMatchesType(const DmEntity* e, int typeHint);

    // ---- 工具 ----

    /// @brief 从当前文档的 EntityTable 收集所有 selected 实体
    QVector<ResolvedEntityRef> collectSelectedEntities() const;

    /// @brief 从当前文档的 EntityTable 收集所有可见实体
    QVector<ResolvedEntityRef> collectAllVisibleEntities() const;

    /// @brief 按 entity_type 过滤实体列表
    static void filterByEntityType(QVector<ResolvedEntityRef>& entities,
                                   int typeHint);

    DmDocument*          m_doc;      ///< 目标文档（非空）
    QVector<TurnRecord>  m_history;  ///< 对话历史（当前版本最多保留 1 条）
};

#endif // CONTEXTRESOLVER_H
