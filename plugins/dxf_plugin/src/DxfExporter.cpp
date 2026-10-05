/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "DxfExporter.h"

#include "DxfCustomEntity.h"
#include "DxfMapping.h"

#include <algorithm>
#include <cctype>
#include <type_traits>
#include <utility>

namespace
{

std::string upper(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::toupper(character));
        });
    return value;
}

} // namespace

DxfExporter::DxfExporter(yicad::plugin::Document document)
    : m_document(std::move(document))
{
}

bool DxfExporter::write(const char* path)
{
    if (path == nullptr || *path == '\0' || !m_document)
    {
        return false;
    }

    dxfRW file(path);
    m_writer = &file;
    m_failed = false;
    m_proxiesAsGraphics = 0;
    collectClasses();
    const auto written = file.write(this, DRW::AC1027, false);
    m_writer = nullptr;
    return written && !m_failed;
}

void DxfExporter::writeHeader(DRW_Header& header)
{
    dxf::writeHeader(m_document.settings(), header);
}

void DxfExporter::collectClasses()
{
    m_classes.clear();
    m_appIds.clear();
    collectClasses(m_document.entities());
    for (const auto& block : exportableBlocks())
    {
        collectClasses(m_document.entities(block));
    }
}

void DxfExporter::collectClasses(yicad::plugin::EntityIterator entities)
{
    yicad::plugin::EntityData value;
    while (entities.next(value))
    {
        const auto* custom = std::get_if<yicad::plugin::CustomEntityData>(&value);
        if (custom == nullptr)
        {
            continue;
        }
        dxf::ForeignEntityData foreign;
        const bool isForeign = dxf::decodeForeign(custom->data(), foreign);
        if (isForeign && foreign.acadProxy)
        {
            // 写成图形，不登记类
            continue;
        }
        auto [entry, added] = m_classes.try_emplace(custom->className());
        if (added)
        {
            entry->second.className = custom->className();
            if (isForeign)
            {
                entry->second.recordName = foreign.recordName;
                entry->second.appName = foreign.appName;
                for (const auto& [code, text] : foreign.records)
                {
                    if (code == 1001)
                    {
                        m_appIds.insert(text);
                    }
                }
            }
            else
            {
                entry->second.recordName = dxf::recordNameFor(custom->className());
                entry->second.appName = dxf::appNameFor(custom->className());
            }
            entry->second.proxyFlags = custom->proxyFlags();
        }
        // 有一个是代理（存盘时类不在）就记"曾是代理"
        entry->second.wasProxy = entry->second.wasProxy || custom->isProxy();
        ++entry->second.instances;
    }
}

void DxfExporter::writeClasses()
{
    if (m_writer == nullptr)
    {
        m_failed = true;
        return;
    }
    for (const auto& [className, entry] : m_classes)
    {
        DRW_Class data;
        data.recName = entry.recordName;
        data.className = entry.className;
        data.appName = entry.appName;
        data.proxyFlag = static_cast<int>(entry.proxyFlags);
        data.instanceCount = entry.instances;
        data.wasaProxyFlag = entry.wasProxy ? 1 : 0;
        data.entityFlag = 1;
        setFailed(m_writer->writeClass(&data));
    }
}

void DxfExporter::writeAppId()
{
    if (m_writer == nullptr)
    {
        m_failed = true;
        return;
    }
    // 别的程序的实体原样写回的扩展数据要用到它们的应用名
    for (const auto& name : m_appIds)
    {
        DRW_AppId data;
        data.name = name;
        data.flags = 0;
        setFailed(m_writer->writeAppId(&data));
    }
}

void DxfExporter::writeLTypes()
{
    if (m_writer == nullptr)
    {
        m_failed = true;
        return;
    }
    // libdxfrw 先写 ByBlock、ByLayer、Continuous 三条，再写这里的；代理图形按写出的顺序给序号
    m_lineTypeIndex = {{"BYBLOCK", 0}, {"BYLAYER", 1}, {"CONTINUOUS", 2}};
    for (const auto& value : m_document.lineTypes())
    {
        if (value.name().empty() || value.complex())
        {
            continue;
        }
        const auto key = dxf::resourceKey(value.name());
        if (m_lineTypeIndex.count(key) != 0)
        {
            continue;
        }
        auto data = dxf::toDxf(value);
        setFailed(m_writer->writeLineType(&data));
        m_lineTypeIndex.emplace(key, static_cast<uint32_t>(m_lineTypeIndex.size()));
    }
}

void DxfExporter::writeLayers()
{
    if (m_writer == nullptr)
    {
        m_failed = true;
        return;
    }
    m_layerIndex.clear();
    for (const auto& value : m_document.layers())
    {
        if (value.name().empty())
        {
            continue;
        }
        auto data = dxf::toDxf(value);
        setFailed(m_writer->writeLayer(&data));
        m_layerIndex.emplace(dxf::resourceKey(value.name()), static_cast<uint32_t>(m_layerIndex.size()));
    }
    // 文档里没有 0 层时 libdxfrw 在最后补一条
    m_layerIndex.emplace("0", static_cast<uint32_t>(m_layerIndex.size()));
}

void DxfExporter::writeTextstyles()
{
    if (m_writer == nullptr)
    {
        m_failed = true;
        return;
    }
    for (const auto& value : m_document.textStyles())
    {
        if (value.name().empty())
        {
            continue;
        }
        auto data = dxf::toDxf(value);
        setFailed(m_writer->writeTextstyle(&data));
    }
}

void DxfExporter::writeDimstyles()
{
    if (m_writer == nullptr)
    {
        m_failed = true;
        return;
    }
    for (const auto& value : m_document.dimensionStyles())
    {
        if (value.name().empty())
        {
            continue;
        }
        auto data = dxf::toDxf(value);
        setFailed(m_writer->writeDimstyle(&data));
    }
}

void DxfExporter::writeBlockRecords()
{
    if (m_writer == nullptr)
    {
        m_failed = true;
        return;
    }
    for (const auto& block : exportableBlocks())
    {
        setFailed(m_writer->writeBlockRecord(block.name()));
    }
}

void DxfExporter::writeBlocks()
{
    if (m_writer == nullptr)
    {
        m_failed = true;
        return;
    }
    for (const auto& block : exportableBlocks())
    {
        auto data = dxf::toDxf(block);
        setFailed(m_writer->writeBlock(&data));
        writeEntities(m_document.entities(block));
    }
}

void DxfExporter::writeEntities()
{
    writeEntities(m_document.entities());
}

bool DxfExporter::isExportableBlock(const yicad::plugin::BlockData& block)
{
    const auto name = upper(block.name());
    return !name.empty() && name.front() != '*' &&
           name != "*MODEL_SPACE" && name != "*PAPER_SPACE" &&
           !dxf::isInternalArrowBlock(name);
}

std::vector<yicad::plugin::BlockData> DxfExporter::exportableBlocks() const
{
    std::vector<yicad::plugin::BlockData> result;
    for (auto block : m_document.blocks())
    {
        if (isExportableBlock(block))
        {
            result.push_back(std::move(block));
        }
    }
    return result;
}

void DxfExporter::writeEntities(yicad::plugin::EntityIterator entities)
{
    if (m_writer == nullptr)
    {
        m_failed = true;
        return;
    }

    yicad::plugin::EntityData value;
    while (entities.next(value))
    {
        std::visit([&](const auto& entity) {
            using T = std::decay_t<decltype(entity)>;
            if constexpr (std::is_same_v<T, yicad::plugin::CustomEntityData>)
            {
                writeCustomEntity(entity, entities);
            }
            else
            {
                setFailed(dxf::writeEntity(*m_writer, entity));
            }
        }, value);
    }
}

void DxfExporter::writeGraphics(const yicad::plugin::EntityIterator& entities)
{
    auto graphics = entities.graphics();
    yicad::plugin::EntityData part;
    while (graphics.next(part))
    {
        std::visit([&](const auto& entity) {
            using T = std::decay_t<decltype(entity)>;
            if constexpr (!std::is_same_v<T, yicad::plugin::CustomEntityData>)
            {
                setFailed(dxf::writeEntity(*m_writer, entity));
            }
        }, part);
    }
}

void DxfExporter::writeCustomEntity(
    const yicad::plugin::CustomEntityData& entity,
    const yicad::plugin::EntityIterator& entities)
{
    dxf::ForeignEntityData foreign;
    const bool isForeign = dxf::decodeForeign(entity.data(), foreign);
    if (isForeign && foreign.acadProxy)
    {
        // 原数据是 DWG 二进制的 ACAD_PROXY_ENTITY：AcDbProxyEntity 的写法随 DXF 版本变，
        // 本机 AutoCAD 2026 不认 2013 版文件里照别的版本写的，整张图纸打不开（2026-10-05 核对）。
        // 没有真实样本核对 2013 版的写法，写成它的图形；原数据仍在 YiCAD 的代理里
        writeGraphics(entities);
        ++m_proxiesAsGraphics;
        return;
    }
    const auto found = m_classes.find(entity.className());
    if (found == m_classes.end())
    {
        return;
    }
    DRW_Point common;
    dxf::applyAttributes(common, entity.attributes());

    // 代理图形：宿主把实体的图形做成基本实体交来，编码成 AutoCAD 的格式
    auto encodeGraphics = [&]() {
        dxf::ProxyGraphicsWriter writer(m_layerIndex, m_lineTypeIndex, entity.attributes());
        auto graphics = entities.graphics();
        yicad::plugin::EntityData part;
        while (graphics.next(part))
        {
            writer.add(part);
        }
        return writer.finish();
    };

    dxf::GroupCodes records;
    const auto& m = entity.transform();
    const bool transformed = !(m.a == 1.0 && m.b == 0.0 && m.c == 0.0 && m.d == 1.0 && m.tx == 0.0 && m.ty == 0.0);
    if (isForeign)
    {
        // 别的程序的实体：子类段原样写回；没变换过时代理图形也用原来的
        const auto graphics = !transformed && !foreign.graphics.empty() ? foreign.graphics : encodeGraphics();
        records = dxf::graphicsCodes(graphics);
        records.insert(records.end(), foreign.records.begin(), foreign.records.end());
    }
    else
    {
        records = dxf::graphicsCodes(encodeGraphics());
        const auto data = dxf::yicadDataCodes(entity.classVersion(), entity.data(), m);
        records.insert(records.end(), data.begin(), data.end());
    }
    setFailed(m_writer->writeCustomEntity(found->second.recordName, &common, records));
}

void DxfExporter::setFailed(bool ok) noexcept
{
    m_failed = m_failed || !ok;
}
