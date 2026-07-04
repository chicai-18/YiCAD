#ifndef YICAD_PLUGIN_SDK_H
#define YICAD_PLUGIN_SDK_H

#include "YiCadPluginAbi.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>

namespace yicad::plugin
{

class Host;
class Document;

/// @brief 在 C ABI 边界内执行插件代码，并把所有异常转换为指定失败值。
/// @param callable 不得把引用保存到本次调用之后的插件侧可调用对象。
/// @param failure 捕获异常时返回的值。
/// @return 插件代码的返回值，或捕获异常后的 failure。
/// @note 插件的导出入口和回调应使用本函数，禁止异常穿过 C ABI。
template<typename Result, typename Callable>
Result invokeNoexcept(Callable&& callable, Result failure) noexcept
{
    try
    {
        return static_cast<Result>(
            std::invoke(std::forward<Callable>(callable)));
    }
    catch (...)
    {
        return failure;
    }
}

/// @brief 在无返回值 C ABI 边界内执行插件代码并吞掉所有异常。
/// @param callable 不得把引用保存到本次调用之后的插件侧可调用对象。
template<typename Callable>
void invokeNoexcept(Callable&& callable) noexcept
{
    try
    {
        std::invoke(std::forward<Callable>(callable));
    }
    catch (...)
    {
    }
}

#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
class ImportSession;
class ImportContainer;
class ImportResource;

namespace detail
{

/// @brief 判断导入子表字段是否同时满足版本和可访问字节数要求。
inline bool hasImportField(
    const YiCadImportApi* api,
    std::size_t offset,
    std::size_t size) noexcept
{
    constexpr auto headerSize =
        offsetof(YiCadImportApi, abiVersion) + sizeof(uint32_t);
    return api != nullptr && api->structSize >= headerSize &&
           api->abiVersion >= YICAD_PLUGIN_ABI_V3_DRAFT &&
           api->structSize >= offset && size <= api->structSize - offset;
}

struct ImportState
{
    const YiCadImportApi* api = nullptr;
    YiCadImportSessionHandle session = nullptr;
};

template<typename Callable>
YiCadImportResult callImport(Callable&& callable) noexcept
{
    return invokeNoexcept<YiCadImportResult>(
        std::forward<Callable>(callable),
        YICAD_IMPORT_ERROR_TRANSACTION_FAILED);
}

} // namespace detail

#define YICAD_SDK_HAS_IMPORT_FUNCTION(api, field)                         \
    (::yicad::plugin::detail::hasImportField(                            \
         (api),                                                          \
         offsetof(YiCadImportApi, field),                                \
         sizeof(((YiCadImportApi*)nullptr)->field)) &&                   \
     (api)->field != nullptr)

/**
 * @brief 导入会话内的非拥有资源句柄包装。
 * @note 资源由宿主持有，只在创建它的活动会话内有效；插件不得释放或跨回调缓存。
 */
class ImportResource
{
public:
    ImportResource() noexcept = default;

    explicit operator bool() const noexcept
    {
        return nativeHandle() != nullptr;
    }

    /// @brief 返回仅供填充 ABI POD 输入结构使用的句柄。
    /// @return 会话已结束或资源为空时返回 nullptr。
    YiCadImportResourceHandle nativeHandle() const noexcept
    {
        return m_state != nullptr && m_state->session != nullptr
            ? m_handle
            : nullptr;
    }

private:
    friend class ImportSession;
    friend class ImportContainer;

    ImportResource(
        std::shared_ptr<detail::ImportState> state,
        YiCadImportResourceHandle handle) noexcept
        : m_state(std::move(state)),
          m_handle(handle)
    {
    }

    std::shared_ptr<detail::ImportState> m_state;
    YiCadImportResourceHandle m_handle = nullptr;
};

/**
 * @brief 导入会话内的非拥有模型空间或块定义容器包装。
 * @note 容器只在所属会话内有效；块容器在 endBlock 成功后立即失效。
 * @note 创建函数返回宿主的确定结果码；无效包装返回 INVALID_HANDLE，截短子表或
 * 空函数指针返回 UNSUPPORTED。输入字符串和数组只需保持到该函数返回。
 */
class ImportContainer
{
public:
    ImportContainer() noexcept = default;

    explicit operator bool() const noexcept
    {
        return m_state != nullptr && m_state->session != nullptr &&
               m_handle != nullptr;
    }

    /// @brief 向容器添加点实体。
    YiCadImportResult createPoint(const YiCadPointDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createPoint),
            sizeof(((YiCadImportApi*)nullptr)->createPoint),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createPoint)
                ? m_state->api->createPoint
                : nullptr);
    }

    /// @brief 向容器添加线段实体。
    YiCadImportResult createLine(const YiCadLineDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createLine),
            sizeof(((YiCadImportApi*)nullptr)->createLine),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createLine)
                ? m_state->api->createLine
                : nullptr);
    }

    /// @brief 向容器添加射线实体。
    YiCadImportResult createRay(const YiCadRayDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createRay),
            sizeof(((YiCadImportApi*)nullptr)->createRay),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createRay)
                ? m_state->api->createRay
                : nullptr);
    }

    /// @brief 向容器添加无限长线实体。
    YiCadImportResult createXLine(const YiCadXLineDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createXLine),
            sizeof(((YiCadImportApi*)nullptr)->createXLine),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createXLine)
                ? m_state->api->createXLine
                : nullptr);
    }

    /// @brief 向容器添加圆弧实体。
    YiCadImportResult createArc(const YiCadArcDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createArc),
            sizeof(((YiCadImportApi*)nullptr)->createArc),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createArc)
                ? m_state->api->createArc
                : nullptr);
    }

    /// @brief 向容器添加圆实体。
    YiCadImportResult createCircle(const YiCadCircleDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createCircle),
            sizeof(((YiCadImportApi*)nullptr)->createCircle),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createCircle)
                ? m_state->api->createCircle
                : nullptr);
    }

    /// @brief 向容器添加椭圆或椭圆弧实体。
    YiCadImportResult createEllipse(
        const YiCadEllipseDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createEllipse),
            sizeof(((YiCadImportApi*)nullptr)->createEllipse),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createEllipse)
                ? m_state->api->createEllipse
                : nullptr);
    }

    /// @brief 向容器添加二维多段线实体。
    YiCadImportResult createPolyline(
        const YiCadPolylineDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createPolyline),
            sizeof(((YiCadImportApi*)nullptr)->createPolyline),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createPolyline)
                ? m_state->api->createPolyline
                : nullptr);
    }

    /// @brief 向容器添加非有理、非周期样条实体。
    YiCadImportResult createSpline(
        const YiCadSplineDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createSpline),
            sizeof(((YiCadImportApi*)nullptr)->createSpline),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createSpline)
                ? m_state->api->createSpline
                : nullptr);
    }

    /// @brief 向容器添加单行文字实体。
    YiCadImportResult createText(const YiCadTextDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createText),
            sizeof(((YiCadImportApi*)nullptr)->createText),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createText)
                ? m_state->api->createText
                : nullptr);
    }

    /// @brief 向容器添加多行文字实体。
    YiCadImportResult createMText(const YiCadMTextDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createMText),
            sizeof(((YiCadImportApi*)nullptr)->createMText),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createMText)
                ? m_state->api->createMText
                : nullptr);
    }

    /// @brief 向容器添加属性定义实体。
    YiCadImportResult createAttributeDefinition(
        const YiCadAttributeDefinitionDataV3& data) const noexcept
    {
        return createEntity(data,
            offsetof(YiCadImportApi, createAttributeDefinition),
            sizeof(((YiCadImportApi*)nullptr)->createAttributeDefinition),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createAttributeDefinition)
                ? m_state->api->createAttributeDefinition
                : nullptr);
    }

    /// @brief 向容器添加块引用属性实体。
    YiCadImportResult createAttribute(
        const YiCadAttributeDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createAttribute),
            sizeof(((YiCadImportApi*)nullptr)->createAttribute),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createAttribute)
                ? m_state->api->createAttribute
                : nullptr);
    }

    /// @brief 向容器添加语义标注实体。
    YiCadImportResult createDimension(
        const YiCadDimensionDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createDimension),
            sizeof(((YiCadImportApi*)nullptr)->createDimension),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createDimension)
                ? m_state->api->createDimension
                : nullptr);
    }

    /// @brief 向容器添加引线实体。
    YiCadImportResult createLeader(
        const YiCadLeaderDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createLeader),
            sizeof(((YiCadImportApi*)nullptr)->createLeader),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createLeader)
                ? m_state->api->createLeader
                : nullptr);
    }

    /// @brief 向容器添加填充实体。
    YiCadImportResult createHatch(const YiCadHatchDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createHatch),
            sizeof(((YiCadImportApi*)nullptr)->createHatch),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createHatch)
                ? m_state->api->createHatch
                : nullptr);
    }

    /// @brief 向容器添加图像引用实体。
    YiCadImportResult createImage(const YiCadImageDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createImage),
            sizeof(((YiCadImportApi*)nullptr)->createImage),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createImage)
                ? m_state->api->createImage
                : nullptr);
    }

    /// @brief 创建块引用，并返回可供属性值引用的非拥有资源包装。
    YiCadImportResult createInsert(
        const YiCadInsertDataV3& data,
        ImportResource& insert) const noexcept
    {
        insert = {};
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        const auto* api = m_state->api;
        if (!YICAD_SDK_HAS_IMPORT_FUNCTION(api, createInsert))
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        YiCadImportResourceHandle handle = nullptr;
        const auto result = detail::callImport([&]() {
            return api->createInsert(
                m_state->session, m_handle, &data, &handle);
        });
        if (result == YICAD_IMPORT_SUCCESS && handle != nullptr)
        {
            insert = ImportResource(m_state, handle);
        }
        return result;
    }

    /// @brief 结束块定义并消费当前块容器；模型空间容器不支持此操作。
    YiCadImportResult endBlock() noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        const auto* api = m_state->api;
        if (!YICAD_SDK_HAS_IMPORT_FUNCTION(api, endBlock))
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        const auto handle = m_handle;
        const auto result = detail::callImport(
            [&]() { return api->endBlock(m_state->session, handle); });
        if (result == YICAD_IMPORT_SUCCESS)
        {
            m_handle = nullptr;
        }
        return result;
    }

private:
    friend class ImportSession;

    ImportContainer(
        std::shared_ptr<detail::ImportState> state,
        YiCadImportContainerHandle handle) noexcept
        : m_state(std::move(state)),
          m_handle(handle)
    {
    }

    template<typename Data, typename Function>
    YiCadImportResult createEntity(
        const Data& data,
        std::size_t,
        std::size_t,
        Function function) const noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        if (function == nullptr)
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        return detail::callImport([&]() {
            return function(m_state->session, m_handle, &data);
        });
    }

    std::shared_ptr<detail::ImportState> m_state;
    YiCadImportContainerHandle m_handle = nullptr;
};

/**
 * @brief 不可复制、可移动的导入会话；析构时自动回滚未结束会话。
 * @note 仅允许在 YiCAD UI 主线程和创建该会话的文件导入回调内使用。
 * @note 包装方法返回宿主的确定结果码；无效包装返回 INVALID_HANDLE，截短子表或
 * 空函数指针返回 UNSUPPORTED。commit 和 rollback 都会消费会话及全部子句柄。
 */
class ImportSession
{
public:
    ImportSession() noexcept = default;
    ImportSession(const ImportSession&) = delete;
    ImportSession& operator=(const ImportSession&) = delete;
    ImportSession(ImportSession&&) noexcept = default;

    ImportSession& operator=(ImportSession&& other) noexcept
    {
        if (this != &other)
        {
            rollback();
            m_state = std::move(other.m_state);
        }
        return *this;
    }

    ~ImportSession()
    {
        rollback();
    }

    explicit operator bool() const noexcept
    {
        return m_state != nullptr && m_state->api != nullptr &&
               m_state->session != nullptr;
    }

    /// @brief 提交并消费会话句柄；失败时宿主自动回滚。
    YiCadImportResult commit() noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        const auto* api = m_state->api;
        if (!YICAD_SDK_HAS_IMPORT_FUNCTION(api, commitImport))
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        const auto handle = std::exchange(m_state->session, nullptr);
        return detail::callImport([&]() { return api->commitImport(handle); });
    }

    /// @brief 回滚并消费会话句柄。
    YiCadImportResult rollback() noexcept
    {
        if (m_state == nullptr || m_state->session == nullptr)
        {
            return YICAD_IMPORT_SUCCESS;
        }
        const auto* api = m_state->api;
        if (!YICAD_SDK_HAS_IMPORT_FUNCTION(api, rollbackImport))
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        const auto handle = std::exchange(m_state->session, nullptr);
        return detail::callImport(
            [&]() { return api->rollbackImport(handle); });
    }

    /// @brief 读取最后一条导入错误，返回包含 NUL 的所需字节数。
    uint32_t lastError(char* buffer, uint32_t bufferSize) const noexcept
    {
        const auto* api = m_state != nullptr ? m_state->api : nullptr;
        if (!YICAD_SDK_HAS_IMPORT_FUNCTION(api, getLastError))
        {
            if (buffer != nullptr && bufferSize > 0)
            {
                buffer[0] = '\0';
            }
            return 1;
        }
        return invokeNoexcept<uint32_t>(
            [&]() { return api->getLastError(buffer, bufferSize); }, 1);
    }

    /// @brief 设置文档级导入元数据；输入字符串在返回前由宿主复制。
    YiCadImportResult setDocumentSettings(
        const YiCadDocumentSettings& settings) const noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        const auto* api = m_state->api;
        if (!YICAD_SDK_HAS_IMPORT_FUNCTION(api, setDocumentSettings))
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        return detail::callImport(
            [&]() { return api->setDocumentSettings(
                m_state->session, &settings); });
    }

    /// @brief 创建或解析简单线型资源。
    YiCadImportResult createLineType(
        const YiCadLineTypeDataV3& data,
        YiCadResourceConflictPolicy conflictPolicy,
        ImportResource& resource) const noexcept
    {
        return createResource(data, conflictPolicy, resource,
            offsetof(YiCadImportApi, createLineType),
            sizeof(((YiCadImportApi*)nullptr)->createLineType),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createLineType)
                ? m_state->api->createLineType
                : nullptr);
    }

    /// @brief 创建或解析图层资源。
    YiCadImportResult createLayer(
        const YiCadLayerDataV3& data,
        YiCadResourceConflictPolicy conflictPolicy,
        ImportResource& resource) const noexcept
    {
        return createResource(data, conflictPolicy, resource,
            offsetof(YiCadImportApi, createLayer),
            sizeof(((YiCadImportApi*)nullptr)->createLayer),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createLayer)
                ? m_state->api->createLayer
                : nullptr);
    }

    /// @brief 创建或解析文字样式资源。
    YiCadImportResult createTextStyle(
        const YiCadTextStyleDataV3& data,
        YiCadResourceConflictPolicy conflictPolicy,
        ImportResource& resource) const noexcept
    {
        return createResource(data, conflictPolicy, resource,
            offsetof(YiCadImportApi, createTextStyle),
            sizeof(((YiCadImportApi*)nullptr)->createTextStyle),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createTextStyle)
                ? m_state->api->createTextStyle
                : nullptr);
    }

    /// @brief 创建或解析标注样式资源。
    YiCadImportResult createDimensionStyle(
        const YiCadDimensionStyleDataV3& data,
        YiCadResourceConflictPolicy conflictPolicy,
        ImportResource& resource) const noexcept
    {
        return createResource(data, conflictPolicy, resource,
            offsetof(YiCadImportApi, createDimensionStyle),
            sizeof(((YiCadImportApi*)nullptr)->createDimensionStyle),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createDimensionStyle)
                ? m_state->api->createDimensionStyle
                : nullptr);
    }

    /// @brief 获取文档唯一模型空间容器。
    YiCadImportResult modelSpace(ImportContainer& container) const noexcept
    {
        container = {};
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        const auto* api = m_state->api;
        if (!YICAD_SDK_HAS_IMPORT_FUNCTION(api, getModelSpace))
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        YiCadImportContainerHandle handle = nullptr;
        const auto result = detail::callImport([&]() {
            return api->getModelSpace(m_state->session, &handle);
        });
        if (result == YICAD_IMPORT_SUCCESS && handle != nullptr)
        {
            container = ImportContainer(m_state, handle);
        }
        return result;
    }

    /// @brief 开始块定义并返回块资源与活动块容器。
    YiCadImportResult beginBlock(
        const YiCadBlockDataV3& data,
        ImportResource& block,
        ImportContainer& container) const noexcept
    {
        block = {};
        container = {};
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        const auto* api = m_state->api;
        if (!YICAD_SDK_HAS_IMPORT_FUNCTION(api, beginBlock))
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        YiCadImportResourceHandle blockHandle = nullptr;
        YiCadImportContainerHandle containerHandle = nullptr;
        const auto result = detail::callImport([&]() {
            return api->beginBlock(m_state->session, &data,
                &blockHandle, &containerHandle);
        });
        if (result == YICAD_IMPORT_SUCCESS && blockHandle != nullptr &&
            containerHandle != nullptr)
        {
            block = ImportResource(m_state, blockHandle);
            container = ImportContainer(m_state, containerHandle);
        }
        return result;
    }

private:
    friend class Document;

    ImportSession(
        const YiCadImportApi* api,
        YiCadImportSessionHandle handle)
        : m_state(std::make_shared<detail::ImportState>(
              detail::ImportState{api, handle}))
    {
    }

    template<typename Data, typename Function>
    YiCadImportResult createResource(
        const Data& data,
        YiCadResourceConflictPolicy conflictPolicy,
        ImportResource& resource,
        std::size_t,
        std::size_t,
        Function function) const noexcept
    {
        resource = {};
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        if (function == nullptr)
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        YiCadImportResourceHandle handle = nullptr;
        const auto result = detail::callImport([&]() {
            return function(
                m_state->session, &data, conflictPolicy, &handle);
        });
        if (result == YICAD_IMPORT_SUCCESS && handle != nullptr)
        {
            resource = ImportResource(m_state, handle);
        }
        return result;
    }

    std::shared_ptr<detail::ImportState> m_state;
};

#undef YICAD_SDK_HAS_IMPORT_FUNCTION
#endif

/// @brief 宿主持有的不透明文档事务；析构时自动回滚未提交事务。
class DocumentTransaction
{
public:
    DocumentTransaction() noexcept = default;
    DocumentTransaction(const DocumentTransaction&) = delete;
    DocumentTransaction& operator=(const DocumentTransaction&) = delete;

    DocumentTransaction(DocumentTransaction&& other) noexcept
        : m_api(other.m_api),
          m_handle(other.m_handle)
    {
        other.m_api = nullptr;
        other.m_handle = nullptr;
    }

    DocumentTransaction& operator=(DocumentTransaction&& other) noexcept
    {
        if (this != &other)
        {
            if (!rollback())
            {
                return *this;
            }
            m_api = other.m_api;
            m_handle = other.m_handle;
            other.m_api = nullptr;
            other.m_handle = nullptr;
        }
        return *this;
    }

    ~DocumentTransaction()
    {
        rollback();
    }

    explicit operator bool() const noexcept
    {
        return m_api != nullptr && m_handle != nullptr;
    }

    bool commit() noexcept
    {
        if (!m_api || !m_handle || !m_api->documentCommitTransaction ||
            m_api->documentCommitTransaction(m_handle) != YICAD_SUCCESS)
        {
            return false;
        }
        m_handle = nullptr;
        return true;
    }

    bool rollback() noexcept
    {
        if (m_handle == nullptr)
        {
            return true;
        }
        if (m_api == nullptr || !m_api->documentRollbackTransaction ||
            m_api->documentRollbackTransaction(m_handle) != YICAD_SUCCESS)
        {
            return false;
        }
        m_handle = nullptr;
        return true;
    }

private:
    friend class Document;

    DocumentTransaction(
        const YiCadHostApi* api,
        YiCadTransactionHandle handle) noexcept
        : m_api(api),
          m_handle(handle)
    {
    }

    const YiCadHostApi* m_api = nullptr;
    YiCadTransactionHandle m_handle = nullptr;
};

/// @brief 只读实体快照迭代器；析构时将不透明句柄归还宿主。
class EntityIterator
{
public:
    EntityIterator() noexcept = default;
    EntityIterator(const EntityIterator&) = delete;
    EntityIterator& operator=(const EntityIterator&) = delete;

    EntityIterator(EntityIterator&& other) noexcept
        : m_api(other.m_api),
          m_handle(other.m_handle)
    {
        other.m_api = nullptr;
        other.m_handle = nullptr;
    }

    EntityIterator& operator=(EntityIterator&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            m_api = other.m_api;
            m_handle = other.m_handle;
            other.m_api = nullptr;
            other.m_handle = nullptr;
        }
        return *this;
    }

    ~EntityIterator()
    {
        reset();
    }

    explicit operator bool() const noexcept
    {
        return m_api != nullptr && m_handle != nullptr;
    }

    bool next(YiCadEntityType& type) noexcept
    {
        return m_api && m_handle && m_api->entityIteratorNext &&
               m_api->entityIteratorNext(m_handle, &type) == YICAD_SUCCESS;
    }

    bool line(YiCadLineData& data) const noexcept
    {
        return m_api && m_handle && m_api->entityIteratorGetLine &&
               m_api->entityIteratorGetLine(m_handle, &data) ==
                   YICAD_SUCCESS;
    }

    bool circle(YiCadCircleData& data) const noexcept
    {
        return m_api && m_handle && m_api->entityIteratorGetCircle &&
               m_api->entityIteratorGetCircle(m_handle, &data) ==
                   YICAD_SUCCESS;
    }

private:
    friend class Document;

    EntityIterator(
        const YiCadHostApi* api,
        YiCadEntityIteratorHandle handle) noexcept
        : m_api(api),
          m_handle(handle)
    {
    }

    void reset() noexcept
    {
        if (m_api && m_handle && m_api->entityIteratorDestroy)
        {
            m_api->entityIteratorDestroy(m_handle);
        }
        m_handle = nullptr;
    }

    const YiCadHostApi* m_api = nullptr;
    YiCadEntityIteratorHandle m_handle = nullptr;
};

class Document
{
public:
    Document() noexcept = default;

    explicit operator bool() const noexcept
    {
        return hasField(
            offsetof(YiCadHostApi, abiVersion),
            sizeof(m_api->abiVersion));
    }

    bool addLine(
        double x1,
        double y1,
        double x2,
        double y2) const noexcept
    {
        return hasField(
                   offsetof(YiCadHostApi, documentAddLine),
                   sizeof(m_api->documentAddLine)) &&
               m_api->documentAddLine != nullptr &&
               m_api->documentAddLine(m_handle, x1, y1, x2, y2) ==
                   YICAD_SUCCESS;
    }

    bool addCircle(
        double centerX,
        double centerY,
        double radius) const noexcept
    {
        return hasField(
                   offsetof(YiCadHostApi, documentAddCircle),
                   sizeof(m_api->documentAddCircle)) &&
               m_api->documentAddCircle != nullptr &&
               m_api->documentAddCircle(
                   m_handle,
                   centerX,
                   centerY,
                   radius) == YICAD_SUCCESS;
    }

    bool regen() const noexcept
    {
        return hasField(
                   offsetof(YiCadHostApi, documentRegen),
                   sizeof(m_api->documentRegen)) &&
               m_api->documentRegen != nullptr &&
               m_api->documentRegen(m_handle) == YICAD_SUCCESS;
    }

    bool zoomAuto() const noexcept
    {
        return hasField(
                   offsetof(YiCadHostApi, documentZoomAuto),
                   sizeof(m_api->documentZoomAuto)) &&
               m_api->documentZoomAuto != nullptr &&
               m_api->documentZoomAuto(m_handle) == YICAD_SUCCESS;
    }

    /// @brief 开始一个整体可撤销的 ABI v2 文档事务。
    DocumentTransaction beginTransaction(const char* name) const noexcept
    {
        if (name == nullptr || *name == '\0' ||
            !hasV2Field(
                offsetof(YiCadHostApi, documentRollbackTransaction),
                sizeof(m_api->documentRollbackTransaction)) ||
            m_api->documentBeginTransaction == nullptr)
        {
            return {};
        }
        return DocumentTransaction(
            m_api, m_api->documentBeginTransaction(m_handle, name));
    }

    /// @brief 创建与文档后续修改无关的只读实体数据快照。
    EntityIterator entities() const noexcept
    {
        if (!hasV2Field(
                offsetof(YiCadHostApi, entityIteratorDestroy),
                sizeof(m_api->entityIteratorDestroy)) ||
            m_api->documentCreateEntityIterator == nullptr)
        {
            return {};
        }
        return EntityIterator(
            m_api, m_api->documentCreateEntityIterator(m_handle));
    }

#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
    /// @brief 判断当前协商宿主是否提供导入会话基础能力。
    bool supportsImport() const noexcept
    {
        return importApiForSession() != nullptr;
    }

    /// @brief 开始一个 ABI v3 草案导入会话。
    ImportSession beginImport() const noexcept
    {
        const auto* importApi = importApiForSession();
        if (importApi == nullptr)
        {
            return {};
        }

        YiCadImportSessionHandle session = nullptr;
        if (importApi->beginImport(m_handle, &session) !=
            YICAD_IMPORT_SUCCESS)
        {
            return {};
        }
        try
        {
            return ImportSession(importApi, session);
        }
        catch (...)
        {
            invokeNoexcept<YiCadImportResult>(
                [&]() { return importApi->rollbackImport(session); },
                YICAD_IMPORT_ERROR_TRANSACTION_FAILED);
            return {};
        }
    }

    /// @brief 读取最后一条导入错误，返回包含 NUL 的所需字节数。
    uint32_t importLastError(
        char* buffer,
        uint32_t bufferSize) const noexcept
    {
        const auto* importApi = importApiForSession();
        if (importApi == nullptr || importApi->getLastError == nullptr)
        {
            if (buffer != nullptr && bufferSize > 0)
            {
                buffer[0] = '\0';
            }
            return 1;
        }
        return importApi->getLastError(buffer, bufferSize);
    }
#endif

private:
    friend class Host;

    Document(
        const YiCadHostApi* api,
        YiCadDocumentHandle handle) noexcept
        : m_api(api),
          m_handle(handle)
    {
    }

    bool hasField(size_t offset, size_t size) const noexcept
    {
#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
        constexpr auto maximumVersion = YICAD_PLUGIN_ABI_V3_DRAFT;
#else
        constexpr auto maximumVersion = YICAD_PLUGIN_ABI_MAX_VERSION;
#endif
        return m_api != nullptr && m_handle != nullptr &&
               m_api->structSize >=
                   offsetof(YiCadHostApi, abiVersion) +
                       sizeof(m_api->abiVersion) &&
               m_api->abiVersion >= YICAD_PLUGIN_ABI_MIN_VERSION &&
               m_api->abiVersion <= maximumVersion &&
               m_api->structSize >= offset + size;
    }

    bool hasV2Field(size_t offset, size_t size) const noexcept
    {
        return hasField(offset, size) &&
               m_api->abiVersion >= YICAD_PLUGIN_ABI_V2;
    }

#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
    const YiCadImportApi* importApiForSession() const noexcept
    {
        if (!hasField(
                offsetof(YiCadHostApi, importApi),
                sizeof(m_api->importApi)) ||
            m_api->abiVersion < YICAD_PLUGIN_ABI_V3_DRAFT ||
            m_api->importApi == nullptr)
        {
            return nullptr;
        }

        const auto* importApi = m_api->importApi;
        const auto headerSize =
            offsetof(YiCadImportApi, abiVersion) +
            sizeof(importApi->abiVersion);
        const auto requiredSize =
            offsetof(YiCadImportApi, getLastError) +
            sizeof(importApi->getLastError);
        return importApi->structSize >= headerSize &&
               importApi->abiVersion >= YICAD_PLUGIN_ABI_V3_DRAFT &&
               importApi->structSize >= requiredSize &&
               importApi->beginImport != nullptr &&
               importApi->commitImport != nullptr &&
               importApi->rollbackImport != nullptr
            ? importApi
            : nullptr;
    }
#endif

    const YiCadHostApi* m_api = nullptr;
    YiCadDocumentHandle m_handle = nullptr;
};

class Host
{
public:
    explicit Host(const YiCadHostApi* api = nullptr) noexcept
        : m_api(api)
    {
    }

    explicit operator bool() const noexcept
    {
        return isCompatible();
    }

    void message(const char* text) const noexcept
    {
        if (text != nullptr &&
            hasField(
                offsetof(YiCadHostApi, message),
                sizeof(m_api->message)) &&
            m_api->message != nullptr)
        {
            m_api->message(text);
        }
    }

    bool registerCommand(
        const char* pluginId,
        const char* commandId,
        const char* displayName,
        YiCadCommandCallback callback,
        void* userData = nullptr) const noexcept
    {
        return pluginId != nullptr && commandId != nullptr &&
               displayName != nullptr && callback != nullptr &&
               hasField(
                   offsetof(YiCadHostApi, registerCommand),
                   sizeof(m_api->registerCommand)) &&
               m_api->registerCommand != nullptr &&
               m_api->registerCommand(
                   pluginId,
                   commandId,
                   displayName,
                   callback,
                   userData) == YICAD_SUCCESS;
    }

    bool registerRibbonButton(
        const char* pluginId,
        const char* tab,
        const char* group,
        const char* commandId,
        const char* iconPath) const noexcept
    {
        return pluginId != nullptr && tab != nullptr && group != nullptr &&
               commandId != nullptr && iconPath != nullptr &&
               hasField(
                   offsetof(YiCadHostApi, registerRibbonButton),
                   sizeof(m_api->registerRibbonButton)) &&
               m_api->registerRibbonButton != nullptr &&
               m_api->registerRibbonButton(
                   pluginId,
                   tab,
                   group,
                   commandId,
                   iconPath) == YICAD_SUCCESS;
    }

    bool registerImportFilter(
        const char* pluginId,
        const char* formatId,
        const char* displayName,
        const char* extension,
        YiCadImportCallback callback,
        void* userData = nullptr) const noexcept
    {
        return pluginId != nullptr && formatId != nullptr &&
               displayName != nullptr && extension != nullptr &&
               callback != nullptr &&
               hasField(
                   offsetof(YiCadHostApi, registerImportFilter),
                   sizeof(m_api->registerImportFilter)) &&
               m_api->registerImportFilter != nullptr &&
               m_api->registerImportFilter(
                   pluginId,
                   formatId,
                   displayName,
                   extension,
                   callback,
                   userData) == YICAD_SUCCESS;
    }

    bool registerExportFilter(
        const char* pluginId,
        const char* formatId,
        const char* displayName,
        const char* extension,
        YiCadExportCallback callback,
        void* userData = nullptr) const noexcept
    {
        return pluginId != nullptr && formatId != nullptr &&
               displayName != nullptr && extension != nullptr &&
               callback != nullptr &&
               hasField(
                   offsetof(YiCadHostApi, registerExportFilter),
                   sizeof(m_api->registerExportFilter)) &&
               m_api->registerExportFilter != nullptr &&
               m_api->registerExportFilter(
                   pluginId,
                   formatId,
                   displayName,
                   extension,
                   callback,
                   userData) == YICAD_SUCCESS;
    }

    Document currentDocument() const noexcept
    {
        if (!hasField(
                offsetof(YiCadHostApi, currentDocument),
                sizeof(m_api->currentDocument)) ||
            m_api->currentDocument == nullptr)
        {
            return {};
        }

        return Document(m_api, m_api->currentDocument());
    }

    /// @brief 将文件回调收到的文档句柄包装为 SDK 文档对象。
    Document document(YiCadDocumentHandle handle) const noexcept
    {
        return isCompatible() && handle != nullptr
            ? Document(m_api, handle)
            : Document();
    }

private:
    bool isCompatible() const noexcept
    {
#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
        constexpr auto maximumVersion = YICAD_PLUGIN_ABI_V3_DRAFT;
#else
        constexpr auto maximumVersion = YICAD_PLUGIN_ABI_MAX_VERSION;
#endif
        return m_api != nullptr &&
               m_api->structSize >=
                   offsetof(YiCadHostApi, abiVersion) +
                       sizeof(m_api->abiVersion) &&
               m_api->abiVersion >= YICAD_PLUGIN_ABI_MIN_VERSION &&
               m_api->abiVersion <= maximumVersion;
    }

    bool hasField(size_t offset, size_t size) const noexcept
    {
        return isCompatible() && m_api->structSize >= offset + size;
    }

    const YiCadHostApi* m_api = nullptr;
};

} // namespace yicad::plugin

#endif
