/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/// @file DxfPlugin.cpp
/// @brief DXF 插件构建骨架和生命周期入口

#include "YiCadPluginSdk.h"

#include <libdxfrw/libdxfrw.h>

namespace
{

constexpr auto PluginId = "com.yicad.dxf";

class DxfPlugin final
{
public:
    bool initialize(
        const YiCadHostApi* api,
        YiCadPluginApi* plugin) noexcept
    {
        yicad::plugin::Host host(api);
        if (!host || plugin == nullptr)
        {
            return false;
        }

        m_host = host;
        plugin->pluginId = PluginId;
        plugin->pluginName = "YiCAD DXF Plugin";
        plugin->pluginVersion = "1.0.0";
        return true;
    }

    void shutdown() noexcept
    {
        m_host = yicad::plugin::Host();
    }

private:
    yicad::plugin::Host m_host;
};

DxfPlugin g_plugin;

} // namespace

YICAD_PLUGIN_EXPORT uint32_t YICAD_PLUGIN_CALL
yicad_plugin_get_abi_version(void)
{
    return YICAD_PLUGIN_ABI_V3;
}

YICAD_PLUGIN_EXPORT YiCadResult YICAD_PLUGIN_CALL
yicad_plugin_init(const YiCadHostApi* host, YiCadPluginApi* plugin)
{
    return yicad::plugin::invokeNoexcept<YiCadResult>([&]() {
        return g_plugin.initialize(host, plugin)
            ? YICAD_SUCCESS
            : YICAD_FAILURE;
    }, YICAD_FAILURE);
}

YICAD_PLUGIN_EXPORT void YICAD_PLUGIN_CALL
yicad_plugin_shutdown(void)
{
    yicad::plugin::invokeNoexcept([&]() { g_plugin.shutdown(); });
}
