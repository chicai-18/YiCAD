/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/// @file DxfProxyGraphics.cpp
/// @brief AutoCAD 代理图形的编码与解码

#include "DxfProxyGraphics.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <optional>
#include <variant>

namespace dxf
{

namespace
{

constexpr double Pi = 3.14159265358979323846;
constexpr double TwoPi = 2.0 * Pi;

// 命令类型（ODA 的 Proxy Entity Graphics；ezdxf ProxyGraphicTypes）
constexpr uint32_t CmdExtents = 1;
constexpr uint32_t CmdCircle = 2;
constexpr uint32_t CmdCircle3P = 3;
constexpr uint32_t CmdArc = 4;
constexpr uint32_t CmdArc3P = 5;
constexpr uint32_t CmdPolyline = 6;
constexpr uint32_t CmdPolygon = 7;
constexpr uint32_t CmdMesh = 8;
constexpr uint32_t CmdShell = 9;
constexpr uint32_t CmdText = 10;
constexpr uint32_t CmdText2 = 11;
constexpr uint32_t CmdXLine = 12;
constexpr uint32_t CmdRay = 13;
constexpr uint32_t CmdColor = 14;
constexpr uint32_t CmdLayer = 16;
constexpr uint32_t CmdLineType = 18;
constexpr uint32_t CmdFill = 20;
constexpr uint32_t CmdTrueColor = 22;
constexpr uint32_t CmdLineWeight = 23;
constexpr uint32_t CmdLineTypeScale = 24;
constexpr uint32_t CmdPushMatrix = 29;
constexpr uint32_t CmdPushMatrix2 = 30;
constexpr uint32_t CmdPopMatrix = 31;
constexpr uint32_t CmdPolylineNormals = 32;
constexpr uint32_t CmdLwPolyline = 33;
constexpr uint32_t CmdUnicodeText = 36;
constexpr uint32_t CmdUnicodeText2 = 38;
constexpr uint32_t CmdEllipticArc = 44;

/// @brief 代理图形里的颜色值：随层 256、随块 0
constexpr uint32_t AciByLayer = 256;
constexpr uint32_t AciByBlock = 0;

std::string upper(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return value;
}

// ---------------------------------------------------------------------------
// 写
// ---------------------------------------------------------------------------

void putU32(std::vector<uint8_t>& out, uint32_t value)
{
    for (int i = 0; i < 4; ++i)
    {
        out.push_back(static_cast<uint8_t>(value >> (8 * i)));
    }
}

void putF64(std::vector<uint8_t>& out, double value)
{
    uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    for (int i = 0; i < 8; ++i)
    {
        out.push_back(static_cast<uint8_t>(bits >> (8 * i)));
    }
}

void putPoint(std::vector<uint8_t>& out, YiCadPoint2d p, double z = 0.0)
{
    putF64(out, p.x);
    putF64(out, p.y);
    putF64(out, z);
}

void putNormal(std::vector<uint8_t>& out)
{
    putPoint(out, {0.0, 0.0}, 1.0);
}

/// @brief UTF-8 转 UTF-16LE，以两个零字节结尾，补齐到 4 字节
void putUnicodeString(std::vector<uint8_t>& out, const std::string& text)
{
    std::size_t i = 0;
    auto unit = [&out](uint32_t value) {
        out.push_back(static_cast<uint8_t>(value & 0xFF));
        out.push_back(static_cast<uint8_t>(value >> 8));
    };
    while (i < text.size())
    {
        const auto c = static_cast<unsigned char>(text[i]);
        uint32_t code = c;
        int extra = 0;
        if (c >= 0xF0) { code = c & 0x07; extra = 3; }
        else if (c >= 0xE0) { code = c & 0x0F; extra = 2; }
        else if (c >= 0xC0) { code = c & 0x1F; extra = 1; }
        ++i;
        for (int k = 0; k < extra && i < text.size(); ++k, ++i)
        {
            code = (code << 6) | (static_cast<unsigned char>(text[i]) & 0x3F);
        }
        if (code >= 0x10000)
        {
            code -= 0x10000;
            unit(0xD800 + (code >> 10));
            unit(0xDC00 + (code & 0x3FF));
        }
        else
        {
            unit(code);
        }
    }
    unit(0);
    while (out.size() % 4 != 0)
    {
        out.push_back(0);
    }
}

bool sameColor(const YiCadColorData& a, const YiCadColorData& b) noexcept
{
    if (a.method != b.method)
    {
        return false;
    }
    switch (a.method)
    {
    case YICAD_COLOR_RGB:
        return a.red == b.red && a.green == b.green && a.blue == b.blue;
    case YICAD_COLOR_ACI:
        return a.aci == b.aci;
    default:
        return true;
    }
}

/// @brief 正的扫角，(0, 2π]
double positiveSweep(double startAngle, double endAngle) noexcept
{
    double sweep = std::fmod(endAngle - startAngle, TwoPi);
    if (sweep <= 1.0e-12)
    {
        sweep += TwoPi;
    }
    return sweep;
}

/// @brief 带凸度的一段：圆心、半径、逆时针的起始角与扫角
void bulgeArc(YiCadPoint2d p1, YiCadPoint2d p2, double bulge, YiCadPoint2d& center, double& radius,
    double& startAngle, double& sweep) noexcept
{
    const double dx = p2.x - p1.x;
    const double dy = p2.y - p1.y;
    const double d = std::hypot(dx, dy);
    const double s = std::fabs(bulge) * d * 0.5;
    radius = (d * d * 0.25 + s * s) / (2.0 * s);
    const YiCadVector2d left{-dy / d, dx / d};
    const YiCadPoint2d mid{(p1.x + p2.x) * 0.5, (p1.y + p2.y) * 0.5};
    const double offset = radius - s;
    center = bulge > 0.0 ? YiCadPoint2d{mid.x + left.x * offset, mid.y + left.y * offset}
                         : YiCadPoint2d{mid.x - left.x * offset, mid.y - left.y * offset};
    sweep = 4.0 * std::atan(std::fabs(bulge));
    const YiCadPoint2d from = bulge > 0.0 ? p1 : p2;
    startAngle = std::atan2(from.y - center.y, from.x - center.x);
}

/// @brief 圆弧上从 start 逆时针转 sweep 的离散点（含两端）
void sampleArc(YiCadPoint2d center, double radius, double start, double sweep, std::vector<YiCadPoint2d>& out)
{
    const int count = std::max(4, static_cast<int>(std::ceil(std::fabs(sweep) / (Pi / 36.0))));
    for (int k = 0; k <= count; ++k)
    {
        const double t = start + sweep * k / count;
        out.push_back({center.x + radius * std::cos(t), center.y + radius * std::sin(t)});
    }
}

/// @brief 椭圆弧上从参数 start 到 end（逆时针）的离散点
void sampleEllipse(YiCadPoint2d center, YiCadVector2d major, double ratio, double start, double end,
    std::vector<YiCadPoint2d>& out)
{
    const YiCadVector2d minor{-major.y * ratio, major.x * ratio};
    double sweep = end - start;
    if (sweep <= 0.0)
    {
        sweep += TwoPi;
    }
    const int count = std::max(8, static_cast<int>(std::ceil(sweep / (Pi / 36.0))));
    for (int k = 0; k <= count; ++k)
    {
        const double t = start + sweep * k / count;
        out.push_back({center.x + major.x * std::cos(t) + minor.x * std::sin(t),
            center.y + major.y * std::cos(t) + minor.y * std::sin(t)});
    }
}

/// @brief 把一条边的离散点接到环上：首点接不上而末点接得上时倒过来接
void appendEdge(std::vector<YiCadPoint2d>& loop, std::vector<YiCadPoint2d> points)
{
    if (points.empty())
    {
        return;
    }
    if (!loop.empty())
    {
        const auto& last = loop.back();
        const double toFirst = std::hypot(points.front().x - last.x, points.front().y - last.y);
        const double toLast = std::hypot(points.back().x - last.x, points.back().y - last.y);
        if (toLast < toFirst)
        {
            std::reverse(points.begin(), points.end());
        }
        if (std::hypot(points.front().x - last.x, points.front().y - last.y) < 1.0e-9)
        {
            points.erase(points.begin());
        }
    }
    loop.insert(loop.end(), points.begin(), points.end());
}

/// @brief 填充的一个环离散成点
std::vector<YiCadPoint2d> hatchLoopPoints(const yicad::plugin::HatchData::Loop& loop)
{
    std::vector<YiCadPoint2d> points;
    if (loop.kind == YICAD_HATCH_LOOP_POLYLINE)
    {
        const std::size_t n = loop.vertices.size();
        for (std::size_t i = 0; i < n; ++i)
        {
            const auto& v = loop.vertices[i];
            points.push_back(v.position);
            if (std::fabs(v.bulge) > 1.0e-12)
            {
                YiCadPoint2d center{};
                double radius = 0.0, start = 0.0, sweep = 0.0;
                const auto& next = loop.vertices[(i + 1) % n].position;
                bulgeArc(v.position, next, v.bulge, center, radius, start, sweep);
                std::vector<YiCadPoint2d> arc;
                sampleArc(center, radius, start, sweep, arc);
                if (v.bulge < 0.0)
                {
                    std::reverse(arc.begin(), arc.end());
                }
                points.insert(points.end(), arc.begin() + 1, arc.end() - 1);
            }
        }
        return points;
    }
    for (const auto& edge : loop.edges)
    {
        std::vector<YiCadPoint2d> part;
        switch (edge.type())
        {
        case YICAD_HATCH_EDGE_LINE:
            part = {edge.startPoint(), edge.endPoint()};
            break;
        case YICAD_HATCH_EDGE_CIRCULAR_ARC:
        {
            const double sweep = positiveSweep(edge.startParameter(), edge.endParameter());
            if (edge.counterClockwise())
            {
                sampleArc(edge.center(), edge.radius(), edge.startParameter(), sweep, part);
            }
            else
            {
                sampleArc(edge.center(), edge.radius(), edge.startParameter(), -(TwoPi - sweep), part);
            }
            break;
        }
        case YICAD_HATCH_EDGE_ELLIPTIC_ARC:
            sampleEllipse(edge.center(), edge.majorAxis(), edge.minorToMajorRatio(), edge.startParameter(),
                edge.endParameter(), part);
            break;
        default:
            // 样条边：按控制多边形近似（宿主交来的图形里不会出现）
            part = edge.controlPoints();
            break;
        }
        appendEdge(points, std::move(part));
    }
    if (points.size() > 1 &&
        std::hypot(points.front().x - points.back().x, points.front().y - points.back().y) < 1.0e-9)
    {
        points.pop_back();
    }
    return points;
}

} // namespace

ProxyGraphicsWriter::ProxyGraphicsWriter(
    const TableIndex& layers,
    const TableIndex& lineTypes,
    const yicad::plugin::EntityAttributes& owner)
    : m_layers(layers)
    , m_lineTypes(lineTypes)
{
    const auto& data = owner.abiData();
    m_color = data.color;
    m_layer = owner.layerName().empty() ? std::string("0") : owner.layerName();
    m_lineType = owner.lineTypeName().empty() ? std::string("BYLAYER") : owner.lineTypeName();
    m_lineWeight = data.lineWidth;
    m_ownerLineTypeScale = data.lineTypeScale > 0.0 ? data.lineTypeScale : 1.0;
}

void ProxyGraphicsWriter::command(uint32_t type, const std::vector<uint8_t>& payload)
{
    putU32(m_body, static_cast<uint32_t>(8 + payload.size()));
    putU32(m_body, type);
    m_body.insert(m_body.end(), payload.begin(), payload.end());
    ++m_count;
}

void ProxyGraphicsWriter::applyAttributes(const yicad::plugin::EntityAttributes& attributes)
{
    const auto& data = attributes.abiData();
    std::vector<uint8_t> payload;
    if (!sameColor(data.color, m_color))
    {
        payload.clear();
        switch (data.color.method)
        {
        case YICAD_COLOR_RGB:
            putU32(payload, 0xC2000000U | (static_cast<uint32_t>(data.color.red) << 16) |
                (static_cast<uint32_t>(data.color.green) << 8) | data.color.blue);
            command(CmdTrueColor, payload);
            break;
        case YICAD_COLOR_ACI:
            putU32(payload, data.color.aci);
            command(CmdColor, payload);
            break;
        case YICAD_COLOR_BY_BLOCK:
            putU32(payload, AciByBlock);
            command(CmdColor, payload);
            break;
        default:
            putU32(payload, AciByLayer);
            command(CmdColor, payload);
            break;
        }
        m_color = data.color;
    }
    const std::string layer = attributes.layerName().empty() ? std::string("0") : attributes.layerName();
    if (upper(layer) != upper(m_layer))
    {
        if (const auto found = m_layers.find(upper(layer)); found != m_layers.end())
        {
            payload.clear();
            putU32(payload, found->second);
            command(CmdLayer, payload);
            m_layer = layer;
        }
    }
    const std::string lineType =
        attributes.lineTypeName().empty() ? std::string("BYLAYER") : attributes.lineTypeName();
    if (upper(lineType) != upper(m_lineType))
    {
        if (const auto found = m_lineTypes.find(upper(lineType)); found != m_lineTypes.end())
        {
            payload.clear();
            putU32(payload, found->second);
            command(CmdLineType, payload);
            m_lineType = lineType;
        }
    }
    if (data.lineWidth != m_lineWeight)
    {
        payload.clear();
        putU32(payload, static_cast<uint32_t>(data.lineWidth));
        command(CmdLineWeight, payload);
        m_lineWeight = data.lineWidth;
    }
    // 线型比例：相对实体自己的（图元的比例里已经乘了实体的）
    const double scale = data.lineTypeScale > 0.0 ? data.lineTypeScale / m_ownerLineTypeScale : 1.0;
    if (std::fabs(scale - m_lineTypeScale) > 1.0e-12)
    {
        payload.clear();
        putF64(payload, scale);
        command(CmdLineTypeScale, payload);
        m_lineTypeScale = scale;
    }
}

void ProxyGraphicsWriter::polyline(const std::vector<YiCadPoint2d>& points, uint32_t type)
{
    if (points.empty())
    {
        return;
    }
    std::vector<uint8_t> payload;
    putU32(payload, static_cast<uint32_t>(points.size()));
    for (const auto& p : points)
    {
        putPoint(payload, p);
    }
    command(type, payload);
}

void ProxyGraphicsWriter::circle(YiCadPoint2d center, double radius)
{
    std::vector<uint8_t> payload;
    putPoint(payload, center);
    putF64(payload, radius);
    putNormal(payload);
    command(CmdCircle, payload);
}

void ProxyGraphicsWriter::arc(YiCadPoint2d center, double radius, double startAngle, double sweep)
{
    std::vector<uint8_t> payload;
    putPoint(payload, center);
    putF64(payload, radius);
    putNormal(payload);
    putPoint(payload, {std::cos(startAngle), std::sin(startAngle)});
    putF64(payload, sweep);
    putU32(payload, 0);  // 弧的类型：只画弧
    command(CmdArc, payload);
}

void ProxyGraphicsWriter::setFill(bool on)
{
    std::vector<uint8_t> payload;
    putU32(payload, on ? 1U : 0U);
    command(CmdFill, payload);
}

void ProxyGraphicsWriter::shell(const std::vector<std::vector<YiCadPoint2d>>& loops, const std::vector<bool>& holes)
{
    std::vector<uint8_t> payload;
    uint32_t vertexCount = 0;
    uint32_t entries = 0;
    for (const auto& loop : loops)
    {
        vertexCount += static_cast<uint32_t>(loop.size());
        entries += 1 + static_cast<uint32_t>(loop.size());
    }
    putU32(payload, vertexCount);
    for (const auto& loop : loops)
    {
        for (const auto& p : loop)
        {
            putPoint(payload, p);
        }
    }
    putU32(payload, entries);
    uint32_t base = 0;
    for (std::size_t i = 0; i < loops.size(); ++i)
    {
        const auto count = static_cast<int32_t>(loops[i].size());
        putU32(payload, static_cast<uint32_t>(holes[i] ? -count : count));
        for (int32_t k = 0; k < count; ++k)
        {
            putU32(payload, base + static_cast<uint32_t>(k));
        }
        base += static_cast<uint32_t>(count);
    }
    // 边、面、顶点三组标志：都没有（少写一组 AutoCAD 读时会崩溃）
    putU32(payload, 0);
    putU32(payload, 0);
    putU32(payload, 0);
    command(CmdShell, payload);
}

void ProxyGraphicsWriter::add(const yicad::plugin::EntityData& entity)
{
    using namespace yicad::plugin;
    std::visit([this](const auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, PointData>)
        {
            applyAttributes(value.attributes);
            polyline({value.position, value.position}, CmdPolyline);
        }
        else if constexpr (std::is_same_v<T, LineData>)
        {
            applyAttributes(value.attributes);
            polyline({value.startPoint, value.endPoint}, CmdPolyline);
        }
        else if constexpr (std::is_same_v<T, RayData> || std::is_same_v<T, XLineData>)
        {
            applyAttributes(value.attributes);
            std::vector<uint8_t> payload;
            putPoint(payload, value.basePoint);
            putPoint(payload, {value.basePoint.x + value.direction.x, value.basePoint.y + value.direction.y});
            command(std::is_same_v<T, RayData> ? CmdRay : CmdXLine, payload);
        }
        else if constexpr (std::is_same_v<T, ArcData>)
        {
            applyAttributes(value.attributes);
            arc(value.center, value.radius, value.startAngle, positiveSweep(value.startAngle, value.endAngle));
        }
        else if constexpr (std::is_same_v<T, CircleData>)
        {
            applyAttributes(value.attributes);
            circle(value.center, value.radius);
        }
        else if constexpr (std::is_same_v<T, EllipseData>)
        {
            applyAttributes(value.attributes);
            std::vector<YiCadPoint2d> points;
            if (value.closed)
            {
                sampleEllipse(value.center, value.majorAxis, value.minorToMajorRatio, 0.0, TwoPi, points);
            }
            else
            {
                sampleEllipse(value.center, value.majorAxis, value.minorToMajorRatio, value.startParameter,
                    value.endParameter, points);
            }
            polyline(points, CmdPolyline);
        }
        else if constexpr (std::is_same_v<T, PolylineData>)
        {
            applyAttributes(value.attributes());
            // 直线段连成一条折线，带凸度的段写成圆弧
            const auto& vertices = value.vertices();
            const std::size_t n = vertices.size();
            const std::size_t segments = value.closed() ? n : (n == 0 ? 0 : n - 1);
            std::vector<YiCadPoint2d> run;
            auto flush = [&]() {
                if (run.size() >= 2)
                {
                    polyline(run, CmdPolyline);
                }
                run.clear();
            };
            if (n == 1)
            {
                polyline({vertices[0].position, vertices[0].position}, CmdPolyline);
            }
            for (std::size_t i = 0; i < segments; ++i)
            {
                const auto& a = vertices[i];
                const auto& b = vertices[(i + 1) % n];
                if (std::fabs(a.bulge) <= 1.0e-12)
                {
                    if (run.empty())
                    {
                        run.push_back(a.position);
                    }
                    run.push_back(b.position);
                    continue;
                }
                flush();
                YiCadPoint2d center{};
                double radius = 0.0, start = 0.0, sweep = 0.0;
                bulgeArc(a.position, b.position, a.bulge, center, radius, start, sweep);
                arc(center, radius, start, sweep);
            }
            flush();
        }
        else if constexpr (std::is_same_v<T, SplineData>)
        {
            // 宿主交来的图形里样条已离散成多段线；万一有，按控制多边形近似
            applyAttributes(value.attributes());
            polyline(value.controlPoints().empty() ? value.fitPoints() : value.controlPoints(), CmdPolyline);
        }
        else if constexpr (std::is_same_v<T, SolidData>)
        {
            applyAttributes(value.attributes());
            const auto& c = value.corners();
            std::vector<YiCadPoint2d> polygon;
            if (c.size() == 4)
            {
                // SOLID 的四个角按"Z"字排：1、2、4、3 才是边界的顺序
                polygon = {c[0], c[1], c[3], c[2]};
            }
            else
            {
                polygon = c;
            }
            if (polygon.size() >= 3)
            {
                setFill(true);
                polyline(polygon, CmdPolygon);
                setFill(false);
            }
        }
        else if constexpr (std::is_same_v<T, HatchData>)
        {
            if (!value.solid() || value.loops().empty())
            {
                return;
            }
            applyAttributes(value.attributes());
            std::vector<std::vector<YiCadPoint2d>> loops;
            std::vector<bool> holes;
            for (const auto& loop : value.loops())
            {
                auto points = hatchLoopPoints(loop);
                if (points.size() >= 3)
                {
                    holes.push_back(loop.role == YICAD_HATCH_LOOP_HOLE);
                    loops.push_back(std::move(points));
                }
            }
            if (!loops.empty())
            {
                setFill(true);
                shell(loops, holes);
                setFill(false);
            }
        }
        else if constexpr (std::is_same_v<T, TextData>)
        {
            // 宿主交来的图形里文字已是笔画；万一有，写成单行文字
            applyAttributes(value.attributes());
            std::vector<uint8_t> payload;
            putPoint(payload, value.insertionPoint());
            putNormal(payload);
            putPoint(payload, {std::cos(value.rotation()), std::sin(value.rotation())});
            putF64(payload, value.height());
            putF64(payload, value.widthFactor() > 0.0 ? value.widthFactor() : 1.0);
            putF64(payload, value.obliqueAngle());
            putUnicodeString(payload, value.text());
            command(CmdUnicodeText, payload);
        }
        // 块参照、标注、引线、多行文字、属性、图片、自定义实体：宿主交来的图形里没有（块已展开）
    }, entity);
}

std::vector<uint8_t> ProxyGraphicsWriter::finish() const
{
    std::vector<uint8_t> result;
    putU32(result, static_cast<uint32_t>(8 + m_body.size()));
    putU32(result, m_count);
    result.insert(result.end(), m_body.begin(), m_body.end());
    return result;
}

// ---------------------------------------------------------------------------
// 读
// ---------------------------------------------------------------------------

namespace
{

/// @brief 按 4 字节对齐读取的字节流
class ByteStream
{
public:
    explicit ByteStream(std::span<const uint8_t> data) : m_data(data) {}

    bool u32(uint32_t& value)
    {
        if (m_offset + 4 > m_data.size())
        {
            return false;
        }
        value = 0;
        for (int i = 0; i < 4; ++i)
        {
            value |= static_cast<uint32_t>(m_data[m_offset + i]) << (8 * i);
        }
        m_offset += 4;
        return true;
    }

    bool i32(int32_t& value)
    {
        uint32_t raw = 0;
        if (!u32(raw))
        {
            return false;
        }
        value = static_cast<int32_t>(raw);
        return true;
    }

    bool f64(double& value)
    {
        if (m_offset + 8 > m_data.size())
        {
            return false;
        }
        uint64_t bits = 0;
        for (int i = 0; i < 8; ++i)
        {
            bits |= static_cast<uint64_t>(m_data[m_offset + i]) << (8 * i);
        }
        std::memcpy(&value, &bits, sizeof(value));
        m_offset += 8;
        return std::isfinite(value);
    }

    bool point3(double& x, double& y, double& z)
    {
        return f64(x) && f64(y) && f64(z);
    }

    /// @brief 以零字节结尾、补齐到 4 字节的字符串（按 Latin-1 转 UTF-8）
    bool paddedString(std::string& out)
    {
        out.clear();
        std::size_t end = m_offset;
        while (end < m_data.size() && m_data[end] != 0)
        {
            ++end;
        }
        if (end >= m_data.size())
        {
            return false;
        }
        for (std::size_t i = m_offset; i < end; ++i)
        {
            const uint8_t c = m_data[i];
            if (c < 0x80)
            {
                out.push_back(static_cast<char>(c));
            }
            else
            {
                out.push_back(static_cast<char>(0xC0 | (c >> 6)));
                out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
            }
        }
        m_offset = align(end + 1);
        return true;
    }

    /// @brief 以两个零字节结尾、补齐到 4 字节的 UTF-16LE 字符串（转 UTF-8）
    bool paddedUnicodeString(std::string& out)
    {
        out.clear();
        std::size_t i = m_offset;
        std::vector<uint32_t> units;
        for (;; i += 2)
        {
            if (i + 1 >= m_data.size())
            {
                return false;
            }
            const uint32_t unit = m_data[i] | (static_cast<uint32_t>(m_data[i + 1]) << 8);
            if (unit == 0)
            {
                break;
            }
            units.push_back(unit);
        }
        m_offset = align(i + 2);
        for (std::size_t k = 0; k < units.size(); ++k)
        {
            uint32_t code = units[k];
            if (code >= 0xD800 && code < 0xDC00 && k + 1 < units.size())
            {
                code = 0x10000 + ((code - 0xD800) << 10) + (units[++k] - 0xDC00);
            }
            if (code < 0x80)
            {
                out.push_back(static_cast<char>(code));
            }
            else if (code < 0x800)
            {
                out.push_back(static_cast<char>(0xC0 | (code >> 6)));
                out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
            }
            else if (code < 0x10000)
            {
                out.push_back(static_cast<char>(0xE0 | (code >> 12)));
                out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
            }
            else
            {
                out.push_back(static_cast<char>(0xF0 | (code >> 18)));
                out.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
            }
        }
        return true;
    }

private:
    static std::size_t align(std::size_t offset) { return (offset + 3) & ~std::size_t(3); }

    std::span<const uint8_t> m_data;
    std::size_t m_offset = 0;
};

/// @brief DWG 的位流（LWPOLYLINE 命令用），读法同 ODA 规范与 ezdxf 的 BitStream
class BitStream
{
public:
    explicit BitStream(std::span<const uint8_t> data) : m_data(data) {}

    bool bit(uint32_t& value)
    {
        if ((m_bit >> 3) >= m_data.size())
        {
            return false;
        }
        value = (m_data[m_bit >> 3] & (0x80 >> (m_bit & 7))) ? 1U : 0U;
        ++m_bit;
        return true;
    }

    bool bits(int count, uint32_t& value)
    {
        value = 0;
        for (int i = 0; i < count; ++i)
        {
            uint32_t b = 0;
            if (!bit(b))
            {
                return false;
            }
            value = (value << 1) | b;
        }
        return true;
    }

    bool byte(uint8_t& value)
    {
        uint32_t v = 0;
        if (!bits(8, v))
        {
            return false;
        }
        value = static_cast<uint8_t>(v);
        return true;
    }

    bool rawLong(uint32_t& value)
    {
        value = 0;
        for (int i = 0; i < 4; ++i)
        {
            uint8_t b = 0;
            if (!byte(b))
            {
                return false;
            }
            value |= static_cast<uint32_t>(b) << (8 * i);
        }
        return true;
    }

    bool rawShort(int32_t& value)
    {
        uint8_t lo = 0, hi = 0;
        if (!byte(lo) || !byte(hi))
        {
            return false;
        }
        value = static_cast<int16_t>(lo | (hi << 8));
        return true;
    }

    bool rawDouble(double& value)
    {
        uint8_t bytes[8];
        for (auto& b : bytes)
        {
            if (!byte(b))
            {
                return false;
            }
        }
        std::memcpy(&value, bytes, sizeof(value));
        return true;
    }

    bool bitShort(int32_t& value)
    {
        uint32_t code = 0;
        if (!bits(2, code))
        {
            return false;
        }
        switch (code)
        {
        case 0: return rawShort(value);
        case 1: { uint8_t b = 0; if (!byte(b)) return false; value = b; return true; }
        case 2: value = 0; return true;
        default: value = 256; return true;
        }
    }

    bool bitLong(int32_t& value)
    {
        uint32_t code = 0;
        if (!bits(2, code))
        {
            return false;
        }
        switch (code)
        {
        case 0: { uint32_t raw = 0; if (!rawLong(raw)) return false; value = static_cast<int32_t>(raw); return true; }
        case 1: { uint8_t b = 0; if (!byte(b)) return false; value = b; return true; }
        default: value = 0; return true;
        }
    }

    bool bitDouble(double& value)
    {
        uint32_t code = 0;
        if (!bits(2, code))
        {
            return false;
        }
        switch (code)
        {
        case 0: return rawDouble(value);
        case 1: value = 1.0; return true;
        default: value = 0.0; return true;
        }
    }

    bool bitDoubleDefault(double defaultValue, double& value)
    {
        uint32_t code = 0;
        if (!bits(2, code))
        {
            return false;
        }
        uint8_t bytes[8];
        std::memcpy(bytes, &defaultValue, sizeof(bytes));
        switch (code)
        {
        case 0:
            value = defaultValue;
            return true;
        case 1:
            for (int i = 0; i < 4; ++i)
            {
                if (!byte(bytes[i])) return false;
            }
            break;
        case 2:
            if (!byte(bytes[4]) || !byte(bytes[5])) return false;
            for (int i = 0; i < 4; ++i)
            {
                if (!byte(bytes[i])) return false;
            }
            break;
        default:
            return rawDouble(value);
        }
        std::memcpy(&value, bytes, sizeof(value));
        return true;
    }

private:
    std::span<const uint8_t> m_data;
    std::size_t m_bit = 0;
};

/// @brief 二维仿射变换，x' = a·x + c·y + tx，y' = b·x + d·y + ty
struct Affine
{
    double a = 1.0, b = 0.0, c = 0.0, d = 1.0, tx = 0.0, ty = 0.0;

    YiCadPoint2d apply(YiCadPoint2d p) const noexcept { return {a * p.x + c * p.y + tx, b * p.x + d * p.y + ty}; }
    YiCadVector2d applyVector(YiCadVector2d v) const noexcept { return {a * v.x + c * v.y, b * v.x + d * v.y}; }
    double determinant() const noexcept { return a * d - b * c; }
    bool identity() const noexcept
    {
        return a == 1.0 && b == 0.0 && c == 0.0 && d == 1.0 && tx == 0.0 && ty == 0.0;
    }
    /// @brief 相似变换（圆仍是圆）；scale 为比例
    bool similarity(double& scale) const noexcept
    {
        const double l0 = std::hypot(a, b);
        const double l1 = std::hypot(c, d);
        scale = l0;
        return l0 > 0.0 && std::fabs(l0 - l1) <= 1.0e-9 * l0 && std::fabs(a * c + b * d) <= 1.0e-9 * l0 * l1;
    }
    Affine operator*(const Affine& m) const noexcept
    {
        return {a * m.a + c * m.b, b * m.a + d * m.b, a * m.c + c * m.d, b * m.c + d * m.d,
            a * m.tx + c * m.ty + tx, b * m.tx + d * m.ty + ty};
    }
};

bool isZNormal(double x, double y, double z, bool& flipped)
{
    const double length = std::sqrt(x * x + y * y + z * z);
    if (!(length > 0.0))
    {
        flipped = false;
        return true;  // 没给法向按 Z 轴
    }
    flipped = z < 0.0;
    return std::fabs(x) <= 1.0e-9 * length && std::fabs(y) <= 1.0e-9 * length;
}

/// @brief 带填充的多边形能做成 SOLID：三角形，或凸四边形（SOLID 按"Z"字排的两个三角形画，凹的画不对）
bool isSolidPolygon(const std::vector<YiCadPoint2d>& points)
{
    if (points.size() == 3)
    {
        return true;
    }
    if (points.size() != 4)
    {
        return false;
    }
    int sign = 0;
    for (std::size_t i = 0; i < 4; ++i)
    {
        const auto& a = points[i];
        const auto& b = points[(i + 1) % 4];
        const auto& c = points[(i + 2) % 4];
        const double cross = (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x);
        const int s = cross > 0.0 ? 1 : (cross < 0.0 ? -1 : 0);
        if (s != 0 && sign != 0 && s != sign)
        {
            return false;
        }
        sign = s != 0 ? s : sign;
    }
    return true;
}

/// @brief 把读到的图元交出：经当前变换算到世界坐标，非相似变换下圆、圆弧、椭圆离散成折线
class ShapeSink
{
public:
    ShapeSink(std::vector<ProxyShape>& out) : m_out(out) {}

    Affine transform;
    ProxyAttributes attributes;
    bool fill = false;

    void polyline(std::vector<YiCadPoint2d> points, std::vector<double> bulges, bool closed)
    {
        ProxyShape shape;
        shape.kind = ProxyShape::Kind::Polyline;
        shape.attributes = attributes;
        double scale = 1.0;
        const bool similar = transform.similarity(scale);
        const bool mirrored = transform.determinant() < 0.0;
        bool hasBulge = std::any_of(bulges.begin(), bulges.end(), [](double v) { return std::fabs(v) > 1.0e-12; });
        if (hasBulge && !similar)
        {
            // 非相似变换下凸度段是椭圆弧：先在原坐标里离散
            std::vector<YiCadPoint2d> sampled;
            const std::size_t n = points.size();
            const std::size_t segments = closed ? n : n - 1;
            for (std::size_t i = 0; i < segments; ++i)
            {
                const auto& p = points[i];
                const auto& q = points[(i + 1) % n];
                sampled.push_back(p);
                const double bulge = i < bulges.size() ? bulges[i] : 0.0;
                if (std::fabs(bulge) > 1.0e-12)
                {
                    YiCadPoint2d center{};
                    double radius = 0.0, start = 0.0, sweep = 0.0;
                    bulgeArc(p, q, bulge, center, radius, start, sweep);
                    std::vector<YiCadPoint2d> arc;
                    sampleArc(center, radius, start, sweep, arc);
                    if (bulge < 0.0)
                    {
                        std::reverse(arc.begin(), arc.end());
                    }
                    sampled.insert(sampled.end(), arc.begin() + 1, arc.end() - 1);
                }
            }
            if (!closed)
            {
                sampled.push_back(points.back());
            }
            points = std::move(sampled);
            bulges.clear();
        }
        for (auto& p : points)
        {
            p = transform.apply(p);
        }
        if (mirrored)
        {
            for (auto& v : bulges)
            {
                v = -v;
            }
        }
        shape.points = std::move(points);
        shape.bulges = std::move(bulges);
        shape.closed = closed;
        m_out.push_back(std::move(shape));
    }

    void solid(std::vector<YiCadPoint2d> points)
    {
        ProxyShape shape;
        shape.kind = ProxyShape::Kind::Solid;
        shape.attributes = attributes;
        for (auto& p : points)
        {
            p = transform.apply(p);
        }
        shape.points = std::move(points);
        m_out.push_back(std::move(shape));
    }

    void fillLoops(std::vector<std::vector<YiCadPoint2d>> loops, std::vector<bool> holes)
    {
        ProxyShape shape;
        shape.kind = ProxyShape::Kind::Fill;
        shape.attributes = attributes;
        for (auto& loop : loops)
        {
            for (auto& p : loop)
            {
                p = transform.apply(p);
            }
        }
        shape.loops = std::move(loops);
        shape.holes = std::move(holes);
        m_out.push_back(std::move(shape));
    }

    /// @brief 圆弧：从 start 逆时针转 sweep；full 为整圆
    void arc(YiCadPoint2d center, double radius, double start, double sweep, bool full)
    {
        double scale = 1.0;
        if (!transform.similarity(scale))
        {
            std::vector<YiCadPoint2d> points;
            sampleArc(center, radius, start, full ? TwoPi : sweep, points);
            if (full)
            {
                points.pop_back();
            }
            polyline(std::move(points), {}, full);
            return;
        }
        ProxyShape shape;
        shape.attributes = attributes;
        shape.center = transform.apply(center);
        shape.radius = radius * scale;
        if (full)
        {
            shape.kind = ProxyShape::Kind::Circle;
            m_out.push_back(std::move(shape));
            return;
        }
        shape.kind = ProxyShape::Kind::Arc;
        const YiCadPoint2d from = transform.apply({center.x + radius * std::cos(start), center.y + radius * std::sin(start)});
        const YiCadPoint2d to = transform.apply(
            {center.x + radius * std::cos(start + sweep), center.y + radius * std::sin(start + sweep)});
        double a0 = std::atan2(from.y - shape.center.y, from.x - shape.center.x);
        double a1 = std::atan2(to.y - shape.center.y, to.x - shape.center.x);
        if (transform.determinant() < 0.0)
        {
            std::swap(a0, a1);
        }
        shape.startAngle = a0;
        shape.endAngle = a1;
        m_out.push_back(std::move(shape));
    }

    void ellipse(YiCadPoint2d center, YiCadVector2d major, double ratio, double start, double end)
    {
        if (!transform.identity())
        {
            std::vector<YiCadPoint2d> points;
            sampleEllipse(center, major, ratio, start, end, points);
            polyline(std::move(points), {}, false);
            return;
        }
        ProxyShape shape;
        shape.kind = ProxyShape::Kind::Ellipse;
        shape.attributes = attributes;
        shape.center = center;
        shape.majorAxis = major;
        shape.ratio = ratio;
        shape.startAngle = start;
        shape.endAngle = end;
        m_out.push_back(std::move(shape));
    }

    void text(YiCadPoint2d position, double rotation, double height, double widthFactor, double oblique,
        std::string value, std::string font)
    {
        ProxyShape shape;
        shape.kind = ProxyShape::Kind::Text;
        shape.attributes = attributes;
        shape.center = transform.apply(position);
        const YiCadVector2d dir = transform.applyVector({std::cos(rotation), std::sin(rotation)});
        shape.rotation = std::atan2(dir.y, dir.x);
        shape.height = height * std::sqrt(std::fabs(transform.determinant()));
        shape.widthFactor = widthFactor > 0.0 ? widthFactor : 1.0;
        shape.obliqueAngle = oblique;
        shape.text = std::move(value);
        shape.font = std::move(font);
        m_out.push_back(std::move(shape));
    }

    void line(YiCadPoint2d base, YiCadPoint2d other, ProxyShape::Kind kind)
    {
        ProxyShape shape;
        shape.kind = kind;
        shape.attributes = attributes;
        shape.center = transform.apply(base);
        const YiCadPoint2d to = transform.apply(other);
        shape.majorAxis = {to.x - shape.center.x, to.y - shape.center.y};
        m_out.push_back(std::move(shape));
    }

private:
    std::vector<ProxyShape>& m_out;
};

/// @brief 三点定圆；共线时返回 false
bool circleThrough(YiCadPoint2d p1, YiCadPoint2d p2, YiCadPoint2d p3, YiCadPoint2d& center, double& radius)
{
    const double d = 2.0 * (p1.x * (p2.y - p3.y) + p2.x * (p3.y - p1.y) + p3.x * (p1.y - p2.y));
    if (std::fabs(d) < 1.0e-15)
    {
        return false;
    }
    const double s1 = p1.x * p1.x + p1.y * p1.y;
    const double s2 = p2.x * p2.x + p2.y * p2.y;
    const double s3 = p3.x * p3.x + p3.y * p3.y;
    center = {(s1 * (p2.y - p3.y) + s2 * (p3.y - p1.y) + s3 * (p1.y - p2.y)) / d,
        (s1 * (p3.x - p2.x) + s2 * (p1.x - p3.x) + s3 * (p2.x - p1.x)) / d};
    radius = std::hypot(p1.x - center.x, p1.y - center.y);
    return true;
}

YiCadColorData aciColor(uint32_t aci)
{
    if (aci == AciByBlock)
    {
        return {YICAD_COLOR_BY_BLOCK, 0, 0, 0, 0, 0};
    }
    if (aci >= 1 && aci <= 255)
    {
        return {YICAD_COLOR_ACI, aci, 0, 0, 0, 0};
    }
    return {YICAD_COLOR_BY_LAYER, 0, 0, 0, 0, 0};
}

} // namespace

bool readProxyGraphics(
    std::span<const uint8_t> data,
    const std::vector<std::string>& layers,
    const std::vector<std::string>& lineTypes,
    bool r2010,
    std::vector<ProxyShape>& shapes)
{
    shapes.clear();
    if (data.size() < 8)
    {
        return false;
    }
    ShapeSink sink(shapes);
    std::vector<Affine> stack;
    std::size_t index = 8;
    while (index + 8 <= data.size())
    {
        uint32_t size = 0, type = 0;
        ByteStream head(data.subspan(index, 8));
        head.u32(size);
        head.u32(type);
        if (size < 8 || index + size > data.size())
        {
            return false;
        }
        const auto payload = data.subspan(index + 8, size - 8);
        index += size;
        ByteStream in(payload);
        double x = 0, y = 0, z = 0, nx = 0, ny = 0, nz = 1;
        bool flipped = false;
        switch (type)
        {
        case CmdCircle:
        {
            double radius = 0.0;
            if (!in.point3(x, y, z) || !in.f64(radius) || !in.point3(nx, ny, nz) || !isZNormal(nx, ny, nz, flipped))
            {
                return false;
            }
            sink.arc({x, y}, radius, 0.0, TwoPi, true);
            break;
        }
        case CmdCircle3P:
        {
            double x2, y2, z2, x3, y3, z3;
            YiCadPoint2d center{};
            double radius = 0.0;
            if (!in.point3(x, y, z) || !in.point3(x2, y2, z2) || !in.point3(x3, y3, z3))
            {
                return false;
            }
            if (circleThrough({x, y}, {x2, y2}, {x3, y3}, center, radius))
            {
                sink.arc(center, radius, 0.0, TwoPi, true);
            }
            break;
        }
        case CmdArc:
        {
            double radius = 0.0, sx, sy, sz, sweep = 0.0;
            if (!in.point3(x, y, z) || !in.f64(radius) || !in.point3(nx, ny, nz) || !in.point3(sx, sy, sz) ||
                !in.f64(sweep) || !isZNormal(nx, ny, nz, flipped))
            {
                return false;
            }
            const double start = std::atan2(sy, sx);
            // 法向为 -Z 时绕 -Z 转即顺时针：换成逆时针
            if (flipped)
            {
                sink.arc({x, y}, radius, start - sweep, sweep, false);
            }
            else
            {
                sink.arc({x, y}, radius, start, sweep, false);
            }
            break;
        }
        case CmdArc3P:
        {
            double x2, y2, z2, x3, y3, z3;
            YiCadPoint2d center{};
            double radius = 0.0;
            if (!in.point3(x, y, z) || !in.point3(x2, y2, z2) || !in.point3(x3, y3, z3))
            {
                return false;
            }
            if (circleThrough({x, y}, {x2, y2}, {x3, y3}, center, radius))
            {
                // 起点 p1、经过 p2、终点 p3
                const double a1 = std::atan2(y - center.y, x - center.x);
                const double a2 = std::atan2(y2 - center.y, x2 - center.x);
                const double a3 = std::atan2(y3 - center.y, x3 - center.x);
                const double ccw13 = positiveSweep(a1, a3);
                const double ccw12 = positiveSweep(a1, a2);
                if (ccw12 < ccw13)
                {
                    sink.arc(center, radius, a1, ccw13, false);
                }
                else
                {
                    sink.arc(center, radius, a3, positiveSweep(a3, a1), false);
                }
            }
            break;
        }
        case CmdPolyline:
        case CmdPolygon:
        case CmdPolylineNormals:
        {
            uint32_t count = 0;
            if (!in.u32(count) || count > 10000000)
            {
                return false;
            }
            std::vector<YiCadPoint2d> points;
            points.reserve(count);
            for (uint32_t i = 0; i < count; ++i)
            {
                if (!in.point3(x, y, z))
                {
                    return false;
                }
                points.push_back({x, y});
            }
            if (type == CmdPolylineNormals && (!in.point3(nx, ny, nz) || !isZNormal(nx, ny, nz, flipped)))
            {
                return false;
            }
            const bool polygon = type == CmdPolygon;
            if (sink.fill && polygon && isSolidPolygon(points))
            {
                sink.solid(std::move(points));
            }
            else if (sink.fill && points.size() > 2)
            {
                sink.fillLoops({std::move(points)}, {false});
            }
            else if (!points.empty())
            {
                sink.polyline(std::move(points), {}, polygon);
            }
            break;
        }
        case CmdLwPolyline:
        {
            BitStream bits(payload);
            uint32_t byteCount = 0;
            int32_t flags = 0, count = 0, bulgeCount = 0, idCount = 0, widthCount = 0;
            double value = 0.0;
            if (!bits.rawLong(byteCount) || !bits.bitShort(flags))
            {
                return false;
            }
            if ((flags & 4) && !bits.bitDouble(value)) return false;     // 统一宽度
            if ((flags & 8) && !bits.bitDouble(value)) return false;     // 标高
            if ((flags & 2) && !bits.bitDouble(value)) return false;     // 厚度
            if (flags & 1)
            {
                if (!bits.bitDouble(nx) || !bits.bitDouble(ny) || !bits.bitDouble(nz) || !isZNormal(nx, ny, nz, flipped))
                {
                    return false;
                }
            }
            if (!bits.bitLong(count) || count < 0 || count > 10000000)
            {
                return false;
            }
            if ((flags & 16) && (!bits.bitLong(bulgeCount) || bulgeCount < 0 || bulgeCount > count)) return false;
            if (r2010 && (flags & 1024) && (!bits.bitLong(idCount) || idCount < 0)) return false;
            if ((flags & 32) && (!bits.bitLong(widthCount) || widthCount < 0)) return false;
            std::vector<YiCadPoint2d> points;
            for (int32_t i = 0; i < count; ++i)
            {
                double px = 0.0, py = 0.0;
                if (i == 0)
                {
                    if (!bits.rawDouble(px) || !bits.rawDouble(py)) return false;
                }
                else if (!bits.bitDoubleDefault(points.back().x, px) || !bits.bitDoubleDefault(points.back().y, py))
                {
                    return false;
                }
                points.push_back({px, py});
            }
            std::vector<double> bulges;
            for (int32_t i = 0; i < bulgeCount; ++i)
            {
                if (!bits.bitDouble(value)) return false;
                bulges.push_back(value);
            }
            // 法向为 -Z 的对象坐标系里 X 轴朝世界的 -X：x 取反、凸度反号
            if (flipped)
            {
                for (auto& p : points) p.x = -p.x;
                for (auto& b : bulges) b = -b;
            }
            if (!points.empty())
            {
                const bool closed = (flags & 512) != 0;
                bulges.resize(closed ? points.size() : (points.size() > 0 ? points.size() - 1 : 0), 0.0);
                sink.polyline(std::move(points), std::move(bulges), closed);
            }
            break;
        }
        case CmdShell:
        {
            uint32_t vertexCount = 0, entries = 0;
            if (!in.u32(vertexCount) || vertexCount > 10000000)
            {
                return false;
            }
            std::vector<YiCadPoint2d> vertices;
            for (uint32_t i = 0; i < vertexCount; ++i)
            {
                if (!in.point3(x, y, z)) return false;
                vertices.push_back({x, y});
            }
            if (!in.u32(entries))
            {
                return false;
            }
            std::vector<std::vector<YiCadPoint2d>> loops;
            std::vector<bool> holes;
            uint32_t read = 0;
            while (read < entries)
            {
                int32_t count = 0;
                if (!in.i32(count) || count == 0)
                {
                    return false;
                }
                const uint32_t n = static_cast<uint32_t>(std::abs(count));
                read += 1 + n;
                std::vector<YiCadPoint2d> loop;
                for (uint32_t k = 0; k < n; ++k)
                {
                    uint32_t vertex = 0;
                    if (!in.u32(vertex) || vertex >= vertices.size()) return false;
                    loop.push_back(vertices[vertex]);
                }
                holes.push_back(count < 0);
                loops.push_back(std::move(loop));
            }
            if (sink.fill)
            {
                // 每个外环与跟在它后面的孔洞各成一块
                for (std::size_t i = 0; i < loops.size();)
                {
                    std::vector<std::vector<YiCadPoint2d>> group{loops[i]};
                    std::vector<bool> groupHoles{false};
                    std::size_t j = i + 1;
                    while (j < loops.size() && holes[j])
                    {
                        group.push_back(loops[j]);
                        groupHoles.push_back(true);
                        ++j;
                    }
                    if (group.front().size() >= 3)
                    {
                        sink.fillLoops(std::move(group), std::move(groupHoles));
                    }
                    i = j;
                }
            }
            else
            {
                for (auto& loop : loops)
                {
                    sink.polyline(std::move(loop), {}, true);
                }
            }
            break;
        }
        case CmdText:
        case CmdUnicodeText:
        {
            double dx, dy, dz, height = 0, width = 1, oblique = 0;
            std::string value;
            if (!in.point3(x, y, z) || !in.point3(nx, ny, nz) || !in.point3(dx, dy, dz) || !in.f64(height) ||
                !in.f64(width) || !in.f64(oblique) || !isZNormal(nx, ny, nz, flipped))
            {
                return false;
            }
            if (!(type == CmdUnicodeText ? in.paddedUnicodeString(value) : in.paddedString(value)))
            {
                return false;
            }
            sink.text({x, y}, std::atan2(dy, dx), height, width, oblique, std::move(value), {});
            break;
        }
        case CmdText2:
        case CmdUnicodeText2:
        {
            const bool unicode = type == CmdUnicodeText2;
            double dx, dy, dz, height = 0, width = 1, oblique = 0, tracking = 0;
            int32_t length = 0, raw = 0;
            uint32_t flag = 0;
            std::string value, font, bigFont, typeface;
            if (!in.point3(x, y, z) || !in.point3(nx, ny, nz) || !in.point3(dx, dy, dz) ||
                !isZNormal(nx, ny, nz, flipped) ||
                !(unicode ? in.paddedUnicodeString(value) : in.paddedString(value)) || !in.i32(length) ||
                !in.i32(raw) || !in.f64(height) || !in.f64(width) || !in.f64(oblique) || !in.f64(tracking))
            {
                return false;
            }
            for (int i = 0; i < 5; ++i)
            {
                if (!in.u32(flag)) return false;
            }
            if (unicode)
            {
                for (int i = 0; i < 4; ++i)
                {
                    if (!in.u32(flag)) return false;
                }
                if (!in.paddedUnicodeString(typeface)) return false;
            }
            if (!(unicode ? in.paddedUnicodeString(font) : in.paddedString(font)))
            {
                return false;
            }
            sink.text({x, y}, std::atan2(dy, dx), height, width, oblique, std::move(value), std::move(font));
            break;
        }
        case CmdXLine:
        case CmdRay:
        {
            double x2, y2, z2;
            if (!in.point3(x, y, z) || !in.point3(x2, y2, z2))
            {
                return false;
            }
            if (x != x2 || y != y2)
            {
                sink.line({x, y}, {x2, y2}, type == CmdRay ? ProxyShape::Kind::Ray : ProxyShape::Kind::XLine);
            }
            break;
        }
        case CmdEllipticArc:
        {
            double major = 0, minor = 0, start = 0, end = 0, angle = 0;
            if (!in.point3(x, y, z) || !in.point3(nx, ny, nz) || !in.f64(major) || !in.f64(minor) ||
                !in.f64(start) || !in.f64(end) || !in.f64(angle) || !isZNormal(nx, ny, nz, flipped) ||
                !(major > 0.0))
            {
                return false;
            }
            sink.ellipse({x, y}, {major * std::cos(angle), major * std::sin(angle)}, std::min(1.0, minor / major),
                start, end);
            break;
        }
        case CmdMesh:
            // 网格是三维的：整个实体按"只读二维的"不要
            return false;
        case CmdColor:
        {
            uint32_t aci = 0;
            if (!in.u32(aci)) return false;
            sink.attributes.hasColor = true;
            sink.attributes.color = aciColor(aci);
            break;
        }
        case CmdTrueColor:
        {
            uint32_t raw = 0;
            if (!in.u32(raw)) return false;
            sink.attributes.hasColor = true;
            switch (raw >> 24)
            {
            case 0xC1:
                sink.attributes.color = {YICAD_COLOR_BY_BLOCK, 0, 0, 0, 0, 0};
                break;
            case 0xC2:
                sink.attributes.color = {YICAD_COLOR_RGB, 0, static_cast<uint8_t>(raw >> 16),
                    static_cast<uint8_t>(raw >> 8), static_cast<uint8_t>(raw), 0};
                break;
            case 0xC3:
                sink.attributes.color = aciColor(raw & 0xFF);
                break;
            default:
                sink.attributes.color = {YICAD_COLOR_BY_LAYER, 0, 0, 0, 0, 0};
                break;
            }
            break;
        }
        case CmdLayer:
        {
            uint32_t layer = 0;
            if (!in.u32(layer)) return false;
            if (layer < layers.size())
            {
                sink.attributes.hasLayer = true;
                sink.attributes.layer = layers[layer];
            }
            break;
        }
        case CmdLineType:
        {
            uint32_t lineType = 0;
            if (!in.u32(lineType)) return false;
            if (lineType < lineTypes.size())
            {
                sink.attributes.hasLineType = true;
                sink.attributes.lineType = lineTypes[lineType];
            }
            break;
        }
        case CmdLineWeight:
        {
            int32_t weight = 0;
            if (!in.i32(weight)) return false;
            sink.attributes.hasLineWeight = true;
            sink.attributes.lineWeight = weight;
            break;
        }
        case CmdLineTypeScale:
        {
            double scale = 1.0;
            if (!in.f64(scale)) return false;
            sink.attributes.lineTypeScale = scale > 0.0 ? scale : 1.0;
            break;
        }
        case CmdFill:
        {
            uint32_t on = 0;
            if (!in.u32(on)) return false;
            sink.fill = on != 0;
            break;
        }
        case CmdPushMatrix:
        case CmdPushMatrix2:
        {
            double m[16];
            for (double& v : m)
            {
                if (!in.f64(v)) return false;
            }
            // 按行存、平移在第 4 列（本机 AutoCAD 核对）；把 XY 平面带出平面的变换（第 3 行的 x、y 项、透视行）当作三维
            if (std::fabs(m[8]) > 1.0e-9 || std::fabs(m[9]) > 1.0e-9 || std::fabs(m[12]) > 1.0e-9 ||
                std::fabs(m[13]) > 1.0e-9 || std::fabs(m[14]) > 1.0e-9 || std::fabs(m[15] - 1.0) > 1.0e-9)
            {
                return false;
            }
            stack.push_back(sink.transform);
            sink.transform = sink.transform * Affine{m[0], m[4], m[1], m[5], m[3], m[7]};
            break;
        }
        case CmdPopMatrix:
            if (!stack.empty())
            {
                sink.transform = stack.back();
                stack.pop_back();
            }
            break;
        default:
            // 范围、标记、厚度、打印样式、裁剪、材质等：不影响二维显示
            (void)CmdExtents;
            break;
        }
    }
    return true;
}

std::string toHex(std::span<const uint8_t> data)
{
    static const char digits[] = "0123456789ABCDEF";
    std::string text;
    text.reserve(data.size() * 2);
    for (const uint8_t b : data)
    {
        text.push_back(digits[b >> 4]);
        text.push_back(digits[b & 0x0F]);
    }
    return text;
}

bool fromHex(const std::string& text, std::vector<uint8_t>& out)
{
    auto value = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
    };
    if (text.size() % 2 != 0)
    {
        return false;
    }
    for (std::size_t i = 0; i < text.size(); i += 2)
    {
        const int hi = value(text[i]);
        const int lo = value(text[i + 1]);
        if (hi < 0 || lo < 0)
        {
            return false;
        }
        out.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }
    return true;
}

} // namespace dxf
