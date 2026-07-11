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
#include <memory>
#include <optional>
#include <unordered_set>
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

bool isInternalArrowBlock(const std::string& blockName)
{
    static const std::unordered_set<std::string> names{
        "_CLOSEDFILLED", "_CLOSEDBLANK", "_CLOSED", "_DOT",
        "_ARCHITECTURALTICK", "_ARCHTICK", "_OBLIQUE", "_OPEN",
        "_ORIGININDICATOR", "_ORIGIN", "_ORIGININDICATOR2", "_ORIGIN2",
        "_RIGHTANGLE", "_OPEN90", "_OPEN30", "_DOTSMALL", "_DOTBLANK",
        "_DOTSMALLBLANK", "_SMALL", "_BOX", "_BOXBLANK", "_BOXFILLED",
        "_DATUMTRIANGLE", "_DATUMBLANK", "_DATUMTRIANGLEFILLED",
        "_DATUMFILLED", "_INTEGRAL", "_NONE",
    };
    return names.find(resourceKey(blockName)) != names.end();
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

namespace
{

constexpr double ExportPi = 3.14159265358979323846;

DRW_Coord coord(YiCadPoint2d value, double z = 0.0) noexcept
{
    return {value.x, value.y, z};
}

std::string nameOr(
    const std::string& value,
    const char* fallback)
{
    return value.empty() ? std::string(fallback) : value;
}

int aciColor(YiCadColorData value, int fallback = DRW::ColorByLayer) noexcept
{
    if (value.method == YICAD_COLOR_BY_BLOCK)
    {
        return DRW::ColorByBlock;
    }
    if (value.method == YICAD_COLOR_BY_LAYER)
    {
        return fallback;
    }
    if (value.method == YICAD_COLOR_ACI && value.aci >= 1 && value.aci <= 255)
    {
        return static_cast<int>(value.aci);
    }
    return fallback;
}

int color24(YiCadColorData value) noexcept
{
    if (value.method != YICAD_COLOR_RGB)
    {
        return -1;
    }
    return (static_cast<int>(value.red) << 16) |
           (static_cast<int>(value.green) << 8) |
           static_cast<int>(value.blue);
}

void setColor(DRW_Entity& target, YiCadColorData value) noexcept
{
    target.color = aciColor(value);
    target.color24 = color24(value);
}

template<typename Entity>
Entity& setAttributes(
    Entity& target,
    const yicad::plugin::EntityAttributes& attributes)
{
    const auto& data = attributes.abiData();
    target.layer = nameOr(attributes.layerName(), "0");
    target.lineType = nameOr(attributes.lineTypeName(), "BYLAYER");
    target.lWeight = DRW_LW_Conv::dxfInt2lineWidth(data.lineWidth);
    target.ltypeScale = data.lineTypeScale;
    target.visible = data.visible != 0;
    setColor(target, data.color);
    return target;
}

double degrees(double radians) noexcept
{
    return radians * DegreesPerRadian;
}

std::string arrowName(int32_t value)
{
    switch (value)
    {
    case 1: return "_CLOSEDBLANK";
    case 2: return "_CLOSED";
    case 3: return "_DOT";
    case 4: return "_ARCHTICK";
    case 5: return "_OBLIQUE";
    case 6: return "_OPEN";
    case 7: return "_ORIGIN";
    case 8: return "_ORIGIN2";
    case 9: return "_OPEN90";
    case 10: return "_OPEN30";
    case 11: return "_DOTSMALL";
    case 12: return "_DOTBLANK";
    case 13: return "_SMALL";
    case 14: return "_BOXBLANK";
    case 15: return "_BOXFILLED";
    case 16: return "_DATUMBLANK";
    case 17: return "_DATUMFILLED";
    case 18: return "_INTEGRAL";
    case 19: return "_NONE";
    default: return {};
    }
}

int32_t dxfDecimalSeparator(int32_t value) noexcept
{
    if (value == 1)
    {
        return ',';
    }
    if (value == 2)
    {
        return ' ';
    }
    return '.';
}

int32_t dxfLinearUnit(int32_t value) noexcept
{
    return std::clamp(value + 1, 1, 6);
}

std::string dimpost(const yicad::plugin::DimensionStyleData& value)
{
    if (value.prefix().empty() && value.suffix().empty())
    {
        return {};
    }
    return value.prefix() + "<>" + value.suffix();
}

DRW_Text textEntity(const yicad::plugin::TextData& value)
{
    DRW_Text result;
    setAttributes(result, value.attributes());
    result.basePoint = coord(value.insertionPoint());
    result.secPoint = coord(value.alignmentPoint());
    result.text = value.text();
    result.height = value.height();
    result.angle = degrees(value.rotation());
    result.widthscale = value.widthFactor();
    result.oblique = degrees(value.obliqueAngle());
    result.style = nameOr(value.textStyleName(), "STANDARD");
    result.alignH = static_cast<DRW_Text::HAlign>(
        std::clamp(static_cast<int>(value.horizontalAlignment()), 0, 5));
    result.alignV = static_cast<DRW_Text::VAlign>(
        std::clamp(static_cast<int>(value.verticalAlignment()), 0, 3));
    return result;
}

void fillCommonDimension(
    DRW_Dimension& target,
    const yicad::plugin::DimensionData& value)
{
    setAttributes(target, value.attributes());
    target.setStyle(nameOr(value.styleName(), "STANDARD"));
    target.setText(value.textOverride());
    target.setTextPoint(coord(value.textPosition()));
    target.setDir(degrees(value.textRotation()));
    target.setTextLineFactor(value.lineSpacingFactor());
    target.setExtrusion({0.0, 0.0, 1.0});
}

std::shared_ptr<DRW_Entity> hatchLine(
    YiCadPoint2d startPoint,
    YiCadPoint2d endPoint)
{
    auto result = std::make_shared<DRW_Line>();
    result->basePoint = coord(startPoint);
    result->secPoint = coord(endPoint);
    return result;
}

std::shared_ptr<DRW_Entity> hatchEdge(
    const yicad::plugin::HatchData::Edge& value)
{
    switch (value.type())
    {
    case YICAD_HATCH_EDGE_LINE:
        return hatchLine(value.startPoint(), value.endPoint());
    case YICAD_HATCH_EDGE_CIRCULAR_ARC:
    {
        auto result = std::make_shared<DRW_Arc>();
        result->basePoint = coord(value.center());
        result->radious = value.radius();
        result->staangle = value.startParameter();
        result->endangle = value.endParameter();
        result->isccw = value.counterClockwise() ? 1 : 0;
        return result;
    }
    case YICAD_HATCH_EDGE_ELLIPTIC_ARC:
    {
        auto result = std::make_shared<DRW_Ellipse>();
        result->basePoint = coord(value.center());
        result->secPoint = coord(value.majorAxis());
        result->ratio = value.minorToMajorRatio();
        result->staparam = value.startParameter();
        result->endparam = value.endParameter();
        result->isccw = value.counterClockwise() ? 1 : 0;
        return result;
    }
    default:
        return {};
    }
}

std::shared_ptr<DRW_HatchLoop> hatchLoop(
    const yicad::plugin::HatchData::Loop& source)
{
    auto loop = std::make_shared<DRW_HatchLoop>(
        source.role == YICAD_HATCH_LOOP_OUTER ? 1 : 0);
    if (source.kind == YICAD_HATCH_LOOP_POLYLINE)
    {
        if (source.vertices.size() < 3)
        {
            return {};
        }
        for (std::size_t index = 0; index < source.vertices.size(); ++index)
        {
            const auto& start = source.vertices[index];
            const auto& end = source.vertices[(index + 1) % source.vertices.size()];
            loop->objlist.push_back(hatchLine(start.position, end.position));
        }
        return loop;
    }

    for (const auto& edge : source.edges)
    {
        auto entity = hatchEdge(edge);
        if (entity == nullptr)
        {
            return {};
        }
        loop->objlist.push_back(std::move(entity));
    }
    return loop->objlist.empty() ? nullptr : loop;
}

} // namespace

void writeHeader(
    const yicad::plugin::DocumentSettings& settings,
    DRW_Header& header)
{
    header.addInt("$INSUNITS", settings.insertionUnits(), 70);
    header.addInt("$MEASUREMENT", settings.measurement(), 70);
    header.addDouble("$LTSCALE", settings.globalLineTypeScale(), 40);
    if (!settings.sourceCodePage().empty())
    {
        header.addStr("$DWGCODEPAGE", settings.sourceCodePage(), 3);
    }
}

DRW_LType toDxf(const yicad::plugin::LineTypeData& value)
{
    DRW_LType result;
    result.name = value.name();
    result.desc = value.description();
    result.path = value.elements();
    result.size = static_cast<int>(result.path.size());
    return result;
}

DRW_Layer toDxf(const yicad::plugin::LayerData& value)
{
    DRW_Layer result;
    result.name = value.name();
    result.flags = (value.frozen() ? 1 : 0) | (value.locked() ? 4 : 0);
    result.lineType = nameOr(value.lineTypeName(), "CONTINUOUS");
    result.color = aciColor(value.color(), 7);
    result.color24 = color24(value.color());
    result.plotF = value.plottable();
    result.lWeight = DRW_LW_Conv::dxfInt2lineWidth(value.lineWidth());
    return result;
}

DRW_Textstyle toDxf(const yicad::plugin::TextStyleData& value)
{
    DRW_Textstyle result;
    result.name = value.name();
    result.font = value.fontFile().empty() ? "txt" : value.fontFile();
    result.bigFont = value.bigFontFile();
    result.height = value.fixedHeight();
    result.width = value.widthFactor();
    result.oblique = degrees(value.obliqueAngle());
    result.genFlag = static_cast<int>(value.generationFlags());
    return result;
}

DRW_Dimstyle toDxf(const yicad::plugin::DimensionStyleData& value)
{
    const auto& data = value.abiData();
    DRW_Dimstyle result;
    result.name = value.name();
    result.dimpost = dimpost(value);
    result.dimtxsty = nameOr(value.textStyleName(), "STANDARD");
    result.dimblk1 = arrowName(data.firstArrow);
    result.dimblk2 = arrowName(data.secondArrow);
    result.dimldrblk = arrowName(data.leaderArrow);
    result.dimasz = data.arrowSize;
    result.dimexo = data.extensionOriginOffset;
    result.dimexe = data.extensionBeyondDimLine;
    result.dimfxlon = data.fixedExtensionLineLengthEnabled != 0 ? 1 : 0;
    result.dimfxl = data.fixedExtensionLineLength;
    result.dimtxt = data.textHeight;
    result.dimlfac = data.measurementScale;
    result.dimtfac = data.fractionHeightScale;
    result.dimgap = data.textOffset;
    result.dimsd1 = data.hideDimLine1 != 0 ? 1 : 0;
    result.dimsd2 = data.hideDimLine2 != 0 ? 1 : 0;
    result.dimse1 = data.hideExtensionLine1 != 0 ? 1 : 0;
    result.dimse2 = data.hideExtensionLine2 != 0 ? 1 : 0;
    result.dimtad = data.textVerticalPosition;
    result.dimjust = data.textHorizontalPosition;
    result.dimupt = data.textDirection != 0 ? 1 : 0;
    result.dimclrd = aciColor(data.dimLineColor);
    result.dimclre = aciColor(data.extensionLineColor);
    result.dimclrt = aciColor(data.textColor);
    result.dimdec = data.linearPrecision;
    result.dimfrac = data.fractionFormat;
    result.dimlunit = dxfLinearUnit(data.linearUnitFormat);
    result.dimdsep = dxfDecimalSeparator(data.decimalSeparator);
    result.dimrnd = data.roundOff;
    result.dimlfac = data.measurementScale;
    result.dimzin = (data.suppressLeadingZeros ? 4 : 0) |
        (data.suppressTrailingZeros ? 8 : 0);
    result.dimaunit = data.angularUnitFormat;
    result.dimadec = data.angularPrecision;
    result.dimazin = (data.suppressAngularLeadingZeros ? 1 : 0) |
        (data.suppressAngularTrailingZeros ? 2 : 0);
    result.dimlwd = data.dimLineWidth;
    result.dimlwe = data.extensionLineWidth;
    return result;
}

DRW_Block toDxf(const yicad::plugin::BlockData& value)
{
    DRW_Block result;
    result.name = value.name();
    result.basePoint = coord(value.basePoint());
    result.flags = static_cast<int>(value.flags());
    return result;
}

bool writeEntity(dxfRW& writer, const yicad::plugin::PointData& value)
{
    DRW_Point data;
    setAttributes(data, value.attributes);
    data.basePoint = coord(value.position);
    return writer.writePoint(&data);
}

bool writeEntity(dxfRW& writer, const yicad::plugin::LineData& value)
{
    DRW_Line data;
    setAttributes(data, value.attributes);
    data.basePoint = coord(value.startPoint);
    data.secPoint = coord(value.endPoint);
    return writer.writeLine(&data);
}

bool writeEntity(dxfRW& writer, const yicad::plugin::RayData& value)
{
    DRW_Ray data;
    setAttributes(data, value.attributes);
    data.basePoint = coord(value.basePoint);
    data.secPoint = coord(value.direction);
    return writer.writeRay(&data);
}

bool writeEntity(dxfRW& writer, const yicad::plugin::XLineData& value)
{
    DRW_Xline data;
    setAttributes(data, value.attributes);
    data.basePoint = coord(value.basePoint);
    data.secPoint = coord(value.direction);
    return writer.writeXline(&data);
}

bool writeEntity(dxfRW& writer, const yicad::plugin::ArcData& value)
{
    DRW_Arc data;
    setAttributes(data, value.attributes);
    data.basePoint = coord(value.center);
    data.radious = value.radius;
    data.staangle = value.startAngle;
    data.endangle = value.endAngle;
    return writer.writeArc(&data);
}

bool writeEntity(dxfRW& writer, const yicad::plugin::CircleData& value)
{
    DRW_Circle data;
    setAttributes(data, value.attributes);
    data.basePoint = coord(value.center);
    data.radious = value.radius;
    return writer.writeCircle(&data);
}

bool writeEntity(dxfRW& writer, const yicad::plugin::EllipseData& value)
{
    DRW_Ellipse data;
    setAttributes(data, value.attributes);
    data.basePoint = coord(value.center);
    data.secPoint = coord(value.majorAxis);
    data.ratio = value.minorToMajorRatio;
    data.staparam = value.closed ? 0.0 : value.startParameter;
    data.endparam = value.closed ? 2.0 * ExportPi : value.endParameter;
    return writer.writeEllipse(&data);
}

bool writeEntity(dxfRW& writer, const yicad::plugin::PolylineData& value)
{
    DRW_LWPolyline data;
    setAttributes(data, value.attributes());
    data.flags = value.closed() ? 1 : 0;
    for (const auto& source : value.vertices())
    {
        DRW_Vertex2D vertex;
        vertex.x = source.position.x;
        vertex.y = source.position.y;
        vertex.stawidth = source.startWidth;
        vertex.endwidth = source.endWidth;
        vertex.bulge = source.bulge;
        data.addVertex(vertex);
    }
    return writer.writeLWPolyline(&data);
}

bool writeEntity(dxfRW& writer, const yicad::plugin::SplineData& value)
{
    if (value.definition() == YICAD_SPLINE_FIT_POINTS)
    {
        return true;
    }

    DRW_Spline data;
    setAttributes(data, value.attributes());
    data.normalVec = {0.0, 0.0, 1.0};
    data.flags = (value.closed() ? 1 : 0) |
        (value.periodic() ? 2 : 0) |
        (value.rational() ? 4 : 0);
    data.degree = static_cast<int>(value.degree());
    data.knotslist = value.knots();
    data.weightlist = value.weights();
    data.nknots = static_cast<dint32>(data.knotslist.size());
    data.ncontrol = static_cast<dint32>(value.controlPoints().size());
    for (const auto& pointValue : value.controlPoints())
    {
        data.controllist.push_back(
            std::make_shared<DRW_Coord>(coord(pointValue)));
    }
    return writer.writeSpline(&data);
}

bool writeEntity(dxfRW& writer, const yicad::plugin::SolidData& value)
{
    if (value.corners().size() < 3)
    {
        return true;
    }
    DRW_Solid data;
    setAttributes(data, value.attributes());
    data.basePoint = coord(value.corners()[0]);
    data.secPoint = coord(value.corners()[1]);
    data.thirdPoint = coord(value.corners().size() == 3
        ? value.corners()[2] : value.corners()[3]);
    data.fourPoint = coord(value.corners().size() == 3
        ? value.corners()[2] : value.corners()[2]);
    return writer.writeSolid(&data);
}

bool writeEntity(dxfRW& writer, const yicad::plugin::TextData& value)
{
    auto data = textEntity(value);
    return writer.writeText(&data);
}

bool writeEntity(dxfRW& writer, const yicad::plugin::MTextData& value)
{
    DRW_MText data;
    setAttributes(data, value.attributes());
    data.basePoint = coord(value.insertionPoint());
    data.secPoint = coord(value.direction());
    data.text = value.contents();
    data.height = value.characterHeight();
    data.widthscale = value.rectangleWidth();
    data.interlin = value.lineSpacingFactor();
    data.textgen = static_cast<int>(value.attachment());
    // libdxfrw 复用 DRW_Text 字段写入 MTEXT 的组码 72 和 73。
    data.alignH = static_cast<DRW_Text::HAlign>(1);
    data.alignV = static_cast<DRW_Text::VAlign>(1);
    data.angle = degrees(std::atan2(value.direction().y, value.direction().x));
    data.style = nameOr(value.textStyleName(), "STANDARD");
    return writer.writeMText(&data);
}

bool writeEntity(dxfRW& writer, const yicad::plugin::DimensionData& value)
{
    std::unique_ptr<DRW_Dimension> data;
    switch (value.kind())
    {
    case YICAD_DIMENSION_LINEAR:
    {
        auto result = std::make_unique<DRW_DimLinear>();
        result->type = 0;
        result->setDimPoint(coord(value.definitionPoint()));
        result->setDef1Point(coord(value.extensionPoint1()));
        result->setDef2Point(coord(value.extensionPoint2()));
        data = std::move(result);
        break;
    }
    case YICAD_DIMENSION_ALIGNED:
    {
        auto result = std::make_unique<DRW_DimAligned>();
        result->type = 1;
        result->setDimPoint(coord(value.definitionPoint()));
        result->setDef1Point(coord(value.extensionPoint1()));
        result->setDef2Point(coord(value.extensionPoint2()));
        data = std::move(result);
        break;
    }
    case YICAD_DIMENSION_ANGULAR:
    {
        auto result = std::make_unique<DRW_DimAngular>();
        result->type = 2;
        result->setFirstLine1(coord(value.line1Start()));
        result->setFirstLine2(coord(value.line1End()));
        result->setSecondLine1(coord(value.line2Start()));
        result->setSecondLine2(coord(value.line2End()));
        result->setDimPoint(coord(value.arcPoint()));
        data = std::move(result);
        break;
    }
    case YICAD_DIMENSION_RADIAL:
    {
        auto result = std::make_unique<DRW_DimRadial>();
        result->type = 4;
        result->setCenterPoint(coord(value.definitionPoint()));
        result->setDiameterPoint(coord(value.featurePoint()));
        result->setLeaderLength(value.leaderLength());
        data = std::move(result);
        break;
    }
    case YICAD_DIMENSION_DIAMETRIC:
    {
        auto result = std::make_unique<DRW_DimDiametric>();
        result->type = 3;
        result->setDiameter2Point(coord(value.definitionPoint()));
        result->setDiameter1Point(coord(value.featurePoint()));
        result->setLeaderLength(value.leaderLength());
        data = std::move(result);
        break;
    }
    default:
        return true;
    }

    fillCommonDimension(*data, value);
    return writer.writeDimension(data.get());
}

bool writeEntity(dxfRW& writer, const yicad::plugin::LeaderData& value)
{
    if (value.vertices().size() < 2)
    {
        return true;
    }
    DRW_Leader data;
    setAttributes(data, value.attributes());
    data.style = nameOr(value.styleName(), "STANDARD");
    data.arrow = value.hasArrow() ? 1 : 0;
    for (const auto& pointValue : value.vertices())
    {
        data.vertexlist.push_back(
            std::make_shared<DRW_Coord>(coord(pointValue)));
    }
    data.vertnum = static_cast<int>(data.vertexlist.size());
    return writer.writeLeader(&data);
}

bool writeEntity(dxfRW& writer, const yicad::plugin::HatchData& value)
{
    DRW_Hatch data;
    setAttributes(data, value.attributes());
    data.name = value.solid() || value.patternName().empty()
        ? "SOLID" : value.patternName();
    data.solid = value.solid() ? 1 : 0;
    data.scale = value.patternScale();
    data.angle = degrees(value.patternAngle());
    for (const auto& source : value.loops())
    {
        auto loop = hatchLoop(source);
        if (loop != nullptr)
        {
            data.appendLoop(loop);
        }
    }
    if (data.looplist.empty())
    {
        return true;
    }
    return writer.writeHatch(&data);
}

bool writeEntity(dxfRW& writer, const yicad::plugin::InsertData& value)
{
    if (value.blockName().empty())
    {
        return true;
    }
    DRW_Insert data;
    setAttributes(data, value.attributes());
    data.name = value.blockName();
    data.basePoint = coord(value.insertionPoint());
    data.xscale = value.scale().x;
    data.yscale = value.scale().y;
    data.zscale = value.scale().z;
    data.angle = value.rotation();
    data.colcount = static_cast<int>(std::max(1U, value.columnCount()));
    data.rowcount = static_cast<int>(std::max(1U, value.rowCount()));
    data.colspace = value.columnSpacing();
    data.rowspace = value.rowSpacing();
    return writer.writeInsert(&data);
}

bool writeEntity(
    dxfRW&,
    const yicad::plugin::AttributeDefinitionData&)
{
    return true;
}

bool writeEntity(dxfRW&, const yicad::plugin::AttributeData&)
{
    return true;
}

bool writeEntity(dxfRW& writer, const yicad::plugin::ImageData& value)
{
    if (value.path().empty() || !value.clipBoundary().empty())
    {
        return true;
    }
    DRW_Image data;
    setAttributes(data, value.attributes());
    data.basePoint = coord(value.insertionPoint());
    data.secPoint = coord(value.uVector());
    data.vVector = coord(value.vVector());
    data.sizeu = value.size().x;
    data.sizev = value.size().y;
    data.brightness = value.brightness();
    data.contrast = value.contrast();
    data.fade = value.fade();
    auto* definition = writer.writeImage(&data, value.path());
    if (definition != nullptr)
    {
        definition->u = value.size().x;
        definition->v = value.size().y;
        definition->up = std::hypot(value.uVector().x, value.uVector().y);
        definition->vp = std::hypot(value.vVector().x, value.vVector().y);
        definition->loaded = 1;
        definition->resolution = 0;
    }
    return definition != nullptr;
}

} // namespace dxf
