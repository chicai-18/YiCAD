#include "HostApi.h"

#include "DmArc.h"
#include "DmAttribute.h"
#include "DmAttributeDefinition.h"
#include "DmBlock.h"
#include "DmBlockReference.h"
#include "DmBlockTable.h"
#include "DmCircle.h"
#include "DmColor.h"
#include "DmDimensionStyle.h"
#include "DmDimAligned.h"
#include "DmDimAngular.h"
#include "DmDimDiametric.h"
#include "DmDimLinear.h"
#include "DmDimRadial.h"
#include "DmDocument.h"
#include "DmEllipse.h"
#include "DmEntity.h"
#include "DmFont.h"
#include "DmFontList.h"
#include "DmLayer.h"
#include "DmLeader.h"
#include "DmLayerTable.h"
#include "DmLine.h"
#include "DmLineType.h"
#include "DmLineTypeTable.h"
#include "DmPoint.h"
#include "DmPolyline.h"
#include "DmPatternList.h"
#include "DmRay.h"
#include "DmRegion.h"
#include "DmSpline.h"
#include "DmHatch.h"
#include "DmImage.h"
#include "DmMText.h"
#include "DmText.h"
#include "DmTextStyle.h"
#include "DmTextStyleTable.h"
#include "TextConsts.h"
#include "DmXline.h"
#include "CmdManager.h"
#include "DocumentCmd.h"
#include "EntityTable.h"
#include "GuiDocumentView.h"
#include "PluginRegistry.h"
#include "Transaction.h"

#include <QString>
#include <QByteArray>
#include <QFileInfo>

#include <cmath>
#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace
{

YiCadResult toResult(bool success) noexcept
{
    return success ? YICAD_SUCCESS : YICAD_FAILURE;
}

bool allFinite(double first, double second, double third, double fourth) noexcept
{
    return std::isfinite(first) && std::isfinite(second) &&
           std::isfinite(third) && std::isfinite(fourth);
}

bool copyUtf8(const char* source, QString& target)
{
    if (source == nullptr)
    {
        return false;
    }
    target = QString::fromUtf8(source);
    return true;
}

#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
enum ImportResourceKind
{
    ImportResourceLineType = 1,
    ImportResourceLayer = 2,
    ImportResourceTextStyle = 3,
    ImportResourceDimensionStyle = 4,
    ImportResourceBlock = 5,
    ImportResourceInsert = 6
};

bool validStructSize(uint32_t actual, std::size_t required) noexcept
{
    return actual >= required;
}

bool copyStringView(const YiCadStringView& source, QString& target) noexcept
{
    try
    {
        if ((source.data == nullptr && source.size != 0) ||
            source.size > static_cast<uint32_t>(std::numeric_limits<int>::max()))
        {
            return false;
        }
        if (source.size == 0)
        {
            target.clear();
            return true;
        }
        if (std::memchr(source.data, '\0', source.size) != nullptr)
        {
            return false;
        }
        const QByteArray bytes(source.data, static_cast<int>(source.size));
        target = QString::fromUtf8(bytes.constData(), bytes.size());
        return target.toUtf8() == bytes;
    }
    catch (...)
    {
        target.clear();
        return false;
    }
}

bool validConflictPolicy(YiCadResourceConflictPolicy policy) noexcept
{
    return policy == YICAD_RESOURCE_CONFLICT_FAIL ||
           policy == YICAD_RESOURCE_CONFLICT_REPLACE ||
           policy == YICAD_RESOURCE_CONFLICT_RENAME;
}

QString uniqueResourceName(const QString& base, const auto& find)
{
    for (uint32_t index = 1; index != UINT32_MAX; ++index)
    {
        const auto candidate = QString("%1 (%2)").arg(base).arg(index);
        if (find(candidate) == nullptr)
        {
            return candidate;
        }
    }
    return {};
}

bool validLineWidth(int32_t width) noexcept
{
    switch (width)
    {
    case -3: case -2: case -1: case 0: case 5: case 9: case 13:
    case 15: case 18: case 20: case 25: case 30: case 35: case 40:
    case 50: case 53: case 60: case 70: case 80: case 90: case 100:
    case 106: case 120: case 140: case 158: case 200: case 211:
        return true;
    default:
        return false;
    }
}

bool toDmColor(const YiCadColorData& source, DmColor& color)
{
    if (!validStructSize(source.structSize, sizeof(YiCadColorData)))
    {
        return false;
    }
    switch (source.method)
    {
    case YICAD_COLOR_BY_LAYER:
        color = DmColor(DM::FlagByLayer);
        return true;
    case YICAD_COLOR_BY_BLOCK:
        color = DmColor(DM::FlagByBlock);
        return true;
    case YICAD_COLOR_ACI:
        if (source.aci < 1 || source.aci > 255)
        {
            return false;
        }
        color = DM::indexColors[source.aci];
        return true;
    case YICAD_COLOR_RGB:
        color = DmColor(source.red, source.green, source.blue);
        return true;
    default:
        return false;
    }
}

bool finitePoint(const YiCadPoint3d& point) noexcept
{
    constexpr double limit = 1.0e150;
    return std::isfinite(point.x) && std::abs(point.x) <= limit &&
           std::isfinite(point.y) && std::abs(point.y) <= limit &&
           std::isfinite(point.z) && std::abs(point.z) <= limit;
}

bool finitePoint(const YiCadPoint2d& point) noexcept
{
    constexpr double limit = 1.0e150;
    return std::isfinite(point.x) && std::abs(point.x) <= limit &&
           std::isfinite(point.y) && std::abs(point.y) <= limit;
}

DmVector toDmVector(const YiCadPoint3d& point)
{
    return DmVector(point.x, point.y, point.z);
}

DmVector toDmVector(const YiCadPoint2d& point)
{
    return DmVector(point.x, point.y);
}

bool validPointArray(const YiCadPoint2dArrayView& points) noexcept
{
    if ((points.data == nullptr && points.count != 0) ||
        points.count > 1000000)
    {
        return false;
    }
    for (uint32_t index = 0; index < points.count; ++index)
    {
        if (!finitePoint(points.data[index]))
        {
            return false;
        }
    }
    return true;
}

bool validDoubleArray(const YiCadDoubleArrayView& values) noexcept
{
    if ((values.data == nullptr && values.count != 0) ||
        values.count > 1000000)
    {
        return false;
    }
    for (uint32_t index = 0; index < values.count; ++index)
    {
        if (!std::isfinite(values.data[index]) ||
            std::abs(values.data[index]) > 1.0e150)
        {
            return false;
        }
    }
    return true;
}
#endif

} // namespace

struct HostApi::DocumentHandleRecord
{
    DmDocument* document = nullptr;
};

struct HostApi::TransactionRecord
{
    DmDocument* document = nullptr;
    std::unique_ptr<Transaction> transaction;
};

struct HostApi::EntityIteratorRecord
{
    struct EntitySnapshot
    {
        YiCadEntityType type = YICAD_ENTITY_UNKNOWN;
        YiCadLineData line{};
        YiCadCircleData circle{};
    };

    std::vector<EntitySnapshot> entities;
    std::size_t nextIndex = 0;
    std::size_t currentIndex = 0;
    bool hasCurrent = false;
};

#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
struct HostApi::ImportSessionRecord
{
    struct ContainerRecord
    {
        DmDocument* document = nullptr;
        DmBlock* block = nullptr;
        bool modelSpace = false;
        bool active = false;
    };

    struct ResourceRecord
    {
        void* object = nullptr;
        ContainerRecord* container = nullptr;
        int kind = 0;
        bool active = false;
        bool finalized = false;
    };

    DmDocument* document = nullptr;
    std::unique_ptr<Transaction> transaction;
    MacroCmd* command = nullptr;
    std::size_t initialUndoCount = 0;
    bool active = false;
    ContainerRecord modelSpace;
    std::vector<std::unique_ptr<ContainerRecord>> containers;
    std::vector<std::unique_ptr<ResourceRecord>> resources;
};
#endif

thread_local HostApi* HostApi::s_activeInstance = nullptr;

HostApi::HostApi(
    PluginHostContext& context,
    PluginRegistry& registry) noexcept
    : m_context(context)
    , m_registry(registry)
#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
    , m_importApi{
          YICAD_IMPORT_API_V3_DRAFT_SIZE,
          YICAD_PLUGIN_ABI_V3_DRAFT,
          &HostApi::beginImport,
          &HostApi::commitImport,
          &HostApi::rollbackImport,
          &HostApi::importGetLastError,
          &HostApi::setDocumentSettings,
          &HostApi::createLineType,
          &HostApi::createLayer,
          &HostApi::createTextStyle,
          &HostApi::createDimensionStyle,
          &HostApi::getModelSpace,
          &HostApi::createPoint,
          &HostApi::createLine,
          &HostApi::createRay,
          &HostApi::createXLine,
          &HostApi::createArc,
          &HostApi::createCircle,
          &HostApi::createEllipse,
          &HostApi::createPolyline,
          &HostApi::createSpline,
          &HostApi::createText,
          &HostApi::createMText,
          &HostApi::beginBlock,
          &HostApi::endBlock,
          &HostApi::createInsert,
          &HostApi::createAttributeDefinition,
          &HostApi::createAttribute,
          &HostApi::createDimension,
          &HostApi::createLeader,
          &HostApi::createHatch,
          &HostApi::createImage}
#endif
    , m_api{
          static_cast<uint32_t>(sizeof(YiCadHostApi)),
#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
          YICAD_PLUGIN_ABI_V3_DRAFT,
#else
          YICAD_PLUGIN_ABI_MAX_VERSION,
#endif
          &HostApi::message,
          &HostApi::registerCommand,
          &HostApi::registerRibbonButton,
          &HostApi::currentDocument,
          &HostApi::documentAddLine,
          &HostApi::documentAddCircle,
          &HostApi::documentRegen,
          &HostApi::documentZoomAuto,
          &HostApi::registerImportFilter,
          &HostApi::registerExportFilter,
          &HostApi::documentBeginTransaction,
          &HostApi::documentCommitTransaction,
          &HostApi::documentRollbackTransaction,
          &HostApi::documentCreateEntityIterator,
          &HostApi::entityIteratorNext,
          &HostApi::entityIteratorGetLine,
          &HostApi::entityIteratorGetCircle,
          &HostApi::entityIteratorDestroy
#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
          , &m_importApi
#endif
      }
{
    if (s_activeInstance == nullptr)
    {
        s_activeInstance = this;
        m_active = true;
    }
}

HostApi::~HostApi()
{
#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
    for (auto& record : m_importSessions)
    {
        if (!record->active)
        {
            continue;
        }
        try
        {
            if (m_context.isDocumentOpen(record->document))
            {
                record->transaction->rollback();
            }
        }
        catch (...)
        {
        }
        releaseImportSession(record.get());
    }
    m_importSessions.clear();
#endif
    for (auto& record : m_transactions)
    {
        try
        {
            if (m_context.isDocumentOpen(record->document))
            {
                record->transaction->rollback();
            }
        }
        catch (...)
        {
        }
    }
    m_transactions.clear();
    m_entityIterators.clear();

    if (s_activeInstance == this)
    {
        s_activeInstance = nullptr;
    }
}

const YiCadHostApi* HostApi::api() const noexcept
{
    return m_active ? &m_api : nullptr;
}

bool HostApi::isActive() const noexcept
{
    return m_active;
}

YiCadDocumentHandle HostApi::documentHandle(DmDocument* document) noexcept
{
    try
    {
        if (!m_active || document == nullptr ||
            !m_context.isDocumentOpen(document))
        {
            return nullptr;
        }
        return handleForDocument(document);
    }
    catch (...)
    {
        return nullptr;
    }
}

bool HostApi::isDocumentHandleValid(
    YiCadDocumentHandle handle) const noexcept
{
    return m_active && resolveDocument(handle) != nullptr;
}

HostApi* HostApi::activeInstance() noexcept
{
    return s_activeInstance;
}

void YICAD_PLUGIN_CALL HostApi::message(const char* text) noexcept
{
    try
    {
        auto* instance = activeInstance();
        QString messageText;
        if (instance != nullptr && copyUtf8(text, messageText))
        {
            instance->m_context.showPluginMessage(messageText);
        }
    }
    catch (...)
    {
    }
}

YiCadResult YICAD_PLUGIN_CALL HostApi::registerCommand(
    const char* pluginId,
    const char* commandId,
    const char* displayName,
    YiCadCommandCallback callback,
    void* userData) noexcept
{
    try
    {
        auto* instance = activeInstance();
        QString plugin;
        QString command;
        QString name;
        if (instance == nullptr || !copyUtf8(pluginId, plugin) ||
            !copyUtf8(commandId, command) ||
            !copyUtf8(displayName, name))
        {
            return YICAD_FAILURE;
        }
        return toResult(instance->m_registry.stageCommand(
            plugin, command, name, callback, userData));
    }
    catch (...)
    {
        return YICAD_FAILURE;
    }
}

YiCadResult YICAD_PLUGIN_CALL HostApi::registerRibbonButton(
    const char* pluginId,
    const char* tab,
    const char* group,
    const char* commandId,
    const char* iconPath) noexcept
{
    try
    {
        auto* instance = activeInstance();
        QString plugin;
        QString tabName;
        QString groupName;
        QString command;
        QString icon;
        if (instance == nullptr || !copyUtf8(pluginId, plugin) ||
            !copyUtf8(tab, tabName) || !copyUtf8(group, groupName) ||
            !copyUtf8(commandId, command) || !copyUtf8(iconPath, icon))
        {
            return YICAD_FAILURE;
        }
        return toResult(instance->m_registry.stageRibbonButton(
            plugin, tabName, groupName, command, icon));
    }
    catch (...)
    {
        return YICAD_FAILURE;
    }
}

YiCadDocumentHandle YICAD_PLUGIN_CALL HostApi::currentDocument() noexcept
{
    try
    {
        auto* instance = activeInstance();
        if (instance == nullptr)
        {
            return nullptr;
        }

        auto* document = instance->m_context.currentDocument();
        auto* view = document == nullptr
            ? nullptr
            : instance->m_context.documentView(document);
        if (document == nullptr ||
            !instance->m_context.isDocumentOpen(document) ||
            view == nullptr || view->getDocument() != document)
        {
            return nullptr;
        }
        return instance->handleForDocument(document);
    }
    catch (...)
    {
        return nullptr;
    }
}

YiCadResult YICAD_PLUGIN_CALL HostApi::documentAddLine(
    YiCadDocumentHandle documentHandle,
    double x1,
    double y1,
    double x2,
    double y2) noexcept
{
    try
    {
        auto* instance = activeInstance();
        if (instance == nullptr || !allFinite(x1, y1, x2, y2) ||
            (x1 == x2 && y1 == y2))
        {
            return YICAD_FAILURE;
        }

        auto* document = instance->resolveDocument(documentHandle);
        auto* table = document == nullptr ? nullptr : document->getEntityTable();
        if (table == nullptr)
        {
            return YICAD_FAILURE;
        }

        auto entity = std::make_unique<DmLine>(
            nullptr, DmVector(x1, y1), DmVector(x2, y2));
        entity->setDocument(document);
        entity->update();

        if (instance->hasActiveTransaction(document))
        {
            table->add(entity.get());
            entity.release();
        }
        else
        {
            Transaction transaction("Plugin: Add Line", document);
            transaction.start();
            try
            {
                table->add(entity.get());
                entity.release();
                transaction.commit();
            }
            catch (...)
            {
                transaction.rollback();
                throw;
            }
        }
        return YICAD_SUCCESS;
    }
    catch (...)
    {
        return YICAD_FAILURE;
    }
}

YiCadResult YICAD_PLUGIN_CALL HostApi::documentAddCircle(
    YiCadDocumentHandle documentHandle,
    double centerX,
    double centerY,
    double radius) noexcept
{
    try
    {
        auto* instance = activeInstance();
        if (instance == nullptr || !std::isfinite(centerX) ||
            !std::isfinite(centerY) || !std::isfinite(radius) ||
            radius <= 0.0)
        {
            return YICAD_FAILURE;
        }

        auto* document = instance->resolveDocument(documentHandle);
        auto* table = document == nullptr ? nullptr : document->getEntityTable();
        if (table == nullptr)
        {
            return YICAD_FAILURE;
        }

        auto entity = std::make_unique<DmCircle>(
            nullptr, CircleData(DmVector(centerX, centerY), radius));
        entity->setDocument(document);
        entity->update();

        if (instance->hasActiveTransaction(document))
        {
            table->add(entity.get());
            entity.release();
        }
        else
        {
            Transaction transaction("Plugin: Add Circle", document);
            transaction.start();
            try
            {
                table->add(entity.get());
                entity.release();
                transaction.commit();
            }
            catch (...)
            {
                transaction.rollback();
                throw;
            }
        }
        return YICAD_SUCCESS;
    }
    catch (...)
    {
        return YICAD_FAILURE;
    }
}

YiCadResult YICAD_PLUGIN_CALL HostApi::documentRegen(
    YiCadDocumentHandle documentHandle) noexcept
{
    try
    {
        auto* instance = activeInstance();
        GuiDocumentView* view = nullptr;
        auto* document = instance == nullptr
            ? nullptr
            : instance->resolveDocument(documentHandle, &view);
        if (document == nullptr || view == nullptr)
        {
            return YICAD_FAILURE;
        }

        document->setDocumentView(view);
        document->regenerate();
        return YICAD_SUCCESS;
    }
    catch (...)
    {
        return YICAD_FAILURE;
    }
}

YiCadResult YICAD_PLUGIN_CALL HostApi::documentZoomAuto(
    YiCadDocumentHandle documentHandle) noexcept
{
    try
    {
        auto* instance = activeInstance();
        GuiDocumentView* view = nullptr;
        auto* document = instance == nullptr
            ? nullptr
            : instance->resolveDocument(documentHandle, &view);
        if (document == nullptr || view == nullptr)
        {
            return YICAD_FAILURE;
        }

        view->zoomAuto();
        return YICAD_SUCCESS;
    }
    catch (...)
    {
        return YICAD_FAILURE;
    }
}

YiCadResult YICAD_PLUGIN_CALL HostApi::registerImportFilter(
    const char* pluginId,
    const char* formatId,
    const char* displayName,
    const char* extension,
    YiCadImportCallback callback,
    void* userData) noexcept
{
    try
    {
        auto* instance = activeInstance();
        QString plugin;
        QString format;
        QString name;
        QString suffix;
        if (instance == nullptr || !copyUtf8(pluginId, plugin) ||
            !copyUtf8(formatId, format) || !copyUtf8(displayName, name) ||
            !copyUtf8(extension, suffix))
        {
            return YICAD_FAILURE;
        }
        return toResult(instance->m_registry.stageImportFilter(
            plugin, format, name, suffix, callback, userData));
    }
    catch (...)
    {
        return YICAD_FAILURE;
    }
}

YiCadResult YICAD_PLUGIN_CALL HostApi::registerExportFilter(
    const char* pluginId,
    const char* formatId,
    const char* displayName,
    const char* extension,
    YiCadExportCallback callback,
    void* userData) noexcept
{
    try
    {
        auto* instance = activeInstance();
        QString plugin;
        QString format;
        QString name;
        QString suffix;
        if (instance == nullptr || !copyUtf8(pluginId, plugin) ||
            !copyUtf8(formatId, format) || !copyUtf8(displayName, name) ||
            !copyUtf8(extension, suffix))
        {
            return YICAD_FAILURE;
        }
        return toResult(instance->m_registry.stageExportFilter(
            plugin, format, name, suffix, callback, userData));
    }
    catch (...)
    {
        return YICAD_FAILURE;
    }
}

YiCadTransactionHandle YICAD_PLUGIN_CALL HostApi::documentBeginTransaction(
    YiCadDocumentHandle documentHandle,
    const char* name) noexcept
{
    try
    {
        auto* instance = activeInstance();
        QString transactionName;
        auto* document = instance == nullptr
            ? nullptr
            : instance->resolveDocument(documentHandle);
        if (document == nullptr || !copyUtf8(name, transactionName) ||
            transactionName.trimmed().isEmpty() ||
            instance->hasActiveTransaction(document) ||
            document->getCmdManager() == nullptr ||
            document->getCmdManager()->getCurrentCmd() != nullptr ||
            document->getCmdManager()->getCurrentGroupCmd() != nullptr)
        {
            return nullptr;
        }

        auto record = std::make_unique<TransactionRecord>();
        record->document = document;
        record->transaction = std::make_unique<Transaction>(
            transactionName.toUtf8().constData(), document);
        record->transaction->start();
        auto* handle = record.get();
        instance->m_transactions.push_back(std::move(record));
        return static_cast<void*>(handle);
    }
    catch (...)
    {
        return nullptr;
    }
}

YiCadResult YICAD_PLUGIN_CALL HostApi::documentCommitTransaction(
    YiCadTransactionHandle transactionHandle) noexcept
{
    try
    {
        auto* instance = activeInstance();
        auto* record = instance == nullptr
            ? nullptr
            : instance->resolveTransaction(transactionHandle);
        if (record == nullptr ||
            !instance->m_context.isDocumentOpen(record->document))
        {
            return YICAD_FAILURE;
        }

        record->transaction->commit();
        instance->m_transactions.erase(std::remove_if(
            instance->m_transactions.begin(),
            instance->m_transactions.end(),
            [record](const auto& item) { return item.get() == record; }));
        return YICAD_SUCCESS;
    }
    catch (...)
    {
        return YICAD_FAILURE;
    }
}

YiCadResult YICAD_PLUGIN_CALL HostApi::documentRollbackTransaction(
    YiCadTransactionHandle transactionHandle) noexcept
{
    try
    {
        auto* instance = activeInstance();
        auto* record = instance == nullptr
            ? nullptr
            : instance->resolveTransaction(transactionHandle);
        if (record == nullptr ||
            !instance->m_context.isDocumentOpen(record->document))
        {
            return YICAD_FAILURE;
        }

        record->transaction->rollback();
        instance->m_transactions.erase(std::remove_if(
            instance->m_transactions.begin(),
            instance->m_transactions.end(),
            [record](const auto& item) { return item.get() == record; }));
        return YICAD_SUCCESS;
    }
    catch (...)
    {
        return YICAD_FAILURE;
    }
}

YiCadEntityIteratorHandle YICAD_PLUGIN_CALL
HostApi::documentCreateEntityIterator(
    YiCadDocumentHandle documentHandle) noexcept
{
    try
    {
        auto* instance = activeInstance();
        auto* document = instance == nullptr
            ? nullptr
            : instance->resolveDocument(documentHandle);
        auto* table = document == nullptr ? nullptr : document->getEntityTable();
        if (table == nullptr)
        {
            return nullptr;
        }

        auto record = std::make_unique<EntityIteratorRecord>();
        record->entities.reserve(static_cast<std::size_t>(table->count()));
        for (auto* entity : *table)
        {
            EntityIteratorRecord::EntitySnapshot snapshot;
            if (auto* line = dynamic_cast<DmLine*>(entity))
            {
                const auto start = line->getStartpoint();
                const auto end = line->getEndpoint();
                snapshot.type = YICAD_ENTITY_LINE;
                snapshot.line = {start.x, start.y, end.x, end.y};
            }
            else if (auto* circle = dynamic_cast<DmCircle*>(entity))
            {
                const auto center = circle->getCenter();
                snapshot.type = YICAD_ENTITY_CIRCLE;
                snapshot.circle = {
                    center.x, center.y, circle->getRadius()};
            }
            record->entities.push_back(snapshot);
        }

        auto* handle = record.get();
        instance->m_entityIterators.push_back(std::move(record));
        return static_cast<void*>(handle);
    }
    catch (...)
    {
        return nullptr;
    }
}

YiCadResult YICAD_PLUGIN_CALL HostApi::entityIteratorNext(
    YiCadEntityIteratorHandle iteratorHandle,
    YiCadEntityType* entityType) noexcept
{
    auto* instance = activeInstance();
    auto* iterator = instance == nullptr
        ? nullptr
        : instance->resolveEntityIterator(iteratorHandle);
    if (iterator == nullptr || entityType == nullptr ||
        iterator->nextIndex >= iterator->entities.size())
    {
        if (iterator != nullptr)
        {
            iterator->hasCurrent = false;
        }
        return YICAD_FAILURE;
    }

    iterator->currentIndex = iterator->nextIndex++;
    iterator->hasCurrent = true;
    *entityType = iterator->entities[iterator->currentIndex].type;
    return YICAD_SUCCESS;
}

YiCadResult YICAD_PLUGIN_CALL HostApi::entityIteratorGetLine(
    YiCadEntityIteratorHandle iteratorHandle,
    YiCadLineData* line) noexcept
{
    auto* instance = activeInstance();
    auto* iterator = instance == nullptr
        ? nullptr
        : instance->resolveEntityIterator(iteratorHandle);
    if (iterator == nullptr || line == nullptr || !iterator->hasCurrent)
    {
        return YICAD_FAILURE;
    }
    const auto& current = iterator->entities[iterator->currentIndex];
    if (current.type != YICAD_ENTITY_LINE)
    {
        return YICAD_FAILURE;
    }
    *line = current.line;
    return YICAD_SUCCESS;
}

YiCadResult YICAD_PLUGIN_CALL HostApi::entityIteratorGetCircle(
    YiCadEntityIteratorHandle iteratorHandle,
    YiCadCircleData* circle) noexcept
{
    auto* instance = activeInstance();
    auto* iterator = instance == nullptr
        ? nullptr
        : instance->resolveEntityIterator(iteratorHandle);
    if (iterator == nullptr || circle == nullptr || !iterator->hasCurrent)
    {
        return YICAD_FAILURE;
    }
    const auto& current = iterator->entities[iterator->currentIndex];
    if (current.type != YICAD_ENTITY_CIRCLE)
    {
        return YICAD_FAILURE;
    }
    *circle = current.circle;
    return YICAD_SUCCESS;
}

void YICAD_PLUGIN_CALL HostApi::entityIteratorDestroy(
    YiCadEntityIteratorHandle iteratorHandle) noexcept
{
    try
    {
        auto* instance = activeInstance();
        auto* iterator = instance == nullptr
            ? nullptr
            : instance->resolveEntityIterator(iteratorHandle);
        if (iterator == nullptr)
        {
            return;
        }
        instance->m_entityIterators.erase(std::remove_if(
            instance->m_entityIterators.begin(),
            instance->m_entityIterators.end(),
            [iterator](const auto& item) { return item.get() == iterator; }));
    }
    catch (...)
    {
    }
}

#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
YiCadImportResult YICAD_PLUGIN_CALL HostApi::beginImport(
    YiCadDocumentHandle documentHandle,
    YiCadImportSessionHandle* session) noexcept
{
    if (session != nullptr)
    {
        *session = nullptr;
    }

    try
    {
        auto* instance = activeInstance();
        if (instance == nullptr || session == nullptr)
        {
            return instance == nullptr
                ? YICAD_IMPORT_ERROR_INVALID_ARGUMENT
                : instance->setImportError(
                      YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
                      "导入会话输出参数无效");
        }

        instance->invalidateClosedImportSessions();
        auto* document = instance->resolveDocument(documentHandle);
        auto* manager = document == nullptr
            ? nullptr
            : document->getCmdManager();
        if (document == nullptr || manager == nullptr)
        {
            return instance->setImportError(
                YICAD_IMPORT_ERROR_INVALID_HANDLE,
                "文档句柄无效或已过期");
        }
        if (instance->hasActiveTransaction(document) ||
            manager->getCurrentCmd() != nullptr ||
            manager->getCurrentGroupCmd() != nullptr)
        {
            return instance->setImportError(
                YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
                "文档已存在活动事务或导入会话");
        }

        auto record = std::make_unique<ImportSessionRecord>();
        record->document = document;
        record->modelSpace.document = document;
        record->modelSpace.modelSpace = true;
        record->modelSpace.active = true;
        record->initialUndoCount = manager->getUndoCount();
        record->transaction = std::make_unique<Transaction>(
            "Plugin: Import", document);
        instance->m_importSessions.reserve(
            instance->m_importSessions.size() + 1);
        record->transaction->start();
        record->command = manager->getCurrentCmd();
        if (record->command == nullptr)
        {
            return instance->setImportError(
                YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
                "宿主无法启动导入事务");
        }

        record->active = true;
        auto* handle = record.get();
        instance->m_importSessions.push_back(std::move(record));
        *session = static_cast<void*>(handle);
        instance->clearImportError();
        return YICAD_IMPORT_SUCCESS;
    }
    catch (const std::bad_alloc&)
    {
        auto* instance = activeInstance();
        return instance == nullptr
            ? YICAD_IMPORT_ERROR_OUT_OF_MEMORY
            : instance->setImportError(
                  YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
                  "创建导入会话时内存不足");
    }
    catch (...)
    {
        auto* instance = activeInstance();
        return instance == nullptr
            ? YICAD_IMPORT_ERROR_TRANSACTION_FAILED
            : instance->setImportError(
                  YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
                  "启动导入事务失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::commitImport(
    YiCadImportSessionHandle sessionHandle) noexcept
{
    auto* instance = activeInstance();
    auto* record = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    if (record == nullptr)
    {
        return instance == nullptr
            ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(
                  YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    if (!instance->m_context.isDocumentOpen(record->document))
    {
        instance->releaseImportSession(record);
        return instance->setImportError(
            YICAD_IMPORT_ERROR_INVALID_HANDLE,
            "导入会话所属文档已关闭");
    }

    auto* manager = record->document->getCmdManager();
    try
    {
        const auto validationResult = instance->validateImportBlocks(record);
        if (validationResult != YICAD_IMPORT_SUCCESS)
        {
            record->transaction->rollback();
            instance->releaseImportSession(record);
            return validationResult;
        }
        if (manager == nullptr || manager->getCurrentCmd() != record->command ||
            manager->getCurrentGroupCmd() != nullptr)
        {
            throw std::runtime_error("导入事务状态不一致");
        }

        if (record->command->size() == 0)
        {
            record->transaction->rollback();
        }
        else
        {
            record->transaction->commit();
        }
        instance->releaseImportSession(record);
        instance->clearImportError();
        return YICAD_IMPORT_SUCCESS;
    }
    catch (...)
    {
        try
        {
            if (manager != nullptr &&
                manager->getCurrentCmd() == record->command)
            {
                record->transaction->rollback();
            }
            else if (manager != nullptr &&
                     manager->getUndoCount() > record->initialUndoCount)
            {
                manager->rollbackAndRemoveAfter(record->initialUndoCount);
            }
        }
        catch (...)
        {
        }
        instance->releaseImportSession(record);
        return instance->setImportError(
            YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "提交导入事务失败，宿主已执行回滚");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::rollbackImport(
    YiCadImportSessionHandle sessionHandle) noexcept
{
    auto* instance = activeInstance();
    auto* record = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    if (record == nullptr)
    {
        return instance == nullptr
            ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(
                  YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    if (!instance->m_context.isDocumentOpen(record->document))
    {
        instance->releaseImportSession(record);
        return instance->setImportError(
            YICAD_IMPORT_ERROR_INVALID_HANDLE,
            "导入会话所属文档已关闭");
    }

    try
    {
        record->transaction->rollback();
        instance->releaseImportSession(record);
        instance->clearImportError();
        return YICAD_IMPORT_SUCCESS;
    }
    catch (...)
    {
        instance->releaseImportSession(record);
        return instance->setImportError(
            YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "回滚导入事务失败");
    }
}

uint32_t YICAD_PLUGIN_CALL HostApi::importGetLastError(
    char* buffer,
    uint32_t bufferSize) noexcept
{
    auto* instance = activeInstance();
    if (instance == nullptr)
    {
        if (buffer != nullptr && bufferSize > 0)
        {
            buffer[0] = '\0';
        }
        return 1;
    }

    const auto& message = instance->m_importLastError;
    const auto required = message.size() + 1;
    if (buffer != nullptr && bufferSize > 0)
    {
        const auto copySize = std::min<std::size_t>(
            message.size(), static_cast<std::size_t>(bufferSize - 1));
        std::memcpy(buffer, message.data(), copySize);
        buffer[copySize] = '\0';
    }
    return required > UINT32_MAX
        ? UINT32_MAX
        : static_cast<uint32_t>(required);
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::setDocumentSettings(
    YiCadImportSessionHandle sessionHandle,
    const YiCadDocumentSettings* settings) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    constexpr auto requiredSize = offsetof(YiCadDocumentSettings, sourceCodePage) +
        sizeof(((YiCadDocumentSettings*)0)->sourceCodePage);
    QString codePage;
    if (settings == nullptr ||
        !validStructSize(settings->structSize, requiredSize) ||
        settings->insertionUnits < 0 || settings->insertionUnits > 20 ||
        (settings->measurement != 0 && settings->measurement != 1) ||
        !std::isfinite(settings->globalLineTypeScale) ||
        settings->globalLineTypeScale <= 0.0 ||
        !copyStringView(settings->sourceCodePage, codePage))
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
            "文档设置包含无效字段");
    }

    try
    {
        QHash<QString, DmVariable> variables;
        variables.insert("$INSUNITS", DmVariable(settings->insertionUnits, 70));
        variables.insert("$MEASUREMENT", DmVariable(settings->measurement, 70));
        variables.insert("$LTSCALE", DmVariable(settings->globalLineTypeScale, 40));
        variables.insert("$DWGCODEPAGE", DmVariable(codePage, 3));
        auto* command = new ModifyDocVariablesCmd(session->document, variables);
        session->document->getCmdManager()->addAndExecuteCmd(command);
        instance->clearImportError();
        return YICAD_IMPORT_SUCCESS;
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "保存文档设置时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "保存文档设置失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createLineType(
    YiCadImportSessionHandle sessionHandle,
    const YiCadLineTypeDataV3* input,
    YiCadResourceConflictPolicy conflictPolicy,
    YiCadImportResourceHandle* resource) noexcept
{
    if (resource != nullptr)
    {
        *resource = nullptr;
    }
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadLineTypeDataV3, complex) +
        sizeof(((YiCadLineTypeDataV3*)0)->complex);
    QString name;
    QString description;
    if (session == nullptr || input == nullptr || resource == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        !validConflictPolicy(conflictPolicy) ||
        !copyStringView(input->name, name) || name.isEmpty() ||
        !copyStringView(input->description, description))
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_ARGUMENT
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
                  "线型参数无效");
    }
    if (input->complex != 0)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_UNSUPPORTED,
            "ABI v3 不支持含文字或形文件的复杂线型");
    }
    if ((input->elements.data == nullptr && input->elements.count != 0) ||
        input->elements.count > 1000000)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
            "线型元素数组无效");
    }

    try
    {
        std::vector<double> elements;
        elements.reserve(input->elements.count);
        double calculatedLength = 0.0;
        for (uint32_t index = 0; index < input->elements.count; ++index)
        {
            const auto value = input->elements.data[index];
            if (!std::isfinite(value))
            {
                return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                    "线型元素包含 NaN 或无穷大");
            }
            elements.push_back(value);
            calculatedLength += std::abs(value);
        }
        if (!std::isfinite(calculatedLength))
        {
            return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                "线型元素总长度超出可表示范围");
        }

        auto* table = session->document->getLineTypeTable();
        auto* existing = table->find(name);
        const auto identical = existing != nullptr &&
            existing->getLineTypeDesp() == description &&
            existing->getLineTypeData() == elements;
        if (identical)
        {
            *resource = instance->registerImportResource(
                session, existing, ImportResourceLineType);
            instance->clearImportError();
            return YICAD_IMPORT_SUCCESS;
        }
        if (existing != nullptr && conflictPolicy == YICAD_RESOURCE_CONFLICT_FAIL)
        {
            return instance->setImportError(YICAD_IMPORT_ERROR_NAME_CONFLICT,
                "同名线型的内容不同");
        }
        if (existing != nullptr && conflictPolicy == YICAD_RESOURCE_CONFLICT_RENAME)
        {
            name = uniqueResourceName(name,
                [table](const QString& value) { return table->find(value); });
            existing = nullptr;
        }

        DmLineType* lineType = existing;
        if (lineType == nullptr)
        {
            lineType = new DmLineType(name);
            lineType->setDocument(session->document);
            lineType->setLineTypeDesp(description);
            lineType->setLineTypeData(elements);
            table->add(lineType);
        }
        else
        {
            table->startModify(lineType);
            lineType->setLineTypeDesp(description);
            lineType->setLineTypeData(elements);
        }
        *resource = instance->registerImportResource(
            session, lineType, ImportResourceLineType);
        instance->clearImportError();
        return YICAD_IMPORT_SUCCESS;
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建线型时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建线型失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createLayer(
    YiCadImportSessionHandle sessionHandle,
    const YiCadLayerDataV3* input,
    YiCadResourceConflictPolicy conflictPolicy,
    YiCadImportResourceHandle* resource) noexcept
{
    if (resource != nullptr)
    {
        *resource = nullptr;
    }
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadLayerDataV3, lineWidth) +
        sizeof(((YiCadLayerDataV3*)0)->lineWidth);
    QString name;
    DmColor color;
    if (session == nullptr || input == nullptr || resource == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        !validConflictPolicy(conflictPolicy) ||
        !copyStringView(input->name, name) || name.isEmpty() ||
        !toDmColor(input->color, color) ||
        !validLineWidth(input->lineWidth))
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_ARGUMENT
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
                  "图层参数无效");
    }

    auto* lineType = input->lineType == nullptr
        ? session->document->getLineTypeTable()->find(LineType::Continuous)
        : static_cast<DmLineType*>(instance->resolveImportResource(
              session, input->lineType, ImportResourceLineType));
    if (lineType == nullptr)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_RESOURCE_NOT_FOUND,
            "图层引用的线型句柄无效");
    }

    try
    {
        auto* table = session->document->getLayerTable();
        auto* existing = table->find(name);
        DmLayerData desired(name,
            DmPen(color, static_cast<DM::LineWidth>(input->lineWidth), lineType),
            input->frozen != 0, input->locked != 0);
        desired.print = input->plottable != 0;
        const auto identical = existing != nullptr &&
            existing->getData().name == desired.name &&
            existing->getData().pen == desired.pen &&
            existing->getData().frozen == desired.frozen &&
            existing->getData().locked == desired.locked &&
            existing->getData().print == desired.print;
        if (identical)
        {
            *resource = instance->registerImportResource(
                session, existing, ImportResourceLayer);
            instance->clearImportError();
            return YICAD_IMPORT_SUCCESS;
        }
        if (existing != nullptr && conflictPolicy == YICAD_RESOURCE_CONFLICT_FAIL)
        {
            return instance->setImportError(YICAD_IMPORT_ERROR_NAME_CONFLICT,
                "同名图层的内容不同");
        }
        if (existing != nullptr && conflictPolicy == YICAD_RESOURCE_CONFLICT_RENAME)
        {
            name = uniqueResourceName(name,
                [table](const QString& value) { return table->find(value); });
            desired.name = name;
            existing = nullptr;
        }

        DmLayer* layer = existing;
        if (layer == nullptr)
        {
            layer = new DmLayer();
            layer->setDocument(session->document);
            layer->setData(desired);
            table->add(layer);
        }
        else
        {
            table->startModify(layer);
            layer->setData(desired);
        }
        *resource = instance->registerImportResource(
            session, layer, ImportResourceLayer);
        instance->clearImportError();
        return YICAD_IMPORT_SUCCESS;
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建图层时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建图层失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createTextStyle(
    YiCadImportSessionHandle sessionHandle,
    const YiCadTextStyleDataV3* input,
    YiCadResourceConflictPolicy conflictPolicy,
    YiCadImportResourceHandle* resource) noexcept
{
    if (resource != nullptr)
    {
        *resource = nullptr;
    }
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadTextStyleDataV3, generationFlags) +
        sizeof(((YiCadTextStyleDataV3*)0)->generationFlags);
    QString name;
    QString fontFile;
    QString bigFontFile;
    constexpr uint32_t validGenerationFlags =
        YICAD_TEXT_GENERATION_BACKWARD |
        YICAD_TEXT_GENERATION_UPSIDE_DOWN |
        YICAD_TEXT_GENERATION_VERTICAL;
    if (session == nullptr || input == nullptr || resource == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        !validConflictPolicy(conflictPolicy) ||
        !copyStringView(input->name, name) || name.isEmpty() ||
        !copyStringView(input->fontFile, fontFile) || fontFile.isEmpty() ||
        !copyStringView(input->bigFontFile, bigFontFile) ||
        !std::isfinite(input->fixedHeight) || input->fixedHeight < 0.0 ||
        !std::isfinite(input->widthFactor) || input->widthFactor <= 0.0 ||
        !std::isfinite(input->obliqueAngle) ||
        (input->generationFlags & ~validGenerationFlags) != 0)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_ARGUMENT
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
                  "文字样式参数无效");
    }

    try
    {
        DmTextStyleData desired;
        desired.name = name;
        desired.defaultHeight = input->fixedHeight;
        desired.widhFactor = input->widthFactor;
        desired.slashAngle = input->obliqueAngle;
        desired.isReverseDirection =
            (input->generationFlags & YICAD_TEXT_GENERATION_BACKWARD) != 0;
        desired.isUpsideDown =
            (input->generationFlags & YICAD_TEXT_GENERATION_UPSIDE_DOWN) != 0;
        desired.isVertical =
            (input->generationFlags & YICAD_TEXT_GENERATION_VERTICAL) != 0;

        auto* font = DMFONTLIST->requestFont(fontFile, false);
        if (font != nullptr && font->getFontType() == FontType::System)
        {
            desired.isSystemFont = true;
            desired.pSysFont = font;
            desired.sysFontFamily = DMFONTLIST->getFontFamilyName(
                font, desired.isSysFontBold, desired.isSysFontItalic);
        }
        else
        {
            desired.isSystemFont = false;
            desired.pAsciiFont = font;
            if (font == nullptr)
            {
                desired.invalidAsciiFont = fontFile;
            }
        }
        if (!bigFontFile.isEmpty())
        {
            desired.isSystemFont = false;
            desired.isUseBigfont = true;
            desired.pBigFont = DMFONTLIST->requestFont(bigFontFile, false);
            if (desired.pBigFont == nullptr)
            {
                desired.invalidBigFont = bigFontFile;
            }
        }

        auto fontName = [](const DmTextStyleData& data) {
            if (!data.invalidSysFontFamily.isEmpty()) return data.invalidSysFontFamily;
            if (!data.invalidAsciiFont.isEmpty()) return data.invalidAsciiFont;
            if (data.pSysFont != nullptr) return data.pSysFont->getFileName();
            if (data.pAsciiFont != nullptr) return data.pAsciiFont->getFileName();
            return QString();
        };
        auto bigFontName = [](const DmTextStyleData& data) {
            if (!data.invalidBigFont.isEmpty()) return data.invalidBigFont;
            return data.pBigFont == nullptr ? QString() : data.pBigFont->getFileName();
        };

        auto* table = session->document->getTextStyleTable();
        auto* existing = table->find(name);
        const auto identical = existing != nullptr && [&] {
            const auto current = existing->getData();
            return fontName(current) == fontName(desired) &&
                bigFontName(current) == bigFontName(desired) &&
                current.defaultHeight == desired.defaultHeight &&
                current.widhFactor == desired.widhFactor &&
                current.slashAngle == desired.slashAngle &&
                current.isReverseDirection == desired.isReverseDirection &&
                current.isUpsideDown == desired.isUpsideDown &&
                current.isVertical == desired.isVertical;
        }();
        if (identical)
        {
            *resource = instance->registerImportResource(
                session, existing, ImportResourceTextStyle);
            if (!existing->isValid())
            {
                instance->setImportError(YICAD_IMPORT_SUCCESS,
                    "文字样式引用的字体缺失，已保留原始字体文件名");
            }
            else
            {
                instance->clearImportError();
            }
            return YICAD_IMPORT_SUCCESS;
        }
        if (existing != nullptr && conflictPolicy == YICAD_RESOURCE_CONFLICT_FAIL)
        {
            return instance->setImportError(YICAD_IMPORT_ERROR_NAME_CONFLICT,
                "同名文字样式的内容不同");
        }
        if (existing != nullptr && conflictPolicy == YICAD_RESOURCE_CONFLICT_RENAME)
        {
            name = uniqueResourceName(name,
                [table](const QString& value) { return table->find(value); });
            desired.name = name;
            existing = nullptr;
        }

        DmTextStyle* textStyle = existing;
        if (textStyle == nullptr)
        {
            textStyle = new DmTextStyle(desired);
            textStyle->setDocument(session->document);
            table->add(textStyle);
        }
        else
        {
            table->startModify(textStyle);
            textStyle->setData(desired);
        }
        *resource = instance->registerImportResource(
            session, textStyle, ImportResourceTextStyle);
        if (!textStyle->isValid())
        {
            instance->setImportError(YICAD_IMPORT_SUCCESS,
                "文字样式引用的字体缺失，已保留原始字体文件名");
        }
        else
        {
            instance->clearImportError();
        }
        return YICAD_IMPORT_SUCCESS;
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建文字样式时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建文字样式失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createDimensionStyle(
    YiCadImportSessionHandle sessionHandle,
    const YiCadDimensionStyleDataV3* input,
    YiCadResourceConflictPolicy conflictPolicy,
    YiCadImportResourceHandle* resource) noexcept
{
    if (resource != nullptr)
    {
        *resource = nullptr;
    }
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(
        YiCadDimensionStyleDataV3, allowUnsupportedFields) +
        sizeof(((YiCadDimensionStyleDataV3*)0)->allowUnsupportedFields);
    QString name;
    QString prefix;
    QString suffix;
    DmColor dimLineColor;
    DmColor extensionLineColor;
    DmColor textColor;
    DmColor textFillColor;
    if (session == nullptr || input == nullptr || resource == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        !validConflictPolicy(conflictPolicy) ||
        !copyStringView(input->name, name) || name.isEmpty() ||
        !copyStringView(input->prefix, prefix) ||
        !copyStringView(input->suffix, suffix) ||
        !toDmColor(input->dimLineColor, dimLineColor) ||
        !toDmColor(input->extensionLineColor, extensionLineColor) ||
        !toDmColor(input->textColor, textColor) ||
        !toDmColor(input->textFillColor, textFillColor) ||
        !validLineWidth(input->dimLineWidth) ||
        !validLineWidth(input->extensionLineWidth))
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_ARGUMENT
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
                  "标注样式参数无效");
    }
    if (input->unsupportedFieldMask != 0 && !input->allowUnsupportedFields)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_UNSUPPORTED,
            "标注样式包含 YiCAD 当前无法表达的字段");
    }

    auto* textStyle = static_cast<DmTextStyle*>(instance->resolveImportResource(
        session, input->textStyle, ImportResourceTextStyle));
    auto* dimLineType = static_cast<DmLineType*>(instance->resolveImportResource(
        session, input->dimLineType, ImportResourceLineType));
    auto* extensionLineType = static_cast<DmLineType*>(instance->resolveImportResource(
        session, input->extensionLineType, ImportResourceLineType));
    if (textStyle == nullptr || dimLineType == nullptr || extensionLineType == nullptr)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_RESOURCE_NOT_FOUND,
            "标注样式引用的文字样式或线型句柄无效");
    }

    const double finiteValues[] = {
        input->extensionBeyondDimLine, input->extensionOriginOffset,
        input->fixedExtensionLineLength, input->arrowSize, input->textHeight,
        input->fractionHeightScale, input->textOffset, input->roundOff,
        input->measurementScale};
    if (!std::all_of(std::begin(finiteValues), std::end(finiteValues),
            [](double value) { return std::isfinite(value); }) ||
        input->extensionBeyondDimLine < 0.0 || input->extensionOriginOffset < 0.0 ||
        input->fixedExtensionLineLength < 0.0 || input->arrowSize < 0.0 ||
        input->textHeight < 0.0 || input->fractionHeightScale <= 0.0 ||
        input->textOffset < 0.0 || input->roundOff < 0.0 ||
        input->measurementScale <= 0.0 ||
        input->firstArrow < 0 || input->firstArrow > 20 ||
        input->secondArrow < 0 || input->secondArrow > 20 ||
        input->leaderArrow < 0 || input->leaderArrow > 20 ||
        input->textVerticalPosition < 0 || input->textVerticalPosition > 4 ||
        input->textHorizontalPosition < 0 || input->textHorizontalPosition > 4 ||
        input->textDirection < 0 || input->textDirection > 1 ||
        input->linearUnitFormat < 0 || input->linearUnitFormat > 5 ||
        input->linearPrecision < 0 || input->linearPrecision > 8 ||
        input->fractionFormat < 0 || input->fractionFormat > 2 ||
        input->decimalSeparator < 0 || input->decimalSeparator > 2 ||
        input->angularUnitFormat < 0 || input->angularUnitFormat > 3 ||
        input->angularPrecision < 0 || input->angularPrecision > 8)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
            "标注样式字段超出 YiCAD 可表达范围");
    }

    try
    {
        DmDimensionStyleData desired;
        desired.name = name;
        desired.setDimLineColor(dimLineColor);
        desired.setDimLineWidth(static_cast<DM::LineWidth>(input->dimLineWidth));
        desired.setDimLineType(dimLineType);
        desired.setHideDimLine1(input->hideDimLine1 != 0);
        desired.setHideDimLine2(input->hideDimLine2 != 0);
        desired.setBoundLineColor(extensionLineColor);
        desired.setBoundLineWidth(static_cast<DM::LineWidth>(input->extensionLineWidth));
        desired.setBoundLineType(extensionLineType);
        desired.setHideBoundLine1(input->hideExtensionLine1 != 0);
        desired.setHideBoundLine2(input->hideExtensionLine2 != 0);
        desired.setExtendDimLine(input->extensionBeyondDimLine);
        desired.setStartPtOffset(input->extensionOriginOffset);
        desired.setIsFixedBoundLineLength(input->fixedExtensionLineLengthEnabled != 0);
        desired.setFixedBoundLineLength(input->fixedExtensionLineLength);
        desired.setFirstArrow(static_cast<DM::ArrowType>(input->firstArrow));
        desired.setSecondArrow(static_cast<DM::ArrowType>(input->secondArrow));
        desired.setLeaderArrow(static_cast<DM::ArrowType>(input->leaderArrow));
        desired.setArrowSize(input->arrowSize);
        desired.setTextStyle(textStyle);
        desired.setTextColor(textColor);
        desired.setTextFillColor(textFillColor);
        desired.setTextHeight(input->textHeight);
        desired.setFractionHeightScale(input->fractionHeightScale);
        desired.setIsDrawTextBoundary(input->drawTextBoundary != 0);
        desired.setTextVerticalPos(static_cast<DmDimensionStyleTextData::TextVerticalPos>(input->textVerticalPosition));
        desired.setTextHorizontalPos(static_cast<DmDimensionStyleTextData::TextHorizontalPos>(input->textHorizontalPosition));
        desired.setViewDirection(static_cast<DmDimensionStyleTextData::ViewDirection>(input->textDirection));
        desired.setOffsetFromDimLine(input->textOffset);
        desired.setUnitFormat(static_cast<DmDimensionStyleUnitData::UnitFormat>(input->linearUnitFormat));
        desired.setPrecision(static_cast<DmDimensionStyleUnitData::Precision>(input->linearPrecision));
        desired.setFractionFormat(static_cast<DmDimensionStyleUnitData::FractionFormat>(input->fractionFormat));
        desired.setDecimalSaparator(static_cast<DmDimensionStyleUnitData::DecimalSaparator>(input->decimalSeparator));
        desired.setRoundOff(input->roundOff);
        desired.setPrefix(prefix);
        desired.setPostfix(suffix);
        desired.setMesureUnitFactor(input->measurementScale);
        desired.setResetPrefix(input->suppressLeadingZeros != 0);
        desired.setResetPostfix(input->suppressTrailingZeros != 0);
        desired.setAngleUnitFormat(static_cast<DmDimensionStyleUnitData::AngleUnitFormat>(input->angularUnitFormat));
        desired.setAnglePrecision(static_cast<DmDimensionStyleUnitData::Precision>(input->angularPrecision));
        desired.setResetAnglePrefix(input->suppressAngularLeadingZeros != 0);
        desired.setResetAnglePostfix(input->suppressAngularTrailingZeros != 0);

        auto same = [](const DmDimensionStyleData& a,
                       const DmDimensionStyleData& b) {
            return a.dimLineColor() == b.dimLineColor() &&
                a.dimLineWidth() == b.dimLineWidth() && a.dimLineType() == b.dimLineType() &&
                a.hideDimLine1() == b.hideDimLine1() && a.hideDimLine2() == b.hideDimLine2() &&
                a.boundLineColor() == b.boundLineColor() &&
                a.boundLineWidth() == b.boundLineWidth() && a.boundLineType() == b.boundLineType() &&
                a.hideBoundLine1() == b.hideBoundLine1() && a.hideBoundLine2() == b.hideBoundLine2() &&
                a.extendDimLine() == b.extendDimLine() && a.startPtOffset() == b.startPtOffset() &&
                a.isFixedBoundLineLength() == b.isFixedBoundLineLength() &&
                a.fixedBoundLineLength() == b.fixedBoundLineLength() &&
                a.firstArrow() == b.firstArrow() && a.secondArrow() == b.secondArrow() &&
                a.leaderArrow() == b.leaderArrow() && a.arrowSize() == b.arrowSize() &&
                a.textStyle() == b.textStyle() && a.textColor() == b.textColor() &&
                a.textFillColor() == b.textFillColor() && a.textHeight() == b.textHeight() &&
                a.fractionHeightScale() == b.fractionHeightScale() &&
                a.isDrawTextBoundary() == b.isDrawTextBoundary() &&
                a.textVerticalPos() == b.textVerticalPos() &&
                a.textHorizontalPos() == b.textHorizontalPos() && a.viewDirection() == b.viewDirection() &&
                a.offsetFromDimLine() == b.offsetFromDimLine() && a.unitFormat() == b.unitFormat() &&
                a.precision() == b.precision() && a.fractionFormat() == b.fractionFormat() &&
                a.decimalSaparator() == b.decimalSaparator() && a.roundOff() == b.roundOff() &&
                a.prefix() == b.prefix() && a.postfix() == b.postfix() &&
                a.mesureUnitFactor() == b.mesureUnitFactor() &&
                a.resetPrefix() == b.resetPrefix() && a.resetPostfix() == b.resetPostfix() &&
                a.angleUnitFormat() == b.angleUnitFormat() &&
                a.anglePrecision() == b.anglePrecision() &&
                a.resetAnglePrefix() == b.resetAnglePrefix() &&
                a.resetAnglePostfix() == b.resetAnglePostfix();
        };

        auto* table = session->document->getDimStyleTable();
        auto* existing = table->find(name);
        if (existing != nullptr && same(existing->getDataConstRef(), desired))
        {
            *resource = instance->registerImportResource(
                session, existing, ImportResourceDimensionStyle);
        }
        else
        {
            if (existing != nullptr && conflictPolicy == YICAD_RESOURCE_CONFLICT_FAIL)
            {
                return instance->setImportError(YICAD_IMPORT_ERROR_NAME_CONFLICT,
                    "同名标注样式的内容不同");
            }
            if (existing != nullptr && conflictPolicy == YICAD_RESOURCE_CONFLICT_RENAME)
            {
                name = uniqueResourceName(name,
                    [table](const QString& value) { return table->find(value); });
                desired.name = name;
                existing = nullptr;
            }
            DmDimensionStyle* style = existing;
            if (style == nullptr)
            {
                style = new DmDimensionStyle(desired);
                style->setDocument(session->document);
                table->add(style);
            }
            else
            {
                table->startModify(style);
                style->updateData(desired);
            }
            *resource = instance->registerImportResource(
                session, style, ImportResourceDimensionStyle);
        }

        if (input->unsupportedFieldMask != 0)
        {
            instance->setImportError(YICAD_IMPORT_SUCCESS,
                "标注样式中显式允许降级的字段已忽略");
        }
        else
        {
            instance->clearImportError();
        }
        return YICAD_IMPORT_SUCCESS;
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建标注样式时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建标注样式失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::getModelSpace(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle* container) noexcept
{
    if (container != nullptr)
    {
        *container = nullptr;
    }
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    if (container == nullptr)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
            "模型空间输出参数为空");
    }
    *container = static_cast<void*>(&session->modelSpace);
    instance->clearImportError();
    return YICAD_IMPORT_SUCCESS;
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createPoint(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle container,
    const YiCadPointDataV3* input) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadPointDataV3, position) +
        sizeof(((YiCadPointDataV3*)0)->position);
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    if (input == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        !finitePoint(input->position))
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_ARGUMENT
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
                  "点实体参数无效");
    }
    try
    {
        return instance->addImportEntity(session, container, input->attributes,
            new DmPoint(nullptr, PointData(toDmVector(input->position))));
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建点实体时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建点实体失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createLine(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle container,
    const YiCadLineDataV3* input) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadLineDataV3, endPoint) +
        sizeof(((YiCadLineDataV3*)0)->endPoint);
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    const auto segmentLength = input == nullptr
        ? 0.0
        : toDmVector(input->startPoint).distanceTo(
              toDmVector(input->endPoint));
    if (input == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        !finitePoint(input->startPoint) || !finitePoint(input->endPoint) ||
        !std::isfinite(segmentLength) || segmentLength <= DM_TOLERANCE)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_ARGUMENT
            : instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                  "线段端点无效或重合");
    }
    try
    {
        return instance->addImportEntity(session, container, input->attributes,
            new DmLine(nullptr, LineData(toDmVector(input->startPoint),
                toDmVector(input->endPoint))));
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建线段实体时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建线段实体失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createRay(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle container,
    const YiCadRayDataV3* input) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadRayDataV3, direction) +
        sizeof(((YiCadRayDataV3*)0)->direction);
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    if (input == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        !finitePoint(input->basePoint) || !finitePoint(input->direction) ||
        !std::isfinite(toDmVector(input->direction).magnitude()) ||
        toDmVector(input->direction).magnitude() <= DM_TOLERANCE)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_ARGUMENT
            : instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                  "射线基点或方向无效");
    }
    try
    {
        return instance->addImportEntity(session, container, input->attributes,
            new DmRay(nullptr, RayData(toDmVector(input->basePoint),
                toDmVector(input->direction).normalize())));
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建射线实体时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建射线实体失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createXLine(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle container,
    const YiCadXLineDataV3* input) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadXLineDataV3, direction) +
        sizeof(((YiCadXLineDataV3*)0)->direction);
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    if (input == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        !finitePoint(input->basePoint) || !finitePoint(input->direction) ||
        !std::isfinite(toDmVector(input->direction).magnitude()) ||
        toDmVector(input->direction).magnitude() <= DM_TOLERANCE)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_ARGUMENT
            : instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                  "无限长线基点或方向无效");
    }
    try
    {
        return instance->addImportEntity(session, container, input->attributes,
            new DmXline(nullptr, XLineData(toDmVector(input->basePoint),
                toDmVector(input->direction).normalize())));
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建无限长线实体时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建无限长线实体失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createArc(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle container,
    const YiCadArcDataV3* input) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadArcDataV3, endAngle) +
        sizeof(((YiCadArcDataV3*)0)->endAngle);
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    const auto normal = input == nullptr
        ? DmVector(false)
        : toDmVector(input->attributes.normal);
    if (input == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        !finitePoint(input->center) || !std::isfinite(input->radius) ||
        !std::isfinite(input->startAngle) || !std::isfinite(input->endAngle) ||
        input->radius <= DM_TOLERANCE || input->radius > 1.0e150 ||
        std::abs(input->endAngle - input->startAngle) <= DM_TOLERANCE ||
        !finitePoint(input->attributes.normal) ||
        normal.magnitude() <= DM_TOLERANCE)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_ARGUMENT
            : instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                  "圆弧半径、角度或法向量无效");
    }
    try
    {
        return instance->addImportEntity(session, container, input->attributes,
            new DmArc(nullptr, ArcData(toDmVector(input->center),
                normal.normalize(), input->radius,
                input->startAngle, input->endAngle)));
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建圆弧实体时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建圆弧实体失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createCircle(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle container,
    const YiCadCircleDataV3* input) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadCircleDataV3, radius) +
        sizeof(((YiCadCircleDataV3*)0)->radius);
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    if (input == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        !finitePoint(input->center) || !std::isfinite(input->radius) ||
        input->radius <= DM_TOLERANCE || input->radius > 1.0e150)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_ARGUMENT
            : instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                  "圆心或半径无效");
    }
    try
    {
        return instance->addImportEntity(session, container, input->attributes,
            new DmCircle(nullptr,
                CircleData(toDmVector(input->center), input->radius)));
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建圆实体时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建圆实体失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createEllipse(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle container,
    const YiCadEllipseDataV3* input) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadEllipseDataV3, closed) +
        sizeof(((YiCadEllipseDataV3*)0)->closed);
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    const auto majorAxis = input == nullptr
        ? DmVector(false)
        : toDmVector(input->majorAxis);
    const auto normal = input == nullptr
        ? DmVector(false)
        : toDmVector(input->attributes.normal);
    if (input == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        !finitePoint(input->center) || !finitePoint(input->majorAxis) ||
        !finitePoint(input->attributes.normal) ||
        !std::isfinite(input->minorToMajorRatio) ||
        !std::isfinite(input->startParameter) ||
        !std::isfinite(input->endParameter) || input->closed > 1 ||
        !std::isfinite(majorAxis.magnitude()) ||
        majorAxis.magnitude() <= DM_TOLERANCE ||
        !std::isfinite(normal.magnitude()) ||
        normal.magnitude() <= DM_TOLERANCE ||
        input->minorToMajorRatio <= 0.0 || input->minorToMajorRatio > 1.0 ||
        (input->closed == 0 &&
         std::abs(input->endParameter - input->startParameter) <= DM_TOLERANCE))
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_ARGUMENT
            : instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                  "椭圆轴、轴比、参数或法向量无效");
    }
    try
    {
        return instance->addImportEntity(session, container, input->attributes,
            new DmEllipse(nullptr, EllipseData(toDmVector(input->center),
                majorAxis, normal.normalize(), input->minorToMajorRatio,
                input->closed != 0, input->startParameter,
                input->endParameter)));
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建椭圆实体时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建椭圆实体失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createPolyline(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle container,
    const YiCadPolylineDataV3* input) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadPolylineDataV3, closed) +
        sizeof(((YiCadPolylineDataV3*)0)->closed);
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    if (input == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        input->vertices.data == nullptr || input->vertices.count < 2 ||
        input->vertices.count > 1000000 ||
        input->closed > 1)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_ARGUMENT
            : instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                  "二维多段线顶点数组或标志无效");
    }
    try
    {
        std::vector<DmVector> vertices;
        std::vector<double> bulges;
        std::vector<double> widths;
        vertices.reserve(input->vertices.count);
        const auto segmentCount = input->closed != 0
            ? input->vertices.count
            : input->vertices.count - 1;
        bulges.reserve(segmentCount);
        widths.reserve(static_cast<std::size_t>(segmentCount) * 2);
        for (uint32_t index = 0; index < input->vertices.count; ++index)
        {
            const auto& vertex = input->vertices.data[index];
            if (!finitePoint(vertex.position) ||
                !std::isfinite(vertex.startWidth) || vertex.startWidth < 0.0 ||
                vertex.startWidth > 1.0e150 ||
                !std::isfinite(vertex.endWidth) || vertex.endWidth < 0.0 ||
                vertex.endWidth > 1.0e150 ||
                !std::isfinite(vertex.bulge) ||
                std::abs(vertex.bulge) > 1.0e12)
            {
                return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                    "二维多段线顶点包含无效坐标、宽度或凸度");
            }
            vertices.emplace_back(vertex.position.x, vertex.position.y);
            if (index < segmentCount)
            {
                bulges.push_back(vertex.bulge);
                widths.push_back(vertex.startWidth);
                widths.push_back(vertex.endWidth);
            }
        }
        for (uint32_t index = 0; index < segmentCount; ++index)
        {
            const auto next = (index + 1) % input->vertices.count;
            const auto length = vertices[index].distanceTo(vertices[next]);
            if (!std::isfinite(length) || length <= DM_TOLERANCE)
            {
                return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                    "二维多段线包含零长度或超出范围的线段");
            }
        }
        PolylineData data(vertices, bulges, widths, input->closed != 0);
        auto entity = std::make_unique<DmPolyline>(nullptr, data);
        if (!entity->isValid())
        {
            return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                "二维多段线段数量与顶点数据不一致");
        }
        return instance->addImportEntity(session, container, input->attributes,
            entity.release());
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建二维多段线时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建二维多段线失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createSpline(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle container,
    const YiCadSplineDataV3* input) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadSplineDataV3, fitPoints) +
        sizeof(((YiCadSplineDataV3*)0)->fitPoints);
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    if (input == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        (input->definition != YICAD_SPLINE_CONTROL_POINTS &&
         input->definition != YICAD_SPLINE_FIT_POINTS) ||
        input->degree < 1 || input->degree > 25 || input->closed > 1 ||
        input->rational > 1 || input->periodic > 1 ||
        !validPointArray(input->controlPoints) ||
        !validDoubleArray(input->knots) ||
        !validDoubleArray(input->weights) ||
        !validPointArray(input->fitPoints))
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_ARGUMENT
            : instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                  "样条定义、次数或数组无效");
    }
    if (input->rational != 0 || input->periodic != 0 ||
        input->weights.count != 0)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_UNSUPPORTED,
            "YiCAD 当前模型不支持有理或周期样条");
    }
    try
    {
        auto toPoints = [](const YiCadPoint2dArrayView& source) {
            std::vector<DmVector> result;
            result.reserve(source.count);
            for (uint32_t index = 0; index < source.count; ++index)
            {
                result.push_back(toDmVector(source.data[index]));
            }
            return result;
        };

        std::unique_ptr<DmSpline> entity;
        if (input->definition == YICAD_SPLINE_CONTROL_POINTS)
        {
            const auto expectedKnots = static_cast<uint64_t>(
                input->controlPoints.count) + input->degree + 1;
            if (input->controlPoints.count < input->degree + 1 ||
                expectedKnots != input->knots.count ||
                input->fitPoints.count != 0 ||
                !std::is_sorted(input->knots.data,
                    input->knots.data + input->knots.count) ||
                input->knots.data[input->degree] >=
                    input->knots.data[input->controlPoints.count])
            {
                return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                    "控制点样条的点数或节点向量无效");
            }
            SplineData data(static_cast<int>(input->degree),
                input->closed != 0, ESplineType::eControlPoints);
            data.setControlPoints(toPoints(input->controlPoints));
            data.setKnots(std::vector<double>(input->knots.data,
                input->knots.data + input->knots.count));
            entity = std::make_unique<DmSpline>(nullptr, data);
        }
        else
        {
            if (input->degree != 3 || input->fitPoints.count < 4 ||
                input->controlPoints.count != 0 || input->knots.count != 0)
            {
                return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                    "拟合点样条当前仅支持至少四点的三次样条");
            }
            SplineData data(3, input->closed != 0, ESplineType::eFitPoints);
            auto fitPoints = toPoints(input->fitPoints);
            double pathLength = 0.0;
            for (std::size_t index = 1; index < fitPoints.size(); ++index)
            {
                pathLength += fitPoints[index - 1].distanceTo(fitPoints[index]);
            }
            if (!std::isfinite(pathLength) || pathLength <= DM_TOLERANCE ||
                (input->closed != 0 &&
                 fitPoints.front().distanceTo(fitPoints.back()) > DM_TOLERANCE))
            {
                return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                    "拟合点样条路径无效或闭合端点不重合");
            }
            data.setFitPoints(fitPoints);
            entity = std::make_unique<DmSpline>(nullptr, data);
            entity->fit();
        }
        if (!entity->isValid())
        {
            return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                "样条数据无法形成有效曲线");
        }
        return instance->addImportEntity(session, container, input->attributes,
            entity.release());
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建样条实体时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建样条实体失败");
    }
}

YiCadImportResult HostApi::buildImportTextData(
    ImportSessionRecord* session,
    const YiCadTextDataV3& input,
    const QString& value,
    TextData& output) noexcept
{
    constexpr auto requiredSize = offsetof(YiCadTextDataV3, textStyle) +
        sizeof(((YiCadTextDataV3*)0)->textStyle);
    if (session == nullptr ||
        !validStructSize(input.structSize, requiredSize) ||
        !finitePoint(input.insertionPoint) ||
        !finitePoint(input.alignmentPoint) ||
        !std::isfinite(input.height) || input.height <= DM_TOLERANCE ||
        input.height > 1.0e150 || !std::isfinite(input.rotation) ||
        !std::isfinite(input.widthFactor) || input.widthFactor <= 0.0 ||
        input.widthFactor > 1.0e12 || !std::isfinite(input.obliqueAngle) ||
        input.horizontalAlignment < YICAD_TEXT_ALIGN_LEFT ||
        input.horizontalAlignment > YICAD_TEXT_ALIGN_FIT ||
        input.verticalAlignment < YICAD_TEXT_ALIGN_BASELINE ||
        input.verticalAlignment > YICAD_TEXT_ALIGN_TOP)
    {
        return setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
            "文字位置、尺寸、角度或对齐方式无效");
    }

    auto* style = input.textStyle == nullptr
        ? session->document->getTextStyleTable()->getActive()
        : static_cast<DmTextStyle*>(resolveImportResource(
              session, input.textStyle, ImportResourceTextStyle));
    if (style == nullptr)
    {
        return setImportError(YICAD_IMPORT_ERROR_RESOURCE_NOT_FOUND,
            "文字引用的文字样式句柄无效");
    }

    output = TextData(toDmVector(input.insertionPoint), input.height,
        static_cast<ETextVertMode>(input.verticalAlignment),
        static_cast<ETextHorzMode>(input.horizontalAlignment), value, style,
        input.rotation, EUpdateMode::NoUpdate);
    output.setAlignment(toDmVector(input.alignmentPoint));
    output.setWidthFactor(input.widthFactor);
    output.setSlashAngle(input.obliqueAngle);
    return YICAD_IMPORT_SUCCESS;
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createText(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle container,
    const YiCadTextDataV3* input) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    QString value;
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    if (input == nullptr || !copyStringView(input->text, value) ||
        value.isEmpty())
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
            "单行文字内容无效");
    }
    try
    {
        TextData textData;
        const auto result = instance->buildImportTextData(
            session, *input, value, textData);
        if (result != YICAD_IMPORT_SUCCESS)
        {
            return result;
        }
        return instance->addImportEntity(session, container, input->attributes,
            new DmText(nullptr, textData));
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建单行文字时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建单行文字失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createMText(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle container,
    const YiCadMTextDataV3* input) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadMTextDataV3, background) +
        sizeof(((YiCadMTextDataV3*)0)->background);
    constexpr auto backgroundSize =
        offsetof(YiCadMTextBackgroundData, borderScaleFactor) +
        sizeof(((YiCadMTextBackgroundData*)0)->borderScaleFactor);
    QString contents;
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    if (input == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        !validStructSize(input->background.structSize, backgroundSize) ||
        !copyStringView(input->contents, contents) || contents.isEmpty() ||
        !finitePoint(input->insertionPoint) ||
        !finitePoint(input->direction) ||
        toDmVector(input->direction).magnitude() <= DM_TOLERANCE ||
        !std::isfinite(input->characterHeight) ||
        input->characterHeight <= DM_TOLERANCE ||
        !std::isfinite(input->rectangleWidth) ||
        input->rectangleWidth <= DM_TOLERANCE ||
        !std::isfinite(input->lineSpacingFactor) ||
        input->lineSpacingFactor < 0.25 || input->lineSpacingFactor > 4.0 ||
        input->attachment < YICAD_MTEXT_TOP_LEFT ||
        input->attachment > YICAD_MTEXT_BOTTOM_RIGHT ||
        input->background.enabled > 1 ||
        input->background.useDrawingBackgroundColor > 1)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
            "多行文字内容、尺寸、方向或排版参数无效");
    }
    if (input->background.enabled != 0)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_UNSUPPORTED,
            "YiCAD 当前多行文字模型不支持背景填充");
    }

    auto* style = input->textStyle == nullptr
        ? session->document->getTextStyleTable()->getActive()
        : static_cast<DmTextStyle*>(instance->resolveImportResource(
              session, input->textStyle, ImportResourceTextStyle));
    if (style == nullptr)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_RESOURCE_NOT_FOUND,
            "多行文字引用的文字样式句柄无效");
    }

    try
    {
        const auto angle = std::atan2(
            input->direction.y, input->direction.x);
        const auto attachment = input->attachment - 1;
        const auto horizontal = attachment % 3;
        const auto vertical = attachment / 3;
        const auto lineSpace = input->lineSpacingFactor *
            input->characterHeight * LINE_HEIGHT_PER_CHAR_HEIGHT;
        MTextData textData(toDmVector(input->insertionPoint),
            input->characterHeight,
            static_cast<EMTextVertMode>(vertical == 0 ? 3 :
                (vertical == 1 ? 2 : 1)),
            static_cast<EMTextHorzMode>(horizontal), lineSpace,
            input->rectangleWidth, contents, style, angle,
            EUpdateMode::NoUpdate);
        textData.setLineSpacingFactor(input->lineSpacingFactor);
        textData.setJustification(static_cast<EMTextMode>(attachment));
        return instance->addImportEntity(session, container, input->attributes,
            new DmMText(nullptr, textData));
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建多行文字时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建多行文字失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::beginBlock(
    YiCadImportSessionHandle sessionHandle,
    const YiCadBlockDataV3* input,
    YiCadImportResourceHandle* blockHandle,
    YiCadImportContainerHandle* containerHandle) noexcept
{
    if (blockHandle != nullptr) *blockHandle = nullptr;
    if (containerHandle != nullptr) *containerHandle = nullptr;
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(
        YiCadBlockDataV3, externalReferencePath) +
        sizeof(((YiCadBlockDataV3*)0)->externalReferencePath);
    QString name;
    QString description;
    QString path;
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    if (input == nullptr || blockHandle == nullptr ||
        containerHandle == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        !copyStringView(input->name, name) || name.isEmpty() ||
        !finitePoint(input->basePoint) ||
        !copyStringView(input->description, description) ||
        !copyStringView(input->externalReferencePath, path))
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
            "块名称、基点或字符串参数无效");
    }
    constexpr uint32_t supportedFlags = YICAD_BLOCK_HAS_ATTRIBUTES;
    if (!description.isEmpty() || !path.isEmpty() ||
        (input->flags & ~supportedFlags) != 0)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_UNSUPPORTED,
            "YiCAD 当前块模型不支持块说明、外部参照或相应块标志");
    }
    auto* table = session->document->getBlockTable();
    if (table == nullptr)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "文档块表不可用");
    }
    if (table->find(name) != nullptr)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_NAME_CONFLICT,
            "块名称已经存在");
    }

    try
    {
        session->containers.reserve(session->containers.size() + 1);
        session->resources.reserve(session->resources.size() + 1);
        auto block = std::make_unique<DmBlock>(session->document,
            DmBlockData(name, toDmVector(input->basePoint), false));
        auto container = std::make_unique<ImportSessionRecord::ContainerRecord>();
        auto resource = std::make_unique<ImportSessionRecord::ResourceRecord>();
        container->document = session->document;
        container->block = block.get();
        container->active = true;
        resource->object = block.get();
        resource->kind = ImportResourceBlock;
        resource->active = true;
        resource->finalized = false;
        if (!table->add(block.get()))
        {
            return instance->setImportError(YICAD_IMPORT_ERROR_NAME_CONFLICT,
                "块定义无法加入文档块表");
        }
        block.release();
        auto* containerPtr = container.get();
        auto* resourcePtr = resource.get();
        session->containers.push_back(std::move(container));
        session->resources.push_back(std::move(resource));
        *blockHandle = static_cast<void*>(resourcePtr);
        *containerHandle = static_cast<void*>(containerPtr);
        instance->clearImportError();
        return YICAD_IMPORT_SUCCESS;
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建块定义时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建块定义失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::endBlock(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle containerHandle) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    auto* container = static_cast<ImportSessionRecord::ContainerRecord*>(
        instance->resolveImportContainer(session, containerHandle));
    if (container == nullptr || container->modelSpace || container->block == nullptr)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
            "块容器句柄无效、已结束或不是块定义");
    }
    for (auto& resource : session->resources)
    {
        if (resource->active && resource->kind == ImportResourceBlock &&
            resource->object == container->block)
        {
            resource->finalized = true;
            container->active = false;
            instance->clearImportError();
            return YICAD_IMPORT_SUCCESS;
        }
    }
    return instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
        "块定义资源句柄已失效");
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createInsert(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle containerHandle,
    const YiCadInsertDataV3* input,
    YiCadImportResourceHandle* insertHandle) noexcept
{
    if (insertHandle != nullptr) *insertHandle = nullptr;
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadInsertDataV3, rowSpacing) +
        sizeof(((YiCadInsertDataV3*)0)->rowSpacing);
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    if (input == nullptr || insertHandle == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        !finitePoint(input->insertionPoint) || !finitePoint(input->scale) ||
        !std::isfinite(input->rotation) ||
        !std::isfinite(input->columnSpacing) ||
        !std::isfinite(input->rowSpacing) ||
        std::abs(input->scale.x) <= 1.0e-6 ||
        std::abs(input->scale.y) <= 1.0e-6 ||
        input->columnCount == 0 || input->rowCount == 0 ||
        input->columnCount > 100000 || input->rowCount > 100000)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
            "块引用位置、比例、旋转或阵列参数无效");
    }
    if (std::abs(input->scale.z - 1.0) > DM_TOLERANCE)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_UNSUPPORTED,
            "YiCAD 二维块引用不支持非 1 的 Z 轴比例");
    }
    auto* targetContainer = static_cast<ImportSessionRecord::ContainerRecord*>(
        instance->resolveImportContainer(session, containerHandle));
    auto* block = static_cast<DmBlock*>(instance->resolveImportResource(
        session, input->block, ImportResourceBlock));
    bool finalized = false;
    for (const auto& resource : session->resources)
    {
        if (resource->active && resource->kind == ImportResourceBlock &&
            resource->object == block)
        {
            finalized = resource->finalized;
            break;
        }
    }
    if (targetContainer == nullptr || block == nullptr || !finalized)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_RESOURCE_NOT_FOUND,
            "块引用只能使用本会话中已经完成定义的块，前向引用不受支持");
    }

    try
    {
        session->resources.reserve(session->resources.size() + 1);
        auto entity = std::make_unique<DmBlockReference>(nullptr,
            DmBlockReferenceData(block->getName(),
                toDmVector(input->insertionPoint),
                DmVector(input->scale.x, input->scale.y), input->rotation,
                static_cast<int>(input->columnCount),
                static_cast<int>(input->rowCount),
                DmVector(input->columnSpacing, input->rowSpacing),
                session->document->getBlockTable(), DM::NoUpdate));
        entity->setBlock(block);
        auto resource = std::make_unique<ImportSessionRecord::ResourceRecord>();
        auto* rawEntity = entity.release();
        resource->object = rawEntity;
        resource->container = targetContainer;
        resource->kind = ImportResourceInsert;
        resource->active = true;
        const auto result = instance->addImportEntity(
            session, containerHandle, input->attributes, rawEntity);
        if (result != YICAD_IMPORT_SUCCESS)
        {
            return result;
        }
        auto* resourcePtr = resource.get();
        session->resources.push_back(std::move(resource));
        *insertHandle = static_cast<void*>(resourcePtr);
        instance->clearImportError();
        return YICAD_IMPORT_SUCCESS;
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建块引用时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建块引用失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createAttributeDefinition(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle containerHandle,
    const YiCadAttributeDefinitionDataV3* input) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(
        YiCadAttributeDefinitionDataV3, flags) +
        sizeof(((YiCadAttributeDefinitionDataV3*)0)->flags);
    QString tag;
    QString prompt;
    QString defaultValue;
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    if (input == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        !copyStringView(input->tag, tag) || tag.trimmed().isEmpty() ||
        !copyStringView(input->prompt, prompt) ||
        !copyStringView(input->defaultValue, defaultValue))
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
            "属性定义的 tag、提示或默认值无效");
    }
    if (input->flags != 0)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_UNSUPPORTED,
            "YiCAD 当前属性定义模型不支持 DXF 属性标志");
    }
    auto* container = static_cast<ImportSessionRecord::ContainerRecord*>(
        instance->resolveImportContainer(session, containerHandle));
    if (container == nullptr || container->modelSpace || container->block == nullptr)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
            "属性定义只能添加到活动块定义容器");
    }
    for (auto* definition : container->block->getAttributeDefinitions())
    {
        if (QString::compare(definition->getTag(), tag,
                Qt::CaseInsensitive) == 0)
        {
            return instance->setImportError(YICAD_IMPORT_ERROR_NAME_CONFLICT,
                "同一块中存在重复的属性定义 tag");
        }
    }

    try
    {
        TextData textData;
        const auto result = instance->buildImportTextData(
            session, input->text, defaultValue, textData);
        if (result != YICAD_IMPORT_SUCCESS)
        {
            return result;
        }
        return instance->addImportEntity(session, containerHandle,
            input->text.attributes,
            new DmAttributeDefinition(nullptr, textData,
                AttributeDefinitionData(tag, prompt)));
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建属性定义时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建属性定义失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createAttribute(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle containerHandle,
    const YiCadAttributeDataV3* input) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadAttributeDataV3, flags) +
        sizeof(((YiCadAttributeDataV3*)0)->flags);
    QString tag;
    QString value;
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    if (input == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        !copyStringView(input->tag, tag) || tag.trimmed().isEmpty() ||
        !copyStringView(input->value, value))
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
            "属性值的 tag 或内容无效");
    }
    if (input->flags != 0)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_UNSUPPORTED,
            "YiCAD 当前属性值模型不支持 DXF 属性标志");
    }
    auto* container = static_cast<ImportSessionRecord::ContainerRecord*>(
        instance->resolveImportContainer(session, containerHandle));
    ImportSessionRecord::ResourceRecord* insertResource = nullptr;
    for (const auto& resource : session->resources)
    {
        if (resource->active && resource->kind == ImportResourceInsert &&
            static_cast<const void*>(resource.get()) == input->insert)
        {
            insertResource = resource.get();
            break;
        }
    }
    if (container == nullptr || insertResource == nullptr ||
        insertResource->container != container)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
            "属性值引用的块引用句柄或所属容器无效");
    }
    auto* insert = static_cast<DmBlockReference*>(insertResource->object);
    auto* block = insert == nullptr ? nullptr : insert->getBlockForInsert();
    if (block == nullptr)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_RESOURCE_NOT_FOUND,
            "属性值所属块定义不存在");
    }
    bool definitionFound = false;
    for (auto* definition : block->getAttributeDefinitions())
    {
        if (QString::compare(definition->getTag(), tag,
                Qt::CaseInsensitive) == 0)
        {
            definitionFound = true;
            break;
        }
    }
    if (!definitionFound)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_RESOURCE_NOT_FOUND,
            "属性值 tag 在块定义中不存在");
    }
    for (auto* attribute : insert->getAttributes())
    {
        if (QString::compare(attribute->getTag(), tag,
                Qt::CaseInsensitive) == 0)
        {
            return instance->setImportError(YICAD_IMPORT_ERROR_NAME_CONFLICT,
                "同一块引用中存在重复的属性值 tag");
        }
    }

    try
    {
        TextData textData;
        auto result = instance->buildImportTextData(
            session, input->text, value, textData);
        if (result != YICAD_IMPORT_SUCCESS)
        {
            return result;
        }
        auto attribute = std::make_unique<DmAttribute>(nullptr, textData,
            AttributeData(tag));
        result = instance->applyImportEntityAttributes(
            session, input->text.attributes, attribute.get());
        if (result != YICAD_IMPORT_SUCCESS)
        {
            return result;
        }
        attribute->update();
        insert->addAttributes({attribute.get()});
        attribute.release();
        auto* table = container->modelSpace
            ? session->document->getEntityTable()
            : &container->block->getEntityTable();
        table->notifyEntityModified(insert);
        instance->clearImportError();
        return YICAD_IMPORT_SUCCESS;
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建属性值时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建属性值失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createDimension(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle container,
    const YiCadDimensionDataV3* input) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadDimensionDataV3, leaderLength) +
        sizeof(((YiCadDimensionDataV3*)0)->leaderLength);
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    if (input == nullptr ||
        !validStructSize(input->structSize, requiredSize))
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
            "标注结构为空或被截短");
    }
    if (input->kind == YICAD_DIMENSION_ORDINATE)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_UNSUPPORTED,
            "YiCAD 当前没有坐标标注语义实体，插件应显式选择是否降级");
    }
    if (input->kind < YICAD_DIMENSION_LINEAR ||
        input->kind > YICAD_DIMENSION_DIAMETRIC)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
            "标注类型无效");
    }

    QString textOverride;
    const YiCadPoint2d points[] = {
        input->definitionPoint, input->textPosition,
        input->extensionPoint1, input->extensionPoint2,
        input->line1Start, input->line1End,
        input->line2Start, input->line2End,
        input->arcPoint, input->featurePoint};
    if (!copyStringView(input->textOverride, textOverride) ||
        !std::all_of(std::begin(points), std::end(points),
            [](const YiCadPoint2d& point) { return finitePoint(point); }) ||
        !std::isfinite(input->textRotation) ||
        !std::isfinite(input->lineSpacingFactor) ||
        input->lineSpacingFactor <= 0.0 ||
        !std::isfinite(input->leaderLength))
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
            "标注点、文字参数或引线长度无效");
    }

    auto* style = input->dimensionStyle == nullptr
        ? session->document->getDimStyleTable()->getActive()
        : static_cast<DmDimensionStyle*>(instance->resolveImportResource(
              session, input->dimensionStyle, ImportResourceDimensionStyle));
    if (style == nullptr)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_RESOURCE_NOT_FOUND,
            "标注引用的标注样式句柄无效");
    }

    auto nonZeroSegment = [](const YiCadPoint2d& first,
                             const YiCadPoint2d& second) {
        return toDmVector(first).distanceTo(toDmVector(second)) > DM_TOLERANCE;
    };
    try
    {
        DmDimensionData common(toDmVector(input->definitionPoint),
            toDmVector(input->textPosition), EMTextVertMode::kTextVertMid,
            EMTextHorzMode::kTextCenter, input->lineSpacingFactor,
            textOverride, input->textRotation, style);
        std::unique_ptr<DmEntity> entity;
        switch (input->kind)
        {
        case YICAD_DIMENSION_LINEAR:
            if (!nonZeroSegment(input->extensionPoint1,
                                input->extensionPoint2))
            {
                return instance->setImportError(
                    YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                    "线性标注的两个尺寸界线起点不能重合");
            }
            entity = std::make_unique<DmDimLinear>(nullptr, common,
                DmDimLinearData(toDmVector(input->extensionPoint1),
                    toDmVector(input->extensionPoint2)));
            break;
        case YICAD_DIMENSION_ALIGNED:
            if (!nonZeroSegment(input->extensionPoint1,
                                input->extensionPoint2))
            {
                return instance->setImportError(
                    YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                    "对齐标注的两个尺寸界线起点不能重合");
            }
            entity = std::make_unique<DmDimAligned>(nullptr, common,
                DmDimAlignedData(toDmVector(input->extensionPoint1),
                    toDmVector(input->extensionPoint2)));
            break;
        case YICAD_DIMENSION_ANGULAR:
            if (!nonZeroSegment(input->line1Start, input->line1End) ||
                !nonZeroSegment(input->line2Start, input->line2End))
            {
                return instance->setImportError(
                    YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                    "角度标注的定义直线不能为零长度");
            }
            entity = std::make_unique<DmDimAngular>(nullptr, common,
                DmDimAngularData(toDmVector(input->line1Start),
                    toDmVector(input->line1End),
                    toDmVector(input->line2Start),
                    toDmVector(input->line2End),
                    toDmVector(input->arcPoint)));
            break;
        case YICAD_DIMENSION_RADIAL:
            if (!nonZeroSegment(input->definitionPoint, input->featurePoint))
            {
                return instance->setImportError(
                    YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                    "半径标注的圆心与箭头点不能重合");
            }
            entity = std::make_unique<DmDimRadial>(nullptr, common,
                DmDimRadialData(toDmVector(input->featurePoint),
                    input->leaderLength));
            break;
        case YICAD_DIMENSION_DIAMETRIC:
            if (!nonZeroSegment(input->definitionPoint, input->featurePoint))
            {
                return instance->setImportError(
                    YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                    "直径标注的两个箭头点不能重合");
            }
            entity = std::make_unique<DmDimDiametric>(nullptr, common,
                DmDimDiametricData(toDmVector(input->featurePoint),
                    input->leaderLength));
            break;
        default:
            break;
        }
        return instance->addImportEntity(session, container,
            input->attributes, entity.release());
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建标注实体时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建标注实体失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createLeader(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle containerHandle,
    const YiCadLeaderDataV3* input) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadLeaderDataV3, text) +
        sizeof(((YiCadLeaderDataV3*)0)->text);
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    if (input == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        !validPointArray(input->vertices) || input->vertices.count < 2 ||
        input->hasArrow > 1 || input->hasText > 1)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
            "引线顶点数组或标志无效");
    }
    for (uint32_t index = 1; index < input->vertices.count; ++index)
    {
        if (toDmVector(input->vertices.data[index - 1]).distanceTo(
                toDmVector(input->vertices.data[index])) <= DM_TOLERANCE)
        {
            return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                "引线包含零长度线段");
        }
    }
    auto* style = input->dimensionStyle == nullptr
        ? session->document->getDimStyleTable()->getActive()
        : static_cast<DmDimensionStyle*>(instance->resolveImportResource(
              session, input->dimensionStyle, ImportResourceDimensionStyle));
    auto* container = static_cast<ImportSessionRecord::ContainerRecord*>(
        instance->resolveImportContainer(session, containerHandle));
    if (style == nullptr)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_RESOURCE_NOT_FOUND,
            "引线引用的标注样式句柄无效");
    }
    if (container == nullptr)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
            "导入容器句柄无效或已过期");
    }

    try
    {
        std::vector<DmVector> vertices;
        vertices.reserve(input->vertices.count);
        for (uint32_t index = 0; index < input->vertices.count; ++index)
        {
            vertices.push_back(toDmVector(input->vertices.data[index]));
        }
        DmLeaderData leaderData(style, vertices);
        if (input->hasArrow == 0)
        {
            leaderData.setLeaderArrow(DM::ArrowType::None);
        }
        auto leader = std::make_unique<DmLeader>(nullptr, leaderData);
        auto result = instance->applyImportEntityAttributes(
            session, input->attributes, leader.get());
        if (result != YICAD_IMPORT_SUCCESS)
        {
            return result;
        }

        std::unique_ptr<DmText> textEntity;
        if (input->hasText != 0)
        {
            QString value;
            if (!copyStringView(input->text.text, value))
            {
                return instance->setImportError(
                    YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
                    "引线文字不是有效的 UTF-8 字符串");
            }
            TextData textData;
            result = instance->buildImportTextData(
                session, input->text, value, textData);
            if (result != YICAD_IMPORT_SUCCESS)
            {
                return result;
            }
            textEntity = std::make_unique<DmText>(nullptr, textData);
            result = instance->applyImportEntityAttributes(
                session, input->text.attributes, textEntity.get());
            if (result != YICAD_IMPORT_SUCCESS)
            {
                return result;
            }
        }

        leader->update();
        if (textEntity != nullptr)
        {
            textEntity->update();
        }
        auto* table = container->modelSpace
            ? session->document->getEntityTable()
            : &container->block->getEntityTable();
        table->add(leader.get());
        leader.release();
        if (textEntity != nullptr)
        {
            table->add(textEntity.get());
            textEntity.release();
        }
        instance->clearImportError();
        return YICAD_IMPORT_SUCCESS;
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建引线实体时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建引线实体失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createHatch(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle containerHandle,
    const YiCadHatchDataV3* input) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadHatchDataV3, loops) +
        sizeof(((YiCadHatchDataV3*)0)->loops);
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    QString patternName;
    if (input == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        input->solid > 1 ||
        !copyStringView(input->patternName, patternName) ||
        !std::isfinite(input->patternScale) || input->patternScale <= 0.0 ||
        !std::isfinite(input->patternAngle) ||
        input->loops.data == nullptr || input->loops.count == 0 ||
        input->loops.count > 100000)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
            "填充图案参数或边界环数组无效");
    }
    if (input->solid == 0 &&
        (patternName.isEmpty() || !DMPATTERNLIST->contains(patternName)))
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_RESOURCE_NOT_FOUND,
            "填充引用的图案名称在 YiCAD 中不存在，插件应先映射为可用图案");
    }
    auto* targetContainer =
        static_cast<ImportSessionRecord::ContainerRecord*>(
            instance->resolveImportContainer(session, containerHandle));
    if (targetContainer == nullptr)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
            "导入容器句柄无效或已过期");
    }

    struct BuiltLoop
    {
        DmEntityContainerPtr boundary;
        YiCadHatchLoopRole role = YICAD_HATCH_LOOP_OUTER;
        uint32_t outerLoopIndex = UINT32_MAX;
    };
    try
    {
        std::vector<BuiltLoop> builtLoops;
        builtLoops.reserve(input->loops.count);
        std::vector<uint32_t> outerIndexes;

        for (uint32_t loopIndex = 0;
             loopIndex < input->loops.count; ++loopIndex)
        {
            const auto& loop = input->loops.data[loopIndex];
            constexpr auto loopSize = offsetof(YiCadHatchLoopDataV3, edges) +
                sizeof(((YiCadHatchLoopDataV3*)0)->edges);
            if (!validStructSize(loop.structSize, loopSize) ||
                (loop.role != YICAD_HATCH_LOOP_OUTER &&
                 loop.role != YICAD_HATCH_LOOP_HOLE) ||
                (loop.kind != YICAD_HATCH_LOOP_POLYLINE &&
                 loop.kind != YICAD_HATCH_LOOP_EDGES))
            {
                return instance->setImportError(
                    YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
                    "填充边界环类型、角色或结构大小无效");
            }
            if (loop.role == YICAD_HATCH_LOOP_OUTER)
            {
                if (loop.outerLoopIndex != UINT32_MAX)
                {
                    return instance->setImportError(
                        YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                        "填充外环不能引用其他外环");
                }
                outerIndexes.push_back(loopIndex);
            }
            else if (loop.outerLoopIndex >= loopIndex ||
                     input->loops.data[loop.outerLoopIndex].role !=
                         YICAD_HATCH_LOOP_OUTER)
            {
                return instance->setImportError(
                    YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                    "填充孔环引用的外环索引无效");
            }

            auto boundary = std::make_shared<DmEntityContainer>(nullptr);
            if (loop.kind == YICAD_HATCH_LOOP_POLYLINE)
            {
                if (loop.edges.count != 0 ||
                    loop.polylineVertices.data == nullptr ||
                    loop.polylineVertices.count < 3 ||
                    loop.polylineVertices.count > 1000000)
                {
                    return instance->setImportError(
                        YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                        "填充折线环的顶点或边数组无效");
                }
                std::vector<DmVector> vertices;
                std::vector<double> bulges;
                std::vector<double> widths;
                vertices.reserve(loop.polylineVertices.count);
                bulges.reserve(loop.polylineVertices.count);
                widths.reserve(static_cast<std::size_t>(
                    loop.polylineVertices.count) * 2);
                for (uint32_t index = 0;
                     index < loop.polylineVertices.count; ++index)
                {
                    const auto& vertex = loop.polylineVertices.data[index];
                    if (!finitePoint(vertex.position) ||
                        !std::isfinite(vertex.startWidth) ||
                        !std::isfinite(vertex.endWidth) ||
                        !std::isfinite(vertex.bulge) ||
                        vertex.startWidth < 0.0 || vertex.endWidth < 0.0 ||
                        std::abs(vertex.bulge) > 1.0e12)
                    {
                        return instance->setImportError(
                            YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                            "填充折线环包含无效顶点");
                    }
                    const auto next = (index + 1) %
                        loop.polylineVertices.count;
                    if (toDmVector(vertex.position).distanceTo(toDmVector(
                            loop.polylineVertices.data[next].position)) <=
                        DM_TOLERANCE)
                    {
                        return instance->setImportError(
                            YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                            "填充折线环包含零长度边");
                    }
                    vertices.push_back(toDmVector(vertex.position));
                    bulges.push_back(vertex.bulge);
                    widths.push_back(vertex.startWidth);
                    widths.push_back(vertex.endWidth);
                }
                auto edge = std::make_unique<DmPolyline>(boundary.get(),
                    PolylineData(vertices, bulges, widths, true));
                if (!edge->isValid())
                {
                    return instance->setImportError(
                        YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                        "填充折线环无法形成有效闭合边界");
                }
                boundary->addEntity(edge.get());
                edge.release();
            }
            else
            {
                if (loop.polylineVertices.count != 0 ||
                    loop.edges.data == nullptr || loop.edges.count == 0 ||
                    loop.edges.count > 1000000)
                {
                    return instance->setImportError(
                        YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                        "填充边环的边或折线数组无效");
                }
                std::vector<std::pair<DmVector, DmVector>> endpoints;
                endpoints.reserve(loop.edges.count);
                for (uint32_t edgeIndex = 0;
                     edgeIndex < loop.edges.count; ++edgeIndex)
                {
                    const auto& source = loop.edges.data[edgeIndex];
                    constexpr auto edgeSize = offsetof(
                        YiCadHatchEdgeDataV3, weights) +
                        sizeof(((YiCadHatchEdgeDataV3*)0)->weights);
                    if (!validStructSize(source.structSize, edgeSize))
                    {
                        return instance->setImportError(
                            YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
                            "填充边结构被截短");
                    }
                    std::unique_ptr<DmEntity> edge;
                    DmVector start(false);
                    DmVector end(false);
                    switch (source.type)
                    {
                    case YICAD_HATCH_EDGE_LINE:
                        if (!finitePoint(source.startPoint) ||
                            !finitePoint(source.endPoint) ||
                            toDmVector(source.startPoint).distanceTo(
                                toDmVector(source.endPoint)) <= DM_TOLERANCE)
                        {
                            return instance->setImportError(
                                YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                                "填充直线边坐标无效");
                        }
                        start = toDmVector(source.startPoint);
                        end = toDmVector(source.endPoint);
                        edge = std::make_unique<DmLine>(boundary.get(),
                            LineData(start, end));
                        break;
                    case YICAD_HATCH_EDGE_CIRCULAR_ARC:
                        if (!finitePoint(source.center) ||
                            !std::isfinite(source.radius) ||
                            source.radius <= DM_TOLERANCE ||
                            !std::isfinite(source.startParameter) ||
                            !std::isfinite(source.endParameter) ||
                            source.counterClockwise > 1 ||
                            std::abs(source.endParameter -
                                source.startParameter) <= DM_TOLERANCE)
                        {
                            return instance->setImportError(
                                YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                                "填充圆弧边参数无效");
                        }
                        start = toDmVector(source.center) + DmVector(
                            std::cos(source.startParameter),
                            std::sin(source.startParameter)) * source.radius;
                        end = toDmVector(source.center) + DmVector(
                            std::cos(source.endParameter),
                            std::sin(source.endParameter)) * source.radius;
                        edge = std::make_unique<DmArc>(boundary.get(),
                            ArcData(toDmVector(source.center),
                                DmVector(0.0, 0.0,
                                    source.counterClockwise != 0 ? 1.0 : -1.0),
                                source.radius, source.startParameter,
                                source.endParameter));
                        break;
                    case YICAD_HATCH_EDGE_ELLIPTIC_ARC:
                    {
                        const auto major = toDmVector(source.majorAxis);
                        if (!finitePoint(source.center) ||
                            !finitePoint(source.majorAxis) ||
                            major.magnitude() <= DM_TOLERANCE ||
                            !std::isfinite(source.minorToMajorRatio) ||
                            source.minorToMajorRatio <= 0.0 ||
                            source.minorToMajorRatio > 1.0 ||
                            !std::isfinite(source.startParameter) ||
                            !std::isfinite(source.endParameter) ||
                            source.counterClockwise > 1 ||
                            std::abs(source.endParameter -
                                source.startParameter) <= DM_TOLERANCE)
                        {
                            return instance->setImportError(
                                YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                                "填充椭圆弧边参数无效");
                        }
                        const DmVector minor(-major.y *
                            source.minorToMajorRatio,
                            major.x * source.minorToMajorRatio);
                        start = toDmVector(source.center) +
                            major * std::cos(source.startParameter) +
                            minor * std::sin(source.startParameter);
                        end = toDmVector(source.center) +
                            major * std::cos(source.endParameter) +
                            minor * std::sin(source.endParameter);
                        edge = std::make_unique<DmEllipse>(boundary.get(),
                            EllipseData(toDmVector(source.center), major,
                                DmVector(0.0, 0.0,
                                    source.counterClockwise != 0 ? 1.0 : -1.0),
                                source.minorToMajorRatio, false,
                                source.startParameter, source.endParameter));
                        break;
                    }
                    case YICAD_HATCH_EDGE_SPLINE:
                    {
                        if (source.rational != 0 || source.periodic != 0 ||
                            source.weights.count != 0)
                        {
                            return instance->setImportError(
                                YICAD_IMPORT_ERROR_UNSUPPORTED,
                                "YiCAD 当前不支持有理或周期填充样条边");
                        }
                        if (source.degree < 1 || source.degree > 25 ||
                            !validPointArray(source.controlPoints) ||
                            !validDoubleArray(source.knots) ||
                            source.controlPoints.count < source.degree + 1 ||
                            source.knots.count !=
                                source.controlPoints.count + source.degree + 1 ||
                            !std::is_sorted(source.knots.data,
                                source.knots.data + source.knots.count))
                        {
                            return instance->setImportError(
                                YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                                "填充样条边的次数、控制点或节点无效");
                        }
                        std::vector<DmVector> controlPoints;
                        controlPoints.reserve(source.controlPoints.count);
                        for (uint32_t index = 0;
                             index < source.controlPoints.count; ++index)
                        {
                            controlPoints.push_back(toDmVector(
                                source.controlPoints.data[index]));
                        }
                        start = controlPoints.front();
                        end = controlPoints.back();
                        SplineData data(static_cast<int>(source.degree), false,
                            ESplineType::eControlPoints);
                        data.setControlPoints(controlPoints);
                        data.setKnots(std::vector<double>(source.knots.data,
                            source.knots.data + source.knots.count));
                        auto spline = std::make_unique<DmSpline>(
                            boundary.get(), data);
                        if (!spline->isValid())
                        {
                            return instance->setImportError(
                                YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                                "填充样条边无效");
                        }
                        edge = std::move(spline);
                        break;
                    }
                    default:
                        return instance->setImportError(
                            YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
                            "填充边类型无效");
                    }
                    endpoints.emplace_back(start, end);
                    boundary->addEntity(edge.get());
                    edge.release();
                }
                for (uint32_t index = 0; index < endpoints.size(); ++index)
                {
                    const auto next = (index + 1) % endpoints.size();
                    if (endpoints[index].second.distanceTo(
                            endpoints[next].first) > 1.0e-5)
                    {
                        return instance->setImportError(
                            YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                            "填充边界环未闭合或边顺序不连续");
                    }
                }
            }
            builtLoops.push_back(
                {boundary, loop.role, loop.outerLoopIndex});
        }

        if (outerIndexes.empty())
        {
            return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                "填充至少需要一个外环");
        }

        std::vector<std::unique_ptr<DmHatch>> hatches;
        hatches.reserve(outerIndexes.size());
        const auto effectivePattern = patternName.isEmpty() && input->solid != 0
            ? QStringLiteral("SOLID") : patternName;
        for (const auto outerIndex : outerIndexes)
        {
            std::vector<DmEntityContainerPtr> holes;
            for (const auto& loop : builtLoops)
            {
                if (loop.role == YICAD_HATCH_LOOP_HOLE &&
                    loop.outerLoopIndex == outerIndex)
                {
                    holes.push_back(loop.boundary);
                }
            }
            auto region = std::make_shared<DmRegion>(nullptr,
                RegionData(builtLoops[outerIndex].boundary, holes));
            HatchData hatchData(input->solid != 0, input->patternScale,
                input->patternAngle, effectivePattern.toStdWString());
            hatchData.setBoundary(region);
            auto hatch = std::make_unique<DmHatch>(nullptr, hatchData);
            auto result = instance->applyImportEntityAttributes(
                session, input->attributes, hatch.get());
            if (result != YICAD_IMPORT_SUCCESS)
            {
                return result;
            }
            hatch->update();
            hatches.push_back(std::move(hatch));
        }

        auto* table = targetContainer->modelSpace
            ? session->document->getEntityTable()
            : &targetContainer->block->getEntityTable();
        for (auto& hatch : hatches)
        {
            table->add(hatch.get());
            hatch.release();
        }
        instance->clearImportError();
        return YICAD_IMPORT_SUCCESS;
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建填充实体时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建填充实体失败");
    }
}

YiCadImportResult YICAD_PLUGIN_CALL HostApi::createImage(
    YiCadImportSessionHandle sessionHandle,
    YiCadImportContainerHandle container,
    const YiCadImageDataV3* input) noexcept
{
    auto* instance = activeInstance();
    auto* session = instance == nullptr
        ? nullptr
        : instance->resolveImportSession(sessionHandle);
    constexpr auto requiredSize = offsetof(YiCadImageDataV3, clipBoundary) +
        sizeof(((YiCadImageDataV3*)0)->clipBoundary);
    if (session == nullptr)
    {
        return instance == nullptr ? YICAD_IMPORT_ERROR_INVALID_HANDLE
            : instance->setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                  "导入会话句柄无效或已过期");
    }
    QString path;
    if (input == nullptr ||
        !validStructSize(input->structSize, requiredSize) ||
        !copyStringView(input->path, path) || path.isEmpty() ||
        !finitePoint(input->insertionPoint) ||
        !finitePoint(input->uVector) || !finitePoint(input->vVector) ||
        !finitePoint(input->size) || input->size.x <= 0.0 ||
        input->size.y <= 0.0 ||
        toDmVector(input->uVector).magnitude() <= DM_TOLERANCE ||
        toDmVector(input->vVector).magnitude() <= DM_TOLERANCE ||
        std::abs(input->uVector.x * input->vVector.y -
            input->uVector.y * input->vVector.x) <= DM_TOLERANCE ||
        input->brightness < 0 || input->brightness > 100 ||
        input->contrast < 0 || input->contrast > 100 ||
        input->fade < 0 || input->fade > 100 ||
        (input->clipBoundary.data == nullptr &&
         input->clipBoundary.count != 0) ||
        input->clipBoundary.count > 1000000)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
            "图像路径、方向、尺寸或显示参数无效");
    }
    if (input->clipBoundary.count != 0)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_UNSUPPORTED,
            "YiCAD 当前图像模型不支持裁剪边界");
    }
    try
    {
        const bool missing = !QFileInfo(path).isFile();
        auto result = instance->addImportEntity(session, container,
            input->attributes,
            new DmImage(nullptr, ImageData(0,
                toDmVector(input->insertionPoint),
                toDmVector(input->uVector), toDmVector(input->vVector),
                toDmVector(input->size), path.toStdString(),
                input->brightness, input->contrast, input->fade)));
        if (result == YICAD_IMPORT_SUCCESS && missing)
        {
            instance->setImportError(YICAD_IMPORT_SUCCESS,
                "外部图片文件不存在，已保留原始引用路径");
        }
        return result;
    }
    catch (const std::bad_alloc&)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "创建图像实体时内存不足");
    }
    catch (...)
    {
        return instance->setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "创建图像实体失败");
    }
}
#endif

YiCadDocumentHandle HostApi::handleForDocument(DmDocument* document)
{
    for (const auto& record : m_documentHandles)
    {
        if (record->document == document)
        {
            return static_cast<void*>(record.get());
        }
    }

    auto record = std::make_unique<DocumentHandleRecord>();
    record->document = document;
    auto* handle = record.get();
    m_documentHandles.push_back(std::move(record));
    return static_cast<void*>(handle);
}

DmDocument* HostApi::resolveDocument(
    YiCadDocumentHandle handle,
    GuiDocumentView** view) const noexcept
{
    if (view != nullptr)
    {
        *view = nullptr;
    }
    if (handle == nullptr)
    {
        return nullptr;
    }

    for (const auto& record : m_documentHandles)
    {
        if (static_cast<const void*>(record.get()) != handle)
        {
            continue;
        }

        auto* document = record->document;
        if (document == nullptr || !m_context.isDocumentOpen(document))
        {
            return nullptr;
        }
        if (view == nullptr)
        {
            return document;
        }

        auto* documentView = m_context.documentView(document);
        if (documentView == nullptr || documentView->getDocument() != document)
        {
            return nullptr;
        }
        *view = documentView;
        return document;
    }
    return nullptr;
}

HostApi::TransactionRecord* HostApi::resolveTransaction(
    YiCadTransactionHandle handle) const noexcept
{
    if (handle == nullptr)
    {
        return nullptr;
    }
    for (const auto& record : m_transactions)
    {
        if (static_cast<const void*>(record.get()) == handle)
        {
            return record.get();
        }
    }
    return nullptr;
}

HostApi::EntityIteratorRecord* HostApi::resolveEntityIterator(
    YiCadEntityIteratorHandle handle) const noexcept
{
    if (handle == nullptr)
    {
        return nullptr;
    }
    for (const auto& record : m_entityIterators)
    {
        if (static_cast<const void*>(record.get()) == handle)
        {
            return record.get();
        }
    }
    return nullptr;
}

bool HostApi::hasActiveTransaction(
    const DmDocument* document) const noexcept
{
    for (const auto& record : m_transactions)
    {
        if (record->document == document)
        {
            return true;
        }
    }
#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
    if (hasActiveImportSession(document))
    {
        return true;
    }
#endif
    return false;
}

#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
HostApi::ImportSessionRecord* HostApi::resolveImportSession(
    YiCadImportSessionHandle handle) const noexcept
{
    if (handle == nullptr)
    {
        return nullptr;
    }
    for (const auto& record : m_importSessions)
    {
        if (record->active && static_cast<const void*>(record.get()) == handle)
        {
            return record.get();
        }
    }
    return nullptr;
}

bool HostApi::hasActiveImportSession(
    const DmDocument* document) const noexcept
{
    for (const auto& record : m_importSessions)
    {
        if (record->active && record->document == document)
        {
            return true;
        }
    }
    return false;
}

void HostApi::invalidateClosedImportSessions() noexcept
{
    for (auto& record : m_importSessions)
    {
        if (record->active && !m_context.isDocumentOpen(record->document))
        {
            releaseImportSession(record.get());
        }
    }
}

void HostApi::releaseImportSession(ImportSessionRecord* record) noexcept
{
    if (record == nullptr)
    {
        return;
    }
    record->active = false;
    record->modelSpace.active = false;
    record->modelSpace.document = nullptr;
    for (auto& container : record->containers)
    {
        container->active = false;
        container->document = nullptr;
        container->block = nullptr;
    }
    for (auto& resource : record->resources)
    {
        resource->active = false;
        resource->object = nullptr;
    }
    record->transaction.reset();
    record->command = nullptr;
    record->document = nullptr;
}

void* HostApi::resolveImportResource(
    ImportSessionRecord* session,
    YiCadImportResourceHandle handle,
    int expectedKind) const noexcept
{
    if (session == nullptr || !session->active || handle == nullptr)
    {
        return nullptr;
    }
    for (const auto& resource : session->resources)
    {
        if (resource->active && resource->kind == expectedKind &&
            static_cast<const void*>(resource.get()) == handle)
        {
            return resource->object;
        }
    }
    return nullptr;
}

YiCadImportResourceHandle HostApi::registerImportResource(
    ImportSessionRecord* session,
    void* object,
    int kind)
{
    if (session == nullptr || !session->active || object == nullptr)
    {
        return nullptr;
    }
    auto resource = std::make_unique<ImportSessionRecord::ResourceRecord>();
    resource->object = object;
    resource->kind = kind;
    resource->active = true;
    auto* handle = resource.get();
    session->resources.push_back(std::move(resource));
    return static_cast<void*>(handle);
}

void* HostApi::resolveImportContainer(
    ImportSessionRecord* session,
    YiCadImportContainerHandle handle) const noexcept
{
    if (session == nullptr || !session->active || handle == nullptr)
    {
        return nullptr;
    }
    if (session->modelSpace.active && session->modelSpace.modelSpace &&
        session->modelSpace.document == session->document &&
        static_cast<const void*>(&session->modelSpace) == handle)
    {
        return &session->modelSpace;
    }
    for (const auto& container : session->containers)
    {
        if (container->active && !container->modelSpace &&
            container->document == session->document &&
            container->block != nullptr &&
            static_cast<const void*>(container.get()) == handle)
        {
            return container.get();
        }
    }
    return nullptr;
}

YiCadImportResult HostApi::applyImportEntityAttributes(
    ImportSessionRecord* session,
    const YiCadEntityAttributes& attributes,
    DmEntity* entity) noexcept
{
    if (session == nullptr || entity == nullptr ||
        !validStructSize(attributes.structSize, sizeof(YiCadEntityAttributes)) ||
        !validLineWidth(attributes.lineWidth) ||
        !std::isfinite(attributes.lineTypeScale) || attributes.visible > 1 ||
        !finitePoint(attributes.normal))
    {
        return setImportError(YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
            "实体公共属性包含无效字段");
    }

    const auto normal = toDmVector(attributes.normal);
    if (std::abs(attributes.lineTypeScale - 1.0) > DM_TOLERANCE ||
        std::abs(normal.x) > DM_TOLERANCE ||
        std::abs(normal.y) > DM_TOLERANCE || normal.z <= DM_TOLERANCE)
    {
        return setImportError(YICAD_IMPORT_ERROR_UNSUPPORTED,
            "二维实体仅支持线型比例 1 和正 Z 轴法向量");
    }

    DmColor color;
    if (!toDmColor(attributes.color, color))
    {
        return setImportError(YICAD_IMPORT_ERROR_INVALID_ARGUMENT,
            "实体颜色字段无效");
    }

    auto* layer = attributes.layer == nullptr
        ? session->document->getLayerTable()->getActive()
        : static_cast<DmLayer*>(resolveImportResource(
              session, attributes.layer, ImportResourceLayer));
    auto* lineType = attributes.lineType == nullptr
        ? DmLineTypeTable::ByLayer
        : static_cast<DmLineType*>(resolveImportResource(
              session, attributes.lineType, ImportResourceLineType));
    if (layer == nullptr || lineType == nullptr)
    {
        return setImportError(YICAD_IMPORT_ERROR_RESOURCE_NOT_FOUND,
            "实体引用的图层或线型句柄无效");
    }

    entity->setDocument(session->document);
    entity->setLayer(layer);
    entity->setPen(DmPen(color,
        static_cast<DM::LineWidth>(attributes.lineWidth), lineType));
    entity->setVisible(attributes.visible != 0);
    return YICAD_IMPORT_SUCCESS;
}

YiCadImportResult HostApi::addImportEntity(
    ImportSessionRecord* session,
    YiCadImportContainerHandle container,
    const YiCadEntityAttributes& attributes,
    DmEntity* rawEntity) noexcept
{
    std::unique_ptr<DmEntity> entity(rawEntity);
    auto* containerRecord = static_cast<ImportSessionRecord::ContainerRecord*>(
        resolveImportContainer(session, container));
    if (session == nullptr || entity == nullptr || containerRecord == nullptr)
    {
        return setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
            "导入容器句柄无效或已过期");
    }
    try
    {
        const auto attributeResult = applyImportEntityAttributes(
            session, attributes, entity.get());
        if (attributeResult != YICAD_IMPORT_SUCCESS)
        {
            return attributeResult;
        }
        entity->update();
        auto* table = containerRecord->modelSpace
            ? session->document->getEntityTable()
            : &containerRecord->block->getEntityTable();
        table->add(entity.get());
        entity.release();
        clearImportError();
        return YICAD_IMPORT_SUCCESS;
    }
    catch (const std::bad_alloc&)
    {
        return setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "添加几何实体时内存不足");
    }
    catch (...)
    {
        return setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "添加几何实体到导入事务失败");
    }
}

YiCadImportResult HostApi::validateImportBlocks(
    ImportSessionRecord* session) noexcept
{
    if (session == nullptr || !session->active)
    {
        return setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
            "导入会话句柄无效或已过期");
    }
    for (const auto& container : session->containers)
    {
        if (container->active)
        {
            return setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                "提交导入前必须结束所有块定义容器");
        }
    }

    try
    {
        std::unordered_map<const DmBlock*, int> states;
        std::function<YiCadImportResult(const DmBlock*)> visit =
            [&](const DmBlock* block) -> YiCadImportResult {
            const auto state = states[block];
            if (state == 1)
            {
                return setImportError(YICAD_IMPORT_ERROR_OUT_OF_RANGE,
                    "块定义存在递归引用");
            }
            if (state == 2)
            {
                return YICAD_IMPORT_SUCCESS;
            }
            states[block] = 1;
            for (auto* entity : block->getEntityTable())
            {
                if (entity->getEntityType() != DM::EntityBlockReference)
                {
                    continue;
                }
                auto* reference = static_cast<DmBlockReference*>(entity);
                auto* target = reference->getBlockForInsert();
                if (target == nullptr)
                {
                    return setImportError(
                        YICAD_IMPORT_ERROR_RESOURCE_NOT_FOUND,
                        "块定义包含悬空块引用");
                }
                const auto result = visit(target);
                if (result != YICAD_IMPORT_SUCCESS)
                {
                    return result;
                }
            }
            states[block] = 2;
            return YICAD_IMPORT_SUCCESS;
        };

        for (const auto& resource : session->resources)
        {
            if (!resource->active || resource->kind != ImportResourceBlock)
            {
                continue;
            }
            if (!resource->finalized)
            {
                return setImportError(YICAD_IMPORT_ERROR_INVALID_HANDLE,
                    "提交导入前必须完成所有块定义");
            }
            const auto result = visit(static_cast<DmBlock*>(resource->object));
            if (result != YICAD_IMPORT_SUCCESS)
            {
                return result;
            }
        }
        return YICAD_IMPORT_SUCCESS;
    }
    catch (const std::bad_alloc&)
    {
        return setImportError(YICAD_IMPORT_ERROR_OUT_OF_MEMORY,
            "校验块引用关系时内存不足");
    }
    catch (...)
    {
        return setImportError(YICAD_IMPORT_ERROR_TRANSACTION_FAILED,
            "校验块引用关系失败");
    }
}

YiCadImportResult HostApi::setImportError(
    YiCadImportResult result,
    const char* message) noexcept
{
    try
    {
        m_importLastError = message == nullptr ? "" : message;
    }
    catch (...)
    {
        m_importLastError.clear();
    }
    return result;
}

void HostApi::clearImportError() noexcept
{
    m_importLastError.clear();
}
#endif
