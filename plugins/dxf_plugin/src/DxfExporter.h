/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef YICAD_DXF_EXPORTER_H
#define YICAD_DXF_EXPORTER_H

#include "DxfInterfaceAdapter.h"
#include "YiCadPluginSdk.h"

#include <libdxfrw/libdxfrw.h>

#include <vector>

/// @brief 将 YiCAD 文档快照直接写出为 DXF。
class DxfExporter final : public DxfInterfaceAdapter
{
public:
    explicit DxfExporter(yicad::plugin::Document document);

    bool write(const char* path);

    void writeHeader(DRW_Header& header) override;
    void writeLTypes() override;
    void writeLayers() override;
    void writeTextstyles() override;
    void writeDimstyles() override;
    void writeBlockRecords() override;
    void writeBlocks() override;
    void writeEntities() override;

private:
    static bool isExportableBlock(const yicad::plugin::BlockData& block);

    std::vector<yicad::plugin::BlockData> exportableBlocks() const;
    void writeEntities(yicad::plugin::EntityIterator entities);
    void setFailed(bool ok) noexcept;

    yicad::plugin::Document m_document;
    dxfRW* m_writer = nullptr;
    bool m_failed = false;
};

#endif
