/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file GiTypes.h
/// @brief GI 的值类型：标志位、线宽段、填充环、字形、图片、ByBlock 属性等（RENDER_PLAN.md 第 4.2 节）

#ifndef GITYPES_H
#define GITYPES_H

#include <cstdint>
#include <vector>

#include <QString>

#include "Datamodel.h"
#include "DmColor.h"
#include "DmVector.h"
#include "GiTransform.h"

class DmLineType;
class IGiFont;
class QImage;

/// @brief 这次生成的用途，实体一般不需要区分
enum class GiRegenType : std::uint8_t
{
    Display,        ///< 显示
    ProxyGraphics,  ///< 代理图形（随图纸存盘）
    Extents,        ///< 范围计算
    Export,         ///< 导出、打印
};

/// @brief 可绘制对象的标志
enum class GiDrawableFlags : std::uint32_t
{
    None = 0,
    ViewDependent = 1u << 0,    ///< 有随视图变化的部分，GS 会调用 viewportDraw
};

/// @brief 多段线的标志
enum class GiPolylineFlags : std::uint32_t
{
    None = 0,
    Closed = 1u << 0,               ///< 闭合：最后一点连回第一点
    ContinuousLinetype = 1u << 1,   ///< 线型生成：整条连续计算弧长，顶点处不重新对齐（第 4.5.1 节）
};

constexpr GiDrawableFlags operator|(GiDrawableFlags a, GiDrawableFlags b)
{
    return static_cast<GiDrawableFlags>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

constexpr GiPolylineFlags operator|(GiPolylineFlags a, GiPolylineFlags b)
{
    return static_cast<GiPolylineFlags>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

/// @brief flags 是否含 flag
constexpr bool hasFlag(GiDrawableFlags flags, GiDrawableFlags flag)
{
    return (static_cast<std::uint32_t>(flags) & static_cast<std::uint32_t>(flag)) != 0;
}

/// @brief flags 是否含 flag
constexpr bool hasFlag(GiPolylineFlags flags, GiPolylineFlags flag)
{
    return (static_cast<std::uint32_t>(flags) & static_cast<std::uint32_t>(flag)) != 0;
}

/// @brief 填充规则
enum class GiFillRule : std::uint8_t
{
    EvenOdd,    ///< 奇偶：被奇数个环包住的区域填充（外环加孔洞）
    NonZero,    ///< 非零环绕数
};

/// @brief 多段线一段的起止宽度（世界单位）；都为 0 时是细线
struct GiSegmentWidth
{
    double start = 0.0;
    double end = 0.0;

    bool operator==(const GiSegmentWidth& other) const = default;
};

/// @brief 填充的一个闭合环：顶点与每段的凸度，最后一点连回第一点
/// @details bulges 为空表示全是直线段，否则与 points 等长，第 i 个是第 i 点到下一点那一段的凸度
struct GiLoop
{
    std::vector<DmVector> points;
    std::vector<double> bulges;
};

/// @brief 内联的线型图案：填充图案线用，不做端点对齐（第 4.5.1 节）
/// @details dashes 与 .lin/.pat 相同：正数是划线，负数是空白，0 是点；长度在实体自身坐标系里，随块缩放
struct GiLinePattern
{
    std::vector<double> dashes;
    double phase = 0.0;     ///< 曲线起点在图案里的位置：曲线上弧长 s 处画的是图案的 (s + phase) 处

    bool operator==(const GiLinePattern& other) const = default;
};

/// @brief 填充图案的一族平行线（对应 ODA 的 OdHatchPatternLine、AutoCAD HATCH 的组码 53/43/44/45/46/49）
/// @details 都在当前坐标系里：base 是这一族里一条线经过的点，图案从这里起算；direction 是线的方向（单位向量）；
///          offset 是从一条线到下一条线的位移（不平行于 direction），第 k 条线经过 base + k·offset，它的图案从那一点起算；
///          dashes 与 .pat 相同（正数划线、负数空白、0 是点），长度沿 direction 量，为空是实线。
///          与 .pat 用角度、线自身坐标系里的位移不同，这里方向与位移都是向量，任意仿射变换后仍是合法的定义：
///          base、offset 照常变换，direction 变换后取单位向量，dashes 乘方向上的伸缩
struct GiHatchPatternLine
{
    DmVector base;
    DmVector direction;
    DmVector offset;
    std::vector<double> dashes;

    bool operator==(const GiHatchPatternLine& other) const = default;
};

/// @brief 填充图案（对应 ODA 的 OdGiHatchPattern）：fill 的区域按这些线族画，而不是涂实
/// @details 图案线由接收方在边界里切出（HatchPatternClipper），图形系统据此在线太密时改画实心（RENDER_PLAN.md 第 4.3.10 节）
struct GiHatchPattern
{
    std::vector<GiHatchPatternLine> lines;

    bool operator==(const GiHatchPattern& other) const = default;
};

/// @brief drawShared 时共享对象里 ByBlock 属性取的值
/// @details 每一项本身可以是 ByLayer（按调用方的图层解析）或 ByBlock（取调用方所在的外层块），
///          块参照传它自己的属性即可
struct GiByBlockTraits
{
    DmColor color = DmColor(DM::FlagByBlock);       ///< 颜色
    DM::LineWidth lineWeight = DM::WidthByBlock;    ///< 线宽
    const DmLineType* lineType = nullptr;           ///< 线型；为空视为 ByBlock
};

/// @brief 一个已排版的字形：字符码与从字形坐标系到当前坐标系的变换
/// @details 变换合成了位置、高度、旋转、宽度系数、倾斜与镜像
struct GiGlyph
{
    char32_t code = 0;
    GiTransform transform;
};

/// @brief 同一字体、同一组属性下的一串字形（第 4.2.1 节）
/// @details 排版（对齐、换行、宽度系数、倾斜）是实体的事，GS 只认排好的字形
struct GiGlyphRun
{
    const IGiFont* font = nullptr;
    std::vector<GiGlyph> glyphs;
};

/// @brief 光栅图像：平行四边形的一个角与两条边，外加像素来源
/// @details 像素 (0, 0) 在 origin 一侧；u 是整张图宽度方向的边，v 是高度方向的边
struct GiImage
{
    DmVector origin;                ///< 左下角
    DmVector u;                     ///< 宽度方向的整条边
    DmVector v;                     ///< 高度方向的整条边
    int width = 0;                  ///< 像素宽
    int height = 0;                 ///< 像素高
    QString path;                   ///< 图片文件路径；为空表示像素内嵌在实体里
    const QImage* pixels = nullptr; ///< 实体里已解码的像素，不持有；可为空
};

#endif // GITYPES_H
