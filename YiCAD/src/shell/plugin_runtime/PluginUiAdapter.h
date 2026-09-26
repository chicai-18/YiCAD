#ifndef PLUGIN_UI_ADAPTER_H
#define PLUGIN_UI_ADAPTER_H

#include <QString>
#include <QStringList>

class PluginRegistry;
class UIRibbonRegistry;

/// @brief 把已提交的插件注册记录接入宿主的命令注册表与 Ribbon 注册表。
///
/// 插件命令以 "pluginId/commandId" 为 ID 注册成即时命令（CommandRegistry），同一字符串也
/// 登记为命令行别名：命令行、keyconfig.xml 与 Ribbon 都按这个 ID 启动，执行前与其它即时命令
/// 一样先结束不可打断的命令。插件声明的 Ribbon 按钮登记进宿主的 UIRibbonRegistry，由
/// UIRibbonManager 与内置类目、扩展一起装配（doc/ARCHITECTURE_EVOLUTION_PLAN.md 7.11 节）。
/// @note registerAll() 必须在 Ribbon 注册表 finalize 之前调用。
/// @note 析构时注销命令；必须先于插件 shutdown 析构，回调地址只在 shutdown 前有效。
class PluginUiAdapter
{
public:
    explicit PluginUiAdapter(PluginRegistry& registry);
    ~PluginUiAdapter();

    PluginUiAdapter(const PluginUiAdapter&) = delete;
    PluginUiAdapter& operator=(const PluginUiAdapter&) = delete;
    PluginUiAdapter(PluginUiAdapter&&) = delete;
    PluginUiAdapter& operator=(PluginUiAdapter&&) = delete;

    /// @brief 把已提交的全部插件命令与 Ribbon 按钮登记进宿主的注册表；只能调用一次。
    /// @param ribbon 宿主的 Ribbon 注册表，尚未 finalize
    /// @return 全部登记成功时返回 true；失败的条目逐条 qWarning 后跳过
    bool registerAll(UIRibbonRegistry& ribbon);

    /// @brief 插件命令在宿主命令注册表里的 ID："pluginId/commandId"
    static QString hostCommandId(const QString& pluginId, const QString& commandId);

private:
    bool registerCommands();
    bool registerRibbonButtons(UIRibbonRegistry& ribbon);

    PluginRegistry& m_registry;
    QStringList m_registeredCommands;   ///< 已登记进 CommandRegistry 的命令 ID，析构时注销
    bool m_registered = false;
};

#endif // PLUGIN_UI_ADAPTER_H
