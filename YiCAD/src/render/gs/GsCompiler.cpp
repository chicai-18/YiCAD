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

/// @file GsCompiler.cpp
/// @brief GsCompiler 实现。曲线的离散算法与旧渲染器的适配器（GLCacheWorldDraw，第 4 阶段随旧渲染器删除）相同，改为 double 运算

#include "GsCompiler.h"

#include <algorithm>
#include <bit>
#include <cmath>

#include <QByteArrayView>
#include <QDateTime>
#include <QFileInfo>
#include <QHash>
#include <QImage>

#include "ConstrainedDelaunayTriangulation.h"
#include "Datamodel.h"
#include "DmLayer.h"
#include "DmLineType.h"
#include "DmLineTypeTable.h"
#include "GeometryMethods.h"
#include "IGiFont.h"
#include "Math2d.h"

namespace
{

constexpr double kBulgeTolerance = 1.0e-5;          ///< 与 DmPolyline 相同：凸度小于它的段是直线
constexpr double kDeviation = 1.0e-3;               ///< 实体自行离散时的弦高容差
constexpr double kFullSweep = 2.0 * M_PI - 1.0e-9;  ///< 扫角达到它即为整圆、整椭圆
constexpr int kEllipseSegments = ELLIPSE_SEGMENT_COUNT;
constexpr int kMinOpenEllipseSegments = 10;

double dot(const DmVector& a, const DmVector& b)
{
    return a.x * b.x + a.y * b.y;
}

float bitsToFloat(std::uint32_t bits)
{
    return std::bit_cast<float>(bits);
}

/// @brief 多段线凸度段的圆弧：圆心、半径与逆时针的起止角，同原先 DmPolyline 生成的 DmArc 的"翻正"角度
void bulgeArc(const DmVector& start, const DmVector& end, double bulge, DmVector& center, double& radius,
              double& startAngle, double& endAngle)
{
    DmVector normal(0.0, 0.0, 1.0);
    double a0 = 0.0;
    double a1 = 0.0;
    GeometryMethods::getArcInfo(start, end, bulge, center, radius, a0, a1, normal);
    if (normal.z < 0.0)
    {
        startAngle = Math2d::correctAngle(M_PI - a1);
        endAngle = Math2d::correctAngle(M_PI - a0);
    }
    else
    {
        startAngle = a0;
        endAngle = a1;
    }
}

/// @brief 圆弧上从 startAngle 逆时针到 endAngle 的点（不含起止点），每 6° 一段，同 DmArc::getPoints
void appendArcInterior(const DmVector& center, double radius, double startAngle, double endAngle,
                       std::vector<DmVector>& pts)
{
    const double a = Math2d::correctAngle(endAngle - startAngle);
    double count = std::round(Math2d::rad2deg(a) / 6.0);
    count = std::max(count, 2.0);
    const double delta = a / count;
    for (int i = 1; i < static_cast<int>(count); ++i)
    {
        pts.emplace_back(center + DmVector(startAngle + delta * i) * radius);
    }
}

/// @brief 图片来源，纹理按它缓存：有文件时是绝对路径、修改时间与大小，否则是内嵌像素的尺寸与内容哈希
QString imageKey(const GiImage& image)
{
    if (!image.path.isEmpty())
    {
        const QFileInfo info(image.path);
        return QStringLiteral("file:%1|%2|%3")
            .arg(info.absoluteFilePath())
            .arg(info.lastModified().toMSecsSinceEpoch())
            .arg(info.size());
    }
    const QImage* pixels = image.pixels;
    const uchar* bits = pixels ? pixels->constBits() : nullptr;
    const qsizetype bytes = bits ? static_cast<qsizetype>(pixels->bytesPerLine()) * pixels->height() : 0;
    return QStringLiteral("bits:%1x%2|%3|%4")
        .arg(pixels ? pixels->width() : 0)
        .arg(pixels ? pixels->height() : 0)
        .arg(pixels ? pixels->bytesPerLine() : 0)
        .arg(qHash(QByteArrayView(reinterpret_cast<const char*>(bits), bytes)));
}

}  // namespace

void GsCompiled::clear()
{
    for (auto& r : records)
    {
        r.clear();
    }
    prims.clear();
    shared.clear();
    infinite.clear();
    images.clear();
    hasNonUniformUse = false;
}

bool GsCompiled::empty() const
{
    for (const auto& r : records)
    {
        if (!r.empty())
        {
            return false;
        }
    }
    return shared.empty() && infinite.empty();
}

GsCompiler::GsCompiler(GsCompileContext& context)
    : m_context(context)
{
}

void GsCompiler::begin(std::uint32_t slot, const DmVector& origin, GsCompiled& out)
{
    m_out = &out;
    m_slot = slot;
    m_origin = origin;
    m_frames.clear();
    m_fillPrims.clear();
}

void GsCompiler::compileNode(const IGiDrawable& drawable, std::uint32_t slot, const DmVector& origin, GsCompiled& out)
{
    begin(slot, origin, out);
    pushFrame(GiTransform(), nullptr, false);
    drawInFrame(drawable);
    m_frames.clear();
    m_out = nullptr;
}

void GsCompiler::compileShared(const IGiDrawable& drawable, const DmVector& origin, GsCompiled& out)
{
    begin(kGsNoSlot, origin, out);
    pushFrame(GiTransform(), nullptr, true);
    drawInFrame(drawable);
    m_frames.clear();
    m_out = nullptr;
}

double GsCompiler::deviation() const
{
    return kDeviation;
}

// ---------------------------------------------------------------------------
// 属性
// ---------------------------------------------------------------------------

void GsCompiler::setColor(const DmColor& color)
{
    frame().color = color;
    frame().resolvedValid = false;
}

void GsCompiler::setLayer(const DmLayer* layer)
{
    frame().layer = layer;
    frame().resolvedValid = false;
}

void GsCompiler::setLineType(const DmLineType* lineType)
{
    frame().lineType = lineType;
    frame().resolvedValid = false;
}

void GsCompiler::setLineWeight(DM::LineWidth weight)
{
    frame().width = weight;
    frame().resolvedValid = false;
}

// 与旧渲染器相同，实体线型比例、内联图案、透明度、子实体标记与屏幕空间图元在第 4 阶段不起作用（阶段 5 起逐项补上）
void GsCompiler::setLineTypeScale(double)
{
}

void GsCompiler::setLinePattern(const GiLinePattern&)
{
}

void GsCompiler::setTransparency(std::uint8_t)
{
}

void GsCompiler::setSelectionMarker(std::int32_t)
{
}

void GsCompiler::setScreenSpace(const DmVector*)
{
}

void GsCompiler::pushFrame(const GiTransform& transform, const GsAttributes* parent, bool sharedTop)
{
    Frame f;
    f.hasParent = parent != nullptr;
    if (parent)
    {
        f.parent = *parent;
    }
    f.sharedTop = sharedTop;
    f.transform = transform;
    m_frames.push_back(std::move(f));
}

void GsCompiler::drawInFrame(const IGiDrawable& drawable)
{
    drawable.setAttributes(*this);
    drawable.worldDraw(*this);
}

const GsAttributes& GsCompiler::attributes()
{
    Frame& f = frame();
    if (!f.resolvedValid)
    {
        f.resolved = resolve(f);
        f.resolvedValid = true;
    }
    return f.resolved;
}

GsAttributes GsCompiler::resolve(const Frame& f)
{
    // 规则同 DmEntity::getPen(true)：ByBlock 取外层解析后的属性，再按图层解析 ByLayer；图层为空取外层的图层。
    // 顶层没有外层，ByBlock 与解析不了的 ByLayer 保持原样，旧渲染器把它们画成 RGB 全 0（即 ACI 7）、1 像素、连续线
    GsAttributes r;
    if (f.layer)
    {
        r.layer = m_context.layerIndex(f.layer);
    }
    else if (f.hasParent)
    {
        r.layer = f.parent.layer;
    }
    else if (f.sharedTop)
    {
        r.layer = kGsLayerInstance;
    }

    // 颜色
    if (f.color.isByBlock())
    {
        if (f.hasParent)
        {
            r.color = f.parent.color;
            if (r.color.kind == GsKind::ByLayer && r.color.layer == kGsLayerNone)
            {
                r.color.layer = r.layer;
            }
        }
        else if (f.sharedTop)
        {
            r.color.kind = GsKind::ByBlock;
        }
    }
    else if (f.color.isByLayer())
    {
        if (r.layer != kGsLayerNone)
        {
            r.color.kind = GsKind::ByLayer;
            r.color.layer = r.layer;
        }
    }
    else
    {
        r.color.rgba = gsPackColor(f.color.red(), f.color.green(), f.color.blue(), f.color.alpha());
    }
    if (r.color.kind == GsKind::ByLayer && r.color.layer == kGsLayerNone)
    {
        r.color = GsColorRef();
    }

    // 线宽
    if (f.width == DM::WidthByBlock)
    {
        if (f.hasParent)
        {
            r.lineWeight = f.parent.lineWeight;
            if (r.lineWeight.kind == GsKind::ByLayer && r.lineWeight.layer == kGsLayerNone)
            {
                r.lineWeight.layer = r.layer;
            }
        }
        else if (f.sharedTop)
        {
            r.lineWeight.kind = GsKind::ByBlock;
        }
        else
        {
            r.lineWeight.code = static_cast<std::int16_t>(DM::WidthByBlock);
        }
    }
    else if (f.width == DM::WidthByLayer)
    {
        if (r.layer != kGsLayerNone)
        {
            r.lineWeight.kind = GsKind::ByLayer;
            r.lineWeight.layer = r.layer;
        }
        else
        {
            r.lineWeight.code = static_cast<std::int16_t>(DM::WidthByLayer);
        }
    }
    else
    {
        r.lineWeight.code = static_cast<std::int16_t>(f.width);
    }
    if (r.lineWeight.kind == GsKind::ByLayer && r.lineWeight.layer == kGsLayerNone)
    {
        r.lineWeight = GsLineWeightRef{GsKind::Value, static_cast<std::int16_t>(DM::WidthByLayer), kGsLayerNone};
    }

    // 线型：空视为 ByBlock
    const DmLineType* lineType = f.lineType ? f.lineType : DmLineTypeTable::ByBlock;
    if (lineType == DmLineTypeTable::ByBlock)
    {
        if (f.hasParent)
        {
            r.lineType = f.parent.lineType;
            if (r.lineType.kind == GsKind::ByLayer && r.lineType.layer == kGsLayerNone)
            {
                r.lineType.layer = r.layer;
            }
        }
        else if (f.sharedTop)
        {
            r.lineType.kind = GsKind::ByBlock;
        }
    }
    else if (lineType == DmLineTypeTable::ByLayer)
    {
        if (r.layer != kGsLayerNone)
        {
            r.lineType.kind = GsKind::ByLayer;
            r.lineType.layer = r.layer;
        }
    }
    else
    {
        r.lineType.index = m_context.lineTypeIndex(lineType);
    }
    if (r.lineType.kind == GsKind::ByLayer && r.lineType.layer == kGsLayerNone)
    {
        r.lineType = GsLineTypeRef();
    }
    return r;
}

std::uint32_t GsCompiler::addPrim(GsDashMode mode, double runLength, std::uint8_t flags)
{
    const GsAttributes& a = attributes();
    GsPrimRecord p;
    p.slot = m_slot;
    p.color = a.color.rgba;
    p.layers0 = static_cast<std::uint32_t>(a.layer) | (static_cast<std::uint32_t>(a.color.layer) << 16);
    p.layers1 = static_cast<std::uint32_t>(a.lineType.layer) | (static_cast<std::uint32_t>(a.lineWeight.layer) << 16);
    p.lineTypeAndWeight = gsPackLineTypeAndWeight(a.lineType.index, a.lineWeight.code);
    p.kinds = static_cast<std::uint32_t>(a.color.kind) | (static_cast<std::uint32_t>(a.lineType.kind) << 2)
            | (static_cast<std::uint32_t>(a.lineWeight.kind) << 4) | (static_cast<std::uint32_t>(mode) << 6)
            | (static_cast<std::uint32_t>(flags) << 8);
    p.runLength = static_cast<float>(runLength);
    p.lineTypeScale = 1.0f;
    m_out->prims.push_back(p);
    return static_cast<std::uint32_t>(m_out->prims.size() - 1);
}

std::uint32_t GsCompiler::fillPrim(std::uint8_t flags)
{
    const GsAttributes& a = attributes();
    for (const FillPrim& f : m_fillPrims)
    {
        if (f.flags == flags && f.attributes == a)
        {
            return f.index;
        }
    }
    const std::uint32_t index = addPrim(GsDashMode::None, 0.0, flags);
    m_fillPrims.push_back({a, flags, index});
    return index;
}

GsTexel GsCompiler::localPoint(const DmVector& p, float z, std::uint32_t prim) const
{
    return GsTexel{static_cast<float>(p.x - m_origin.x), static_cast<float>(p.y - m_origin.y), z, bitsToFloat(prim)};
}

// ---------------------------------------------------------------------------
// 嵌套、共享与变换
// ---------------------------------------------------------------------------

void GsCompiler::draw(const IGiDrawable& drawable)
{
    const GsAttributes parent = attributes();
    pushFrame(frame().transform, &parent, false);
    drawInFrame(drawable);
    m_frames.pop_back();
}

void GsCompiler::drawShared(const IGiDrawable& drawable, const GiTransform& transform, const GiByBlockTraits& byBlock)
{
    // GiByBlockTraits 按调用方的上下文解析：ByLayer 取调用方的图层，ByBlock 取调用方的外层（同原先的 GLCacheWorldDraw）
    Frame caller;
    caller.color = byBlock.color;
    caller.width = byBlock.lineWeight;
    caller.lineType = byBlock.lineType;
    caller.layer = frame().layer;
    caller.hasParent = frame().hasParent;
    caller.parent = frame().parent;
    caller.sharedTop = frame().sharedTop;
    const GsAttributes parent = resolve(caller);
    const GiTransform composed = frame().transform * transform;

    // 顶层实体里的块参照：展开到叶子后若有非相似变换下带虚线的内容，就在本单元里按世界坐标展开（第 4.3.3 节）。
    // 共享对象（块定义）里的嵌套块参照照常记成引用：最终的变换要乘上插入的变换，到顶层实体才知道
    if (m_slot != kGsNoSlot)
    {
        bool nonUniform = false;
        if (m_context.needsFlatten(drawable, composed, parent, &nonUniform))
        {
            pushFrame(composed, &parent, false);
            drawInFrame(drawable);
            m_frames.pop_back();
            return;
        }
        m_out->hasNonUniformUse = m_out->hasNonUniformUse || nonUniform;
    }
    m_out->shared.push_back({&drawable, composed, parent});
}

void GsCompiler::pushTransform(const GiTransform& transform)
{
    Frame& f = frame();
    f.savedTransforms.push_back(f.transform);
    f.transform = f.transform * transform;
}

void GsCompiler::popTransform()
{
    Frame& f = frame();
    if (!f.savedTransforms.empty())
    {
        f.transform = f.savedTransforms.back();
        f.savedTransforms.pop_back();
    }
}

void GsCompiler::glyphRun(const GiGlyphRun& run)
{
    if (!run.font)
    {
        return;
    }
    // 每个字形如同一次 drawShared：字形里的 ByBlock 取字形串当时的属性，字形几何全文档共用一份
    const GsAttributes parent = attributes();
    for (const GiGlyph& g : run.glyphs)
    {
        const IGiDrawable* glyph = run.font->glyph(g.code);
        if (!glyph)
        {
            continue;
        }
        m_out->shared.push_back({glyph, frame().transform * g.transform, parent});
    }
}

// ---------------------------------------------------------------------------
// 图元
// ---------------------------------------------------------------------------

void GsCompiler::polyline(std::span<const DmVector> points, std::span<const double> bulges,
                          std::span<const GiSegmentWidth> widths, GiPolylineFlags flags)
{
    const std::size_t n = points.size();
    if (n < 2)
    {
        return;
    }
    const bool closed = hasFlag(flags, GiPolylineFlags::Closed);
    const std::size_t segments = closed ? n : n - 1;
    auto bulgeAt = [&bulges](std::size_t i) { return i < bulges.size() ? bulges[i] : 0.0; };
    const GiTransform& m = frame().transform;

    if (hasFlag(flags, GiPolylineFlags::ContinuousLinetype) && widths.empty())
    {
        // 整条连续：连成一条线串（原先的 LineStrip，样条离散后也是它），圆弧段每 6° 一段
        std::vector<DmVector> strip;
        strip.reserve(n + 1);
        for (std::size_t i = 0; i < segments; ++i)
        {
            const DmVector& s = points[i];
            const DmVector& e = points[(i + 1) % n];
            strip.emplace_back(m.apply(s));
            const double b = bulgeAt(i);
            if (std::fabs(b) >= kBulgeTolerance)
            {
                DmVector center;
                double radius = 0.0, a0 = 0.0, a1 = 0.0;
                bulgeArc(s, e, b, center, radius, a0, a1);
                std::vector<DmVector> interior;
                appendArcInterior(center, radius, a0, a1, interior);
                if (b < 0.0)
                {
                    std::reverse(interior.begin(), interior.end());
                }
                for (const DmVector& p : interior)
                {
                    strip.emplace_back(m.apply(p));
                }
            }
        }
        if (!closed)
        {
            strip.emplace_back(m.apply(points[n - 1]));
        }
        addStrip(strip, closed);
        return;
    }

    // 每段单独
    for (std::size_t i = 0; i < segments; ++i)
    {
        const DmVector& s = points[i];
        const DmVector& e = points[(i + 1) % n];
        const double b = bulgeAt(i);
        const GiSegmentWidth w = i < widths.size() ? widths[i] : GiSegmentWidth{};
        if (w.start != 0.0 || w.end != 0.0)
        {
            addWideSegment(s, e, b, w);
        }
        else if (std::fabs(b) < kBulgeTolerance)
        {
            addLine(m.apply(s), m.apply(e));
        }
        else
        {
            addBulgeArc(s, e, b);
        }
    }
}

void GsCompiler::circle(const DmVector& center, double radius)
{
    addTransformedArc(center, radius, 0.0, 2.0 * M_PI, true);
}

void GsCompiler::arc(const DmVector& center, double radius, double startAngle, double sweepAngle)
{
    if (sweepAngle < 0.0)
    {
        startAngle += sweepAngle;
        sweepAngle = -sweepAngle;
    }
    addTransformedArc(center, radius, startAngle, sweepAngle, false);
}

void GsCompiler::ellipseArc(const DmVector& center, const DmVector& majorAxis, double ratio, double startParam,
                            double endParam)
{
    const bool closed = endParam - startParam >= kFullSweep;
    addTransformedEllipse(center, majorAxis, ratio, startParam, endParam, closed);
}

void GsCompiler::nurbs(const GiNurbs& curve)
{
    // B 样条对仿射变换不变：变换控制点后离散
    GiNurbs transformed = curve;
    const GiTransform& m = frame().transform;
    for (DmVector& p : transformed.controlPoints)
    {
        p = m.apply(p);
    }
    addStrip(m_context.sampleNurbs(transformed), transformed.closed);
}

void GsCompiler::fill(std::span<const GiLoop> loops, GiFillRule)
{
    // 约束 Delaunay 三角剖分按嵌套深度判断内外，即奇偶规则
    const GiTransform& m = frame().transform;
    std::vector<DmVector> outer;
    std::vector<std::vector<DmVector>> holes;
    for (std::size_t li = 0; li < loops.size(); ++li)
    {
        const GiLoop& loop = loops[li];
        std::vector<DmVector> pts;
        const std::size_t n = loop.points.size();
        for (std::size_t i = 0; i < n; ++i)
        {
            pts.emplace_back(m.apply(loop.points[i]));
            const double b = i < loop.bulges.size() ? loop.bulges[i] : 0.0;
            if (std::fabs(b) >= kBulgeTolerance)
            {
                DmVector center;
                double radius = 0.0, a0 = 0.0, a1 = 0.0;
                bulgeArc(loop.points[i], loop.points[(i + 1) % n], b, center, radius, a0, a1);
                std::vector<DmVector> interior;
                appendArcInterior(center, radius, a0, a1, interior);
                if (b < 0.0)
                {
                    std::reverse(interior.begin(), interior.end());
                }
                for (const DmVector& p : interior)
                {
                    pts.emplace_back(m.apply(p));
                }
            }
        }
        if (li == 0)
        {
            outer = std::move(pts);
        }
        else
        {
            holes.emplace_back(std::move(pts));
        }
    }
    std::vector<std::array<DmVector, 3>> tris;
    ConstrainedDelaunayTriangulation::triangulatePoints(outer, holes, tris);
    if (tris.empty())
    {
        return;
    }
    const std::uint32_t prim = fillPrim(kGsPrimFlagFill);
    for (const auto& tri : tris)
    {
        addTriangle(tri[0], tri[1], tri[2], prim);
    }
}

void GsCompiler::triangles(std::span<const DmVector> vertices, std::span<const std::uint32_t> indices)
{
    const GiTransform& m = frame().transform;
    std::uint32_t prim = 0;
    bool hasPrim = false;
    for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        if (indices[i] >= vertices.size() || indices[i + 1] >= vertices.size() || indices[i + 2] >= vertices.size())
        {
            continue;
        }
        if (!hasPrim)
        {
            prim = fillPrim(kGsPrimFlagFill);
            hasPrim = true;
        }
        addTriangle(m.apply(vertices[indices[i]]), m.apply(vertices[indices[i + 1]]),
                    m.apply(vertices[indices[i + 2]]), prim);
    }
}

void GsCompiler::image(const GiImage& image)
{
    const GiTransform& m = frame().transform;
    const DmVector origin = m.apply(image.origin);
    const DmVector u = m.applyVector(image.u);
    const DmVector v = m.applyVector(image.v);
    const std::uint32_t prim = fillPrim(kGsPrimFlagFill);
    auto& records = m_out->records[static_cast<std::size_t>(GsClass::Image)];
    records.push_back(GsTexel{static_cast<float>(origin.x - m_origin.x), static_cast<float>(origin.y - m_origin.y),
                              static_cast<float>(u.x), static_cast<float>(u.y)});
    records.push_back(GsTexel{static_cast<float>(v.x), static_cast<float>(v.y), 0.0f, bitsToFloat(prim)});

    // 纹理按图片来源缓存，只有新的来源才解码、上传（RENDER_PLAN.md 1.2 步）
    const QString path = image.path;
    const QImage* pixels = image.pixels;
    m_out->images.push_back({imageKey(image), [path, pixels]() {
                                 if (!path.isEmpty())
                                 {
                                     return QImage(path);
                                 }
                                 return pixels ? QImage(*pixels) : QImage();
                             }});
}

void GsCompiler::point(const DmVector& position)
{
    const DmVector p = frame().transform.apply(position);
    const std::uint32_t prim = fillPrim(kGsPrimFlagPoint);
    m_out->records[static_cast<std::size_t>(GsClass::Point)].push_back(localPoint(p, 0.0f, prim));
}

void GsCompiler::ray(const DmVector& base, const DmVector& direction)
{
    const GiTransform& m = frame().transform;
    const std::uint32_t prim = addPrim(GsDashMode::Infinite, 0.0, 0);
    m_out->infinite.push_back({m.apply(base), m.applyVector(direction), true, prim});
}

void GsCompiler::xline(const DmVector& base, const DmVector& direction)
{
    const GiTransform& m = frame().transform;
    const std::uint32_t prim = addPrim(GsDashMode::Infinite, 0.0, 0);
    m_out->infinite.push_back({m.apply(base), m.applyVector(direction), false, prim});
}

// ---------------------------------------------------------------------------
// 变换后的圆弧、椭圆、多段线段
// ---------------------------------------------------------------------------

void GsCompiler::addTransformedArc(const DmVector& center, double radius, double startAngle, double sweepAngle,
                                   bool full)
{
    const GiTransform& m = frame().transform;
    double scale = 1.0;
    if (m.isSimilarity(&scale))
    {
        const DmVector c = m.apply(center);
        const double r = radius * scale;
        if (full)
        {
            addArcRecord(c, r, 0.0, 2.0 * M_PI, true);
            return;
        }
        // 相似变换下仍是圆弧：旋转平移起始角；含镜像时方向反转，逆时针起点是原终点的像
        const double theta = std::atan2(m.b(), m.a());
        const double start = m.determinant() >= 0.0 ? startAngle + theta : theta - startAngle - sweepAngle;
        addArcRecord(c, r, Math2d::correctAngle(start), sweepAngle, false);
        return;
    }
    // 非等比缩放或错切：圆的像是椭圆
    addTransformedEllipse(center, DmVector(radius, 0.0), 1.0, startAngle, startAngle + sweepAngle, full);
}

void GsCompiler::addTransformedEllipse(const DmVector& center, const DmVector& majorAxis, double ratio,
                                       double startParam, double endParam, bool closed)
{
    const GiTransform& m = frame().transform;
    if (m.isIdentity(0.0))
    {
        addEllipse(center, majorAxis, ratio, startParam, endParam, closed);
        return;
    }
    // p(t) = c + cos t·u + sin t·v，u、v 是一对共轭半径；换成主轴 a、b：p = c + cos(t - t0)·a + sin(t - t0)·b
    const DmVector u = m.applyVector(majorAxis);
    const DmVector v = m.applyVector(DmVector(-majorAxis.y, majorAxis.x) * ratio);
    double t0 = 0.5 * std::atan2(2.0 * dot(u, v), dot(u, u) - dot(v, v));
    DmVector a = u * std::cos(t0) + v * std::sin(t0);
    DmVector b = u * -std::sin(t0) + v * std::cos(t0);
    if (dot(a, a) < dot(b, b))
    {
        t0 += M_PI_2;
        const DmVector major = b;
        b = a * -1.0;
        a = major;
    }
    const double lenA = std::sqrt(dot(a, a));
    if (lenA <= 0.0)
    {
        return;
    }
    const double newRatio = std::sqrt(dot(b, b)) / lenA;
    const bool sameOrientation = a.x * b.y - a.y * b.x >= 0.0;
    const double s = sameOrientation ? startParam - t0 : t0 - endParam;
    const double e = sameOrientation ? endParam - t0 : t0 - startParam;
    addEllipse(m.apply(center), a, newRatio, s, e, closed);
}

void GsCompiler::addBulgeArc(const DmVector& start, const DmVector& end, double bulge)
{
    const GiTransform& m = frame().transform;
    double scale = 1.0;
    if (m.isSimilarity(&scale))
    {
        // 端点先变换，再由凸度求圆弧；镜像时凸度反号（与原先的复制品相同）
        const double b = m.determinant() >= 0.0 ? bulge : -bulge;
        DmVector center;
        double radius = 0.0, a0 = 0.0, a1 = 0.0;
        bulgeArc(m.apply(start), m.apply(end), b, center, radius, a0, a1);
        double sweep = a1 - a0;
        if (sweep <= 0.0)
        {
            sweep += 2.0 * M_PI;
        }
        addArcRecord(center, radius, a0, sweep, false);
        return;
    }
    DmVector center;
    double radius = 0.0, a0 = 0.0, a1 = 0.0;
    bulgeArc(start, end, bulge, center, radius, a0, a1);
    double sweep = a1 - a0;
    if (sweep < 0.0)
    {
        sweep += 2.0 * M_PI;
    }
    addTransformedEllipse(center, DmVector(radius, 0.0), 1.0, a0, a0 + sweep, false);
}

void GsCompiler::addWideSegment(const DmVector& startPt, const DmVector& endPt, double bulge,
                                const GiSegmentWidth& width)
{
    const GiTransform& m = frame().transform;
    const std::uint32_t prim = fillPrim(kGsPrimFlagFill);
    const double startWeight = width.start;
    const double endWeight = width.end;
    if (std::fabs(bulge) < kBulgeTolerance)
    {
        DmVector dir = (startPt - endPt).normalize();
        DmVector vDir = DmVector(dir).rotate(M_PI_2);
        const DmVector pt1 = startPt + vDir * startWeight * 0.5;
        const DmVector pt2 = startPt - vDir * startWeight * 0.5;
        const DmVector pt3 = endPt + vDir * endWeight * 0.5;
        const DmVector pt4 = endPt - vDir * endWeight * 0.5;
        addQuad(m.apply(pt1), m.apply(pt2), m.apply(pt4), m.apply(pt3), prim);
        return;
    }

    // 圆弧：由一段段四边形拼成，同原先 DmPolyline::getEntitiesByInfo
    DmVector center(true), normal(0.0, 0.0, 1.0);
    double startAngle = 0.0, endAngle = 0.0, radius = 0.0;
    GeometryMethods::getArcInfo(startPt, endPt, bulge, center, radius, startAngle, endAngle, normal);
    double start = startAngle;
    double end = endAngle;
    if (start > end)
    {
        start -= 2 * DM_PI;
    }
    const double weightDelta = endWeight - startWeight;
    const double delta = std::abs(end - start);
    DmVector lastPt1(true), lastPt2(true);
    bool first = true;
    for (int i = 0; i <= DM_CURVE_VERTEXS; i++)
    {
        const double t = static_cast<double>(i) / DM_CURVE_VERTEXS;
        const double angle = normal.z > 0 ? start + t * delta : -start + M_PI - t * delta;
        const double curWeight = startWeight + t * weightDelta;
        const DmVector curPt1(center.x + (radius - curWeight / 2.0) * std::cos(angle),
                              center.y + (radius - curWeight / 2.0) * std::sin(angle));
        const DmVector curPt2(center.x + (radius + curWeight / 2.0) * std::cos(angle),
                              center.y + (radius + curWeight / 2.0) * std::sin(angle));
        if (!first)
        {
            addQuad(m.apply(lastPt1), m.apply(lastPt2), m.apply(curPt2), m.apply(curPt1), prim);
        }
        first = false;
        lastPt1 = curPt1;
        lastPt2 = curPt2;
    }
}

// ---------------------------------------------------------------------------
// 记录
// ---------------------------------------------------------------------------

void GsCompiler::addLine(const DmVector& a, const DmVector& b)
{
    const double length = a.distanceTo(b);
    const std::uint32_t prim = addPrim(GsDashMode::Open, length, 0);
    auto& points = m_out->records[static_cast<std::size_t>(GsClass::Segment)];
    points.push_back(localPoint(a, 0.0f, prim));
    points.push_back(localPoint(b, static_cast<float>(length), prim | kGsPointBreak));
}

void GsCompiler::addArcRecord(const DmVector& center, double radius, double start, double sweep, bool closed)
{
    const double length = radius * sweep;
    const std::uint32_t prim = addPrim(closed ? GsDashMode::Closed : GsDashMode::Open, length, 0);
    auto& arcs = m_out->records[static_cast<std::size_t>(GsClass::Arc)];
    arcs.push_back(GsTexel{static_cast<float>(center.x - m_origin.x), static_cast<float>(center.y - m_origin.y),
                           static_cast<float>(radius), static_cast<float>(start)});
    arcs.push_back(GsTexel{static_cast<float>(sweep), 0.0f, 0.0f, bitsToFloat(prim)});
}

void GsCompiler::addEllipse(const DmVector& center, const DmVector& majorAxis, double ratio, double startParam,
                            double endParam, bool closed)
{
    // 原 DmEllipse::updateVertices 的分段：开放椭圆弧按扫角比例取 120 段（至少 10 段），整椭圆 120 段从参数 0 起
    const double radius = std::sqrt(dot(majorAxis, majorAxis));
    if (radius <= 0.0)
    {
        return;
    }
    const double cosa = majorAxis.x / radius;
    const double sina = majorAxis.y / radius;
    auto at = [&](double t) {
        const double tx = radius * std::cos(t);
        const double ty = ratio * radius * std::sin(t);
        return DmVector(tx * cosa - ty * sina + center.x, tx * sina + ty * cosa + center.y);
    };
    std::vector<DmVector> pts;
    if (!closed)
    {
        const double s = Math2d::correctAngle(startParam);
        const double e = Math2d::correctAngle(endParam);
        double delta = e - s;
        if (delta <= 0.0)
        {
            delta += 2.0 * M_PI;
        }
        int count = static_cast<int>(std::ceil(delta / (2.0 * M_PI) * kEllipseSegments));
        count = std::max(kMinOpenEllipseSegments, count);
        pts.reserve(count + 1);
        for (int i = 0; i <= count; ++i)
        {
            pts.push_back(at(s + delta * i / count));
        }
        addStrip(pts, false);
    }
    else
    {
        pts.reserve(kEllipseSegments);
        for (int i = 0; i < kEllipseSegments; ++i)
        {
            pts.push_back(at(2.0 * M_PI * i / kEllipseSegments));
        }
        addStrip(pts, true);
    }
}

void GsCompiler::addStrip(const std::vector<DmVector>& pts, bool closed)
{
    // 原 DmLineStrip::updateVertices：去掉相邻的重复点，闭合时末点不与首点重复
    if (pts.empty())
    {
        return;
    }
    std::vector<DmVector> unique;
    unique.reserve(pts.size() + 1);
    for (const DmVector& p : pts)
    {
        if (!unique.empty() && static_cast<float>(p.x) == static_cast<float>(unique.back().x)
            && static_cast<float>(p.y) == static_cast<float>(unique.back().y))
        {
            continue;
        }
        unique.push_back(p);
    }
    if (closed && unique.size() > 1 && static_cast<float>(unique.front().x) == static_cast<float>(unique.back().x)
        && static_cast<float>(unique.front().y) == static_cast<float>(unique.back().y))
    {
        unique.pop_back();
    }
    if (unique.size() < 2)
    {
        return;
    }
    if (closed)
    {
        unique.push_back(unique.front());
    }
    double total = 0.0;
    for (std::size_t i = 1; i < unique.size(); ++i)
    {
        total += unique[i - 1].distanceTo(unique[i]);
    }
    const std::uint32_t prim = addPrim(closed ? GsDashMode::Closed : GsDashMode::Open, total, 0);
    auto& points = m_out->records[static_cast<std::size_t>(GsClass::Segment)];
    double s = 0.0;
    for (std::size_t i = 0; i < unique.size(); ++i)
    {
        if (i > 0)
        {
            s += unique[i - 1].distanceTo(unique[i]);
        }
        const bool last = i + 1 == unique.size();
        points.push_back(localPoint(unique[i], static_cast<float>(s), last ? (prim | kGsPointBreak) : prim));
    }
}

void GsCompiler::addTriangle(const DmVector& a, const DmVector& b, const DmVector& c, std::uint32_t prim)
{
    auto& vertices = m_out->records[static_cast<std::size_t>(GsClass::Fill)];
    vertices.push_back(localPoint(a, 0.0f, prim));
    vertices.push_back(localPoint(b, 0.0f, prim));
    vertices.push_back(localPoint(c, 0.0f, prim));
}

void GsCompiler::addQuad(const DmVector& a, const DmVector& b, const DmVector& c, const DmVector& d,
                         std::uint32_t prim)
{
    addTriangle(a, b, c, prim);
    addTriangle(a, c, d, prim);
}
