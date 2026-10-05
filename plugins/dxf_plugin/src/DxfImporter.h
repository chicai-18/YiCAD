/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef YICAD_DXF_IMPORTER_H
#define YICAD_DXF_IMPORTER_H

#include "DxfInterfaceAdapter.h"
#include "DxfMapping.h"
#include "DxfProxyGraphics.h"
#include "YiCadPluginSdk.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class DxfImporter final : public DxfInterfaceAdapter
{
public:
    explicit DxfImporter(yicad::plugin::Document document);

    bool read(const char* path);

    void addHeader(const DRW_Header* source) override;
    void addLType(const DRW_LType& source) override;
    void addLayer(const DRW_Layer& source) override;
    void addTextStyle(const DRW_Textstyle& source) override;
    void addDimStyle(const DRW_Dimstyle& source) override;
    void addBlock(const DRW_Block& source) override;
    void endBlock() override;
    void addPoint(const DRW_Point& source) override;
    void addLine(const DRW_Line& source) override;
    void addRay(const DRW_Ray& source) override;
    void addXline(const DRW_Xline& source) override;
    void addArc(const DRW_Arc& source) override;
    void addCircle(const DRW_Circle& source) override;
    void addEllipse(const DRW_Ellipse& source) override;
    void addLWPolyline(const DRW_LWPolyline& source) override;
    void addPolyline(const DRW_Polyline& source) override;
    void addSpline(const DRW_Spline* source) override;
    void addTrace(const DRW_Trace& source) override;
    void addSolid(const DRW_Solid& source) override;
    void addText(const DRW_Text& source) override;
    void addMText(const DRW_MText& source) override;
    void addInsert(const DRW_Insert& source) override;
    void addDimAlign(const DRW_DimAligned* source) override;
    void addDimLinear(const DRW_DimLinear* source) override;
    void addDimRadial(const DRW_DimRadial* source) override;
    void addDimDiametric(const DRW_DimDiametric* source) override;
    void addDimAngular(const DRW_DimAngular* source) override;
    void addDimAngular3P(const DRW_DimAngular3p* source) override;
    void addLeader(const DRW_Leader* source) override;
    void addHatch(const DRW_Hatch* source) override;
    void addImage(const DRW_Image* source) override;
    void linkImage(const DRW_ImageDef* source) override;
    /// @brief CLASSES 段的类登记：自定义实体按记录名找它
    void addClass(const DRW_Class& source) override;
    /// @brief 自定义实体（RENDER_PLAN.md 第 8.4 步）：YiCAD 的类按数据建（类不在时为代理），
    /// 别的程序的二维实体读成代理、数据原样保留；三维的、没登记类的跳过。
    /// ACAD_PROXY_ENTITY 按组码 91 的序号找 CLASSES 段登记的类，见 DxfCustomEntity.h
    void addUnknownEntity(const DRW_UnknownEntity& source) override;

private:
    /// @brief CLASSES 段登记的一个类
    struct ClassInfo
    {
        std::string className;
        std::string appName;
        uint32_t proxyFlags = 0;
        bool entity = false;
    };

    /// @brief 把代理图形解出的一个图元建进收集代理图形的容器
    void createProxyShape(const dxf::ProxyShape& shape, const yicad::plugin::ImportContainer& graphics);
    struct PendingInsert
    {
        std::string blockName;
        yicad::plugin::ImportContainer container;
        YiCadPoint2d insertionPoint{};
        YiCadVector3d scale{1.0, 1.0, 1.0};
        double rotation = 0.0;
        uint32_t columnCount = 1;
        uint32_t rowCount = 1;
        double columnSpacing = 0.0;
        double rowSpacing = 0.0;
        yicad::plugin::EntityAttributes attributes;
    };

    struct PendingImage
    {
        uint32_t definitionHandle = 0;
        YiCadPoint2d insertionPoint{};
        YiCadVector2d uVector{};
        YiCadVector2d vVector{};
        YiCadVector2d size{};
        int32_t brightness = 50;
        int32_t contrast = 50;
        int32_t fade = 0;
        yicad::plugin::EntityAttributes attributes;
    };

    bool accepts(const DRW_Entity& source) const noexcept;
    void setFailed(YiCadImportResult result) noexcept;
    yicad::plugin::ImportResource ensureLineType(const std::string& name);
    yicad::plugin::ImportResource ensureTextStyle(const std::string& name);
    yicad::plugin::ImportResource ensureDimensionStyle(
        const std::string& name);
    bool createInsert(
        const PendingInsert& pending,
        const yicad::plugin::ImportResource& block);
    bool resolvePendingInserts();
    bool resolvePendingImages();

    yicad::plugin::Document m_document;
    yicad::plugin::ImportSession m_session;
    yicad::plugin::ImportContainer m_modelSpace;
    yicad::plugin::ImportContainer m_currentContainer;
    dxf::ResourceMap m_lineTypes;
    dxf::ResourceMap m_layers;
    dxf::ResourceMap m_textStyles;
    dxf::ResourceMap m_dimensionStyles;
    dxf::ResourceMap m_blocks;
    std::unordered_set<std::string> m_finalizedBlocks;
    std::unordered_map<uint32_t, std::string> m_imageDefinitions;
    std::unordered_map<std::string, ClassInfo> m_classes;   ///< 按记录名（大写）
    std::vector<std::string> m_classOrder;                  ///< CLASSES 段的记录名（大写），按顺序：ACAD_PROXY_ENTITY 按序号找类
    std::vector<std::string> m_layerOrder;                  ///< LAYER 表的记录顺序（代理图形按序号引用）
    std::vector<std::string> m_lineTypeOrder;               ///< LTYPE 表的记录顺序
    dxf::ResourceMap m_textStylesByFont;                    ///< 字体文件名（大写）到文字样式
    dxfRW* m_reader = nullptr;                              ///< 读文件期间的 libdxfrw，查版本用
    std::vector<PendingInsert> m_pendingInserts;
    std::vector<PendingImage> m_pendingImages;
    std::string m_currentBlock;
    bool m_inBlock = false;
    bool m_skipBlock = false;
    bool m_failed = false;
};

#endif
