#include "HostApi.h"

#include "DmCircle.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "CmdManager.h"
#include "EntityTable.h"
#include "GuiDocumentView.h"
#include "PluginRegistry.h"
#include "Transaction.h"

#include <QString>

#include <cmath>
#include <algorithm>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

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
    DmDocument* document = nullptr;
    std::unique_ptr<Transaction> transaction;
    MacroCmd* command = nullptr;
    std::size_t initialUndoCount = 0;
    bool active = false;
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
          &HostApi::importGetLastError}
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
    record->transaction.reset();
    record->command = nullptr;
    record->document = nullptr;
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
