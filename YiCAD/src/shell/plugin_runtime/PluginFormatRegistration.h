#ifndef PLUGIN_FORMAT_REGISTRATION_H
#define PLUGIN_FORMAT_REGISTRATION_H

#include <vector>

class HostApi;
class PluginManager;
class PluginRegistry;

/// @brief 把已提交的插件导入导出格式登记进 Model 的格式注册表（FilterRegistry），析构时注销。
///
/// 每种格式登记一个工厂，查找时创建 PluginFileIOAdapter。只登记构造时处于活动状态的插件；
/// 插件的活动集合在 PluginManager::loadAll() 与 shutdownAll() 之间不变。
/// @note 在 loadAll() 之后构造；必须先于插件 shutdown 析构，回调地址只在 shutdown 前有效。
class PluginFormatRegistration
{
public:
    PluginFormatRegistration(PluginRegistry& registry, PluginManager& manager, HostApi& hostApi);
    ~PluginFormatRegistration();

    PluginFormatRegistration(const PluginFormatRegistration&) = delete;
    PluginFormatRegistration& operator=(const PluginFormatRegistration&) = delete;
    PluginFormatRegistration(PluginFormatRegistration&&) = delete;
    PluginFormatRegistration& operator=(PluginFormatRegistration&&) = delete;

private:
    std::vector<int> m_ids;  ///< FilterRegistry 的登记号，析构时注销
};

#endif // PLUGIN_FORMAT_REGISTRATION_H
