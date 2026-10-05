/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/// @file DemoPipe.cpp
/// @brief 示例实体"管道"的实现

#include "DemoPipe.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace demo
{

namespace
{

constexpr double Pi = 3.14159265358979323846;
constexpr uint32_t MaxVertices = 100000;

YiCadPoint2d add(YiCadPoint2d a, YiCadVector2d b) noexcept { return {a.x + b.x, a.y + b.y}; }
YiCadVector2d sub(YiCadPoint2d a, YiCadPoint2d b) noexcept { return {a.x - b.x, a.y - b.y}; }
YiCadVector2d scaled(YiCadVector2d v, double s) noexcept { return {v.x * s, v.y * s}; }
double length(YiCadVector2d v) noexcept { return std::hypot(v.x, v.y); }

/// @brief 第 index 段的单位方向；重合时为 X 轴
YiCadVector2d direction(const PipeData& data, std::size_t index) noexcept
{
    const YiCadVector2d d = sub(data.vertices[index + 1], data.vertices[index]);
    const double len = length(d);
    return len > 0.0 ? scaled(d, 1.0 / len) : YiCadVector2d{1.0, 0.0};
}

YiCadVector2d leftNormal(YiCadVector2d d) noexcept { return {-d.y, d.x}; }

/// @brief 改管径的夹点：起点处、第一段左侧距中心线管径一半
YiCadPoint2d diameterGrip(const PipeData& data) noexcept
{
    return add(data.vertices.front(), scaled(leftNormal(direction(data, 0)), data.diameter * 0.5));
}

/// @brief 标注文字 "DN管径"
std::string label(const PipeData& data)
{
    return "DN" + std::to_string(static_cast<long long>(std::llround(data.diameter)));
}

/// @brief 第一段中点的箭头：尖、左后角、凹口、右后角
std::vector<YiCadPoint2d> arrow(const PipeData& data)
{
    const double r = data.diameter * 0.5;
    const YiCadVector2d d = direction(data, 0);
    const YiCadVector2d n = leftNormal(d);
    const YiCadPoint2d mid{(data.vertices[0].x + data.vertices[1].x) * 0.5,
        (data.vertices[0].y + data.vertices[1].y) * 0.5};
    return {add(mid, scaled(d, r)),
        add(add(mid, scaled(d, -r)), scaled(n, r * 0.5)),
        add(mid, scaled(d, -r * 0.5)),
        add(add(mid, scaled(d, -r)), scaled(n, -r * 0.5))};
}

/// @brief 标注文字的位置：第一段中点外侧（左侧）1.6 倍管径一半处，沿第一段方向
YiCadTextPlacementV4 labelPlacement(const PipeData& data) noexcept
{
    const double r = data.diameter * 0.5;
    const YiCadVector2d d = direction(data, 0);
    const YiCadPoint2d mid{(data.vertices[0].x + data.vertices[1].x) * 0.5,
        (data.vertices[0].y + data.vertices[1].y) * 0.5};
    const YiCadPoint2d anchor = add(mid, scaled(leftNormal(d), r * 1.6));
    auto placement = yicad::plugin::makeTextPlacement(anchor, r * 0.8);
    placement.rotation = std::atan2(d.y, d.x);
    placement.horizontalAlignment = YICAD_TEXT_ALIGN_CENTER;
    return placement;
}

/// @brief 点到线段最近的点
YiCadPoint2d nearestOnSegment(YiCadPoint2d a, YiCadPoint2d b, YiCadPoint2d p) noexcept
{
    const YiCadVector2d ab = sub(b, a);
    const double len2 = ab.x * ab.x + ab.y * ab.y;
    if (!(len2 > 0.0))
    {
        return a;
    }
    const double t = std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / len2, 0.0, 1.0);
    return add(a, scaled(ab, t));
}

} // namespace

std::vector<uint8_t> PipeClass::encodeData(const PipeData& data)
{
    yicad::plugin::ByteWriter writer;
    writer.u32(static_cast<uint32_t>(data.vertices.size()));
    for (const auto& vertex : data.vertices)
    {
        writer.point(vertex);
    }
    writer.f64(data.diameter);
    return writer.take();
}

PipeData PipeClass::decodeData(std::span<const uint8_t> bytes)
{
    yicad::plugin::ByteReader reader(bytes);
    const uint32_t count = reader.u32();
    if (count < 2 || count > MaxVertices)
    {
        throw std::invalid_argument("demo pipe needs at least two vertices");
    }
    PipeData data;
    data.vertices.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        const YiCadPoint2d vertex = reader.point();
        if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y))
        {
            throw std::invalid_argument("demo pipe vertex is not finite");
        }
        data.vertices.push_back(vertex);
    }
    data.diameter = reader.f64();
    if (!std::isfinite(data.diameter) || data.diameter <= 0.0 || !reader.atEnd())
    {
        throw std::invalid_argument("demo pipe diameter is invalid");
    }
    return data;
}

void PipeClass::worldDraw(const PipeData& data, const yicad::plugin::Gi& gi) const
{
    const double r = data.diameter * 0.5;
    const std::size_t n = data.vertices.size();
    // 每段两条边线
    for (std::size_t i = 0; i + 1 < n; ++i)
    {
        const YiCadVector2d offset = scaled(leftNormal(direction(data, i)), r);
        const YiCadPoint2d left[2] = {add(data.vertices[i], offset), add(data.vertices[i + 1], offset)};
        const YiCadPoint2d right[2] = {add(data.vertices[i], scaled(offset, -1.0)),
            add(data.vertices[i + 1], scaled(offset, -1.0))};
        gi.polyline(left);
        gi.polyline(right);
    }
    // 朝外的半圆端头：起点从左侧法向转到右侧，终点从右侧法向转到左侧
    const YiCadVector2d d0 = direction(data, 0);
    const YiCadVector2d dn = direction(data, n - 2);
    gi.arc(data.vertices.front(), r, std::atan2(d0.y, d0.x) + Pi * 0.5, Pi);
    gi.arc(data.vertices.back(), r, std::atan2(dn.y, dn.x) - Pi * 0.5, Pi);
    // 中心线：文档里有 CENTER 线型时用它（属性设了就一直生效，所以颜色、线型不同的图元放在后面）
    if (const auto center = gi.lineType("CENTER"))
    {
        gi.setLineType(center);
    }
    gi.polyline(data.vertices);
    // 第一段中点的红色实心箭头与标注
    gi.setColor({YICAD_COLOR_RGB, 0, 255, 0, 0, 0});
    const yicad::plugin::GiLoop loop{arrow(data), {}};
    gi.fill(std::span<const yicad::plugin::GiLoop>(&loop, 1));
    gi.text(label(data), gi.textStyle("Standard"), labelPlacement(data));
}

YiCadExtents2d PipeClass::extents(const PipeData& data) const
{
    YiCadExtents2d result{{std::numeric_limits<double>::max(), std::numeric_limits<double>::max()},
        {-std::numeric_limits<double>::max(), -std::numeric_limits<double>::max()}};
    for (const auto& vertex : data.vertices)
    {
        result.minPoint.x = std::min(result.minPoint.x, vertex.x);
        result.minPoint.y = std::min(result.minPoint.y, vertex.y);
        result.maxPoint.x = std::max(result.maxPoint.x, vertex.x);
        result.maxPoint.y = std::max(result.maxPoint.y, vertex.y);
    }
    // 端头、边线在管径一半以内，标注在 1.6 倍外加字高；文字长度按每字一个字高估
    const double r = data.diameter * 0.5;
    const double margin = std::max(r * 2.6, static_cast<double>(label(data).size()) * r * 0.8);
    result.minPoint.x -= margin;
    result.minPoint.y -= margin;
    result.maxPoint.x += margin;
    result.maxPoint.y += margin;
    return result;
}

void PipeClass::transform(PipeData& data, const YiCadMatrix2d& matrix) const
{
    for (auto& vertex : data.vertices)
    {
        vertex = yicad::plugin::transformPoint(matrix, vertex);
    }
    // 管径按面积比例缩放（等比时即缩放比例）
    data.diameter *= std::sqrt(std::fabs(yicad::plugin::matrixDeterminant(matrix)));
}

std::vector<YiCadPoint2d> PipeClass::grips(const PipeData& data) const
{
    std::vector<YiCadPoint2d> result = data.vertices;
    result.push_back(diameterGrip(data));
    return result;
}

void PipeClass::moveGrips(PipeData& data, std::span<const uint32_t> indices, YiCadVector2d offset) const
{
    const std::size_t n = data.vertices.size();
    for (const uint32_t index : indices)
    {
        if (index < n)
        {
            data.vertices[index] = add(data.vertices[index], offset);
        }
        else if (index == n)
        {
            // 管径 = 起点到拖到的位置的距离的两倍
            const YiCadPoint2d to = add(diameterGrip(data), offset);
            data.diameter = 2.0 * length(sub(to, data.vertices.front()));
        }
    }
    if (!(data.diameter > 0.0))
    {
        throw std::invalid_argument("demo pipe diameter must stay positive");
    }
}

std::vector<YiCadPoint2d> PipeClass::snapPoints(const PipeData& data, uint32_t snapMode, YiCadPoint2d pick) const
{
    std::vector<YiCadPoint2d> result;
    const std::size_t n = data.vertices.size();
    switch (snapMode)
    {
    case YICAD_SNAP_ENDPOINT:
        result = data.vertices;
        break;
    case YICAD_SNAP_MIDPOINT:
        for (std::size_t i = 0; i + 1 < n; ++i)
        {
            result.push_back({(data.vertices[i].x + data.vertices[i + 1].x) * 0.5,
                (data.vertices[i].y + data.vertices[i + 1].y) * 0.5});
        }
        break;
    case YICAD_SNAP_NEAREST:
        for (std::size_t i = 0; i + 1 < n; ++i)
        {
            result.push_back(nearestOnSegment(data.vertices[i], data.vertices[i + 1], pick));
        }
        break;
    default:
        // 管道没有圆心
        break;
    }
    return result;
}

bool PipeClass::explode(const PipeData& data, const yicad::plugin::ImportContainer& out) const
{
    const double r = data.diameter * 0.5;
    const std::size_t n = data.vertices.size();
    // 中心线：一条多段线
    std::vector<YiCadVertex2d> centerline;
    for (const auto& vertex : data.vertices)
    {
        centerline.push_back({vertex, 0.0, 0.0, 0.0});
    }
    if (out.createPolyline(yicad::plugin::PolylineData(std::move(centerline))) != YICAD_IMPORT_SUCCESS)
    {
        return false;
    }
    // 边线
    for (std::size_t i = 0; i + 1 < n; ++i)
    {
        const YiCadVector2d offset = scaled(leftNormal(direction(data, i)), r);
        if (out.createLine(add(data.vertices[i], offset), add(data.vertices[i + 1], offset)) !=
                YICAD_IMPORT_SUCCESS ||
            out.createLine(add(data.vertices[i], scaled(offset, -1.0)),
                add(data.vertices[i + 1], scaled(offset, -1.0))) != YICAD_IMPORT_SUCCESS)
        {
            return false;
        }
    }
    // 端头：圆弧的起止角（逆时针）
    const YiCadVector2d d0 = direction(data, 0);
    const YiCadVector2d dn = direction(data, n - 2);
    const double a0 = std::atan2(d0.y, d0.x) + Pi * 0.5;
    const double an = std::atan2(dn.y, dn.x) - Pi * 0.5;
    if (out.createArc(data.vertices.front(), r, a0, a0 + Pi) != YICAD_IMPORT_SUCCESS ||
        out.createArc(data.vertices.back(), r, an, an + Pi) != YICAD_IMPORT_SUCCESS)
    {
        return false;
    }
    // 箭头与标注：红色（图层、线型沿用被炸开的管道）
    yicad::plugin::EntityAttributes red;
    red.setColor({YICAD_COLOR_RGB, 0, 255, 0, 0, 0});
    std::vector<YiCadVertex2d> loop;
    for (const auto& corner : arrow(data))
    {
        loop.push_back({corner, 0.0, 0.0, 0.0});
    }
    yicad::plugin::HatchData fill;
    fill.setSolid().addPolylineLoop(std::move(loop)).setAttributes(red);
    const YiCadTextPlacementV4 placement = labelPlacement(data);
    yicad::plugin::TextData text(label(data));
    text.setPlacement(placement.insertionPoint, placement.alignmentPoint)
        .setMetrics(placement.height, placement.rotation)
        .setAlignment(placement.horizontalAlignment, placement.verticalAlignment)
        .setAttributes(red);
    return out.createHatch(fill) == YICAD_IMPORT_SUCCESS && out.createText(text) == YICAD_IMPORT_SUCCESS;
}

} // namespace demo
