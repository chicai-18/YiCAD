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

/// @file DirectEntityExecutor.h
/// @brief 直接绘图执行器 —— 解析 ParsedCommand，直接创建图元（点/线/圆/矩形/椭圆）
///
/// 职责：
///   - 接收 LLMCommandBridge 产出的 ParsedCommand
///   - 针对 draw_point / draw_line / draw_circle / draw_rectangle / draw_ellipse / draw_arc
///     六种意图，从 params 中提取几何参数并直接构造 DmEntity 子类
///   - 通过 Transaction 包裹创建过程，保证 Undo/Redo
///   - 通过 EntityTable::add 标准路径添加实体
///   - 不做修改、不做删除
///
/// 已知限制（当前版本）：
///   - 仅支持 6 种直接创建绘制命令
///   - 不支持块参照、标注、文字、样条曲线、多段线(自由形式)
///   - 不支持 selection / missing_inputs 处理
///   - params 校验较宽容（缺参静默失败）
///   - 椭圆仅支持完整闭合椭圆，不支持椭圆弧
///   - 矩形使用轴对齐角点模式（corner1/corner2）

#ifndef DIRECTENTITYEXECUTOR_H
#define DIRECTENTITYEXECUTOR_H

#include "LLMCommandBridge.h"

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVector>
#include "DmVector.h"
#include "DmId.h"

class DmDocument;
class DmEntity;
class DmId;
class DmPoint;
class DmLine;
class DmCircle;
class DmPolyline;
class DmArc;
class DmEllipse;
class DmText;

// ============================================================================
// 执行结果
// ============================================================================

/// @brief 单个实体创建结果
struct EntityCreateResult
{
    DmId    entityId;       ///< 创建的实体 ID（失败时无效）
    QString entityType;     ///< 实体类型名（如 "DmLine"），调试用
};

/// @brief DirectEntityExecutor::execute() 返回值
struct ExecutorResult
{
    bool    success = false;        ///< 整体执行是否成功
    QString errorMessage;           ///< 失败时的错误描述（人类可读）

    /// 本次调用创建的所有实体（可多个，如矩形内部可能产生一条多段线）
    QVector<EntityCreateResult> createdEntities;
};

// ============================================================================
// DirectEntityExecutor
// ============================================================================

/// @brief 直接绘图执行器
///
/// 无状态；构造时传入 DmDocument*，后续反复调用 execute()。
class DirectEntityExecutor : public QObject
{
    Q_OBJECT
public:
    /// @brief 构造函数
    /// @param doc 目标文档（非空）
    /// @param parent 父 QObject（可选）
    explicit DirectEntityExecutor(DmDocument* doc, QObject* parent = nullptr);

    ~DirectEntityExecutor() = default;

    /// @brief 执行一条已解析的命令
    /// @param cmd  LLMCommandBridge::parse() 产出的 ParsedCommand
    /// @return ExecutorResult  调用方应首先检查 success 字段
    ///
    /// 对于未知 intent 或非绘制类 intent，返回 success=false 并附带错误信息。
    ExecutorResult execute(const ParsedCommand& cmd);

    /// @brief 执行复合绘制命令（intent == DrawCompound）
    /// @param cmd  ParsedCommand，其 steps 数组含多个图元步骤
    /// @return ExecutorResult  所有步骤在一个 Transaction 内原子执行
    ///
    /// 任意步骤失败则整体回滚；全部成功则一次性提交。
    ExecutorResult executeCompound(const ParsedCommand& cmd);

private:
    // ---- 各 intent 的实现 ----
    ExecutorResult executeDrawPoint(const ParsedCommand& cmd);
    ExecutorResult executeDrawLine(const ParsedCommand& cmd);
    ExecutorResult executeDrawCircle(const ParsedCommand& cmd);
    ExecutorResult executeDrawRectangle(const ParsedCommand& cmd);
    ExecutorResult executeDrawEllipse(const ParsedCommand& cmd);
    ExecutorResult executeDrawArc(const ParsedCommand& cmd);
    ExecutorResult executeDrawText(const ParsedCommand& cmd);

    // ---- 图元创建（纯工厂方法，不含 Transaction 管理） ----

    /// @brief 从 params 创建 DmPoint 实体（调用方接管生命周期）
    /// @param params    JSON 参数对象
    /// @param errorOut  失败时填充错误描述
    /// @return 创建的实体（调用方负责 delete），失败返回 nullptr
    static DmPoint* createPointEntity(const QJsonObject& params, QString& errorOut);

    /// @brief 从 params 创建 DmLine 实体
    static DmLine* createLineEntity(const QJsonObject& params, QString& errorOut);

    /// @brief 从 params 创建 DmCircle 实体
    static DmCircle* createCircleEntity(const QJsonObject& params, QString& errorOut);

    /// @brief 从 params 创建 DmPolyline 实体（矩形）
    static DmPolyline* createRectangleEntity(const QJsonObject& params, QString& errorOut);

    /// @brief 从 params 创建 DmEllipse 实体
    static DmEllipse* createEllipseEntity(const QJsonObject& params, QString& errorOut);

    /// @brief 从 params 创建 DmArc 实体
    static DmArc* createArcEntity(const QJsonObject& params, QString& errorOut);

    /// @brief 从 params 创建 DmText 实体
    DmText* createTextEntity(const QJsonObject& params, QString& errorOut);

    /// @brief 将 CommandIntent 映射为实体类型名（如 DrawLine → "DmLine"）
    static QString entityTypeName(CommandIntent intent);

    // ---- 工具函数：从 JSON params 提取参数 ----

    /// @brief 从 JSON 对象中提取 DmVector（期望 [x, y] 数组）
    static bool extractPoint(const QJsonObject& params,
                             const QString&     key,
                             DmVector&          out,
                             QString&           errorDetail);

    /// @brief 从 JSON 对象中提取 double 值
    static bool extractDouble(const QJsonObject& params,
                              const QString&     key,
                              double&            out,
                              QString&           errorDetail);

    // ---- 工具函数：完成实体创建的标准收尾 ----

    /// @brief 将实体通过事务添加到文档（setDocument → add → commit）
    /// @param entity  已 new 出的实体（由本函数接管生命周期）
    /// @param txName  事务名称
    /// @param typeName  实体类型名（调试用）
    /// @param result  输出：填充 createdEntities
    void finalizeEntity(DmEntity*          entity,
                        const std::string& txName,
                        const QString&     typeName,
                        ExecutorResult&    result);

    DmDocument* m_doc;  ///< 目标文档（非空）
};

#endif // DIRECTENTITYEXECUTOR_H
