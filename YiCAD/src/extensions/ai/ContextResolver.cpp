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

/// @file ContextResolver.cpp
/// @brief ContextResolver 实现 —— 上下文 → 实体 ID 解析

#include "ContextResolver.h"

#include "Datamodel.h"       // DM::EntityType
#include "DmDocument.h"      // DmDocument, getEntityTable()
#include "DmEntity.h"        // DmEntity, isSelected(), getId(), getEntityType()
#include "EntityTable.h"     // EntityTable, iterator

#include <QJsonObject>
#include <QJsonValue>

// ============================================================================
// 内部：entity_type 字符串 → DM::EntityType 映射表
// ============================================================================

namespace {

struct EntityTypeEntry
{
    const char*  jsonName;    // JSON 中的字符串，如 "line"
    int          dmType;      // DM::EntityLine 等枚举值
    const char*  displayName; // 人类可读显示名，如 "DmLine"
};

constexpr EntityTypeEntry kEntityTypeTable[] = {
    { "point",              DM::EntityPoint,              "DmPoint"        },
    { "line",               DM::EntityLine,               "DmLine"         },
    { "polyline",           DM::EntityPolyline,           "DmPolyline"     },
    { "arc",                DM::EntityArc,                "DmArc"          },
    { "circle",             DM::EntityCircle,             "DmCircle"       },
    { "ellipse",            DM::EntityEllipse,            "DmEllipse"      },
    { "rectangle",          DM::EntityPolyline,           "DmPolyline"     },
    { "spline",             DM::EntitySpline,             "DmSpline"       },
    { "text",               DM::EntityText,               "DmText"         },
    { "mtext",              DM::EntityMText,              "DmMText"        },
    { "hatch",              DM::EntityHatch,              "DmHatch"        },
    { "block_reference",    DM::EntityBlockReference,     "DmBlockReference"},
    { "construction_line",  DM::EntityConstructionLine,   "DmConstructionLine"},
    { "dimension",          DM::EntityDimAligned,         "DmDimension"    },
};

constexpr int kEntityTypeTableSize =
    sizeof(kEntityTypeTable) / sizeof(kEntityTypeTable[0]);

/// @brief 将 JSON entity_type 字符串映射到 DM::EntityType 枚举值
int mapEntityTypeString(const QString& s)
{
    const QByteArray key = s.trimmed().toLower().toLatin1();
    for (int i = 0; i < kEntityTypeTableSize; ++i) {
        if (key == kEntityTypeTable[i].jsonName) {
            return kEntityTypeTable[i].dmType;
        }
    }
    return DM::EntityUnknown;  // 未识别 → 不过滤
}

/// @brief 将 DM::EntityType 枚举值映射到人类可读字符串
const char* mapEntityTypeDisplayName(int dmType)
{
    for (int i = 0; i < kEntityTypeTableSize; ++i) {
        if (kEntityTypeTable[i].dmType == dmType) {
            return kEntityTypeTable[i].displayName;
        }
    }
    return "DmEntity";
}

} // anonymous namespace

// ============================================================================
// ResolvedEntityRef
// ============================================================================

QString ResolvedEntityRef::describe() const
{
    return QObject::tr("[%1] %2 (resolved via %3)")
        .arg(entityId.isValid()
                 ? QString::fromStdString(entityId.asString())
                 : QStringLiteral("invalid"))
        .arg(entityType, howResolved);
}

// ============================================================================
// ResolvedSelection
// ============================================================================

QVector<DmId> ResolvedSelection::entityIds() const
{
    QVector<DmId> ids;
    ids.reserve(entities.size());
    for (const auto& ref : entities) {
        ids.append(ref.entityId);
    }
    return ids;
}

// ============================================================================
// ContextResolver 构造 & 历史管理
// ============================================================================

ContextResolver::ContextResolver(DmDocument* doc, QObject* parent)
    : QObject(parent)
    , m_doc(doc)
{
}

void ContextResolver::recordTurn(const TurnRecord& turn)
{
    // 当前版本只保留最近 1 轮（最小连续对话）
    if (m_history.size() >= 1) {
        m_history.pop_front();
    }
    m_history.append(turn);
}

const QVector<TurnRecord>& ContextResolver::history() const
{
    return m_history;
}

void ContextResolver::clearHistory()
{
    m_history.clear();
}

// ============================================================================
// 核心解析：resolve()
// ============================================================================

ResolvedSelection ContextResolver::resolve(const SelectionSpec& spec) const
{
    switch (spec.mode) {

    case SelectionMode::CurrentSelection:
        return resolveCurrentSelection(spec.raw);

    case SelectionMode::LastCreated:
        return resolveLastCreated(spec.raw);

    case SelectionMode::All:
        return resolveAll(spec.raw);

    case SelectionMode::None: {
        ResolvedSelection result;
        result.ok          = true;
        result.explanation = tr("No selection required (None mode).");
        return result;
    }

    }

    // unreachable, but keep compiler happy
    ResolvedSelection fail;
    fail.ok           = false;
    fail.errorMessage = tr("ContextResolver: unknown SelectionMode.");
    return fail;
}

// ============================================================================
// resolveCurrentSelection
// ============================================================================

ResolvedSelection ContextResolver::resolveCurrentSelection(
    const QJsonObject& raw) const
{
    ResolvedSelection result;

    QVector<ResolvedEntityRef> entities = collectSelectedEntities();

    // 类型过滤（如果 raw 中有 entity_type 提示）
    const int typeHint = entityTypeFromHint(raw);
    if (typeHint != DM::EntityUnknown) {
        filterByEntityType(entities, typeHint);
    }

    if (entities.isEmpty()) {
        result.ok           = false;

        const QString typeName = entityTypeName(typeHint);
        if (typeHint != DM::EntityUnknown) {
            result.errorMessage = tr(
                "ContextResolver: no selected entities matching type hint '%1'.")
                .arg(typeName);
        } else {
            result.errorMessage = tr(
                "ContextResolver: no entities currently selected.");
        }
        return result;
    }

    result.ok = true;
    result.entities = entities;

    if (typeHint != DM::EntityUnknown) {
        result.explanation = tr(
            "Resolved %1 entity/entities from current selection (filtered by type: %2).")
            .arg(entities.size())
            .arg(entityTypeName(typeHint));
    } else {
        result.explanation = tr(
            "Resolved %1 entity/entities from current selection.")
            .arg(entities.size());
    }

    return result;
}

// ============================================================================
// resolveLastCreated
// ============================================================================

ResolvedSelection ContextResolver::resolveLastCreated(
    const QJsonObject& raw) const
{
    ResolvedSelection result;

    const int typeHint = entityTypeFromHint(raw);

    // ---- 策略 1：从对话历史中获取上轮创建的实体 ----
    if (!m_history.isEmpty()) {
        const TurnRecord& lastTurn = m_history.last();

        QVector<ResolvedEntityRef> entities;
        for (const DmId& id : lastTurn.createdEntityIds) {
            if (!id.isValid())
                continue;

            // 查文档确认实体仍然存在
            DmEntity* e = nullptr;
            if (m_doc) {
                EntityTable* table = m_doc->getEntityTable();
                if (table) {
                    e = table->find(id);
                }
            }
            if (!e)
                continue;

            // 类型过滤
            if (typeHint != DM::EntityUnknown && !entityMatchesType(e, typeHint))
                continue;

            ResolvedEntityRef ref;
            ref.entityId    = id;
            ref.entityType  = entityTypeName(e->getEntityType());
            ref.howResolved = QStringLiteral("previous_turn");
            entities.append(ref);
        }

        if (!entities.isEmpty()) {
            result.ok       = true;
            result.entities = entities;

            const QString typeInfo = (typeHint != DM::EntityUnknown)
                ? tr(" (filtered by type: %1)").arg(entityTypeName(typeHint))
                : QString();
            result.explanation = tr(
                "Resolved %1 entity/entities from previous turn (user said: \"%2\")%3.")
                .arg(entities.size())
                .arg(lastTurn.userInput, typeInfo);
            return result;
        }
    }

    // ---- 策略 2（回退）：遍历文档所有实体取最近添加的 ----
    // 注意：YiCAD 的 EntityTable 没有直接的"创建时间"字段，
    // 所以这里做一个保守回退：取最后遍历到的（潜在最近添加的）实体
    {
        QVector<ResolvedEntityRef> entities;

        if (m_doc) {
            EntityTable* table = m_doc->getEntityTable();
            if (table) {
                DmEntity* lastEntity = nullptr;
                for (auto e : *table) {
                    if (e && e->isVisible()) {
                        lastEntity = e;  // 遍历完 lastEntity 就是最后一个
                    }
                }

                if (lastEntity) {
                    if (typeHint == DM::EntityUnknown
                        || entityMatchesType(lastEntity, typeHint))
                    {
                        ResolvedEntityRef ref;
                        ref.entityId    = lastEntity->getId();
                        ref.entityType  = entityTypeName(lastEntity->getEntityType());
                        ref.howResolved = QStringLiteral("last_created");
                        entities.append(ref);
                    }
                }
            }
        }

        if (!entities.isEmpty()) {
            result.ok       = true;
            result.entities = entities;
            result.explanation = tr(
                "Resolved 1 entity as last created (fallback: no history available).");
            return result;
        }
    }

    // ---- 什么都没有 ----
    result.ok           = false;

    const QString typeName = (typeHint != DM::EntityUnknown)
        ? entityTypeName(typeHint) : QStringLiteral("any");
    result.errorMessage = tr(
        "ContextResolver: no last-created entity found (type hint: %1). "
        "Create an entity first, then reference it in the next turn.")
        .arg(typeName);
    return result;
}

// ============================================================================
// resolveAll
// ============================================================================

ResolvedSelection ContextResolver::resolveAll(const QJsonObject& raw) const
{
    ResolvedSelection result;

    QVector<ResolvedEntityRef> entities = collectAllVisibleEntities();

    const int typeHint = entityTypeFromHint(raw);
    if (typeHint != DM::EntityUnknown) {
        filterByEntityType(entities, typeHint);
    }

    if (entities.isEmpty()) {
        result.ok           = false;
        result.errorMessage = tr(
            "ContextResolver: no visible entities in document.");
        return result;
    }

    result.ok       = true;
    result.entities = entities;
    result.explanation = tr(
        "Resolved %1 entity/entities (all visible).")
        .arg(entities.size());
    return result;
}

// ============================================================================
// 实体收集工具
// ============================================================================

QVector<ResolvedEntityRef> ContextResolver::collectSelectedEntities() const
{
    QVector<ResolvedEntityRef> entities;

    if (!m_doc)
        return entities;

    EntityTable* table = m_doc->getEntityTable();
    if (!table)
        return entities;

    for (auto e : *table) {
        if (e && e->isSelected()) {
            ResolvedEntityRef ref;
            ref.entityId    = e->getId();
            ref.entityType  = entityTypeName(e->getEntityType());
            ref.howResolved = QStringLiteral("selected");
            entities.append(ref);
        }
    }

    return entities;
}

QVector<ResolvedEntityRef> ContextResolver::collectAllVisibleEntities() const
{
    QVector<ResolvedEntityRef> entities;

    if (!m_doc)
        return entities;

    EntityTable* table = m_doc->getEntityTable();
    if (!table)
        return entities;

    for (auto e : *table) {
        if (e && e->isVisible()) {
            ResolvedEntityRef ref;
            ref.entityId    = e->getId();
            ref.entityType  = entityTypeName(e->getEntityType());
            ref.howResolved = QStringLiteral("all");
            entities.append(ref);
        }
    }

    return entities;
}

// ============================================================================
// 类型过滤
// ============================================================================

int ContextResolver::entityTypeFromHint(const QJsonObject& raw)
{
    if (raw.isEmpty())
        return DM::EntityUnknown;

    const QJsonValue val = raw.value(QStringLiteral("entity_type"));
    if (val.isUndefined() || !val.isString())
        return DM::EntityUnknown;

    return mapEntityTypeString(val.toString());
}

QString ContextResolver::entityTypeName(int entityType)
{
    return QString::fromLatin1(mapEntityTypeDisplayName(entityType));
}

bool ContextResolver::entityMatchesType(const DmEntity* e, int typeHint)
{
    if (!e)
        return false;
    if (typeHint == DM::EntityUnknown)
        return true;  // 无类型提示时不过滤

    return e->getEntityType() == typeHint;
}

void ContextResolver::filterByEntityType(
    QVector<ResolvedEntityRef>& entities, int typeHint)
{
    if (typeHint == DM::EntityUnknown)
        return;

    QVector<ResolvedEntityRef> filtered;
    filtered.reserve(entities.size());

    for (const auto& ref : entities) {
        if (!ref.entityId.isValid())
            continue;

        // 需要根据 DmId 找到 DmEntity 做实体会员检查
        // 但为了效率，这里用 entityType 字符串做近似匹配
        // （entityType 来自 entityTypeName，与映射表一致）
        const QString expectedType =
            QString::fromLatin1(mapEntityTypeDisplayName(typeHint));

        // "rectangle" → DmPolyline 的特殊处理
        // 如果 map 中 rectangle 映射到 DmPolyline，则两者都接受
        if (ref.entityType == expectedType) {
            filtered.append(ref);
        }
    }

    entities = filtered;
}
