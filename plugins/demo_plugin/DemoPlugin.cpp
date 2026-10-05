/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/// @file DemoPlugin.cpp
/// @brief 演示命令、Ribbon、.demo 文件导入导出与自定义实体"管道"

#include "DemoPipe.h"
#include "YiCadPluginSdk.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{

constexpr const char* PluginId = "com.yicad.demo";
constexpr const char* CommandId = "demo.add-line";
constexpr const char* AddPipeCommandId = "demo.add-pipe";
constexpr const char* GrowPipesCommandId = "demo.pipe-grow";
constexpr const char* FormatId = "demo";

std::filesystem::path utf8Path(const char* path)
{
    return std::filesystem::path(
        std::u8string(reinterpret_cast<const char8_t*>(path)));
}

/// @brief 自定义实体是不是可以按管道解码的原实体（代理的数据不归插件改）
bool isPipe(const yicad::plugin::CustomEntityData& entity)
{
    return entity.className() == demo::PipeClass::ClassName && !entity.isProxy() &&
           entity.classVersion() == demo::PipeClass::Version;
}

class DemoPlugin
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
        plugin->pluginName = "YiCAD Demo Plugin";
        plugin->pluginVersion = "1.1.0";

        yicad::plugin::EntityClassInfo pipeInfo;
        pipeInfo.className = demo::PipeClass::ClassName;
        pipeInfo.classVersion = demo::PipeClass::Version;
        // 插件不在时，管道（代理）可以删除、移动旋转缩放镜像、复制，可以改图层与颜色
        pipeInfo.proxyFlags = YICAD_PROXY_ERASE | YICAD_PROXY_TRANSFORM | YICAD_PROXY_CLONING |
                              YICAD_PROXY_LAYER_CHANGE | YICAD_PROXY_COLOR_CHANGE;

        const bool commandRegistered = m_host.registerCommand(
            PluginId,
            CommandId,
            "Add demo line",
            &DemoPlugin::executeCommand,
            this);
        const bool addPipeRegistered = m_host.registerCommand(
            PluginId,
            AddPipeCommandId,
            "Add demo pipe",
            &DemoPlugin::addPipeCommand,
            this);
        const bool growRegistered = m_host.registerCommand(
            PluginId,
            GrowPipesCommandId,
            "Double demo pipe diameters",
            &DemoPlugin::growPipesCommand,
            this);
        const bool ribbonRegistered =
            m_host.registerRibbonButton(PluginId, "Demo", "Draw", CommandId, "") &&
            m_host.registerRibbonButton(PluginId, "Demo", "Draw", AddPipeCommandId, "") &&
            m_host.registerRibbonButton(PluginId, "Demo", "Draw", GrowPipesCommandId, "");
        const bool classRegistered = m_host.registerEntityClass(PluginId, m_pipe, pipeInfo);
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
        return commandRegistered && addPipeRegistered && growRegistered &&
               ribbonRegistered && classRegistered && importRegistered &&
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

    static void YICAD_PLUGIN_CALL addPipeCommand(void* userData) noexcept
    {
        yicad::plugin::invokeNoexcept([&]() {
            auto* plugin = static_cast<DemoPlugin*>(userData);
            if (plugin != nullptr)
            {
                plugin->addDemoPipe();
            }
        });
    }

    static void YICAD_PLUGIN_CALL growPipesCommand(void* userData) noexcept
    {
        yicad::plugin::invokeNoexcept([&]() {
            auto* plugin = static_cast<DemoPlugin*>(userData);
            if (plugin != nullptr)
            {
                plugin->growDemoPipes();
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

            return plugin->importFileContents(document, input);
        }, YICAD_FAILURE);
    }

    template<typename AddLine, typename AddCircle, typename AddPipe>
    static bool readEntities(
        std::ifstream& input,
        AddLine&& addLine,
        AddCircle&& addCircle,
        AddPipe&& addPipe)
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
            else if (type == "PIPE")
            {
                demo::PipeData pipe;
                std::size_t count = 0;
                if (fields >> pipe.diameter >> count && count >= 2 && count <= 100000)
                {
                    pipe.vertices.resize(count);
                    bool ok = true;
                    for (auto& vertex : pipe.vertices)
                    {
                        ok = ok && static_cast<bool>(fields >> vertex.x >> vertex.y);
                    }
                    added = ok && addPipe(pipe);
                }
            }

            std::string trailing;
            if (!added || (fields >> trailing))
            {
                return false;
            }
        }
        return input.eof();
    }

    static YiCadResult importFileContents(
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
            },
            [&](const demo::PipeData& pipe) {
                yicad::plugin::CustomEntityData entity(demo::PipeClass::ClassName,
                    demo::PipeClass::Version, demo::PipeClass::encodeData(pipe));
                entity.setAttributes(attributes);
                return modelSpace.createCustomEntity(entity) == YICAD_IMPORT_SUCCESS;
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
            yicad::plugin::EntityData entity;
            while (entities.next(entity))
            {
                if (const auto* line =
                        std::get_if<yicad::plugin::LineData>(&entity))
                {
                    output << "LINE " << line->startPoint.x << ' '
                           << line->startPoint.y << ' ' << line->endPoint.x
                           << ' ' << line->endPoint.y << '\n';
                }
                else if (const auto* circle =
                             std::get_if<yicad::plugin::CircleData>(&entity))
                {
                    output << "CIRCLE " << circle->center.x << ' '
                           << circle->center.y << ' ' << circle->radius << '\n';
                }
                else if (const auto* custom =
                             std::get_if<yicad::plugin::CustomEntityData>(&entity);
                         custom != nullptr && isPipe(*custom))
                {
                    const auto pipe = demo::PipeClass::decodeData(custom->data());
                    output << "PIPE " << pipe.diameter << ' ' << pipe.vertices.size();
                    for (const auto& vertex : pipe.vertices)
                    {
                        output << ' ' << vertex.x << ' ' << vertex.y;
                    }
                    output << '\n';
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

    /// @brief 在固定位置建一根管道（插件接口没有交互取点）
    void addDemoPipe()
    {
        const auto document = m_host.currentDocument();
        if (!document)
        {
            m_host.message("No active document.");
            return;
        }
        auto transaction = document.beginTransaction("Add demo pipe");
        if (!transaction)
        {
            m_host.message("Could not start a transaction.");
            return;
        }
        demo::PipeData pipe;
        pipe.vertices = {{0.0, 0.0}, {100.0, 0.0}, {100.0, 60.0}};
        pipe.diameter = 10.0;
        const yicad::plugin::CustomEntityData entity(
            demo::PipeClass::ClassName, demo::PipeClass::Version, demo::PipeClass::encodeData(pipe));
        yicad::plugin::EntityRef created;
        if (transaction.createCustomEntity(entity, created) != YICAD_IMPORT_SUCCESS || !transaction.commit())
        {
            m_host.message("Could not add the demo pipe.");
            return;
        }
        document.zoomAuto();
    }

    /// @brief 把文档模型空间里所有管道的管径加倍，一次撤销全部恢复
    void growDemoPipes()
    {
        const auto document = m_host.currentDocument();
        if (!document)
        {
            m_host.message("No active document.");
            return;
        }
        std::vector<std::pair<yicad::plugin::EntityRef, demo::PipeData>> pipes;
        {
            auto entities = document.entities();
            yicad::plugin::EntityData entity;
            while (entities.next(entity))
            {
                const auto* custom = std::get_if<yicad::plugin::CustomEntityData>(&entity);
                if (custom != nullptr && isPipe(*custom))
                {
                    pipes.emplace_back(custom->entity(), demo::PipeClass::decodeData(custom->data()));
                }
            }
        }
        if (pipes.empty())
        {
            m_host.message("No demo pipes.");
            return;
        }
        auto transaction = document.beginTransaction("Grow demo pipes");
        if (!transaction)
        {
            m_host.message("Could not start a transaction.");
            return;
        }
        for (auto& [entity, pipe] : pipes)
        {
            pipe.diameter *= 2.0;
            if (transaction.setCustomEntityData(entity, demo::PipeClass::encodeData(pipe)) !=
                YICAD_IMPORT_SUCCESS)
            {
                m_host.message("Could not change a demo pipe.");
                return;
            }
        }
        transaction.commit();
    }

    yicad::plugin::Host m_host;
    demo::PipeClass m_pipe;
};

DemoPlugin g_plugin;

} // namespace

YICAD_PLUGIN_EXPORT uint32_t YICAD_PLUGIN_CALL
yicad_plugin_get_abi_version(void)
{
    return YICAD_PLUGIN_ABI_V4;
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
