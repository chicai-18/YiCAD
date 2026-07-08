/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/// @file DxfPlugin.cpp
/// @brief DXF 插件生命周期和导入过滤器入口

#include "DxfImporter.h"
#include "YiCadPluginSdk.h"

namespace
{

constexpr auto PluginId = "com.yicad.dxf";
constexpr auto FormatId = "dxf";

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
        return m_host.registerImportFilter(
            PluginId,
            FormatId,
            "DXF Drawing",
            "dxf",
            &DxfPlugin::importFile,
            this);
    }

    void shutdown() noexcept
    {
        m_host = yicad::plugin::Host();
    }

private:
    static YiCadResult YICAD_PLUGIN_CALL importFile(
        YiCadDocumentHandle handle,
        const char* path,
        void* userData) noexcept
    {
        return yicad::plugin::invokeNoexcept<YiCadResult>([&]() {
            auto* self = static_cast<DxfPlugin*>(userData);
            if (self == nullptr || path == nullptr || *path == '\0')
            {
                return YICAD_FAILURE;
            }
            const auto document = self->m_host.document(handle);
            return DxfImporter(document).read(path)
                ? YICAD_SUCCESS
                : YICAD_FAILURE;
        }, YICAD_FAILURE);
    }

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
