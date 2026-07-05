/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/// @file DemoPlugin.cpp
/// @brief 演示命令、Ribbon 和 .demo 文件导入导出

#include "YiCadPluginSdk.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

namespace
{

constexpr const char* PluginId = "com.yicad.demo";
constexpr const char* CommandId = "demo.add-line";
constexpr const char* FormatId = "demo";
constexpr uint32_t SupportedAbiVersion = YICAD_PLUGIN_ABI_MAX_VERSION;

std::filesystem::path utf8Path(const char* path)
{
    return std::filesystem::path(
        std::u8string(reinterpret_cast<const char8_t*>(path)));
}

class DemoPlugin
{
public:
    bool initialize(
        const YiCadHostApi* api,
        YiCadPluginApi* plugin) noexcept
    {
        yicad::plugin::Host host(api);
        if (!host || plugin == nullptr ||
            plugin->structSize < YICAD_PLUGIN_API_V1_SIZE ||
            plugin->abiVersion < YICAD_PLUGIN_ABI_MIN_VERSION ||
            plugin->abiVersion > SupportedAbiVersion)
        {
            return false;
        }

        m_host = host;
        plugin->pluginId = PluginId;
        plugin->pluginName = "YiCAD Demo Plugin";
        plugin->pluginVersion = "1.0.0";

        const bool commandRegistered = m_host.registerCommand(
            PluginId,
            CommandId,
            "Add demo line",
            &DemoPlugin::executeCommand,
            this);
        const bool ribbonRegistered = m_host.registerRibbonButton(
            PluginId,
            "Demo",
            "Draw",
            CommandId,
            "");
        const bool importRegistered = m_host.registerImportFilter(
            PluginId,
            FormatId,
            "YiCAD Demo Drawing",
            "demo",
            &DemoPlugin::importFile,
            this);
        const bool exportRegistered = m_host.registerExportFilter(
            PluginId,
            FormatId,
            "YiCAD Demo Drawing",
            "demo",
            &DemoPlugin::exportFile,
            this);
        return commandRegistered && ribbonRegistered && importRegistered &&
               exportRegistered;
    }

    void shutdown() noexcept
    {
        m_host = yicad::plugin::Host();
    }

private:
    static void YICAD_PLUGIN_CALL executeCommand(void* userData) noexcept
    {
        yicad::plugin::invokeNoexcept([&]() {
            auto* plugin = static_cast<DemoPlugin*>(userData);
            if (plugin != nullptr)
            {
                plugin->addDemoLine();
            }
        });
    }

    static YiCadResult YICAD_PLUGIN_CALL importFile(
        YiCadDocumentHandle handle,
        const char* filePath,
        void* userData) noexcept
    {
        return yicad::plugin::invokeNoexcept<YiCadResult>([&]() {
            auto* plugin = static_cast<DemoPlugin*>(userData);
            if (plugin == nullptr || filePath == nullptr ||
                *filePath == '\0')
            {
                return YICAD_FAILURE;
            }

            const auto document = plugin->m_host.document(handle);
            std::ifstream input(
                utf8Path(filePath), std::ios::binary);
            std::string line;
            if (!document || !input || !std::getline(input, line) ||
                line != "YICAD_DEMO_V2")
            {
                return YICAD_FAILURE;
            }

            if (document.supportsImport())
            {
                return plugin->importV3(document, input);
            }
            return plugin->importV2(document, input);
        }, YICAD_FAILURE);
    }

    template<typename AddLine, typename AddCircle>
    static bool readEntities(
        std::ifstream& input,
        AddLine&& addLine,
        AddCircle&& addCircle)
    {
        std::string line;
        while (std::getline(input, line))
        {
            if (line.empty())
            {
                continue;
            }

            std::istringstream fields(line);
            std::string type;
            fields >> type;
            bool added = false;
            if (type == "LINE")
            {
                double x1 = 0.0;
                double y1 = 0.0;
                double x2 = 0.0;
                double y2 = 0.0;
                added = static_cast<bool>(
                    fields >> x1 >> y1 >> x2 >> y2) &&
                    addLine(x1, y1, x2, y2);
            }
            else if (type == "CIRCLE")
            {
                double centerX = 0.0;
                double centerY = 0.0;
                double radius = 0.0;
                added = static_cast<bool>(
                    fields >> centerX >> centerY >> radius) &&
                    addCircle(centerX, centerY, radius);
            }

            std::string trailing;
            if (!added || (fields >> trailing))
            {
                return false;
            }
        }
        return input.eof();
    }

    static YiCadResult importV2(
        const yicad::plugin::Document& document,
        std::ifstream& input)
    {
        auto transaction = document.beginTransaction("Import demo drawing");
        if (!transaction)
        {
            return YICAD_FAILURE;
        }

        const auto imported = readEntities(input,
            [&](double x1, double y1, double x2, double y2) {
                return document.addLine(x1, y1, x2, y2);
            },
            [&](double centerX, double centerY, double radius) {
                return document.addCircle(centerX, centerY, radius);
            });
        if (!imported || !transaction.commit())
        {
            return YICAD_FAILURE;
        }
        return YICAD_SUCCESS;
    }

    static YiCadResult importV3(
        const yicad::plugin::Document& document,
        std::ifstream& input)
    {
        auto session = document.beginImport();
        if (!session)
        {
            return YICAD_FAILURE;
        }

        auto layerData = yicad::plugin::LayerData("Demo Import");
        layerData.setColor(
            {YICAD_COLOR_RGB, 0, 80, 160, 240, 0});

        yicad::plugin::ImportResource layer;
        yicad::plugin::ImportContainer modelSpace;
        if (session.createLayer(layerData, YICAD_RESOURCE_CONFLICT_FAIL,
                layer) != YICAD_IMPORT_SUCCESS ||
            session.modelSpace(modelSpace) != YICAD_IMPORT_SUCCESS)
        {
            return YICAD_FAILURE;
        }

        auto attributes = yicad::plugin::EntityAttributes{};
        attributes.setLayer(layer);
        const auto imported = readEntities(input,
            [&](double x1, double y1, double x2, double y2) {
                return modelSpace.createLine(
                    {x1, y1}, {x2, y2}, attributes) ==
                    YICAD_IMPORT_SUCCESS;
            },
            [&](double centerX, double centerY, double radius) {
                return modelSpace.createCircle(
                    {centerX, centerY}, radius, attributes) ==
                    YICAD_IMPORT_SUCCESS;
            });
        return imported && session.commit() == YICAD_IMPORT_SUCCESS
            ? YICAD_SUCCESS
            : YICAD_FAILURE;
    }

    static YiCadResult YICAD_PLUGIN_CALL exportFile(
        YiCadDocumentHandle handle,
        const char* filePath,
        void* userData) noexcept
    {
        return yicad::plugin::invokeNoexcept<YiCadResult>([&]() {
            auto* plugin = static_cast<DemoPlugin*>(userData);
            if (plugin == nullptr || filePath == nullptr ||
                *filePath == '\0')
            {
                return YICAD_FAILURE;
            }

            const auto document = plugin->m_host.document(handle);
            auto entities = document.entities();
            if (!document || !entities)
            {
                return YICAD_FAILURE;
            }

            std::ofstream output(
                utf8Path(filePath),
                std::ios::binary | std::ios::trunc);
            if (!output)
            {
                return YICAD_FAILURE;
            }
            output << "YICAD_DEMO_V2\n" << std::setprecision(17);
            YiCadEntityType type = YICAD_ENTITY_UNKNOWN;
            while (entities.next(type))
            {
                if (type == YICAD_ENTITY_LINE)
                {
                    YiCadLineData line{};
                    if (!entities.line(line))
                    {
                        return YICAD_FAILURE;
                    }
                    output << "LINE " << line.x1 << ' ' << line.y1 << ' '
                           << line.x2 << ' ' << line.y2 << '\n';
                }
                else if (type == YICAD_ENTITY_CIRCLE)
                {
                    YiCadCircleData circle{};
                    if (!entities.circle(circle))
                    {
                        return YICAD_FAILURE;
                    }
                    output << "CIRCLE " << circle.centerX << ' '
                           << circle.centerY << ' ' << circle.radius << '\n';
                }
            }
            output.flush();
            return output ? YICAD_SUCCESS : YICAD_FAILURE;
        }, YICAD_FAILURE);
    }

    void addDemoLine() noexcept
    {
        const auto document = m_host.currentDocument();
        if (!document)
        {
            m_host.message("No active document.");
            return;
        }

        if (!document.addLine(0.0, 0.0, 100.0, 100.0))
        {
            m_host.message("Could not add the demo line.");
            return;
        }

        document.regen();
        document.zoomAuto();
    }

    yicad::plugin::Host m_host;
};

DemoPlugin g_plugin;

} // namespace

YICAD_PLUGIN_EXPORT uint32_t YICAD_PLUGIN_CALL
yicad_plugin_get_abi_version(void)
{
    return SupportedAbiVersion;
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
