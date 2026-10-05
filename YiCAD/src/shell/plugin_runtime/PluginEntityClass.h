/// @file PluginEntityClass.h
/// @brief 插件登记的实体类：把 DmPluginEntityClass 的调用转成插件的 C 函数（RENDER_PLAN.md 第 4.8.3 节）

#ifndef PLUGIN_ENTITY_CLASS_H
#define PLUGIN_ENTITY_CLASS_H

#include "YiCadPluginAbi.h"

#include "DmPluginEntity.h"

#include <QString>

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_set>

class HostApi;

/// @brief 插件登记的一个实体类，见文件说明
/// @details 插件的函数表在登记时复制一份；每次调用都隔离插件抛出的异常（失败时返回 false）。
///          插件 shutdown 之前插件运行时调用 detach()：收回这个类建的全部实例缓存，之后 alive() 为假，
///          留在撤销栈、剪贴板里的插件实体不再调用插件（只画记下的 GI 流）
class PluginEntityClass final : public DmPluginEntityClass
{
public:
    /// @param pluginId 登记它的插件
    /// @param table 插件的函数表（已校验）；复制一份
    /// @param host 炸开时开临时导入会话用
    PluginEntityClass(const QString& pluginId, const YiCadEntityClassV4& table, HostApi& host);
    ~PluginEntityClass() override;

    PluginEntityClass(const PluginEntityClass&) = delete;
    PluginEntityClass& operator=(const PluginEntityClass&) = delete;

    /// @brief 登记它的插件
    const QString& pluginId() const { return m_pluginId; }

    /// @brief 插件 shutdown 之前调用：收回全部实例缓存，之后不再调用插件；可重复调用
    void detach() noexcept;

    // DmPluginEntityClass
    QString name() const override { return m_name; }
    std::uint32_t version() const override { return m_table.classVersion; }
    DmProxyFlags proxyFlags() const override { return static_cast<DmProxyFlags>(m_table.proxyFlags); }
    bool threadSafeDraw() const override;
    bool alive() const override { return m_alive.load(std::memory_order_acquire); }

    void* createCache(const std::string& data) override;
    void destroyCache(void* cache) override;

    bool worldDraw(const std::string& data, void* cache, IGiWorldDraw& wd, const DmEntity* entity) const override;
    bool extents(const std::string& data, void* cache, DmVector& minCorner, DmVector& maxCorner) const override;
    bool transform(const std::string& data, void* cache, const GiTransform& transform,
                   std::string& out) const override;

    bool hasGrips() const override { return m_table.getGrips != nullptr && m_table.moveGrips != nullptr; }
    bool grips(const std::string& data, void* cache, std::vector<DmVector>& out) const override;
    bool moveGrips(const std::string& data, void* cache, std::span<const std::uint32_t> indices,
                   const DmVector& offset, std::string& out) const override;

    bool hasSnapPoints() const override { return m_table.getSnapPoints != nullptr; }
    bool snapPoints(const std::string& data, void* cache, SnapMode mode, const DmVector& pick,
                    std::vector<DmVector>& out) const override;

    bool hasExplode() const override { return m_table.explode != nullptr; }
    bool explode(const std::string& data, void* cache, const DmEntity& entity,
                 std::vector<DmEntity*>& out) const override;

    bool hasUpgrade() const override { return m_table.upgrade != nullptr; }
    bool upgrade(std::uint32_t fromVersion, const std::string& data, std::string& out) const override;

    /// @brief 一个实体的数据字节的上限（256 MB）
    static constexpr std::size_t kMaxDataSize = std::size_t(1) << 28;

private:
    QString m_pluginId;
    QString m_name;
    std::string m_nameUtf8;                 ///< 类名，日志用
    YiCadEntityClassV4 m_table{};           ///< 插件的函数表
    HostApi& m_host;
    std::atomic<bool> m_alive{true};
    std::mutex m_cacheMutex;                ///< 保护 m_caches
    std::unordered_set<void*> m_caches;     ///< 这个类建的、还没释放的实例缓存
};

#endif // PLUGIN_ENTITY_CLASS_H
