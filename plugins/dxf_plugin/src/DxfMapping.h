/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef YICAD_DXF_MAPPING_H
#define YICAD_DXF_MAPPING_H

#include "YiCadPluginSdk.h"

#include <drw_entities.h>
#include <drw_header.h>
#include <drw_objects.h>
#include <libdxfrw.h>

#include <string>
#include <unordered_map>

namespace dxf
{

using ResourceMap =
    std::unordered_map<std::string, yicad::plugin::ImportResource>;

std::string resourceKey(const std::string& value);
/// @brief 判断块名称是否为 YiCAD 或 DXF 内置箭头块
bool isInternalArrowBlock(const std::string& blockName);
YiCadPoint2d point(const DRW_Coord& value) noexcept;
YiCadVector2d vector(const DRW_Coord& value) noexcept;
YiCadColorData color(int aci, int color24) noexcept;
int32_t lineWidth(DRW_LW_Conv::lineWidth value) noexcept;

bool headerInt(const DRW_Header& header, const char* name, int32_t& value);
bool headerDouble(const DRW_Header& header, const char* name, double& value);
bool headerString(
    const DRW_Header& header,
    const char* name,
    std::string& value);

yicad::plugin::EntityAttributes toAttributes(
    const DRW_Entity& value,
    const ResourceMap& layers,
    const ResourceMap& lineTypes);
yicad::plugin::LineTypeData toLineType(const DRW_LType& value);
yicad::plugin::LayerData toLayer(
    const DRW_Layer& value,
    const ResourceMap& lineTypes);
yicad::plugin::TextStyleData toTextStyle(const DRW_Textstyle& value);
yicad::plugin::DimensionStyleData toDimensionStyle(
    const DRW_Dimstyle& value,
    const yicad::plugin::ImportResource& textStyle,
    const yicad::plugin::ImportResource& lineType);
yicad::plugin::TextData toText(
    const DRW_Text& value,
    const ResourceMap& layers,
    const ResourceMap& lineTypes,
    const ResourceMap& textStyles);
yicad::plugin::InsertData toInsert(
    const DRW_Insert& value,
    const yicad::plugin::ImportResource& block,
    const ResourceMap& layers,
    const ResourceMap& lineTypes);
yicad::plugin::SolidData toSolid(
    const DRW_Trace& value,
    const ResourceMap& layers,
    const ResourceMap& lineTypes);

void writeHeader(
    const yicad::plugin::DocumentSettings& settings,
    DRW_Header& header);
DRW_LType toDxf(const yicad::plugin::LineTypeData& value);
DRW_Layer toDxf(const yicad::plugin::LayerData& value);
DRW_Textstyle toDxf(const yicad::plugin::TextStyleData& value);
DRW_Dimstyle toDxf(const yicad::plugin::DimensionStyleData& value);
DRW_Block toDxf(const yicad::plugin::BlockData& value);

bool writeEntity(dxfRW& writer, const yicad::plugin::PointData& value);
bool writeEntity(dxfRW& writer, const yicad::plugin::LineData& value);
bool writeEntity(dxfRW& writer, const yicad::plugin::RayData& value);
bool writeEntity(dxfRW& writer, const yicad::plugin::XLineData& value);
bool writeEntity(dxfRW& writer, const yicad::plugin::ArcData& value);
bool writeEntity(dxfRW& writer, const yicad::plugin::CircleData& value);
bool writeEntity(dxfRW& writer, const yicad::plugin::EllipseData& value);
bool writeEntity(dxfRW& writer, const yicad::plugin::PolylineData& value);
bool writeEntity(dxfRW& writer, const yicad::plugin::SplineData& value);
bool writeEntity(dxfRW& writer, const yicad::plugin::SolidData& value);
bool writeEntity(dxfRW& writer, const yicad::plugin::TextData& value);
bool writeEntity(dxfRW& writer, const yicad::plugin::MTextData& value);
bool writeEntity(dxfRW& writer, const yicad::plugin::DimensionData& value);
bool writeEntity(dxfRW& writer, const yicad::plugin::LeaderData& value);
bool writeEntity(dxfRW& writer, const yicad::plugin::HatchData& value);
bool writeEntity(dxfRW& writer, const yicad::plugin::InsertData& value);
bool writeEntity(
    dxfRW& writer,
    const yicad::plugin::AttributeDefinitionData& value);
bool writeEntity(dxfRW& writer, const yicad::plugin::AttributeData& value);
bool writeEntity(dxfRW& writer, const yicad::plugin::ImageData& value);

} // namespace dxf

#endif
