/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "DxfExporter.h"

#include "DxfMapping.h"

#include <algorithm>
#include <cctype>
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
    const auto written = file.write(this, DRW::AC1027, false);
    m_writer = nullptr;
    return written && !m_failed;
}

void DxfExporter::writeHeader(DRW_Header& header)
{
    dxf::writeHeader(m_document.settings(), header);
}

void DxfExporter::writeLTypes()
{
    if (m_writer == nullptr)
    {
        m_failed = true;
        return;
    }
    for (const auto& value : m_document.lineTypes())
    {
        if (value.name().empty() || value.complex())
        {
            continue;
        }
        auto data = dxf::toDxf(value);
        setFailed(m_writer->writeLineType(&data));
    }
}

void DxfExporter::writeLayers()
{
    if (m_writer == nullptr)
    {
        m_failed = true;
        return;
    }
    for (const auto& value : m_document.layers())
    {
        if (value.name().empty())
        {
            continue;
        }
        auto data = dxf::toDxf(value);
        setFailed(m_writer->writeLayer(&data));
    }
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
            setFailed(dxf::writeEntity(*m_writer, entity));
        }, value);
    }
}

void DxfExporter::setFailed(bool ok) noexcept
{
    m_failed = m_failed || !ok;
}
