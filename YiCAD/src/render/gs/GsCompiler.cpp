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
#include "HatchPatternClipper.h"
#include "IGiFont.h"
#include "Math2d.h"

namespace
{

constexpr double kBulgeTolerance = 1.0e-5;          ///< 与 DmPolyline 相同：凸度小于它的段是直线
constexpr double kDeviation = 1.0e-3;               ///< 实体自行离散时的弦高容差
constexpr double kFullSweep = 2.0 * M_PI - 1.0e-9;  ///< 扫角达到它即为整圆、整椭圆
constexpr int kMinOpenEllipseSegments = 10;
constexpr int kMaxCurveSegments = 1000000;          ///< 一条曲线最多离散成这么多段（极端放大时的保护）

double dot(const DmVector& a, const DmVector& b)
{
    return a.x * b.x + a.y * b.y;
}

float bitsToFloat(std::uint32_t bits)
{
    return std::bit_cast<float>(bits);
}

/// @brief x 对 period 取模，结果在 [0, period)
double positiveMod(double x, double period)
{
    const double r = std::fmod(x, period);
    return r < 0.0 ? r + period : r;
}

/// @brief 变换下长度的比例：相似变换为它的比例，否则取面积比例的平方根（填充图案线在非等比变换下的曲线用）
double lengthScaleOf(const GiTransform& m)
{
    double scale = 1.0;
    if (m.isSimilarity(&scale))
    {
        return scale;
    }
    return std::sqrt(std::fabs(m.determinant()));
}

}  // namespace

void gsPatternMetrics(const std::vector<double>& dashes, double& period, double& firstDashCenter)
{
    period = 0.0;
    firstDashCenter = 0.0;
    bool haveDash = false;
    for (std::size_t i = 0; i < dashes.size() && i < 12; ++i)
    {
        const double v = dashes[i];
        if (v > 1.0e-8 && !haveDash)
        {
            firstDashCenter = period + v * 0.5;
            haveDash = true;
        }
        period += std::abs(v);
    }
}

namespace
{

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
    runs.clear();
    hasPieces = false;
    curveTolerance = 0.0;
    defaultCurveTolerance = 0.0;
    hasNurbs = false;
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

void GsCompiler::compileNode(const IGiDrawable& drawable, std::uint32_t slot, const DmVector& origin, GsCompiled& out,
                             double tolerance)
{
    begin(slot, origin, out);
    m_tolerance = tolerance;
    pushFrame(GiTransform(), nullptr, false, 1.0);
    drawInFrame(drawable);
    m_frames.clear();
    m_out = nullptr;
}

void GsCompiler::compileShared(const IGiDrawable& drawable, const DmVector& origin, GsCompiled& out)
{
    begin(kGsNoSlot, origin, out);
    m_tolerance = 0.0;
    pushFrame(GiTransform(), nullptr, true, 1.0);
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

void GsCompiler::setLineTypeScale(double scale)
{
    frame().lineTypeScale = scale;
}

void GsCompiler::setLinePattern(const GiLinePattern& pattern)
{
    frame().hasPattern = !pattern.dashes.empty();
    frame().pattern = pattern;
}

void GsCompiler::setFill(const GiHatchPattern* pattern)
{
    frame().hasFill = pattern != nullptr;
    frame().fill = pattern ? *pattern : GiHatchPattern();
}

// 透明度、子实体标记与屏幕空间图元还不起作用
void GsCompiler::setTransparency(std::uint8_t)
{
}

void GsCompiler::setSelectionMarker(std::int32_t)
{
}

void GsCompiler::setScreenSpace(const DmVector*)
{
}

void GsCompiler::pushFrame(const GiTransform& transform, const GsAttributes* parent, bool sharedTop,
                           double parentLineTypeScale)
{
    Frame f;
    f.hasParent = parent != nullptr;
    if (parent)
    {
        f.parent = *parent;
    }
    f.sharedTop = sharedTop;
    f.transform = transform;
    f.parentLineTypeScale = parentLineTypeScale;
    m_frames.push_back(std::move(f));
}

double GsCompiler::lineTypeScale() const
{
    const Frame& f = m_frames.back();
    return f.lineTypeScale * f.parentLineTypeScale;
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

    // 线型：空视为 ByBlock（GI 的约定）；随层、随块是线型所属文档线型表里的保留记录
    const DmLineType* lineType = f.lineType;
    if (!lineType || DmLineTypeTable::isByBlock(lineType))
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
    else if (DmLineTypeTable::isByLayer(lineType))
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
            | (static_cast<std::uint32_t>(a.lineWeight.kind) << 4) | (static_cast<std::uint32_t>(mode) << kGsKindsDashShift)
            | (static_cast<std::uint32_t>(flags) << kGsKindsFlagsShift);
    p.runLength = static_cast<float>(runLength);
    p.lineTypeScale = static_cast<float>(lineTypeScale());
    m_out->prims.push_back(p);
    return static_cast<std::uint32_t>(m_out->prims.size() - 1);
}

std::uint32_t GsCompiler::addRunPrim(const RunPlan& plan, std::size_t segment, double segmentLength)
{
    const std::uint32_t index = addPrim(plan.mode, segmentLength, 0);
    GsPrimRecord& p = m_out->prims[index];
    if (plan.pattern)
    {
        // 填充图案线：线型取内联图案（值），不按图层、块解析
        const GsAttributes& a = attributes();
        p.lineTypeAndWeight = gsPackLineTypeAndWeight(plan.patternIndex, a.lineWeight.code);
        p.layers1 = static_cast<std::uint32_t>(kGsLayerNone) | (static_cast<std::uint32_t>(a.lineWeight.layer) << 16);
        p.kinds &= ~(3u << 2);
    }
    if (segment < plan.dash.size())
    {
        p.dash = plan.dash[segment];
    }
    if (!plan.cuts.empty())
    {
        p.kinds |= kGsKindsPiece;
        if (segment == 0)
        {
            p.kinds |= kGsKindsRunStart;
        }
        if (segment == plan.cuts.size())
        {
            p.kinds |= kGsKindsRunEnd;
        }
    }
    return index;
}

GsCompiler::RunPlan GsCompiler::planRun(GsDashMode mode, double length, double patternScale)
{
    RunPlan plan;
    plan.mode = mode;
    const Frame& f = frame();
    if (f.hasPattern)
    {
        if (patternScale < 0.0)
        {
            patternScale = lengthScaleOf(f.transform);
        }
        // 填充图案线（第 4.5.1 节）：相位锚定在图案原点，不做端点对齐；图案在实体自身的坐标系里，随块缩放
        plan.mode = GsDashMode::Pattern;
        plan.pattern = true;
        plan.patternIndex = m_context.patternIndex(f.pattern.dashes);
        double period = 0.0;
        double center = 0.0;
        gsPatternMetrics(f.pattern.dashes, period, center);
        period *= patternScale;
        const double phase = period > 0.0 ? positiveMod(f.pattern.phase * patternScale, period) : 0.0;
        if (period > 0.0 && m_context.splitLongRuns() && length / period > kGsPieceLimitPeriods)
        {
            const double step = kGsPiecePeriods * period;
            const int pieces = static_cast<int>(std::floor(length / step));
            for (int k = 0; k < pieces; ++k)
            {
                const double start = k * step;
                if (k > 0)
                {
                    plan.cuts.push_back(start);
                }
                plan.dash.push_back({static_cast<float>(positiveMod(phase + start, period)),
                                     static_cast<float>(patternScale), 0.0f, 0.0f});
            }
        }
        else
        {
            plan.dash.push_back({static_cast<float>(phase), static_cast<float>(patternScale), 0.0f, 0.0f});
        }
        return plan;
    }

    // 线型：只有顶层几何分段（共享几何的插入各有自己的线型比例），周期按当时的 LTSCALE 与图层表算
    if (m_slot == kGsNoSlot || !m_context.splitLongRuns())
    {
        return plan;
    }
    const GsLineTypeRef& lineType = attributes().lineType;
    const bool tracked = (lineType.kind == GsKind::Value && lineType.index != 0) || lineType.kind == GsKind::ByLayer;
    const double scale = lineTypeScale();
    if (!tracked || scale <= 0.0)
    {
        return plan;
    }
    // 长度摘要：LTSCALE、线型、图层的线型改了，模型据此判断这个分块要不要重新分段
    auto summary = std::find_if(m_out->runs.begin(), m_out->runs.end(),
                                [&lineType](const GsRunSummary& r) { return r.lineType == lineType; });
    if (summary == m_out->runs.end())
    {
        m_out->runs.push_back({lineType, length / scale});
    }
    else
    {
        summary->length = std::max(summary->length, length / scale);
    }
    double patternPeriod = 0.0;
    double center = 0.0;
    if (!m_context.lineTypeMetrics(lineType, patternPeriod, center))
    {
        return plan;
    }
    const double chain = scale * m_context.globalLineTypeScale();
    const double period = patternPeriod * chain;
    if (!(period > 0.0) || length / period <= kGsPieceLimitPeriods)
    {
        return plan;
    }

    m_out->hasPieces = true;
    const double step = kGsPiecePeriods * period;
    const int pieces = static_cast<int>(std::floor(length / step));
    if (mode == GsDashMode::Closed)
    {
        // 整周期：周期数取 round（至少 2 个，同着色器的 strokeOf），图案拉伸到正好 n 个周期，各段起点的相位按拉伸后的周期算
        const double n = std::max(std::round(length / period), 2.0);
        const double stretched = length / n;
        for (int k = 0; k < pieces; ++k)
        {
            const double start = k * step;
            if (k > 0)
            {
                plan.cuts.push_back(start);
            }
            plan.dash.push_back({static_cast<float>(positiveMod(start, stretched)),
                                 static_cast<float>(stretched / period), 0.0f, 0.0f});
        }
        return plan;
    }
    // 居中：头部划线到 head，尾部划线从 length - head 起，中间从第一段划线的中点对准 head 起周期重复
    center *= chain;
    const double head = (length / period - std::floor(length / period)) * 0.5 * period;
    for (int k = 0; k < pieces; ++k)
    {
        const double start = k * step;
        if (k > 0)
        {
            plan.cuts.push_back(start);
        }
        plan.dash.push_back({static_cast<float>(positiveMod(start - head + center, period)),
                             static_cast<float>(head - start), static_cast<float>(length - head - start), 0.0f});
    }
    return plan;
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
    pushFrame(frame().transform, &parent, false, lineTypeScale());
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
            pushFrame(composed, &parent, false, lineTypeScale());
            drawInFrame(drawable);
            m_frames.pop_back();
            return;
        }
        m_out->hasNonUniformUse = m_out->hasNonUniformUse || nonUniform;
    }
    m_out->shared.push_back({&drawable, composed, parent, lineTypeScale()});
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
    std::vector<const IGiDrawable*> glyphs(run.glyphs.size(), nullptr);
    for (std::size_t i = 0; i < run.glyphs.size(); ++i)
    {
        const GiGlyph& g = run.glyphs[i];
        glyphs[i] = m_context.glyph(*run.font, g.code);
        if (!glyphs[i])
        {
            continue;
        }
        m_out->shared.push_back({glyphs[i], frame().transform * g.transform, parent, lineTypeScale(), true});
    }
    addTextBar(run, glyphs);
}

void GsCompiler::addTextBar(const GiGlyphRun& run, const std::vector<const IGiDrawable*>& glyphs)
{
    if (run.glyphs.empty())
    {
        return;
    }
    // 以第一个字形的坐标系为准（x 沿基线、y 向上，字高为 1）：各字形的横向范围换到这个坐标系里取并集，
    // 细条是 y = 0.5 处从最左到最右的一条线，线宽取字高（RENDER_PLAN.md 第 4.3.10 节：小字画沿基线的细长矩形）
    const GiTransform first = frame().transform * run.glyphs.front().transform;
    if (first.determinant() == 0.0)
    {
        return;
    }
    const GiTransform inverse = first.inverse();
    double low = 0.0;
    double high = 0.0;
    bool any = false;
    for (std::size_t i = 0; i < run.glyphs.size(); ++i)
    {
        const GiGlyph& g = run.glyphs[i];
        const IGiDrawable* glyph = glyphs[i];
        double minX = 0.0;
        double maxX = 0.0;
        if (!glyph || !m_context.glyphExtent(*glyph, minX, maxX))
        {
            continue;
        }
        const GiTransform toFirst = inverse * (frame().transform * g.transform);
        for (const DmVector& corner : {DmVector(minX, 0.0), DmVector(maxX, 0.0), DmVector(minX, 1.0), DmVector(maxX, 1.0)})
        {
            const double x = toFirst.apply(corner).x;
            low = any ? std::min(low, x) : x;
            high = any ? std::max(high, x) : x;
            any = true;
        }
    }
    if (!any || !(high > low))
    {
        return;
    }
    // 字高：字形坐标里竖直的一个单位在本单元里垂直于基线的长度（|det| / 基线方向的伸缩，倾斜不影响）
    const DmVector baseline = first.applyVector(DmVector(1.0, 0.0));
    const double baselineLength = std::hypot(baseline.x, baseline.y);
    if (!(baselineLength > 0.0))
    {
        return;
    }
    const double height = std::fabs(first.determinant()) / baselineLength;
    const DmVector a = first.apply(DmVector(low, 0.5));
    const DmVector b = first.apply(DmVector(high, 0.5));
    const double length = a.distanceTo(b);
    const std::uint32_t prim = addPrim(GsDashMode::None, length, kGsPrimFlagTextBar);
    m_out->prims[prim].dash = {0.0f, 1.0f, static_cast<float>(height), 0.0f};
    auto& points = m_out->records[static_cast<std::size_t>(GsClass::Segment)];
    points.push_back(localPoint(a, 0.0f, prim));
    points.push_back(localPoint(b, static_cast<float>(length), prim | kGsPointBreak));
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
            // 填充图案线的图案按这一段在变换下的长度比例伸缩
            const double local = s.distanceTo(e);
            const DmVector worldDir = m.applyVector(e - s);
            const double k = local > 0.0 ? std::hypot(worldDir.x, worldDir.y) / local : 1.0;
            addLine(m.apply(s), m.apply(e), k);
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
    // B 样条对仿射变换不变：变换控制点后离散。默认的弦高容差按控制点包围框（曲线在它里面）的尺寸
    GiNurbs transformed = curve;
    const GiTransform& m = frame().transform;
    double minX = 0.0, minY = 0.0, maxX = 0.0, maxY = 0.0;
    bool first = true;
    for (DmVector& p : transformed.controlPoints)
    {
        p = m.apply(p);
        minX = first ? p.x : std::min(minX, p.x);
        minY = first ? p.y : std::min(minY, p.y);
        maxX = first ? p.x : std::max(maxX, p.x);
        maxY = first ? p.y : std::max(maxY, p.y);
        first = false;
    }
    const double size = std::max(maxX - minX, maxY - minY);
    const double defaultTolerance = std::max(size * kGsCurveRelativeTolerance, 1.0e-12);
    const double tolerance = gsCurveTolerance(defaultTolerance, m_tolerance);
    m_out->curveTolerance = std::max(m_out->curveTolerance, tolerance);
    m_out->defaultCurveTolerance = std::max(m_out->defaultCurveTolerance, defaultTolerance);
    m_out->hasNurbs = true;
    const std::shared_ptr<const std::vector<DmVector>> points = m_context.sampleNurbs(transformed, tolerance);
    if (points)
    {
        addStrip(*points, transformed.closed);
    }
}

void GsCompiler::fill(std::span<const GiLoop> loops, GiFillRule)
{
    if (frame().hasFill && !frame().fill.lines.empty())
    {
        addHatchPattern(loops);
        return;
    }
    std::vector<std::array<DmVector, 3>> tris;
    triangulate(loops, tris);
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

void GsCompiler::addHatchPattern(std::span<const GiLoop> loops)
{
    // 图案线在本层坐标里切（与 DmHatch 切划线实体的同一份算法），端点再变换到本单元：直线在仿射下仍是直线，
    // 图案沿线的伸缩是方向在变换下的长度，线距是 |det| / 方向的伸缩
    const GiTransform m = frame().transform;
    const GiHatchPattern pattern = frame().fill;
    std::vector<std::array<DmVector, 3>> tris;
    triangulate(loops, tris);
    std::vector<HatchPatternRun> runs;
    for (std::size_t family = 0; family < pattern.lines.size(); ++family)
    {
        const GiHatchPatternLine& line = pattern.lines[family];
        runs.clear();
        if (!HatchPatternClipper::clip(loops, line, runs))
        {
            continue;
        }
        const double len = std::hypot(line.direction.x, line.direction.y);
        const DmVector dir = m.applyVector(line.direction / len);
        const double stretch = std::hypot(dir.x, dir.y);
        if (!(stretch > 0.0))
        {
            continue;
        }
        const double spacing = HatchPatternClipper::spacing(line) * std::fabs(m.determinant()) / stretch;
        for (const HatchPatternRun& run : runs)
        {
            addHatchLine(m.apply(run.start), m.apply(run.end), line.dashes, run.phase, stretch, spacing);
        }

        // 过密时的替身：同一份三角形，按这一族的覆盖率画。划线占周期的比例，加上每条划线、每个点两端的圆头（各半个线宽，
        // 着色器里按线宽换算）：画图案线时划线两端按线宽延伸成圆头，点画成直径等于线宽的圆
        if (tris.empty())
        {
            continue;
        }
        const double period = HatchPatternClipper::period(line.dashes);
        double ink = 1.0;
        double marks = 0.0;
        if (period > 0.0)
        {
            ink = 0.0;
            for (double d : line.dashes)
            {
                if (d >= 0.0)
                {
                    ink += d;
                    marks += 1.0;
                }
            }
            ink /= period;
            marks /= period * stretch;
        }
        const std::uint32_t prim = addPrim(GsDashMode::None, 0.0, kGsPrimFlagFill | kGsPrimFlagHatchCover);
        m_out->prims[prim].dash = {static_cast<float>(spacing), static_cast<float>(ink), static_cast<float>(marks),
                                   static_cast<float>(period * stretch)};
        for (const auto& tri : tris)
        {
            addTriangle(tri[0], tri[1], tri[2], prim);
        }
    }
}

void GsCompiler::addHatchLine(const DmVector& a, const DmVector& b, const std::vector<double>& dashes, double phase,
                              double patternScale, double spacing)
{
    const double length = a.distanceTo(b);
    if (!(length > 0.0))
    {
        return;
    }
    auto& points = m_out->records[static_cast<std::size_t>(GsClass::Segment)];
    if (dashes.empty() || !(HatchPatternClipper::period(dashes) > 0.0))
    {
        // 实线族：整段一条连续线，不按填充的线型
        const std::uint32_t prim = addPrim(GsDashMode::None, length, kGsPrimFlagHatchLine);
        m_out->prims[prim].dash = {0.0f, 1.0f, static_cast<float>(spacing), 0.0f};
        points.push_back(localPoint(a, 0.0f, prim));
        points.push_back(localPoint(b, static_cast<float>(length), prim | kGsPointBreak));
        return;
    }
    // 按图案与相位画：借本层的内联图案走 planRun（图案线的画法与分段，第 4.5.1、4.5.5 节）
    Frame& f = frame();
    const bool hadPattern = f.hasPattern;
    const GiLinePattern saved = f.pattern;
    f.hasPattern = true;
    f.pattern = GiLinePattern{dashes, phase};
    const RunPlan plan = planRun(GsDashMode::Open, length, patternScale);
    f.hasPattern = hadPattern;
    f.pattern = saved;
    double start = 0.0;
    for (std::size_t k = 0; k <= plan.cuts.size(); ++k)
    {
        const bool last = k == plan.cuts.size();
        const double end = last ? length : plan.cuts[k];
        const DmVector pa = k == 0 ? a : a + (b - a) * (start / length);
        const DmVector pb = last ? b : a + (b - a) * (end / length);
        const std::uint32_t prim = addRunPrim(plan, k, end - start);
        GsPrimRecord& p = m_out->prims[prim];
        p.kinds |= static_cast<std::uint32_t>(kGsPrimFlagHatchLine) << kGsKindsFlagsShift;
        p.dash[2] = static_cast<float>(spacing);
        points.push_back(localPoint(pa, 0.0f, prim));
        points.push_back(localPoint(pb, static_cast<float>(end - start), prim | kGsPointBreak));
        start = end;
    }
}

void GsCompiler::triangulate(std::span<const GiLoop> loops, std::vector<std::array<DmVector, 3>>& tris)
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
    ConstrainedDelaunayTriangulation::triangulatePoints(outer, holes, tris);
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

void GsCompiler::addLine(const DmVector& a, const DmVector& b, double patternScale)
{
    const double length = a.distanceTo(b);
    const RunPlan plan = planRun(GsDashMode::Open, length, patternScale);
    auto& points = m_out->records[static_cast<std::size_t>(GsClass::Segment)];
    // 每段两个点，弧长参数从 0 起（不分段时就是整条线）
    double start = 0.0;
    for (std::size_t k = 0; k <= plan.cuts.size(); ++k)
    {
        const bool last = k == plan.cuts.size();
        const double end = last ? length : plan.cuts[k];
        const DmVector pa = k == 0 ? a : a + (b - a) * (start / length);
        const DmVector pb = last ? b : a + (b - a) * (end / length);
        const std::uint32_t prim = addRunPrim(plan, k, end - start);
        points.push_back(localPoint(pa, 0.0f, prim));
        points.push_back(localPoint(pb, static_cast<float>(end - start), prim | kGsPointBreak));
        start = end;
    }
}

void GsCompiler::addArcRecord(const DmVector& center, double radius, double start, double sweep, bool closed,
                              double patternScale)
{
    const double length = radius * sweep;
    const RunPlan plan = planRun(closed ? GsDashMode::Closed : GsDashMode::Open, length, patternScale);
    auto& arcs = m_out->records[static_cast<std::size_t>(GsClass::Arc)];
    // 分段时每段一条圆弧记录，弧长参数各自从 0 起
    double s0 = 0.0;
    for (std::size_t k = 0; k <= plan.cuts.size(); ++k)
    {
        const double s1 = k == plan.cuts.size() ? length : plan.cuts[k];
        const double pieceStart = plan.cuts.empty() ? start : start + s0 / radius;
        const double pieceSweep = plan.cuts.empty() ? sweep : (s1 - s0) / radius;
        const std::uint32_t prim = addRunPrim(plan, k, s1 - s0);
        arcs.push_back(GsTexel{static_cast<float>(center.x - m_origin.x), static_cast<float>(center.y - m_origin.y),
                               static_cast<float>(radius), static_cast<float>(pieceStart)});
        arcs.push_back(GsTexel{static_cast<float>(pieceSweep), 0.0f, 0.0f, bitsToFloat(prim)});
        s0 = s1;
    }
}

void GsCompiler::addEllipse(const DmVector& center, const DmVector& majorAxis, double ratio, double startParam,
                            double endParam, bool closed)
{
    // 默认同原 DmEllipse::updateVertices 的分段：开放椭圆弧按扫角比例取 120 段（至少 10 段），整椭圆 120 段从参数 0 起。
    // 参数等分时最大弦高约为 长半轴 × Δt² / 8（在长轴两端），所以 120 段对应的弦高与长半轴成正比；
    // 节点的容差更细时按它加密（第 4.3.10 节：放大后重新离散）
    const double radius = std::sqrt(dot(majorAxis, majorAxis));
    if (radius <= 0.0)
    {
        return;
    }
    const double defaultStep = 2.0 * M_PI / kGsEllipseSegments;
    const double defaultTolerance = radius * defaultStep * defaultStep / 8.0;
    const double tolerance = gsCurveTolerance(defaultTolerance, m_tolerance);
    m_out->curveTolerance = std::max(m_out->curveTolerance, tolerance);
    m_out->defaultCurveTolerance = std::max(m_out->defaultCurveTolerance, defaultTolerance);
    const double step = std::sqrt(8.0 * tolerance / radius);
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
        // 与原先相同的写法（扫角比例 × 120），节点的容差更细时按步长加密
        int count = step < defaultStep ? static_cast<int>(std::min<double>(std::ceil(delta / step), kMaxCurveSegments))
                                       : static_cast<int>(std::ceil(delta / (2.0 * M_PI) * kGsEllipseSegments));
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
        const int count = step < defaultStep
                              ? static_cast<int>(std::min<double>(std::ceil(2.0 * M_PI / step), kMaxCurveSegments))
                              : kGsEllipseSegments;
        pts.reserve(count);
        for (int i = 0; i < count; ++i)
        {
            pts.push_back(at(2.0 * M_PI * i / count));
        }
        addStrip(pts, true);
    }
}

void GsCompiler::addStrip(const std::vector<DmVector>& pts, bool closed, double patternScale)
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
    const RunPlan plan = planRun(closed ? GsDashMode::Closed : GsDashMode::Open, total, patternScale);
    auto& points = m_out->records[static_cast<std::size_t>(GsClass::Segment)];
    // 分段时在分界处断开：插入插值点，前一段到此结束，后一段从它开始、弧长参数从 0 起
    std::size_t piece = 0;
    double pieceStart = 0.0;
    auto pieceEnd = [&plan, total](std::size_t k) { return k < plan.cuts.size() ? plan.cuts[k] : total; };
    std::uint32_t prim = addRunPrim(plan, 0, pieceEnd(0));
    double s = 0.0;
    points.push_back(localPoint(unique[0], 0.0f, prim));
    for (std::size_t i = 1; i < unique.size(); ++i)
    {
        const double d = unique[i - 1].distanceTo(unique[i]);
        while (piece < plan.cuts.size() && plan.cuts[piece] < s + d)
        {
            const DmVector cut = unique[i - 1] + (unique[i] - unique[i - 1]) * ((plan.cuts[piece] - s) / d);
            points.push_back(localPoint(cut, static_cast<float>(plan.cuts[piece] - pieceStart), prim | kGsPointBreak));
            pieceStart = plan.cuts[piece];
            ++piece;
            prim = addRunPrim(plan, piece, pieceEnd(piece) - pieceStart);
            points.push_back(localPoint(cut, 0.0f, prim));
        }
        s += d;
        const bool last = i + 1 == unique.size();
        points.push_back(localPoint(unique[i], static_cast<float>(s - pieceStart), last ? (prim | kGsPointBreak) : prim));
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
