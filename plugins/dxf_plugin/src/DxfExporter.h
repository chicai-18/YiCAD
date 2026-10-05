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
#include "DxfProxyGraphics.h"
#include "YiCadPluginSdk.h"

#include <libdxfrw.h>

#include <map>
#include <set>
#include <string>
#include <vector>

/// @brief 将 YiCAD 文档快照直接写出为 DXF。
/// @details 自定义实体照 AutoCAD 写（RENDER_PLAN.md 第 8.4 步）：CLASSES 段登记、自己的类型名、
///          代理图形与数据，见 DxfCustomEntity.h。原数据是 DWG 二进制的别的程序的代理写成它的图形（基本实体）。
class DxfExporter final : public DxfInterfaceAdapter
{
public:
    explicit DxfExporter(yicad::plugin::Document document);

    bool write(const char* path);

    /// @brief 上次写出时写成图形的代理个数（原数据是 DWG 二进制，2013 版 DXF 里写不回原样）
    int proxiesWrittenAsGraphics() const noexcept { return m_proxiesAsGraphics; }

    void writeHeader(DRW_Header& header) override;
    void writeClasses() override;
    void writeAppId() override;
    void writeLTypes() override;
    void writeLayers() override;
    void writeTextstyles() override;
    void writeDimstyles() override;
    void writeBlockRecords() override;
    void writeBlocks() override;
    void writeEntities() override;

private:
    /// @brief CLASSES 段的一条
    struct ClassEntry
    {
        std::string recordName;
        std::string className;
        std::string appName;
        uint32_t proxyFlags = 0;
        bool wasProxy = false;
        int instances = 0;
    };

    static bool isExportableBlock(const yicad::plugin::BlockData& block);

    std::vector<yicad::plugin::BlockData> exportableBlocks() const;
    /// @brief 写之前扫一遍自定义实体：CLASSES 段要在实体之前写出
    void collectClasses();
    void collectClasses(yicad::plugin::EntityIterator entities);
    void writeEntities(yicad::plugin::EntityIterator entities);
    void writeCustomEntity(
        const yicad::plugin::CustomEntityData& entity,
        const yicad::plugin::EntityIterator& entities);
    /// @brief 把自定义实体写成它的图形（基本实体）
    void writeGraphics(const yicad::plugin::EntityIterator& entities);
    void setFailed(bool ok) noexcept;

    yicad::plugin::Document m_document;
    dxfRW* m_writer = nullptr;
    bool m_failed = false;
    int m_proxiesAsGraphics = 0;
    std::map<std::string, ClassEntry> m_classes;    ///< 按类名
    std::set<std::string> m_appIds;                 ///< 别的程序的实体的扩展数据用到的应用名
    dxf::TableIndex m_layerIndex;                   ///< 图层在写出的 LAYER 表里的序号（代理图形用）
    dxf::TableIndex m_lineTypeIndex;                ///< 线型在写出的 LTYPE 表里的序号
};

#endif
