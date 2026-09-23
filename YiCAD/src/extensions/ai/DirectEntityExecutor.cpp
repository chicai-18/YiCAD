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

/// @file DirectEntityExecutor.cpp
/// @brief DirectEntityExecutor 实现 —— 直接创建点/线/圆/矩形/椭圆

#include "DirectEntityExecutor.h"

#include "CircleData.h"
#include "DmCircle.h"
#include "DmDocument.h"
#include "ArcData.h"
#include "DmArc.h"
#include "DmEllipse.h"
#include "DmText.h"
#include "DmTextStyle.h"
#include "DmTextStyleTable.h"
#include "DmEntity.h"
#include "DmId.h"
#include "DmLine.h"
#include "DmPoint.h"
#include "DmPolyline.h"
#include "DmVector.h"
#include "EllipseData.h"
#include "EntityTable.h"
#include "LineData.h"
#include "PointData.h"
#include "PolylineData.h"
#include "Transaction.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#include <cmath>  // M_PI

// ============================================================================
// DirectEntityExecutor 公开接口
// ============================================================================

DirectEntityExecutor::DirectEntityExecutor(DmDocument* doc, QObject* parent)
    : QObject(parent)
    , m_doc(doc)
{
}

ExecutorResult DirectEntityExecutor::execute(const ParsedCommand& cmd)
{
    // 仅处理直接创建绘制命令
    switch (cmd.intent) {
    case CommandIntent::DrawPoint:
        return executeDrawPoint(cmd);
    case CommandIntent::DrawLine:
        return executeDrawLine(cmd);
    case CommandIntent::DrawCircle:
        return executeDrawCircle(cmd);
    case CommandIntent::DrawRectangle:
        return executeDrawRectangle(cmd);
    case CommandIntent::DrawEllipse:
        return executeDrawEllipse(cmd);
    case CommandIntent::DrawArc:
        return executeDrawArc(cmd);
    case CommandIntent::DrawText:
        return executeDrawText(cmd);

    default: {
        ExecutorResult fail;
        fail.success      = false;
        fail.errorMessage = tr(
            "DirectEntityExecutor: unsupported intent '%1'. "
            "Only draw_point / draw_line / draw_circle / draw_rectangle / draw_ellipse / draw_arc "
            "are supported.").arg(intentToString(cmd.intent));
        return fail;
    }
    }
}

// ============================================================================
// 复合绘制命令
// ============================================================================

ExecutorResult DirectEntityExecutor::executeCompound(const ParsedCommand& cmd)
{
    ExecutorResult result;

    if (cmd.steps.isEmpty()) {
        result.success      = false;
        result.errorMessage = tr(
            "DirectEntityExecutor::draw_compound: steps array is empty.");
        return result;
    }

    // 所有步骤在一个 Transaction 内原子执行
    Transaction t(tr("Create Compound").toStdString(), m_doc);
    t.start();

    int i = 0;
    for (const CommandStep& step : cmd.steps)
    {
        ++i;

        DmEntity* entity = nullptr;
        QString   stepError;

        switch (step.intent)
        {
        case CommandIntent::DrawPoint:
            entity = createPointEntity(step.params, stepError);
            break;
        case CommandIntent::DrawLine:
            entity = createLineEntity(step.params, stepError);
            break;
        case CommandIntent::DrawCircle:
            entity = createCircleEntity(step.params, stepError);
            break;
        case CommandIntent::DrawRectangle:
            entity = createRectangleEntity(step.params, stepError);
            break;
        case CommandIntent::DrawEllipse:
            entity = createEllipseEntity(step.params, stepError);
            break;
        case CommandIntent::DrawArc:
            entity = createArcEntity(step.params, stepError);
            break;
        case CommandIntent::DrawText:
            entity = createTextEntity(step.params, stepError);
            break;
        default:
            t.rollback();
            result.success      = false;
            result.errorMessage = tr(
                "draw_compound step %1: unsupported intent '%2'")
                    .arg(i).arg(intentToString(step.intent));
            return result;
        }

        if (!entity) {
            t.rollback();
            result.success      = false;
            result.errorMessage = tr(
                "draw_compound step %1: failed to create entity -- %2")
                    .arg(i).arg(stepError);
            return result;
        }

        entity->setDocument(m_doc);
        m_doc->getEntityTable()->add(entity);

        EntityCreateResult ecr;
        ecr.entityId   = entity->getId();
        ecr.entityType = entityTypeName(step.intent);
        result.createdEntities.append(ecr);
    }

    t.commit();

    result.success = true;
    return result;
}

// ============================================================================
// 各 intent 实现
// ============================================================================

ExecutorResult DirectEntityExecutor::executeDrawPoint(const ParsedCommand& cmd)
{
    ExecutorResult result;

    QString err;
    DmPoint* entity = createPointEntity(cmd.params, err);
    if (!entity) {
        result.success      = false;
        result.errorMessage = tr("DirectEntityExecutor::draw_point -- %1").arg(err);
        return result;
    }

    finalizeEntity(entity, tr("Create Point").toStdString(), QStringLiteral("DmPoint"), result);
    return result;
}

ExecutorResult DirectEntityExecutor::executeDrawLine(const ParsedCommand& cmd)
{
    ExecutorResult result;

    QString err;
    DmLine* entity = createLineEntity(cmd.params, err);
    if (!entity) {
        result.success      = false;
        result.errorMessage = tr("DirectEntityExecutor::draw_line -- %1").arg(err);
        return result;
    }

    finalizeEntity(entity, tr("Create Line").toStdString(), QStringLiteral("DmLine"), result);
    return result;
}

ExecutorResult DirectEntityExecutor::executeDrawCircle(const ParsedCommand& cmd)
{
    ExecutorResult result;

    QString err;
    DmCircle* entity = createCircleEntity(cmd.params, err);
    if (!entity) {
        result.success      = false;
        result.errorMessage = tr("DirectEntityExecutor::draw_circle -- %1").arg(err);
        return result;
    }

    finalizeEntity(entity, tr("Create Circle").toStdString(), QStringLiteral("DmCircle"), result);
    return result;
}

ExecutorResult DirectEntityExecutor::executeDrawRectangle(const ParsedCommand& cmd)
{
    ExecutorResult result;

    QString err;
    DmPolyline* entity = createRectangleEntity(cmd.params, err);
    if (!entity) {
        result.success      = false;
        result.errorMessage = tr("DirectEntityExecutor::draw_rectangle -- %1").arg(err);
        return result;
    }

    finalizeEntity(entity, tr("Create Rectangle").toStdString(), QStringLiteral("DmPolyline"), result);
    return result;
}

ExecutorResult DirectEntityExecutor::executeDrawEllipse(const ParsedCommand& cmd)
{
    ExecutorResult result;

    QString err;
    DmEllipse* entity = createEllipseEntity(cmd.params, err);
    if (!entity) {
        result.success      = false;
        result.errorMessage = tr("DirectEntityExecutor::draw_ellipse -- %1").arg(err);
        return result;
    }

    finalizeEntity(entity, tr("Create Ellipse").toStdString(), QStringLiteral("DmEllipse"), result);
    return result;
}

ExecutorResult DirectEntityExecutor::executeDrawArc(const ParsedCommand& cmd)
{
    ExecutorResult result;

    QString err;
    DmArc* entity = createArcEntity(cmd.params, err);
    if (!entity) {
        result.success      = false;
        result.errorMessage = tr("DirectEntityExecutor::draw_arc -- %1").arg(err);
        return result;
    }

    finalizeEntity(entity, tr("Create Arc").toStdString(), QStringLiteral("DmArc"), result);
    return result;
}

ExecutorResult DirectEntityExecutor::executeDrawText(const ParsedCommand& cmd)
{
    ExecutorResult result;

    QString err;
    DmText* entity = createTextEntity(cmd.params, err);
    if (!entity) {
        result.success      = false;
        result.errorMessage = tr("DirectEntityExecutor::draw_text -- %1").arg(err);
        return result;
    }

    finalizeEntity(entity, tr("Create Text").toStdString(), QStringLiteral("DmText"), result);
    return result;
}

// ============================================================================
// 图元创建（纯工厂方法，不含 Transaction 管理）
// ============================================================================

DmPoint* DirectEntityExecutor::createPointEntity(const QJsonObject& params, QString& errorOut)
{
    DmVector pos;
    if (!extractPoint(params, QStringLiteral("position"), pos, errorOut))
        return nullptr;

    PointData data(pos);
    return new DmPoint(nullptr, data);
}

DmLine* DirectEntityExecutor::createLineEntity(const QJsonObject& params, QString& errorOut)
{
    DmVector start, end;
    if (!extractPoint(params, QStringLiteral("start"), start, errorOut))
        return nullptr;
    if (!extractPoint(params, QStringLiteral("end"), end, errorOut))
        return nullptr;

    LineData data(start, end);
    return new DmLine(nullptr, data);
}

DmCircle* DirectEntityExecutor::createCircleEntity(const QJsonObject& params, QString& errorOut)
{
    DmVector center;
    double  radius = 0.0;
    if (!extractPoint(params, QStringLiteral("center"), center, errorOut))
        return nullptr;
    if (!extractDouble(params, QStringLiteral("radius"), radius, errorOut))
        return nullptr;

    CircleData data(center, radius);
    return new DmCircle(nullptr, data);
}

DmPolyline* DirectEntityExecutor::createRectangleEntity(const QJsonObject& params, QString& errorOut)
{
    DmVector corner1, corner2;
    if (!extractPoint(params, QStringLiteral("corner1"), corner1, errorOut))
        return nullptr;
    if (!extractPoint(params, QStringLiteral("corner2"), corner2, errorOut))
        return nullptr;

    // 用 corner1/corner2 推导四个角点（轴对齐矩形）
    DmVector p1(corner1);
    DmVector p2(corner2.x, corner1.y);
    DmVector p3(corner2);
    DmVector p4(corner1.x, corner2.y);

    std::vector<DmVector> pts{ p1, p2, p3, p4 };
    std::vector<double>   bulges(4, 0.0);
    std::vector<double>   weights(8, 0.0);

    PolylineData polyData(pts, bulges, weights, /*isClosed=*/true);
    return new DmPolyline(nullptr, polyData);
}

DmEllipse* DirectEntityExecutor::createEllipseEntity(const QJsonObject& params, QString& errorOut)
{
    DmVector center;
    double  majorRadius = 0.0;
    double  minorRadius = 0.0;
    double  angle       = 0.0;

    if (!extractPoint(params, QStringLiteral("center"), center, errorOut))
        return nullptr;
    if (!extractDouble(params, QStringLiteral("major_radius"), majorRadius, errorOut))
        return nullptr;
    if (!extractDouble(params, QStringLiteral("minor_radius"), minorRadius, errorOut))
        return nullptr;
    // angle 可选，默认为 0
    extractDouble(params, QStringLiteral("angle"), angle, errorOut);

    const double angleRad = angle * M_PI / 180.0;

    // 长轴端点向量（相对于中心，长度 = majorRadius）
    DmVector majorP(majorRadius * std::cos(angleRad),
                    majorRadius * std::sin(angleRad));
    // 法向量（2D 绘图，Z 轴向上）
    DmVector normal(0.0, 0.0, 1.0);
    // 短轴与长轴的比率
    const double ratio = (majorRadius > 0.0)
                         ? (minorRadius / majorRadius)
                         : 1.0;

    EllipseData data(center, majorP, normal, ratio,
                     /*isClosed=*/true,
                     /*startParam=*/0.0,
                     /*endParam=*/2.0 * M_PI);

    return new DmEllipse(nullptr, data);
}

DmArc* DirectEntityExecutor::createArcEntity(const QJsonObject& params, QString& errorOut)
{
    DmVector center;
    double  radius      = 0.0;
    double  startAngle  = 0.0;
    double  endAngle    = 0.0;

    if (!extractPoint(params, QStringLiteral("center"), center, errorOut))
        return nullptr;
    if (!extractDouble(params, QStringLiteral("radius"), radius, errorOut))
        return nullptr;
    if (!extractDouble(params, QStringLiteral("start_angle"), startAngle, errorOut))
        return nullptr;
    if (!extractDouble(params, QStringLiteral("end_angle"), endAngle, errorOut))
        return nullptr;

    // LLM 传入的是角度制，转为弧度制
    const double startRad = startAngle * M_PI / 180.0;
    const double endRad   = endAngle   * M_PI / 180.0;

    // 2D 绘图，法向量固定为 Z 轴向上
    DmVector normal(0.0, 0.0, 1.0);

    ArcData data(center, normal, radius, startRad, endRad);
    return new DmArc(nullptr, data);
}

DmText* DirectEntityExecutor::createTextEntity(const QJsonObject& params, QString& errorOut)
{
    // ---- 1. 提取必选参数 ----
    DmVector position;
    if (!extractPoint(params, QStringLiteral("position"), position, errorOut))
        return nullptr;

    double height = 0.0;
    if (!extractDouble(params, QStringLiteral("height"), height, errorOut))
        return nullptr;

    QString text;
    if (!params.contains(QStringLiteral("text"))) {
        errorOut = tr("missing required param 'text'");
        return nullptr;
    }
    text = params.value(QStringLiteral("text")).toString();
    if (text.isEmpty()) {
        errorOut = tr("param 'text' must be a non-empty string");
        return nullptr;
    }

    // ---- 2. 提取可选参数 ----
    double angle = 0.0;
    extractDouble(params, QStringLiteral("angle"), angle, errorOut);
    errorOut.clear(); // angle 可选，清空 extractDouble 可能设置的错误

    // 水平对齐：left / center / right（默认 left）
    ETextHorzMode halign = ETextHorzMode::kTextLeft;
    if (params.contains(QStringLiteral("halign"))) {
        const QString ha = params.value(QStringLiteral("halign")).toString().toLower();
        if (ha == QStringLiteral("center"))
            halign = ETextHorzMode::kTextCenter;
        else if (ha == QStringLiteral("right"))
            halign = ETextHorzMode::kTextRight;
        else if (ha == QStringLiteral("mid"))
            halign = ETextHorzMode::kTextMid;
    }

    // 垂直对齐：top / middle / bottom / baseline（默认 baseline）
    ETextVertMode valign = ETextVertMode::kTextBase;
    if (params.contains(QStringLiteral("valign"))) {
        const QString va = params.value(QStringLiteral("valign")).toString().toLower();
        if (va == QStringLiteral("top"))
            valign = ETextVertMode::kTextTop;
        else if (va == QStringLiteral("middle"))
            valign = ETextVertMode::kTextVertMid;
        else if (va == QStringLiteral("bottom"))
            valign = ETextVertMode::kTextBottom;
    }

    // ---- 3. 获取文字样式 ----
    DmTextStyle* style = nullptr;
    if (m_doc) {
        DmTextStyleTable* styleTable = m_doc->getTextStyleTable();
        if (styleTable) {
            style = styleTable->getActive();
            if (!style) {
                style = DmTextStyleTable::getDefaultStyle();
            }
        }
    }
    if (!style) {
        style = DmTextStyleTable::getDefaultStyle();
    }

    // ---- 4. 构造 TextData 并创建 DmText ----
    const double angleRad = angle * M_PI / 180.0;

    TextData textData(position, height, valign, halign, text, style, angleRad);
    // 非左对齐时 DmText 使用 m_ptAlignment 定位，必须设置对齐点
    textData.setAlignment(position);
    return new DmText(nullptr, textData);
}

QString DirectEntityExecutor::entityTypeName(CommandIntent intent)
{
    switch (intent) {
    case CommandIntent::DrawPoint:     return QStringLiteral("DmPoint");
    case CommandIntent::DrawLine:      return QStringLiteral("DmLine");
    case CommandIntent::DrawCircle:    return QStringLiteral("DmCircle");
    case CommandIntent::DrawRectangle: return QStringLiteral("DmPolyline");
    case CommandIntent::DrawEllipse:   return QStringLiteral("DmEllipse");
    case CommandIntent::DrawArc:       return QStringLiteral("DmArc");
    case CommandIntent::DrawText:      return QStringLiteral("DmText");
    default:                           return QStringLiteral("DmEntity");
    }
}

// ============================================================================
// 工具函数：参数提取
// ============================================================================

bool DirectEntityExecutor::extractPoint(const QJsonObject& params,
                                        const QString&     key,
                                        DmVector&          out,
                                        QString&           errorDetail)
{
    if (!params.contains(key)) {
        errorDetail = tr("missing required param '%1'").arg(key);
        return false;
    }

    const QJsonValue val = params.value(key);
    if (!val.isArray()) {
        errorDetail = tr("param '%1' must be a JSON array [x, y]").arg(key);
        return false;
    }

    const QJsonArray arr = val.toArray();
    if (arr.size() < 2) {
        errorDetail = tr("param '%1' array must have at least 2 elements (x, y)")
                          .arg(key);
        return false;
    }

    // 元素应为数值类型（QJsonValue::isDouble 对整数也返回 true）
    if (!arr.at(0).isDouble()) {
        errorDetail = tr("param '%1'[0] (x) is not a number").arg(key);
        return false;
    }
    if (!arr.at(1).isDouble()) {
        errorDetail = tr("param '%1'[1] (y) is not a number").arg(key);
        return false;
    }

    const double x = arr.at(0).toDouble();
    const double y = arr.at(1).toDouble();

    out = DmVector(x, y);
    return true;
}

bool DirectEntityExecutor::extractDouble(const QJsonObject& params,
                                         const QString&     key,
                                         double&            out,
                                         QString&           errorDetail)
{
    if (!params.contains(key)) {
        errorDetail = tr("missing required param '%1'").arg(key);
        return false;
    }

    const QJsonValue val = params.value(key);
    if (!val.isDouble()) {
        errorDetail = tr("param '%1' must be a number").arg(key);
        return false;
    }

    out = val.toDouble();
    return true;
}

// ============================================================================
// 工具函数：标准事务收尾
// ============================================================================

void DirectEntityExecutor::finalizeEntity(DmEntity*          entity,
                                          const std::string& txName,
                                          const QString&     typeName,
                                          ExecutorResult&    result)
{
    if (!m_doc || !entity) {
        // 防御：构造失败或文档无效
        delete entity;
        result.success      = false;
        result.errorMessage = tr(
            "DirectEntityExecutor: internal error — null document or entity.");
        return;
    }

    // ---- 标准创建路径（与 ActionDrawCircle::trigger 一致） ----
    Transaction t(txName, m_doc);
    t.start();

    entity->setDocument(m_doc);       // 设置文档、活动图层、活动画笔
    entity->update();
    m_doc->getEntityTable()->add(entity);  // add 自动创建 EntityTableAddCmd，纳入 Undo/Redo

    t.commit();
    // -----------------------------------------------------------

    // 填充结果
    EntityCreateResult ecr;
    ecr.entityId   = entity->getId();
    ecr.entityType = typeName;

    result.success = true;
    result.createdEntities.append(ecr);
}
