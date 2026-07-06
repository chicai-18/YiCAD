#ifndef PLUGIN_UI_ADAPTER_H
#define PLUGIN_UI_ADAPTER_H

#include <QString>

class PluginRegistry;
class SARibbonBar;
class UIActionHandler;
class UICommandWidget;

/// @brief 将已提交的插件注册记录适配到现有 Ribbon 和命令系统。
/// @note Registry 只保存数据和回调，本类持有的 Qt 对象均归 Ribbon 所有。
/// @note Ribbon、Registry、ActionHandler 和 CommandWidget 必须比本对象生存更久。
class PluginUiAdapter
{
public:
    PluginUiAdapter(
        SARibbonBar& ribbon,
        PluginRegistry& registry,
        UIActionHandler& actionHandler,
        UICommandWidget& commandWidget);
    ~PluginUiAdapter();

    PluginUiAdapter(const PluginUiAdapter&) = delete;
    PluginUiAdapter& operator=(const PluginUiAdapter&) = delete;
    PluginUiAdapter(PluginUiAdapter&&) = delete;
    PluginUiAdapter& operator=(PluginUiAdapter&&) = delete;

    /// @brief 物化尚未创建的按钮，并刷新外部命令自动补全。
    /// @return 所有待物化记录均成功创建时返回 true。
    bool materialize();

private:
    SARibbonBar& m_ribbon;
    PluginRegistry& m_registry;
    UIActionHandler& m_actionHandler;
    UICommandWidget& m_commandWidget;
    int m_materializedRibbonCount = 0;
};

#endif // PLUGIN_UI_ADAPTER_H
