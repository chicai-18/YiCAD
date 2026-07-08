/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "DxfMapping.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace dxf
{
namespace
{

constexpr double DegreesPerRadian = 57.2957795130823208768;

const yicad::plugin::ImportResource* findResource(
    const ResourceMap& resources,
    const std::string& name)
{
    const auto found = resources.find(resourceKey(name));
    return found == resources.end() ? nullptr : &found->second;
}

YiCadColorData tableColor(int value) noexcept
{
    value = std::abs(value);
    if (value >= 1 && value <= 255)
    {
        return {YICAD_COLOR_ACI, static_cast<uint32_t>(value), 0, 0, 0, 0};
    }
    return {YICAD_COLOR_BY_LAYER, 0, 0, 0, 0, 0};
}

YiCadColorData styleColor(int value) noexcept
{
    if (value == DRW::ColorByBlock)
    {
        return {YICAD_COLOR_BY_BLOCK, 0, 0, 0, 0, 0};
    }
    if (value == DRW::ColorByLayer)
    {
        return {YICAD_COLOR_BY_LAYER, 0, 0, 0, 0, 0};
    }
    return tableColor(value);
}

int32_t arrowType(const std::string& name)
{
    static const std::unordered_map<std::string, int32_t> arrows{
        {"", 0},
        {"_CLOSEDBLANK", 1},
        {"_CLOSED", 2},
        {"_DOT", 3},
        {"_ARCHTICK", 4},
        {"_OBLIQUE", 5},
        {"_OPEN", 6},
        {"_ORIGIN", 7},
        {"_ORIGIN2", 8},
        {"_OPEN90", 9},
        {"_OPEN30", 10},
        {"_DOTSMALL", 11},
        {"_DOTBLANK", 12},
        {"_SMALL", 13},
        {"_BOXBLANK", 14},
        {"_BOXFILLED", 15},
        {"_DATUMBLANK", 16},
        {"_DATUMFILLED", 17},
        {"_INTEGRAL", 18},
        {"_NONE", 19},
    };
    const auto found = arrows.find(resourceKey(name));
    return found == arrows.end() ? 0 : found->second;
}

int32_t decimalSeparator(int value) noexcept
{
    if (value == ',')
    {
        return 1;
    }
    if (value == ' ')
    {
        return 2;
    }
    return 0;
}

std::pair<std::string, std::string> dimensionAffixes(
    const std::string& value)
{
    const auto marker = value.find("<>");
    if (marker == std::string::npos)
    {
        return {"", value};
    }
    return {value.substr(0, marker), value.substr(marker + 2)};
}

} // namespace

std::string resourceKey(const std::string& value)
{
    std::string result = value;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::toupper(character));
        });
    return result;
}

YiCadPoint2d point(const DRW_Coord& value) noexcept
{
    return {value.x, value.y};
}

YiCadVector2d vector(const DRW_Coord& value) noexcept
{
    return {value.x, value.y};
}

YiCadColorData color(int aci, int color24) noexcept
{
    if (color24 >= 0)
    {
        return {
            YICAD_COLOR_RGB,
            0,
            static_cast<uint8_t>((color24 >> 16) & 0xff),
            static_cast<uint8_t>((color24 >> 8) & 0xff),
            static_cast<uint8_t>(color24 & 0xff),
            0};
    }
    if (aci == DRW::ColorByBlock)
    {
        return {YICAD_COLOR_BY_BLOCK, 0, 0, 0, 0, 0};
    }
    if (aci == DRW::ColorByLayer)
    {
        return {YICAD_COLOR_BY_LAYER, 0, 0, 0, 0, 0};
    }
    return tableColor(aci);
}

int32_t lineWidth(DRW_LW_Conv::lineWidth value) noexcept
{
    return DRW_LW_Conv::lineWidth2dxfInt(value);
}

bool headerInt(const DRW_Header& header, const char* name, int32_t& value)
{
    const auto found = header.vars.find(name);
    if (found == header.vars.end() || found->second == nullptr ||
        found->second->type() != DRW_Variant::INTEGER)
    {
        return false;
    }
    value = found->second->content.i;
    return true;
}

bool headerDouble(const DRW_Header& header, const char* name, double& value)
{
    const auto found = header.vars.find(name);
    if (found == header.vars.end() || found->second == nullptr)
    {
        return false;
    }
    if (found->second->type() == DRW_Variant::DOUBLE)
    {
        value = found->second->content.d;
        return true;
    }
    if (found->second->type() == DRW_Variant::INTEGER)
    {
        value = found->second->content.i;
        return true;
    }
    return false;
}

bool headerString(
    const DRW_Header& header,
    const char* name,
    std::string& value)
{
    const auto found = header.vars.find(name);
    if (found == header.vars.end() || found->second == nullptr ||
        found->second->type() != DRW_Variant::STRING ||
        found->second->content.s == nullptr)
    {
        return false;
    }
    value = *found->second->content.s;
    return true;
}

yicad::plugin::EntityAttributes toAttributes(
    const DRW_Entity& value,
    const ResourceMap& layers,
    const ResourceMap& lineTypes)
{
    yicad::plugin::EntityAttributes result;
    if (const auto* layer = findResource(layers, value.layer))
    {
        result.setLayer(*layer);
    }
    const auto lineTypeKey = resourceKey(value.lineType);
    if (lineTypeKey != "BYLAYER" && lineTypeKey != "BYBLOCK")
    {
        if (const auto* lineType = findResource(lineTypes, value.lineType))
        {
            result.setLineType(*lineType);
        }
    }
    result.setColor(color(value.color, value.color24))
        .setLineWidth(lineWidth(value.lWeight))
        .setVisible(value.visible);
    return result;
}

yicad::plugin::LineTypeData toLineType(const DRW_LType& value)
{
    return yicad::plugin::LineTypeData(value.name)
        .setDescription(value.desc)
        .setElements(value.path);
}

yicad::plugin::LayerData toLayer(
    const DRW_Layer& value,
    const ResourceMap& lineTypes)
{
    yicad::plugin::LayerData result(value.name);
    result.setFrozen((value.flags & 1) != 0)
        .setLocked((value.flags & 4) != 0)
        .setPlottable(value.plotF)
        .setColor(color(std::abs(value.color), value.color24))
        .setLineWidth(lineWidth(value.lWeight));
    if (const auto* lineType = findResource(lineTypes, value.lineType))
    {
        result.setLineType(*lineType);
    }
    return result;
}

yicad::plugin::TextStyleData toTextStyle(const DRW_Textstyle& value)
{
    auto font = value.font.empty() ? std::string("txt") : value.font;
    uint32_t flags = 0;
    if ((value.genFlag & 2) != 0)
    {
        flags |= YICAD_TEXT_GENERATION_BACKWARD;
    }
    if ((value.genFlag & 4) != 0)
    {
        flags |= YICAD_TEXT_GENERATION_UPSIDE_DOWN;
    }
    return yicad::plugin::TextStyleData(value.name)
        .setFontFiles(std::move(font), value.bigFont)
        .setMetrics(value.height, value.width, value.oblique / DegreesPerRadian)
        .setGenerationFlags(flags);
}

yicad::plugin::DimensionStyleData toDimensionStyle(
    const DRW_Dimstyle& value,
    const yicad::plugin::ImportResource& textStyle,
    const yicad::plugin::ImportResource& lineType)
{
    const auto [prefix, suffix] = dimensionAffixes(value.dimpost);
    const auto linearFormat =
        value.dimlunit >= 1 && value.dimlunit <= 6 ? value.dimlunit - 1 : 1;
    const auto angularFormat =
        value.dimaunit >= 0 && value.dimaunit <= 3 ? value.dimaunit : 0;
    return yicad::plugin::DimensionStyleData(value.name)
        .setResources(textStyle, lineType, lineType)
        .setColors(
            styleColor(value.dimclrd),
            styleColor(value.dimclre),
            styleColor(value.dimclrt),
            {YICAD_COLOR_BY_LAYER, 0, 0, 0, 0, 0})
        .setLineWidths(
            DRW_LW_Conv::lineWidth2dxfInt(
                DRW_LW_Conv::dxfInt2lineWidth(value.dimlwd)),
            DRW_LW_Conv::lineWidth2dxfInt(
                DRW_LW_Conv::dxfInt2lineWidth(value.dimlwe)))
        .setLineSuppression(
            value.dimsd1 != 0,
            value.dimsd2 != 0,
            value.dimse1 != 0,
            value.dimse2 != 0)
        .setExtensionGeometry(
            std::max(0.0, value.dimexe),
            std::max(0.0, value.dimexo),
            value.dimfxlon != 0,
            std::max(0.0, value.dimfxl))
        .setArrows(
            arrowType(value.dimblk1),
            arrowType(value.dimblk2),
            arrowType(value.dimldrblk),
            std::max(0.0, value.dimasz))
        .setTextLayout(
            std::max(0.0, value.dimtxt),
            value.dimtfac > 0.0 ? value.dimtfac : 1.0,
            false,
            std::clamp(value.dimtad, 0, 4),
            std::clamp(value.dimjust, 0, 4),
            value.dimupt != 0 ? 1 : 0,
            std::max(0.0, value.dimgap))
        .setLinearFormat(
            linearFormat,
            std::clamp(value.dimdec, 0, 8),
            std::clamp(value.dimfrac, 0, 2),
            decimalSeparator(value.dimdsep),
            std::max(0.0, value.dimrnd),
            prefix,
            suffix,
            value.dimlfac > 0.0 ? value.dimlfac : 1.0,
            (value.dimzin & 4) != 0,
            (value.dimzin & 8) != 0)
        .setAngularFormat(
            angularFormat,
            std::clamp(value.dimadec, 0, 8),
            (value.dimazin & 1) != 0,
            (value.dimazin & 2) != 0);
}

yicad::plugin::TextData toText(
    const DRW_Text& value,
    const ResourceMap& layers,
    const ResourceMap& lineTypes,
    const ResourceMap& textStyles)
{
    yicad::plugin::TextData result(value.text);
    result.setPlacement(point(value.basePoint), point(value.secPoint))
        .setMetrics(
            value.height,
            value.angle / DegreesPerRadian,
            value.widthscale,
            value.oblique / DegreesPerRadian)
        .setAlignment(
            static_cast<YiCadTextHorizontalAlignment>(
                std::clamp(static_cast<int>(value.alignH), 0, 5)),
            static_cast<YiCadTextVerticalAlignment>(
                std::clamp(static_cast<int>(value.alignV), 0, 3)))
        .setAttributes(toAttributes(value, layers, lineTypes));
    if (const auto* style = findResource(textStyles, value.style))
    {
        result.setStyle(*style);
    }
    return result;
}

yicad::plugin::InsertData toInsert(
    const DRW_Insert& value,
    const yicad::plugin::ImportResource& block,
    const ResourceMap& layers,
    const ResourceMap& lineTypes)
{
    return yicad::plugin::InsertData(block)
        .setPlacement(
            point(value.basePoint),
            {value.xscale, value.yscale, 1.0},
            value.angle)
        .setArray(
            static_cast<uint32_t>(std::max(1, value.colcount)),
            static_cast<uint32_t>(std::max(1, value.rowcount)),
            value.colspace,
            value.rowspace)
        .setAttributes(toAttributes(value, layers, lineTypes));
}

yicad::plugin::SolidData toSolid(
    const DRW_Trace& value,
    const ResourceMap& layers,
    const ResourceMap& lineTypes)
{
    std::vector<YiCadPoint2d> corners{
        point(value.basePoint),
        point(value.secPoint),
    };
    if (value.thirdPoint.x == value.fourPoint.x &&
        value.thirdPoint.y == value.fourPoint.y)
    {
        corners.push_back(point(value.thirdPoint));
    }
    else
    {
        corners.push_back(point(value.fourPoint));
        corners.push_back(point(value.thirdPoint));
    }
    return yicad::plugin::SolidData(std::move(corners))
        .setAttributes(toAttributes(value, layers, lineTypes));
}

} // namespace dxf
