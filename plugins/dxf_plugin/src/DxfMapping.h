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

#include <libdxfrw/drw_entities.h>
#include <libdxfrw/drw_header.h>
#include <libdxfrw/drw_objects.h>

#include <string>
#include <unordered_map>

namespace dxf
{

using ResourceMap =
    std::unordered_map<std::string, yicad::plugin::ImportResource>;

std::string resourceKey(const std::string& value);
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

} // namespace dxf

#endif
