/// @file PluginEntityClass.cpp
/// @brief 插件实体类的实现

#include "PluginEntityClass.h"

#include "HostApi.h"
#include "PluginAbiConvert.h"
#include "PluginGi.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{

using namespace plugin_abi;

YiCadByteView viewOf(const std::string& data) noexcept
{
    return {reinterpret_cast<const uint8_t*>(data.data()), static_cast<uint32_t>(data.size())};
}

/// @brief 字节输出：追加到 std::string，超过上限时失败
YiCadResult YICAD_PLUGIN_CALL appendBytes(void* context, const uint8_t* data, uint32_t size) noexcept
{
    try
    {
        auto* out = static_cast<std::string*>(context);
        if (out == nullptr || (data == nullptr && size != 0) ||
            size > PluginEntityClass::kMaxDataSize - out->size())
        {
            return YICAD_FAILURE;
        }
        out->append(reinterpret_cast<const char*>(data), size);
        return YICAD_SUCCESS;
    }
    catch (...)
    {
        return YICAD_FAILURE;
    }
}

/// @brief 点输出：追加到 std::vector<DmVector>，点必须有限
YiCadResult YICAD_PLUGIN_CALL appendPoints(void* context, const YiCadPoint2d* points, uint32_t count) noexcept
{
    try
    {
        auto* out = static_cast<std::vector<DmVector>*>(context);
        if (out == nullptr || (points == nullptr && count != 0) || count > 1000000 - std::min<std::size_t>(out->size(), 1000000))
        {
            return YICAD_FAILURE;
        }
        for (uint32_t i = 0; i < count; ++i)
        {
            if (!finitePoint(points[i]))
            {
                return YICAD_FAILURE;
            }
            out->push_back(toDmVector(points[i]));
        }
        return YICAD_SUCCESS;
    }
    catch (...)
    {
        return YICAD_FAILURE;
    }
}

YiCadByteSink byteSink(std::string& out) noexcept
{
    return {static_cast<uint32_t>(sizeof(YiCadByteSink)), &out, &appendBytes};
}

YiCadPointSink pointSink(std::vector<DmVector>& out) noexcept
{
    return {static_cast<uint32_t>(sizeof(YiCadPointSink)), &out, &appendPoints};
}

/// @brief 调插件：返回值不是成功或抛异常都算失败
template<typename Call>
bool succeeded(Call&& call) noexcept
{
    try
    {
        return call() == YICAD_SUCCESS;
    }
    catch (...)
    {
        return false;
    }
}

uint32_t snapModeOf(DmPluginEntityClass::SnapMode mode) noexcept
{
    switch (mode)
    {
    case DmPluginEntityClass::SnapMode::Endpoint:
        return YICAD_SNAP_ENDPOINT;
    case DmPluginEntityClass::SnapMode::Midpoint:
        return YICAD_SNAP_MIDPOINT;
    case DmPluginEntityClass::SnapMode::Center:
        return YICAD_SNAP_CENTER;
    case DmPluginEntityClass::SnapMode::Nearest:
        return YICAD_SNAP_NEAREST;
    }
    return YICAD_SNAP_NEAREST;
}

} // namespace

PluginEntityClass::PluginEntityClass(const QString& pluginId, const YiCadEntityClassV4& table, HostApi& host)
    : m_pluginId(pluginId)
    , m_host(host)
{
    // 插件给的结构可能比宿主的短：只复制它声明的部分，缺的函数为空
    const std::size_t size = std::min<std::size_t>(table.structSize, sizeof(YiCadEntityClassV4));
    std::memcpy(&m_table, &table, size);
    m_table.structSize = static_cast<uint32_t>(sizeof(YiCadEntityClassV4));
    QString name;
    copyStringView(table.className, name);
    m_name = name;
    m_nameUtf8 = name.toStdString();
    // 类名的视图指向插件的内存，只在登记期间有效；之后用自己的副本
    m_table.className = {m_nameUtf8.data(), static_cast<uint32_t>(m_nameUtf8.size())};
}

PluginEntityClass::~PluginEntityClass()
{
    detach();
}

void PluginEntityClass::detach() noexcept
{
    const std::lock_guard<std::mutex> lock(m_cacheMutex);
    if (!m_alive.load(std::memory_order_acquire))
    {
        return;
    }
    if (m_table.destroyCache != nullptr)
    {
        for (void* cache : m_caches)
        {
            try
            {
                m_table.destroyCache(m_table.userData, cache);
            }
            catch (...)
            {
            }
        }
    }
    m_caches.clear();
    m_alive.store(false, std::memory_order_release);
}

bool PluginEntityClass::threadSafeDraw() const
{
    return (m_table.flags & YICAD_ENTITY_CLASS_THREAD_SAFE_DRAW) != 0;
}

void* PluginEntityClass::createCache(const std::string& data)
{
    if (!alive() || m_table.createCache == nullptr || m_table.destroyCache == nullptr)
    {
        return nullptr;
    }
    void* cache = nullptr;
    if (!succeeded([&] { return m_table.createCache(m_table.userData, viewOf(data), &cache); }) || cache == nullptr)
    {
        return nullptr;
    }
    const std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_caches.insert(cache);
    return cache;
}

void PluginEntityClass::destroyCache(void* cache)
{
    if (cache == nullptr)
    {
        return;
    }
    const std::lock_guard<std::mutex> lock(m_cacheMutex);
    // 不是这个类建的、或已在卸载时收回的缓存不交给插件
    if (!alive() || m_caches.erase(cache) == 0 || m_table.destroyCache == nullptr)
    {
        return;
    }
    try
    {
        m_table.destroyCache(m_table.userData, cache);
    }
    catch (...)
    {
    }
}

bool PluginEntityClass::worldDraw(const std::string& data, void* cache, IGiWorldDraw& wd,
                                  const DmEntity* entity) const
{
    if (!alive())
    {
        return false;
    }
    PluginGiContext context(wd, entity, m_nameUtf8.c_str());
    return succeeded([&] {
        return m_table.worldDraw(m_table.userData, viewOf(data), cache, &pluginGiApi(), &context);
    });
}

bool PluginEntityClass::extents(const std::string& data, void* cache, DmVector& minCorner, DmVector& maxCorner) const
{
    YiCadExtents2d value{};
    if (!alive() || !succeeded([&] { return m_table.getExtents(m_table.userData, viewOf(data), cache, &value); }) ||
        !finitePoint(value.minPoint) || !finitePoint(value.maxPoint) || value.minPoint.x > value.maxPoint.x ||
        value.minPoint.y > value.maxPoint.y)
    {
        return false;
    }
    minCorner = toDmVector(value.minPoint);
    maxCorner = toDmVector(value.maxPoint);
    return true;
}

bool PluginEntityClass::transform(const std::string& data, void* cache, const GiTransform& transform,
                                  std::string& out) const
{
    out.clear();
    const YiCadMatrix2d matrix = toAbiMatrix(transform);
    const YiCadByteSink sink = byteSink(out);
    return alive() &&
           succeeded([&] { return m_table.transform(m_table.userData, viewOf(data), cache, &matrix, &sink); }) &&
           !out.empty();
}

bool PluginEntityClass::grips(const std::string& data, void* cache, std::vector<DmVector>& out) const
{
    out.clear();
    const YiCadPointSink sink = pointSink(out);
    return alive() && m_table.getGrips != nullptr &&
           succeeded([&] { return m_table.getGrips(m_table.userData, viewOf(data), cache, &sink); });
}

bool PluginEntityClass::moveGrips(const std::string& data, void* cache, std::span<const std::uint32_t> indices,
                                  const DmVector& offset, std::string& out) const
{
    out.clear();
    const YiCadByteSink sink = byteSink(out);
    const YiCadVector2d delta{offset.x, offset.y};
    return alive() && m_table.moveGrips != nullptr && !indices.empty() &&
           succeeded([&] {
               return m_table.moveGrips(m_table.userData, viewOf(data), cache, indices.data(),
                                        static_cast<uint32_t>(indices.size()), delta, &sink);
           }) &&
           !out.empty();
}

bool PluginEntityClass::snapPoints(const std::string& data, void* cache, SnapMode mode, const DmVector& pick,
                                   std::vector<DmVector>& out) const
{
    out.clear();
    const YiCadPointSink sink = pointSink(out);
    const YiCadPoint2d point{pick.x, pick.y};
    return alive() && m_table.getSnapPoints != nullptr &&
           succeeded([&] {
               return m_table.getSnapPoints(m_table.userData, viewOf(data), cache, snapModeOf(mode), point, &sink);
           });
}

bool PluginEntityClass::explode(const std::string& data, void* cache, const DmEntity& entity,
                                std::vector<DmEntity*>& out) const
{
    if (!alive() || m_table.explode == nullptr)
    {
        return false;
    }
    return m_host.runExplodeSession(
        entity,
        [&](YiCadImportSessionHandle session, YiCadImportContainerHandle container) -> YiCadResult {
            try
            {
                return m_table.explode(m_table.userData, viewOf(data), cache, session, container);
            }
            catch (...)
            {
                return YICAD_FAILURE;
            }
        },
        out);
}

bool PluginEntityClass::upgrade(std::uint32_t fromVersion, const std::string& data, std::string& out) const
{
    out.clear();
    const YiCadByteSink sink = byteSink(out);
    return alive() && m_table.upgrade != nullptr &&
           succeeded([&] { return m_table.upgrade(m_table.userData, fromVersion, viewOf(data), &sink); }) &&
           !out.empty();
}
