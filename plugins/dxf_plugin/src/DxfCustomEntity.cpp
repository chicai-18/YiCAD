/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "DxfCustomEntity.h"

#include "DxfProxyGraphics.h"

#include <drw_interface.h>

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace dxf
{

namespace
{

/// @brief 别的程序的实体数据的标记头
constexpr char ForeignMagic[] = "YCDXFRAW";
constexpr uint32_t ForeignVersion = 1;

std::string formatDouble(double value)
{
    std::ostringstream text;
    text.precision(17);
    text << value;
    return text.str();
}

bool parseDouble(const std::string& text, double& value)
{
    char* end = nullptr;
    value = std::strtod(text.c_str(), &end);
    return end != text.c_str() && std::isfinite(value);
}

int parseInt(const std::string& text)
{
    return std::atoi(text.c_str());
}

/// @brief 原样保留的 ACAD_PROXY_ENTITY 子类段里代理图形的占位：160 组码不会是空的（见 ForeignEntityData::acadProxy）
const std::pair<int, std::string> ProxyGraphicsPlaceholder{160, std::string()};

} // namespace

std::string recordNameFor(const std::string& className)
{
    std::string result;
    result.reserve(className.size());
    for (const unsigned char c : className)
    {
        result.push_back(std::isalnum(c) ? static_cast<char>(std::toupper(c)) : '_');
    }
    return result;
}

std::string appNameFor(const std::string& className)
{
    const auto dot = className.rfind('.');
    return dot == std::string::npos ? className : className.substr(0, dot);
}

std::vector<uint8_t> encodeForeign(const ForeignEntityData& value)
{
    yicad::plugin::ByteWriter writer;
    for (const char c : std::string_view(ForeignMagic))
    {
        writer.u8(static_cast<uint8_t>(c));
    }
    writer.u32(ForeignVersion).string(value.recordName).string(value.appName).u8(value.acadProxy ? 1 : 0);
    writer.u32(static_cast<uint32_t>(value.records.size()));
    for (const auto& [code, text] : value.records)
    {
        writer.i32(code).string(text);
    }
    writer.u32(static_cast<uint32_t>(value.graphics.size()));
    for (const uint8_t b : value.graphics)
    {
        writer.u8(b);
    }
    return writer.take();
}

bool decodeForeign(std::span<const uint8_t> bytes, ForeignEntityData& value)
{
    const std::string_view magic(ForeignMagic);
    if (bytes.size() < magic.size() ||
        std::string_view(reinterpret_cast<const char*>(bytes.data()), magic.size()) != magic)
    {
        return false;
    }
    try
    {
        yicad::plugin::ByteReader reader(bytes.subspan(magic.size()));
        if (reader.u32() != ForeignVersion)
        {
            return false;
        }
        value = {};
        value.recordName = reader.string();
        value.appName = reader.string();
        value.acadProxy = reader.u8() != 0;
        const uint32_t count = reader.u32();
        for (uint32_t i = 0; i < count; ++i)
        {
            const int code = reader.i32();
            value.records.emplace_back(code, reader.string());
        }
        const uint32_t graphics = reader.u32();
        if (graphics > reader.remaining())
        {
            return false;
        }
        for (uint32_t i = 0; i < graphics; ++i)
        {
            value.graphics.push_back(reader.u8());
        }
        return reader.atEnd();
    }
    catch (...)
    {
        return false;
    }
}

bool parseCustomEntity(const DRW_UnknownEntity& entity, ParsedCustomEntity& out)
{
    out = {};
    std::string graphicsHex;
    std::string proxyGraphicsHex;
    double ex = 0.0, ey = 0.0, ez = 1.0;
    // 公共属性（AcDbEntity）→ [ACAD_PROXY_ENTITY 的 AcDbProxyEntity] → 子类段
    enum class Section { Common, Proxy, Subclass };
    Section section = Section::Common;
    bool inGroup = false;           // 102 {… } 应用组（反应器、扩展字典：指向没保留的对象，不要）
    bool inXData = false;
    bool inProxyGraphics = false;   // AcDbProxyEntity 里 92/160 之后的 310
    std::string dataHex;
    // 子类段开始：YiCAD 的类读数据，别的程序的类原样保留
    auto beginSubclass = [&](int code, const std::string& text) {
        section = Section::Subclass;
        out.yicad = text == YiCadSubclassMarker;
        if (!out.yicad)
        {
            out.subclassRecords.emplace_back(code, text);
        }
    };
    for (const auto& [code, text] : entity.records)
    {
        if (code == 102)
        {
            inGroup = !text.empty() && text.front() == '{';
            continue;
        }
        if (inGroup)
        {
            continue;
        }
        if (section == Section::Common)
        {
            if (code == 100 && text == "AcDbProxyEntity")
            {
                section = Section::Proxy;
                out.acadProxy = true;
                out.proxyRecords.emplace_back(code, text);
                continue;
            }
            if (code == 100 && text != "AcDbEntity")
            {
                beginSubclass(code, text);
                continue;
            }
            switch (code)
            {
            case 8: out.layer = text; break;
            case 6: out.lineType = text; break;
            case 62: out.color = parseInt(text); break;
            case 420: out.color24 = parseInt(text); break;
            case 370: out.lineWeight = parseInt(text); break;
            case 48: parseDouble(text, out.lineTypeScale); break;
            case 60: out.visible = parseInt(text) == 0; break;
            case 67: out.paperSpace = parseInt(text) == 1; break;
            case 310: graphicsHex += text; break;
            default:
                // 句柄、所有者、字节数（92/160）、材质、透明度、打印样式……：不保留
                break;
            }
            continue;
        }
        if (section == Section::Proxy)
        {
            if (code == 100 && out.proxyDxfData)
            {
                // 原数据是 DXF 组码：从这里起同普通记录的子类段
                beginSubclass(code, text);
                continue;
            }
            if (inProxyGraphics && code == 310)
            {
                proxyGraphicsHex += text;
                continue;
            }
            inProxyGraphics = false;
            switch (code)
            {
            case 91: out.proxyClassId = parseInt(text); break;
            case 70: out.proxyDxfData = parseInt(text) == 1; break;
            case 92:
            case 160:
                // 代理图形（新版本 AutoCAD 在这里再写一份）：取出来，原样保留的组码里换成占位
                inProxyGraphics = true;
                out.proxyRecords.push_back(ProxyGraphicsPlaceholder);
                continue;
            default:
                break;
            }
            out.proxyRecords.emplace_back(code, text);
            continue;
        }
        if (code >= 1000)
        {
            inXData = true;
        }
        if (code == 210) parseDouble(text, ex);
        if (code == 220) parseDouble(text, ey);
        if (code == 230) parseDouble(text, ez);
        if (out.yicad && !inXData)
        {
            switch (code)
            {
            case 90: out.version = static_cast<uint32_t>(std::strtoul(text.c_str(), nullptr, 10)); break;
            case 310: dataHex += text; break;
            case 40: parseDouble(text, out.transform.a); break;
            case 41: parseDouble(text, out.transform.b); break;
            case 42: parseDouble(text, out.transform.c); break;
            case 43: parseDouble(text, out.transform.d); break;
            case 44: parseDouble(text, out.transform.tx); break;
            case 45: parseDouble(text, out.transform.ty); break;
            default: break;
            }
            continue;
        }
        if (!out.yicad)
        {
            out.subclassRecords.emplace_back(code, text);
        }
    }
    if (graphicsHex.empty())
    {
        graphicsHex = std::move(proxyGraphicsHex);
    }
    const double length = std::sqrt(ex * ex + ey * ey + ez * ez);
    out.planar = length > 0.0 && ez > 0.0 && std::fabs(ex) <= 1.0e-9 * length && std::fabs(ey) <= 1.0e-9 * length;
    return fromHex(graphicsHex, out.graphics) && fromHex(dataHex, out.data);
}

GroupCodes graphicsCodes(std::span<const uint8_t> graphics)
{
    GroupCodes codes;
    if (graphics.empty())
    {
        return codes;
    }
    codes.emplace_back(160, std::to_string(graphics.size()));
    for (std::size_t offset = 0; offset < graphics.size(); offset += 127)
    {
        codes.emplace_back(310, toHex(graphics.subspan(offset, std::min<std::size_t>(127, graphics.size() - offset))));
    }
    return codes;
}

GroupCodes yicadDataCodes(uint32_t version, std::span<const uint8_t> data, const YiCadMatrix2d& transform)
{
    GroupCodes codes;
    codes.emplace_back(100, YiCadSubclassMarker);
    codes.emplace_back(90, std::to_string(version));
    codes.emplace_back(91, std::to_string(data.size()));
    for (std::size_t offset = 0; offset < data.size(); offset += 127)
    {
        codes.emplace_back(310, toHex(data.subspan(offset, std::min<std::size_t>(127, data.size() - offset))));
    }
    const bool identity = transform.a == 1.0 && transform.b == 0.0 && transform.c == 0.0 && transform.d == 1.0 &&
                          transform.tx == 0.0 && transform.ty == 0.0;
    if (!identity)
    {
        codes.emplace_back(40, formatDouble(transform.a));
        codes.emplace_back(41, formatDouble(transform.b));
        codes.emplace_back(42, formatDouble(transform.c));
        codes.emplace_back(43, formatDouble(transform.d));
        codes.emplace_back(44, formatDouble(transform.tx));
        codes.emplace_back(45, formatDouble(transform.ty));
    }
    return codes;
}

} // namespace dxf
