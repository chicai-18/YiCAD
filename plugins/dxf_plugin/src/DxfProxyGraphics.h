/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/// @file DxfProxyGraphics.h
/// @brief AutoCAD 代理图形（DXF 组码 160/92 + 310）的编码与解码（RENDER_PLAN.md 第 8.4 步）
///
/// 格式：8 字节头（总字节数、命令数），之后逐条命令：4 字节长度（含这 8 字节）、4 字节类型、数据；
/// 数据按 4 字节对齐，小端序。命令类型与数据布局同 ODA 的说明与 ezdxf 的 proxygraphic.py，
/// 下面几点已在本机 AutoCAD 2026 上核对（2026-10-05）：
/// - 图层（16）、线型（18）给的是序号，即该表在 DXF 文件里的记录顺序；
/// - 真彩色（22）为 0xC2RRGGBB；颜色（14）为 ACI，0 随块、256 随层；
/// - 变换（29）是 16 个 double，按行存，平移在第 4 列（x' = m0·x + m1·y + m3）；
/// - SHELL（9）的面里负的顶点数表示孔洞，面表之后要有边、面、顶点三组标志，少一组 AutoCAD 会崩溃；
/// - 填充（20）开着时 POLYGON、SHELL 画成实心。

#ifndef YICAD_DXF_PROXY_GRAPHICS_H
#define YICAD_DXF_PROXY_GRAPHICS_H

#include "YiCadPluginSdk.h"

#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace dxf
{

/// @brief 表名（大写）到它在 DXF 文件里的序号
using TableIndex = std::unordered_map<std::string, uint32_t>;

/// @brief 把自定义实体的图形（宿主做成的基本实体，属性已解析）写成 AutoCAD 代理图形
class ProxyGraphicsWriter
{
public:
    /// @param owner 自定义实体自己的属性：与它相同的属性不写（代理图形的初值即实体的属性）
    ProxyGraphicsWriter(
        const TableIndex& layers,
        const TableIndex& lineTypes,
        const yicad::plugin::EntityAttributes& owner);

    /// @brief 写一个基本实体；块、文字、图案填充、图片等由宿主事先做成线条，这里不认识的跳过
    void add(const yicad::plugin::EntityData& entity);

    bool empty() const noexcept { return m_count == 0; }

    /// @brief 加上头，交出全部字节
    std::vector<uint8_t> finish() const;

private:
    void applyAttributes(const yicad::plugin::EntityAttributes& attributes);
    void command(uint32_t type, const std::vector<uint8_t>& payload);
    void polyline(const std::vector<YiCadPoint2d>& points, uint32_t type);
    void circle(YiCadPoint2d center, double radius);
    void arc(YiCadPoint2d center, double radius, double startAngle, double sweep);
    void setFill(bool on);
    void shell(const std::vector<std::vector<YiCadPoint2d>>& loops, const std::vector<bool>& holes);

    const TableIndex& m_layers;
    const TableIndex& m_lineTypes;
    std::vector<uint8_t> m_body;
    uint32_t m_count = 0;
    // 当前属性（初值为实体自己的）
    YiCadColorData m_color{};
    std::string m_layer;
    std::string m_lineType;
    int32_t m_lineWeight = -1;
    double m_lineTypeScale = 1.0;
    double m_ownerLineTypeScale = 1.0;
};

/// @brief 代理图形里一个图元的属性：没给的项沿用自定义实体自己的
struct ProxyAttributes
{
    bool hasColor = false;
    YiCadColorData color{};
    bool hasLayer = false;
    std::string layer;
    bool hasLineType = false;
    std::string lineType;
    bool hasLineWeight = false;
    int32_t lineWeight = -1;
    double lineTypeScale = 1.0;
};

/// @brief 代理图形解出的一个图元（二维，已按变换算到世界坐标）
struct ProxyShape
{
    enum class Kind
    {
        Circle,
        Arc,
        Polyline,       ///< 顶点、凸度（可为空）、闭合
        Fill,           ///< 实心：loops 第一个是外环，holes 标出孔洞
        Solid,          ///< 填充的三角形或凸四边形（POLYGON，写出时 SOLID 就写成它）：points 按边界顺序
        Text,
        Ray,
        XLine,
        Ellipse,
    };

    Kind kind = Kind::Polyline;
    ProxyAttributes attributes;
    std::vector<YiCadPoint2d> points;
    std::vector<double> bulges;
    bool closed = false;
    std::vector<std::vector<YiCadPoint2d>> loops;
    std::vector<bool> holes;
    YiCadPoint2d center{};
    double radius = 0.0;
    double startAngle = 0.0;    ///< 圆弧、椭圆弧的起点（弧度）
    double endAngle = 0.0;      ///< 逆时针到终点（弧度）
    YiCadVector2d majorAxis{};
    double ratio = 1.0;
    std::string text;
    double height = 0.0;
    double rotation = 0.0;
    double widthFactor = 1.0;
    double obliqueAngle = 0.0;
    std::string font;           ///< 文字的字体文件（TEXT2、UNICODE_TEXT2 给出），可为空
};

/// @brief 解码 AutoCAD 代理图形
/// @param layers、lineTypes 表在 DXF 文件里按记录顺序的名字
/// @param r2010 DXF 是否为 R2010 起（LWPOLYLINE 命令的格式随版本）
/// @param[out] shapes 解出的图元
/// @return false：数据不完整，或含三维的图元（法向不是 Z 轴、网格等），按"只读二维的"整个不要
bool readProxyGraphics(
    std::span<const uint8_t> data,
    const std::vector<std::string>& layers,
    const std::vector<std::string>& lineTypes,
    bool r2010,
    std::vector<ProxyShape>& shapes);

/// @brief 字节转大写十六进制
std::string toHex(std::span<const uint8_t> data);
/// @brief 十六进制转字节；含非十六进制字符或长度为奇数时返回 false
bool fromHex(const std::string& text, std::vector<uint8_t>& out);

} // namespace dxf

#endif
