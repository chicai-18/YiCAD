/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "DxfImporter.h"

#include "DxfCustomEntity.h"

#include <libdxfrw.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

namespace
{

constexpr double Pi = 3.14159265358979323846;
constexpr double DegreesPerRadian = 57.2957795130823208768;

bool nonZero(YiCadVector2d value) noexcept
{
    return std::hypot(value.x, value.y) > 1.0e-9;
}

std::vector<YiCadVertex2d> vertices(const DRW_LWPolyline& source)
{
    std::vector<YiCadVertex2d> result;
    result.reserve(source.vertlist.size());
    for (const auto& vertex : source.vertlist)
    {
        if (vertex == nullptr)
        {
            continue;
        }
        const auto startWidth =
            vertex->stawidth != 0.0 ? vertex->stawidth : source.width;
        const auto endWidth =
            vertex->endwidth != 0.0 ? vertex->endwidth : source.width;
        result.push_back({
            {vertex->x, vertex->y},
            std::max(0.0, startWidth),
            std::max(0.0, endWidth),
            vertex->bulge});
    }
    return result;
}

std::vector<YiCadVertex2d> vertices(const DRW_Polyline& source)
{
    std::vector<YiCadVertex2d> result;
    result.reserve(source.vertlist.size());
    for (const auto& vertex : source.vertlist)
    {
        if (vertex == nullptr)
        {
            continue;
        }
        const auto startWidth =
            vertex->stawidth != 0.0 ? vertex->stawidth : source.defstawidth;
        const auto endWidth =
            vertex->endwidth != 0.0 ? vertex->endwidth : source.defendwidth;
        result.push_back({
            dxf::point(vertex->basePoint),
            std::max(0.0, startWidth),
            std::max(0.0, endWidth),
            vertex->bulge});
    }
    return result;
}

yicad::plugin::DimensionData commonDimension(
    YiCadDimensionKind kind,
    const DRW_Dimension& source,
    const yicad::plugin::ImportResource& style,
    yicad::plugin::EntityAttributes attributes)
{
    auto text = source.getText();
    if (text == "<>")
    {
        text.clear();
    }
    return yicad::plugin::DimensionData(kind)
        .setStyle(style)
        .setText(
            std::move(text),
            dxf::point(source.getTextPoint()),
            source.getDir() / DegreesPerRadian,
            source.getTextLineFactor())
        .setAttributes(std::move(attributes));
}

yicad::plugin::HatchData::Edge hatchEdge(const DRW_Entity& source)
{
    switch (source.eType)
    {
    case DRW::LINE:
    {
        const auto& line = static_cast<const DRW_Line&>(source);
        return yicad::plugin::HatchData::Edge::line(
            dxf::point(line.basePoint),
            dxf::point(line.secPoint));
    }
    case DRW::ARC:
    {
        const auto& arc = static_cast<const DRW_Arc&>(source);
        return yicad::plugin::HatchData::Edge::circularArc(
            dxf::point(arc.basePoint),
            arc.radious,
            arc.staangle,
            arc.endangle,
            arc.isccw != 0);
    }
    case DRW::ELLIPSE:
    {
        const auto& ellipse = static_cast<const DRW_Ellipse&>(source);
        return yicad::plugin::HatchData::Edge::ellipticArc(
            dxf::point(ellipse.basePoint),
            dxf::vector(ellipse.secPoint),
            ellipse.ratio,
            ellipse.staparam,
            ellipse.endparam,
            ellipse.isccw != 0);
    }
    case DRW::SPLINE:
    {
        const auto& spline = static_cast<const DRW_Spline&>(source);
        std::vector<YiCadPoint2d> controlPoints;
        controlPoints.reserve(spline.controllist.size());
        for (const auto& point : spline.controllist)
        {
            if (point != nullptr)
            {
                controlPoints.push_back(dxf::point(*point));
            }
        }
        return yicad::plugin::HatchData::Edge::spline(
            static_cast<uint32_t>(std::max(0, spline.degree)),
            std::move(controlPoints),
            spline.knotslist);
    }
    default:
        return yicad::plugin::HatchData::Edge::line({}, {});
    }
}

bool isSupportedHatchEdge(const DRW_Entity& source)
{
    if (source.eType == DRW::LINE || source.eType == DRW::ARC ||
        source.eType == DRW::ELLIPSE)
    {
        return true;
    }
    if (source.eType != DRW::SPLINE)
    {
        return false;
    }
    const auto& spline = static_cast<const DRW_Spline&>(source);
    return (spline.flags & (2 | 4)) == 0 && spline.weightlist.empty() &&
           spline.degree >= 1 && !spline.controllist.empty();
}

} // namespace

DxfImporter::DxfImporter(yicad::plugin::Document document)
    : m_document(std::move(document))
{
}

bool DxfImporter::read(const char* path)
{
    if (path == nullptr || *path == '\0' || !m_document)
    {
        return false;
    }

    m_session = m_document.beginImport();
    if (!m_session ||
        m_session.modelSpace(m_modelSpace) != YICAD_IMPORT_SUCCESS)
    {
        return false;
    }

    m_currentContainer = m_modelSpace;
    // 随块线型的资源先建好：R12 等图纸的线型表里没有 ByBlock 记录，实体却可以写 BYBLOCK
    ensureLineType("ByBlock");
    dxfRW file(path);
    m_reader = &file;
    const bool readOk = file.read(this, true);
    m_reader = nullptr;
    if (!readOk || m_failed ||
        !resolvePendingInserts() || !resolvePendingImages())
    {
        return false;
    }
    return m_session.commit() == YICAD_IMPORT_SUCCESS;
}

void DxfImporter::addHeader(const DRW_Header* source)
{
    if (source == nullptr)
    {
        return;
    }
    int32_t insertionUnits = 0;
    int32_t measurement = 0;
    double lineTypeScale = 1.0;
    double currentLineTypeScale = 1.0;
    std::string codePage;
    dxf::headerInt(*source, "$INSUNITS", insertionUnits);
    dxf::headerInt(*source, "$MEASUREMENT", measurement);
    dxf::headerDouble(*source, "$LTSCALE", lineTypeScale);
    dxf::headerDouble(*source, "$CELTSCALE", currentLineTypeScale);
    dxf::headerString(*source, "$DWGCODEPAGE", codePage);
    if (insertionUnits < 0 || insertionUnits > 20)
    {
        insertionUnits = 0;
    }
    if (measurement != 0 && measurement != 1)
    {
        measurement = 0;
    }
    if (!std::isfinite(lineTypeScale) || lineTypeScale <= 0.0)
    {
        lineTypeScale = 1.0;
    }
    if (!std::isfinite(currentLineTypeScale) || currentLineTypeScale <= 0.0)
    {
        currentLineTypeScale = 1.0;
    }
    const auto settings = yicad::plugin::DocumentSettings{}
        .setInsertionUnits(insertionUnits)
        .setMeasurement(measurement)
        .setGlobalLineTypeScale(lineTypeScale)
        .setCurrentEntityLineTypeScale(currentLineTypeScale)
        .setSourceCodePage(std::move(codePage));
    setFailed(m_session.setDocumentSettings(settings));
}

void DxfImporter::addLType(const DRW_LType& source)
{
    // 代理图形按记录序号引用线型：每条记录都记下顺序
    m_lineTypeOrder.push_back(source.name);
    if (source.name.empty())
    {
        return;
    }
    yicad::plugin::ImportResource resource;
    const auto result = m_session.createLineType(
        dxf::toLineType(source),
        YICAD_RESOURCE_CONFLICT_REPLACE,
        resource);
    setFailed(result);
    if (result == YICAD_IMPORT_SUCCESS)
    {
        m_lineTypes.insert_or_assign(
            dxf::resourceKey(source.name), resource);
    }
}

void DxfImporter::addLayer(const DRW_Layer& source)
{
    // 代理图形按记录序号引用图层：每条记录都记下顺序
    m_layerOrder.push_back(source.name);
    if (source.name.empty())
    {
        return;
    }
    yicad::plugin::ImportResource resource;
    const auto result = m_session.createLayer(
        dxf::toLayer(source, m_lineTypes),
        YICAD_RESOURCE_CONFLICT_REPLACE,
        resource);
    setFailed(result);
    if (result == YICAD_IMPORT_SUCCESS)
    {
        m_layers.insert_or_assign(
            dxf::resourceKey(source.name), resource);
    }
}

void DxfImporter::addTextStyle(const DRW_Textstyle& source)
{
    if (source.name.empty())
    {
        return;
    }
    yicad::plugin::ImportResource resource;
    const auto result = m_session.createTextStyle(
        dxf::toTextStyle(source),
        YICAD_RESOURCE_CONFLICT_REPLACE,
        resource);
    setFailed(result);
    if (result == YICAD_IMPORT_SUCCESS)
    {
        m_textStyles.insert_or_assign(
            dxf::resourceKey(source.name), resource);
        if (!source.font.empty())
        {
            m_textStylesByFont.try_emplace(dxf::resourceKey(source.font), resource);
        }
    }
}

void DxfImporter::addDimStyle(const DRW_Dimstyle& source)
{
    if (source.name.empty())
    {
        return;
    }
    const auto textStyle = ensureTextStyle(source.dimtxsty);
    const auto lineType = ensureLineType("CONTINUOUS");
    if (!textStyle || !lineType)
    {
        m_failed = true;
        return;
    }
    yicad::plugin::ImportResource resource;
    const auto result = m_session.createDimensionStyle(
        dxf::toDimensionStyle(source, textStyle, lineType),
        YICAD_RESOURCE_CONFLICT_REPLACE,
        resource);
    setFailed(result);
    if (result == YICAD_IMPORT_SUCCESS)
    {
        m_dimensionStyles.insert_or_assign(
            dxf::resourceKey(source.name), resource);
    }
}

void DxfImporter::addBlock(const DRW_Block& source)
{
    m_inBlock = true;
    m_currentBlock = dxf::resourceKey(source.name);
    m_skipBlock = source.name.empty() ||
        m_currentBlock == "*MODEL_SPACE" ||
        m_currentBlock == "*PAPER_SPACE" ||
        m_currentBlock.rfind("*MODEL_SPACE", 0) == 0 ||
        m_currentBlock.rfind("*PAPER_SPACE", 0) == 0 ||
        dxf::isInternalArrowBlock(m_currentBlock);
    if (m_skipBlock)
    {
        m_currentContainer = {};
        return;
    }

    auto data = yicad::plugin::BlockData(source.name)
        .setBasePoint(dxf::point(source.basePoint))
        .setFlags(static_cast<uint32_t>(source.flags) &
            YICAD_BLOCK_HAS_ATTRIBUTES);
    yicad::plugin::ImportResource block;
    yicad::plugin::ImportContainer container;
    const auto result = m_session.beginBlock(data, block, container);
    setFailed(result);
    if (result == YICAD_IMPORT_SUCCESS)
    {
        m_blocks.insert_or_assign(m_currentBlock, block);
        m_currentContainer = container;
    }
}

void DxfImporter::endBlock()
{
    if (!m_inBlock)
    {
        return;
    }
    if (!m_skipBlock)
    {
        setFailed(m_currentContainer.endBlock());
        if (!m_failed)
        {
            m_finalizedBlocks.insert(m_currentBlock);
        }
    }
    m_currentContainer = m_modelSpace;
    m_currentBlock.clear();
    m_inBlock = false;
    m_skipBlock = false;
}

void DxfImporter::addPoint(const DRW_Point& source)
{
    if (accepts(source))
    {
        setFailed(m_currentContainer.createPoint(
            dxf::point(source.basePoint),
            dxf::toAttributes(source, m_layers, m_lineTypes)));
    }
}

void DxfImporter::addLine(const DRW_Line& source)
{
    if (accepts(source))
    {
        setFailed(m_currentContainer.createLine(
            dxf::point(source.basePoint),
            dxf::point(source.secPoint),
            dxf::toAttributes(source, m_layers, m_lineTypes)));
    }
}

void DxfImporter::addRay(const DRW_Ray& source)
{
    const auto direction = dxf::vector(source.secPoint);
    if (accepts(source) && nonZero(direction))
    {
        setFailed(m_currentContainer.createRay(
            dxf::point(source.basePoint),
            direction,
            dxf::toAttributes(source, m_layers, m_lineTypes)));
    }
}

void DxfImporter::addXline(const DRW_Xline& source)
{
    const auto direction = dxf::vector(source.secPoint);
    if (accepts(source) && nonZero(direction))
    {
        setFailed(m_currentContainer.createXLine(
            dxf::point(source.basePoint),
            direction,
            dxf::toAttributes(source, m_layers, m_lineTypes)));
    }
}

void DxfImporter::addArc(const DRW_Arc& source)
{
    if (accepts(source) && source.radious > 0.0)
    {
        setFailed(m_currentContainer.createArc(
            dxf::point(source.basePoint),
            source.radious,
            source.staangle,
            source.endangle,
            dxf::toAttributes(source, m_layers, m_lineTypes)));
    }
}

void DxfImporter::addCircle(const DRW_Circle& source)
{
    if (accepts(source) && source.radious > 0.0)
    {
        setFailed(m_currentContainer.createCircle(
            dxf::point(source.basePoint),
            source.radious,
            dxf::toAttributes(source, m_layers, m_lineTypes)));
    }
}

void DxfImporter::addEllipse(const DRW_Ellipse& source)
{
    const auto majorAxis = dxf::vector(source.secPoint);
    if (!accepts(source) || !nonZero(majorAxis) ||
        source.ratio <= 0.0 || source.ratio > 1.0)
    {
        return;
    }
    const auto span = std::abs(source.endparam - source.staparam);
    setFailed(m_currentContainer.createEllipse(
        dxf::point(source.basePoint),
        majorAxis,
        source.ratio,
        source.staparam,
        source.endparam,
        std::abs(span - 2.0 * Pi) < 1.0e-8,
        dxf::toAttributes(source, m_layers, m_lineTypes)));
}

void DxfImporter::addLWPolyline(const DRW_LWPolyline& source)
{
    if (!accepts(source))
    {
        return;
    }
    auto points = vertices(source);
    if (points.size() < 2)
    {
        return;
    }
    auto data = yicad::plugin::PolylineData(std::move(points))
        .setClosed((source.flags & 1) != 0)
        .setAttributes(dxf::toAttributes(
            source, m_layers, m_lineTypes));
    setFailed(m_currentContainer.createPolyline(data));
}

void DxfImporter::addPolyline(const DRW_Polyline& source)
{
    if (!accepts(source) || (source.flags & (8 | 16 | 64)) != 0)
    {
        return;
    }
    auto points = vertices(source);
    if (points.size() < 2)
    {
        return;
    }
    auto data = yicad::plugin::PolylineData(std::move(points))
        .setClosed((source.flags & 1) != 0)
        .setAttributes(dxf::toAttributes(
            source, m_layers, m_lineTypes));
    setFailed(m_currentContainer.createPolyline(data));
}

void DxfImporter::addSpline(const DRW_Spline* source)
{
    if (source == nullptr || !accepts(*source) ||
        source->degree < 1 || (source->flags & (2 | 4)) != 0 ||
        !source->weightlist.empty())
    {
        return;
    }

    yicad::plugin::SplineData data;
    if (!source->controllist.empty())
    {
        std::vector<YiCadPoint2d> points;
        points.reserve(source->controllist.size());
        for (const auto& value : source->controllist)
        {
            if (value != nullptr)
            {
                points.push_back(dxf::point(*value));
            }
        }
        if (points.size() < static_cast<std::size_t>(source->degree + 1) ||
            source->knotslist.size() !=
                points.size() + static_cast<std::size_t>(source->degree) + 1)
        {
            return;
        }
        data.setControlPoints(
            static_cast<uint32_t>(source->degree),
            std::move(points),
            source->knotslist);
    }
    else
    {
        if (source->degree != 3 || source->fitlist.size() < 4)
        {
            return;
        }
        std::vector<YiCadPoint2d> points;
        points.reserve(source->fitlist.size());
        for (const auto& value : source->fitlist)
        {
            if (value != nullptr)
            {
                points.push_back(dxf::point(*value));
            }
        }
        if (points.size() < 4)
        {
            return;
        }
        data.setFitPoints(3, std::move(points));
    }
    data.setClosed((source->flags & 1) != 0)
        .setAttributes(dxf::toAttributes(
            *source, m_layers, m_lineTypes));
    setFailed(m_currentContainer.createSpline(data));
}

void DxfImporter::addTrace(const DRW_Trace& source)
{
    if (accepts(source))
    {
        setFailed(m_currentContainer.createSolid(
            dxf::toSolid(source, m_layers, m_lineTypes)));
    }
}

void DxfImporter::addSolid(const DRW_Solid& source)
{
    addTrace(source);
}

void DxfImporter::addText(const DRW_Text& source)
{
    if (!accepts(source) || source.text.empty() || source.height <= 0.0)
    {
        return;
    }
    ensureTextStyle(source.style);
    setFailed(m_currentContainer.createText(
        dxf::toText(
            source, m_layers, m_lineTypes, m_textStyles)));
}

void DxfImporter::addMText(const DRW_MText& source)
{
    if (!accepts(source) || source.text.empty() || source.height <= 0.0)
    {
        return;
    }
    const auto style = ensureTextStyle(source.style);
    auto direction = dxf::vector(source.secPoint);
    if (!nonZero(direction))
    {
        const auto angle = source.angle / DegreesPerRadian;
        direction = {std::cos(angle), std::sin(angle)};
    }
    auto data = yicad::plugin::MTextData(source.text)
        .setPlacement(dxf::point(source.basePoint), direction)
        .setLayout(
            source.height,
            std::max(0.0, source.widthscale),
            source.interlin > 0.0 ? source.interlin : 1.0,
            static_cast<YiCadMTextAttachment>(
                std::clamp(source.textgen, 1, 9)))
        .setAttributes(dxf::toAttributes(
            source, m_layers, m_lineTypes));
    if (style)
    {
        data.setStyle(style);
    }
    setFailed(m_currentContainer.createMText(data));
}

void DxfImporter::addInsert(const DRW_Insert& source)
{
    if (!accepts(source) || source.name.empty() ||
        std::abs(source.xscale) <= 1.0e-9 ||
        std::abs(source.yscale) <= 1.0e-9)
    {
        return;
    }
    PendingInsert pending;
    pending.blockName = dxf::resourceKey(source.name);
    if (dxf::isInternalArrowBlock(pending.blockName))
    {
        return;
    }
    pending.container = m_currentContainer;
    pending.insertionPoint = dxf::point(source.basePoint);
    pending.scale = {source.xscale, source.yscale, 1.0};
    pending.rotation = source.angle;
    pending.columnCount =
        static_cast<uint32_t>(std::max(1, source.colcount));
    pending.rowCount =
        static_cast<uint32_t>(std::max(1, source.rowcount));
    pending.columnSpacing = source.colspace;
    pending.rowSpacing = source.rowspace;
    pending.attributes =
        dxf::toAttributes(source, m_layers, m_lineTypes);

    const auto block = m_blocks.find(pending.blockName);
    if (block == m_blocks.end() ||
        !m_finalizedBlocks.contains(pending.blockName))
    {
        m_pendingInserts.push_back(std::move(pending));
        return;
    }
    createInsert(pending, block->second);
}

void DxfImporter::addDimAlign(const DRW_DimAligned* source)
{
    if (source == nullptr || !accepts(*source))
    {
        return;
    }
    auto data = commonDimension(
        YICAD_DIMENSION_ALIGNED,
        *source,
        ensureDimensionStyle(source->getStyle()),
        dxf::toAttributes(*source, m_layers, m_lineTypes))
        .setDefinitionPoints(
            dxf::point(source->getDimPoint()),
            dxf::point(source->getDef1Point()),
            dxf::point(source->getDef2Point()));
    setFailed(m_currentContainer.createDimension(data));
}

void DxfImporter::addDimLinear(const DRW_DimLinear* source)
{
    if (source == nullptr || !accepts(*source))
    {
        return;
    }
    auto data = commonDimension(
        YICAD_DIMENSION_LINEAR,
        *source,
        ensureDimensionStyle(source->getStyle()),
        dxf::toAttributes(*source, m_layers, m_lineTypes))
        .setDefinitionPoints(
            dxf::point(source->getDimPoint()),
            dxf::point(source->getDef1Point()),
            dxf::point(source->getDef2Point()));
    setFailed(m_currentContainer.createDimension(data));
}

void DxfImporter::addDimRadial(const DRW_DimRadial* source)
{
    if (source == nullptr || !accepts(*source))
    {
        return;
    }
    auto data = commonDimension(
        YICAD_DIMENSION_RADIAL,
        *source,
        ensureDimensionStyle(source->getStyle()),
        dxf::toAttributes(*source, m_layers, m_lineTypes))
        .setDefinitionPoints(
            dxf::point(source->getCenterPoint()), {}, {})
        .setFeaturePoint(
            dxf::point(source->getDiameterPoint()),
            source->getLeaderLength());
    setFailed(m_currentContainer.createDimension(data));
}

void DxfImporter::addDimDiametric(const DRW_DimDiametric* source)
{
    if (source == nullptr || !accepts(*source))
    {
        return;
    }
    auto data = commonDimension(
        YICAD_DIMENSION_DIAMETRIC,
        *source,
        ensureDimensionStyle(source->getStyle()),
        dxf::toAttributes(*source, m_layers, m_lineTypes))
        .setDefinitionPoints(
            dxf::point(source->getDiameter2Point()), {}, {})
        .setFeaturePoint(
            dxf::point(source->getDiameter1Point()),
            source->getLeaderLength());
    setFailed(m_currentContainer.createDimension(data));
}

void DxfImporter::addDimAngular(const DRW_DimAngular* source)
{
    if (source == nullptr || !accepts(*source))
    {
        return;
    }
    auto data = commonDimension(
        YICAD_DIMENSION_ANGULAR,
        *source,
        ensureDimensionStyle(source->getStyle()),
        dxf::toAttributes(*source, m_layers, m_lineTypes))
        .setAngularLines(
            dxf::point(source->getFirstLine1()),
            dxf::point(source->getFirstLine2()),
            dxf::point(source->getSecondLine1()),
            dxf::point(source->getSecondLine2()),
            dxf::point(source->getDimPoint()));
    setFailed(m_currentContainer.createDimension(data));
}

void DxfImporter::addDimAngular3P(const DRW_DimAngular3p* source)
{
    if (source == nullptr || !accepts(*source))
    {
        return;
    }
    const auto vertex = dxf::point(source->getVertexPoint());
    auto data = commonDimension(
        YICAD_DIMENSION_ANGULAR,
        *source,
        ensureDimensionStyle(source->getStyle()),
        dxf::toAttributes(*source, m_layers, m_lineTypes))
        .setAngularLines(
            vertex,
            dxf::point(source->getFirstLine()),
            vertex,
            dxf::point(source->getSecondLine()),
            dxf::point(source->getDimPoint()));
    setFailed(m_currentContainer.createDimension(data));
}

void DxfImporter::addLeader(const DRW_Leader* source)
{
    if (source == nullptr || !accepts(*source))
    {
        return;
    }
    std::vector<YiCadPoint2d> points;
    points.reserve(source->vertexlist.size());
    for (const auto& point : source->vertexlist)
    {
        if (point != nullptr)
        {
            points.push_back(dxf::point(*point));
        }
    }
    if (points.size() < 2)
    {
        return;
    }
    auto data = yicad::plugin::LeaderData(std::move(points))
        .setArrow(source->arrow != 0)
        .setStyle(ensureDimensionStyle(source->style))
        .setAttributes(dxf::toAttributes(
            *source, m_layers, m_lineTypes));
    setFailed(m_currentContainer.createLeader(data));
}

void DxfImporter::addHatch(const DRW_Hatch* source)
{
    if (source == nullptr || !accepts(*source) || source->looplist.empty())
    {
        return;
    }
    yicad::plugin::HatchData data;
    if (source->solid != 0)
    {
        data.setSolid();
    }
    else
    {
        data.setPattern(
            source->name.empty() ? "ANSI31" : source->name,
            source->scale > 0.0 ? source->scale : 1.0,
            source->angle / DegreesPerRadian);
    }
    data.setAttributes(
        dxf::toAttributes(*source, m_layers, m_lineTypes));

    uint32_t lastOuter = UINT32_MAX;
    uint32_t loopIndex = 0;
    for (const auto& loop : source->looplist)
    {
        if (loop == nullptr)
        {
            continue;
        }
        const bool declaredOuter = (loop->type & (1 | 16)) != 0;
        const bool isOuter = declaredOuter || lastOuter == UINT32_MAX;
        const auto role =
            isOuter ? YICAD_HATCH_LOOP_OUTER : YICAD_HATCH_LOOP_HOLE;
        const auto outerIndex = isOuter ? UINT32_MAX : lastOuter;
        bool added = false;
        if ((loop->type & 2) != 0 && loop->objlist.size() == 1 &&
            loop->objlist.front() != nullptr &&
            loop->objlist.front()->eType == DRW::LWPOLYLINE)
        {
            const auto& polyline = static_cast<const DRW_LWPolyline&>(
                *loop->objlist.front());
            auto points = vertices(polyline);
            if (points.size() >= 3)
            {
                data.addPolylineLoop(
                    std::move(points), role, outerIndex);
                added = true;
            }
        }
        else
        {
            std::vector<yicad::plugin::HatchData::Edge> edges;
            edges.reserve(loop->objlist.size());
            for (const auto& edge : loop->objlist)
            {
                if (edge == nullptr || !isSupportedHatchEdge(*edge))
                {
                    edges.clear();
                    break;
                }
                edges.push_back(hatchEdge(*edge));
            }
            if (!edges.empty())
            {
                data.addEdgeLoop(std::move(edges), role, outerIndex);
                added = true;
            }
        }
        if (added)
        {
            if (isOuter)
            {
                lastOuter = loopIndex;
            }
            ++loopIndex;
        }
    }
    if (loopIndex != 0)
    {
        setFailed(m_currentContainer.createHatch(data));
    }
}

void DxfImporter::addImage(const DRW_Image* source)
{
    if (source == nullptr || m_inBlock || !accepts(*source) ||
        source->sizeu <= 0.0 || source->sizev <= 0.0)
    {
        return;
    }
    PendingImage image;
    image.definitionHandle = source->ref;
    image.insertionPoint = dxf::point(source->basePoint);
    image.uVector = dxf::vector(source->secPoint);
    image.vVector = dxf::vector(source->vVector);
    image.size = {source->sizeu, source->sizev};
    image.brightness = source->brightness;
    image.contrast = source->contrast;
    image.fade = source->fade;
    image.attributes =
        dxf::toAttributes(*source, m_layers, m_lineTypes);
    m_pendingImages.push_back(std::move(image));
}

void DxfImporter::linkImage(const DRW_ImageDef* source)
{
    if (source != nullptr && source->handle != 0 && !source->name.empty())
    {
        m_imageDefinitions.insert_or_assign(source->handle, source->name);
    }
}

bool DxfImporter::accepts(const DRW_Entity& source) const noexcept
{
    return !m_failed && !m_skipBlock && m_currentContainer &&
        (m_inBlock || source.space == DRW::ModelSpace);
}

void DxfImporter::setFailed(YiCadImportResult result) noexcept
{
    m_failed = m_failed || result != YICAD_IMPORT_SUCCESS;
}

yicad::plugin::ImportResource DxfImporter::ensureLineType(
    const std::string& name)
{
    const auto actualName = name.empty() ? std::string("CONTINUOUS") : name;
    const auto key = dxf::resourceKey(actualName);
    if (const auto found = m_lineTypes.find(key);
        found != m_lineTypes.end())
    {
        return found->second;
    }
    yicad::plugin::ImportResource resource;
    const auto result = m_session.createLineType(
        yicad::plugin::LineTypeData(actualName),
        YICAD_RESOURCE_CONFLICT_REPLACE,
        resource);
    setFailed(result);
    if (result == YICAD_IMPORT_SUCCESS)
    {
        m_lineTypes.insert_or_assign(key, resource);
    }
    return resource;
}

yicad::plugin::ImportResource DxfImporter::ensureTextStyle(
    const std::string& name)
{
    const auto actualName = name.empty() ? std::string("STANDARD") : name;
    const auto key = dxf::resourceKey(actualName);
    if (const auto found = m_textStyles.find(key);
        found != m_textStyles.end())
    {
        return found->second;
    }
    yicad::plugin::ImportResource resource;
    auto data = yicad::plugin::TextStyleData(actualName)
        .setFontFiles("txt");
    const auto result = m_session.createTextStyle(
        data,
        YICAD_RESOURCE_CONFLICT_REPLACE,
        resource);
    setFailed(result);
    if (result == YICAD_IMPORT_SUCCESS)
    {
        m_textStyles.insert_or_assign(key, resource);
    }
    return resource;
}

yicad::plugin::ImportResource DxfImporter::ensureDimensionStyle(
    const std::string& name)
{
    const auto actualName = name.empty() ? std::string("STANDARD") : name;
    const auto key = dxf::resourceKey(actualName);
    if (const auto found = m_dimensionStyles.find(key);
        found != m_dimensionStyles.end())
    {
        return found->second;
    }
    const auto textStyle = ensureTextStyle("STANDARD");
    const auto lineType = ensureLineType("CONTINUOUS");
    yicad::plugin::ImportResource resource;
    auto data = yicad::plugin::DimensionStyleData(actualName)
        .setResources(textStyle, lineType, lineType);
    const auto result = m_session.createDimensionStyle(
        data,
        YICAD_RESOURCE_CONFLICT_REPLACE,
        resource);
    setFailed(result);
    if (result == YICAD_IMPORT_SUCCESS)
    {
        m_dimensionStyles.insert_or_assign(key, resource);
    }
    return resource;
}

bool DxfImporter::createInsert(
    const PendingInsert& pending,
    const yicad::plugin::ImportResource& block)
{
    auto data = yicad::plugin::InsertData(block)
        .setPlacement(
            pending.insertionPoint,
            pending.scale,
            pending.rotation)
        .setArray(
            pending.columnCount,
            pending.rowCount,
            pending.columnSpacing,
            pending.rowSpacing)
        .setAttributes(pending.attributes);
    yicad::plugin::ImportResource insert;
    const auto result = pending.container.createInsert(data, insert);
    setFailed(result);
    return result == YICAD_IMPORT_SUCCESS;
}

bool DxfImporter::resolvePendingInserts()
{
    for (const auto& pending : m_pendingInserts)
    {
        const auto block = m_blocks.find(pending.blockName);
        if (block == m_blocks.end() ||
            !m_finalizedBlocks.contains(pending.blockName) ||
            !createInsert(pending, block->second))
        {
            return false;
        }
    }
    return true;
}

bool DxfImporter::resolvePendingImages()
{
    for (const auto& pending : m_pendingImages)
    {
        const auto definition =
            m_imageDefinitions.find(pending.definitionHandle);
        if (definition == m_imageDefinitions.end())
        {
            return false;
        }
        auto data = yicad::plugin::ImageData(definition->second)
            .setGeometry(
                pending.insertionPoint,
                pending.uVector,
                pending.vVector,
                pending.size)
            .setDisplay(
                pending.brightness,
                pending.contrast,
                pending.fade)
            .setAttributes(pending.attributes);
        const auto result = m_modelSpace.createImage(data);
        setFailed(result);
        if (result != YICAD_IMPORT_SUCCESS)
        {
            return false;
        }
    }
    return true;
}

void DxfImporter::addClass(const DRW_Class& source)
{
    // 序号按段里的全部登记算（ACAD_PROXY_ENTITY 的组码 91 是序号加 500）
    m_classOrder.push_back(dxf::resourceKey(source.recName));
    if (source.recName.empty())
    {
        return;
    }
    ClassInfo info;
    info.className = source.className;
    info.appName = source.appName;
    info.proxyFlags = static_cast<uint32_t>(source.proxyFlag) & YICAD_PROXY_ALL;
    info.entity = source.entityFlag == 1;
    m_classes.insert_or_assign(dxf::resourceKey(source.recName), std::move(info));
}

void DxfImporter::addUnknownEntity(const DRW_UnknownEntity& source)
{
    if (m_failed || m_skipBlock || !m_currentContainer)
    {
        return;
    }
    dxf::ParsedCustomEntity parsed;
    if (!dxf::parseCustomEntity(source, parsed) || (parsed.paperSpace && !m_inBlock))
    {
        return;
    }
    // 类：ACAD_PROXY_ENTITY 按组码 91 的序号找，别的按记录名找
    std::string recordName = dxf::resourceKey(source.recordName);
    if (parsed.acadProxy)
    {
        const int index = parsed.proxyClassId - dxf::AcadProxyClassIdBase;
        if (recordName != dxf::AcadProxyRecordName || index < 0 || index >= static_cast<int>(m_classOrder.size()))
        {
            return;
        }
        recordName = m_classOrder[static_cast<std::size_t>(index)];
    }
    // 没在 CLASSES 段登记成实体的类型（libdxfrw 不认识的 AutoCAD 实体等）照旧跳过
    const auto found = m_classes.find(recordName);
    if (found == m_classes.end() || !found->second.entity || found->second.className.empty())
    {
        return;
    }
    const ClassInfo& info = found->second;
    const bool r2010 = m_reader != nullptr && m_reader->getVersion() >= DRW::AC1024;
    std::vector<dxf::ProxyShape> shapes;
    const bool planar = parsed.planar &&
        (parsed.graphics.empty() ||
         dxf::readProxyGraphics(parsed.graphics, m_layerOrder, m_lineTypeOrder, r2010, shapes));
    if (!parsed.yicad && !planar)
    {
        // 别的程序的三维实体：只读二维的（用户的决定），跳过
        return;
    }

    std::vector<uint8_t> data;
    uint32_t version = 0;
    if (parsed.yicad)
    {
        data = std::move(parsed.data);
        version = parsed.version;
    }
    else
    {
        dxf::ForeignEntityData foreign;
        foreign.recordName = recordName;
        foreign.appName = info.appName;
        // 原数据是 DWG 二进制的 ACAD_PROXY_ENTITY 整段保留；原数据是 DXF 组码的同普通记录
        foreign.acadProxy = parsed.acadProxy && !parsed.proxyDxfData;
        foreign.records = foreign.acadProxy ? std::move(parsed.proxyRecords) : std::move(parsed.subclassRecords);
        foreign.graphics = parsed.graphics;
        data = dxf::encodeForeign(foreign);
    }

    // 代理图形：类不在（插件没装、别的程序的类）时代理按它显示；类在时宿主丢掉它
    yicad::plugin::ImportContainer graphics;
    const bool hasGraphics = !shapes.empty() &&
        m_session.beginProxyGraphics(graphics) == YICAD_IMPORT_SUCCESS;
    if (hasGraphics)
    {
        for (const auto& shape : shapes)
        {
            createProxyShape(shape, graphics);
        }
    }

    DRW_Point common;
    common.layer = parsed.layer;
    common.lineType = parsed.lineType;
    common.color = parsed.color;
    common.color24 = parsed.color24;
    common.lWeight = DRW_LW_Conv::dxfInt2lineWidth(parsed.lineWeight);
    common.ltypeScale = parsed.lineTypeScale;
    common.visible = parsed.visible;
    if (dxf::resourceKey(parsed.lineType) != "BYLAYER")
    {
        ensureLineType(parsed.lineType);
    }
    yicad::plugin::CustomEntityData entity(info.className, version, std::move(data));
    entity.setProxyFlags(info.proxyFlags)
        .setTransform(parsed.transform)
        .setAttributes(dxf::toAttributes(common, m_layers, m_lineTypes));
    setFailed(m_currentContainer.createCustomEntity(entity, hasGraphics ? &graphics : nullptr));
}

void DxfImporter::createProxyShape(const dxf::ProxyShape& shape, const yicad::plugin::ImportContainer& graphics)
{
    // 代理图形里没给的属性沿用自定义实体自己的：颜色、线宽随块，图层、线型为空
    yicad::plugin::EntityAttributes attributes;
    const auto& given = shape.attributes;
    attributes.setColor(given.hasColor ? given.color : YiCadColorData{YICAD_COLOR_BY_BLOCK, 0, 0, 0, 0, 0});
    attributes.setLineWidth(given.hasLineWeight ? given.lineWeight : -2);
    attributes.setLineTypeScale(given.lineTypeScale);
    if (given.hasLayer)
    {
        if (const auto layer = m_layers.find(dxf::resourceKey(given.layer)); layer != m_layers.end())
        {
            attributes.setLayer(layer->second);
        }
    }
    if (given.hasLineType)
    {
        attributes.setLineType(ensureLineType(given.lineType));
    }
    YiCadImportResult result = YICAD_IMPORT_SUCCESS;
    switch (shape.kind)
    {
    case dxf::ProxyShape::Kind::Circle:
        result = graphics.createCircle(shape.center, shape.radius, attributes);
        break;
    case dxf::ProxyShape::Kind::Arc:
        result = graphics.createArc(shape.center, shape.radius, shape.startAngle, shape.endAngle, attributes);
        break;
    case dxf::ProxyShape::Kind::Polyline:
        if (shape.points.size() == 1)
        {
            result = graphics.createPoint(shape.points.front(), attributes);
        }
        else if (shape.points.size() == 2 && !shape.closed && shape.bulges.empty())
        {
            result = graphics.createLine(shape.points[0], shape.points[1], attributes);
        }
        else if (shape.points.size() >= 2)
        {
            std::vector<YiCadVertex2d> vertices;
            for (std::size_t i = 0; i < shape.points.size(); ++i)
            {
                vertices.push_back({shape.points[i], 0.0, 0.0, i < shape.bulges.size() ? shape.bulges[i] : 0.0});
            }
            result = graphics.createPolyline(
                yicad::plugin::PolylineData(std::move(vertices)).setClosed(shape.closed).setAttributes(attributes));
        }
        break;
    case dxf::ProxyShape::Kind::Fill:
    {
        yicad::plugin::HatchData hatch;
        hatch.setSolid();
        for (std::size_t i = 0; i < shape.loops.size(); ++i)
        {
            std::vector<YiCadVertex2d> loop;
            for (const auto& p : shape.loops[i])
            {
                loop.push_back({p, 0.0, 0.0, 0.0});
            }
            if (shape.holes[i])
            {
                hatch.addPolylineLoop(std::move(loop), YICAD_HATCH_LOOP_HOLE, 0);
            }
            else
            {
                hatch.addPolylineLoop(std::move(loop));
            }
        }
        hatch.setAttributes(attributes);
        result = graphics.createHatch(hatch);
        break;
    }
    case dxf::ProxyShape::Kind::Solid:
    {
        // 边界顺序换成 SOLID 的"Z"字顺序：1、2、4、3
        const auto& p = shape.points;
        yicad::plugin::SolidData solid(p.size() == 4 ? std::vector<YiCadPoint2d>{p[0], p[1], p[3], p[2]} : p);
        solid.setAttributes(attributes);
        result = graphics.createSolid(solid);
        break;
    }
    case dxf::ProxyShape::Kind::Text:
    {
        if (shape.text.empty() || !(shape.height > 0.0))
        {
            break;
        }
        yicad::plugin::TextData text(shape.text);
        text.setPlacement(shape.center, shape.center)
            .setMetrics(shape.height, shape.rotation, shape.widthFactor, shape.obliqueAngle)
            .setAttributes(attributes);
        if (!shape.font.empty())
        {
            if (const auto style = m_textStylesByFont.find(dxf::resourceKey(shape.font));
                style != m_textStylesByFont.end())
            {
                text.setStyle(style->second);
            }
        }
        result = graphics.createText(text);
        break;
    }
    case dxf::ProxyShape::Kind::Ray:
        result = graphics.createRay(shape.center, shape.majorAxis, attributes);
        break;
    case dxf::ProxyShape::Kind::XLine:
        result = graphics.createXLine(shape.center, shape.majorAxis, attributes);
        break;
    case dxf::ProxyShape::Kind::Ellipse:
    {
        const bool closed = std::fabs(shape.endAngle - shape.startAngle - 2.0 * Pi) < 1.0e-9 ||
                            std::fabs(shape.endAngle - shape.startAngle) < 1.0e-12;
        result = graphics.createEllipse(shape.center, shape.majorAxis, shape.ratio,
            closed ? 0.0 : shape.startAngle, closed ? 2.0 * Pi : shape.endAngle, closed, attributes);
        break;
    }
    }
    // 代理图形里个别图元建不成（退化的圆、空文字等）不影响导入：代理照样建，只是少画它
    (void)result;
}
