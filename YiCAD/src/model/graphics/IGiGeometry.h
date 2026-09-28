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

/// @file IGiGeometry.h
/// @brief GI 的图元词汇（RENDER_PLAN.md 第 4.2.1 节）

#ifndef IGIGEOMETRY_H
#define IGIGEOMETRY_H

#include <cstdint>
#include <span>

#include "GiTypes.h"

class IGiDrawable;
struct GiNurbs;

/// @brief 图元词汇。坐标一律 double，处在当前模型变换下（块内实体即块定义坐标系）
/// @details 这套词汇同时是实体要用的全部绘图能力与 GS、各后端要支持的全部图元，宁可少而完整。
///          圆、圆弧、椭圆弧、NURBS、带凸度的多段线都以解析形式给出，离散由 GS 负责（第 4.2.2 节）
class IGiGeometry
{
public:
    virtual ~IGiGeometry() = default;

    /// @brief 多段线
    /// @param points 顶点
    /// @param bulges 每段的凸度；为空表示全是直线段，否则每段一个（闭合时与 points 等长，否则少一个）
    /// @param widths 每段的起止宽度；为空表示细线，否则与 bulges 的段数相同
    /// @param flags 闭合、线型生成
    virtual void polyline(std::span<const DmVector> points, std::span<const double> bulges,
                          std::span<const GiSegmentWidth> widths, GiPolylineFlags flags) = 0;

    /// @brief 整圆，从 0° 方向起逆时针
    virtual void circle(const DmVector& center, double radius) = 0;

    /// @brief 圆弧，从 startAngle 起转过 sweepAngle（弧度，正为逆时针）
    virtual void arc(const DmVector& center, double radius, double startAngle, double sweepAngle) = 0;

    /// @brief 椭圆弧，参数从 startParam 到 endParam 逆时针；endParam - startParam 为 2π 时是整椭圆
    /// @param majorAxis 长轴向量（圆心到长轴端点）
    /// @param ratio 短轴与长轴之比
    virtual void ellipseArc(const DmVector& center, const DmVector& majorAxis, double ratio,
                            double startParam, double endParam) = 0;

    /// @brief B 样条（NURBS）曲线
    virtual void nurbs(const GiNurbs& curve) = 0;

    /// @brief 填充区域，由 GS 三角剖分
    virtual void fill(std::span<const GiLoop> loops, GiFillRule rule) = 0;

    /// @brief 已剖分好的三角形；indices 每 3 个一组
    virtual void triangles(std::span<const DmVector> vertices, std::span<const std::uint32_t> indices) = 0;

    /// @brief 已排版的字形
    virtual void glyphRun(const GiGlyphRun& run) = 0;

    /// @brief 光栅图像
    virtual void image(const GiImage& image) = 0;

    /// @brief 点
    virtual void point(const DmVector& position) = 0;

    /// @brief 射线：从 base 沿 direction 无限延伸
    virtual void ray(const DmVector& base, const DmVector& direction) = 0;

    /// @brief 构造线：过 base、沿 direction 两端无限延伸
    virtual void xline(const DmVector& base, const DmVector& direction) = 0;

    /// @brief 画一个嵌套的可绘制对象：按它自己的属性（setAttributes）画，它的 ByBlock 取当前属性
    /// @details 对应 AutoCAD 的 AcGiGeometry::draw。与 drawShared 不同，GS 不为它单独缓存，
    ///          它的图元是调用方的一部分（标注、引线的内容，块参照的属性，块定义里的实体）
    virtual void draw(const IGiDrawable& drawable) = 0;

    /// @brief 画一个共享的可绘制对象（块定义），由 GS 缓存一次、按变换实例化
    /// @param byBlock 共享对象里 ByBlock 属性取的值，按调用方当时的上下文解析
    virtual void drawShared(const IGiDrawable& drawable, const GiTransform& transform,
                            const GiByBlockTraits& byBlock) = 0;

    /// @brief 之后的图元先经 transform 再经当前模型变换
    virtual void pushTransform(const GiTransform& transform) = 0;

    /// @brief 恢复上一次 pushTransform 之前的模型变换
    virtual void popTransform() = 0;
};

#endif // IGIGEOMETRY_H
