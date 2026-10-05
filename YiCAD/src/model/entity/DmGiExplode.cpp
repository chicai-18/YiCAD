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

/// @file DmGiExplode.cpp
/// @brief 把 GI 图元做成基本实体的实现

#include "DmGiExplode.h"

#include <algorithm>
#include <cmath>
#include <optional>

#include "DmArc.h"
#include "DmBlock.h"
#include "DmBlockReference.h"
#include "DmCircle.h"
#include "DmDocument.h"
#include "DmEllipse.h"
#include "DmHatch.h"
#include "DmImage.h"
#include "DmLayer.h"
#include "DmLine.h"
#include "DmLineTypeTable.h"
#include "DmPoint.h"
#include "DmPolyline.h"
#include "DmRay.h"
#include "DmRegion.h"
#include "DmSolid.h"
#include "DmSpline.h"
#include "DmXline.h"
#include "GiNurbs.h"
#include "GiTransform.h"
#include "HatchPatternClipper.h"
#include "IGiFont.h"
#include "IGiGeometry.h"
#include "IGiSubEntityTraits.h"
#include "Math2d.h"

namespace
{
constexpr double kBulgeTolerance = 1.0e-12;            ///< 凸度小于它按直线段
constexpr double kFullSweep = 2.0 * M_PI - 1.0e-12;    ///< 扫角不小于它按整圆、整椭圆
/// @brief 嵌套绘制、块的层数上限：插件实体可以画块，块里又有这个插件实体时不至于无限递归
constexpr std::size_t kMaxNesting = 64;

double dot(const DmVector& a, const DmVector& b)
{
    return a.x * b.x + a.y * b.y;
}

/// @brief 带凸度的一段：圆心、半径，与从 startAngle 逆时针转过 sweep 的圆弧
/// @details 凸度为正时圆弧从起点逆时针到终点，为负时顺时针（即从终点逆时针到起点）；
///          圆心在弦的左侧距中点 (r - s) 处（s = |凸度|·弦长/2），凸度为负时左右对调
void bulgeArc(const DmVector& p1, const DmVector& p2, double bulge, DmVector& center, double& radius,
              double& startAngle, double& sweep)
{
    const DmVector chord = p2 - p1;
    const double d = std::hypot(chord.x, chord.y);
    const double s = std::fabs(bulge) * d * 0.5;
    radius = (d * d * 0.25 + s * s) / (2.0 * s);
    const DmVector left = DmVector(-chord.y, chord.x) / d;
    const DmVector mid = (p1 + p2) * 0.5;
    center = bulge > 0.0 ? mid + left * (radius - s) : mid - left * (radius - s);
    sweep = 4.0 * std::atan(std::fabs(bulge));
    const DmVector from = bulge > 0.0 ? p1 : p2;
    startAngle = std::atan2(from.y - center.y, from.x - center.x);
}

bool isByBlockLineType(const DmLineType* lineType)
{
    return lineType == nullptr || DmLineTypeTable::isByBlock(lineType);
}

bool isByLayerLineType(const DmLineType* lineType)
{
    return lineType != nullptr && DmLineTypeTable::isByLayer(lineType);
}

/// @brief 点是否在多边形里（射线法，按弦近似带凸度的边）
bool insidePolygon(const DmVector& p, const std::vector<DmVector>& polygon)
{
    bool inside = false;
    const std::size_t n = polygon.size();
    for (std::size_t i = 0, j = n - 1; i < n; j = i++)
    {
        const DmVector& a = polygon[i];
        const DmVector& b = polygon[j];
        if ((a.y > p.y) != (b.y > p.y) && p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x)
        {
            inside = !inside;
        }
    }
    return inside;
}

/// @brief 图元属性：颜色、线宽、线型（为空即 ByBlock）、图层（为空即取外层）、线型比例
struct Attributes
{
    DmColor color = DmColor(DM::FlagByBlock);
    DM::LineWidth width = DM::WidthByBlock;
    const DmLineType* lineType = nullptr;
    const DmLayer* layer = nullptr;
    double lineTypeScale = 1.0;
};

/// @brief GI 的接收方：每层嵌套一个帧，记本层设的属性、外层的属性与模型变换
class ExplodeDraw final : public IGiWorldDraw, public IGiGeometry, public IGiSubEntityTraits
{
public:
    ExplodeDraw(DmDocument* document, DmGiExplode::Purpose purpose, std::vector<DmGiExplode::Item>& out)
        : m_document(document)
        , m_purpose(purpose)
        , m_out(out)
    {
    }

    void run(const IGiDrawable& drawable)
    {
        pushFrame(GiTransform(), nullptr, false);
        drawInFrame(drawable);
        m_frames.pop_back();
    }

    // IGiWorldDraw
    IGiGeometry& geometry() override { return *this; }
    IGiSubEntityTraits& traits() override { return *this; }
    GiRegenType regenType() const override { return GiRegenType::Display; }
    double deviation() const override { return 0.0; }
    bool isDragging() const override { return false; }

    // IGiSubEntityTraits
    void setColor(const DmColor& color) override { frame().set.color = color; }
    void setLayer(const DmLayer* layer) override { frame().set.layer = layer; }
    void setLineType(const DmLineType* lineType) override { frame().set.lineType = lineType; }
    void setLineTypeScale(double scale) override { frame().set.lineTypeScale = scale; }
    /// 内联图案做不成实体的线型；几何照样做成实体，按当前线型
    void setLinePattern(const GiLinePattern&) override {}
    void setFill(const GiHatchPattern* pattern) override
    {
        frame().fill = pattern ? std::optional<GiHatchPattern>(*pattern) : std::nullopt;
    }
    void setLineWeight(DM::LineWidth weight) override { frame().set.width = weight; }
    void setTransparency(std::uint8_t) override {}
    void setSelectionMarker(std::int32_t) override {}
    void setScreenSpace(const DmVector* anchor) override { frame().screenSpace = anchor != nullptr; }

    // IGiGeometry
    void polyline(std::span<const DmVector> points, std::span<const double> bulges,
                  std::span<const GiSegmentWidth> widths, GiPolylineFlags flags) override;
    void circle(const DmVector& center, double radius) override;
    void arc(const DmVector& center, double radius, double startAngle, double sweepAngle) override;
    void ellipseArc(const DmVector& center, const DmVector& majorAxis, double ratio, double startParam,
                    double endParam) override;
    void nurbs(const GiNurbs& curve) override;
    void fill(std::span<const GiLoop> loops, GiFillRule rule) override;
    void triangles(std::span<const DmVector> vertices, std::span<const std::uint32_t> indices) override;
    void glyphRun(const GiGlyphRun& run) override;
    void image(const GiImage& image) override;
    void point(const DmVector& position) override;
    void ray(const DmVector& base, const DmVector& direction) override;
    void xline(const DmVector& base, const DmVector& direction) override;
    void draw(const IGiDrawable& drawable) override;
    void drawShared(const IGiDrawable& drawable, const GiTransform& transform,
                    const GiByBlockTraits& byBlock) override;
    void pushTransform(const GiTransform& transform) override;
    void popTransform() override;

private:
    struct Frame
    {
        Attributes set;                         ///< 本层设的属性
        bool hasParent = false;                 ///< 有没有外层
        Attributes parent;                      ///< 外层解析后的属性
        GiTransform transform;                  ///< 当前模型变换
        std::vector<GiTransform> saved;         ///< pushTransform 之前的变换
        std::optional<GiHatchPattern> fill;     ///< 之后的 fill 按这个图案填，为空是实心
        bool screenSpace = false;               ///< 之后的图元以像素为单位，不做成实体
        bool glyph = false;                     ///< 在字形里
    };

    Frame& frame() { return m_frames.back(); }
    const Frame& frame() const { return m_frames.back(); }

    void pushFrame(const GiTransform& transform, const Attributes* parent, bool glyph);
    void drawInFrame(const IGiDrawable& drawable);
    Attributes resolve(const Frame& f) const;

    /// @brief 按当前属性设置实体，交出
    void addEntity(DmEntity* entity) { addEntityWith(entity, resolve(frame())); }
    void addEntityWith(DmEntity* entity, const Attributes& attributes);

    void addLine(const DmVector& a, const DmVector& b) { addEntity(new DmLine(a, b)); }
    void addTransformedArc(const DmVector& center, double radius, double startAngle, double sweep, bool full);
    void addTransformedEllipse(const DmVector& center, const DmVector& majorAxis, double ratio, double startParam,
                               double endParam, bool closed);
    /// @brief 变换一个环：相似变换下保留凸度（镜像时反号），否则把圆弧段离散成点
    GiLoop transformLoop(const GiLoop& loop) const;
    DmPolyline* makeClosedPolyline(const GiLoop& loop) const;
    /// @brief 把一个环做成填充边界的边：直线段为直线，圆弧段为圆弧（与 DXF 导入建的边界相同，导出也认）
    static void addLoopEdges(DmEntityContainer& boundary, const GiLoop& loop);
    void addSolidFill(const std::vector<GiLoop>& loops);
    bool addBlockReference(const IGiDrawable& drawable, const GiTransform& transform, const Attributes& attributes);

    DmDocument* m_document;
    DmGiExplode::Purpose m_purpose;
    std::vector<DmGiExplode::Item>& m_out;
    std::vector<Frame> m_frames;
};

void ExplodeDraw::pushFrame(const GiTransform& transform, const Attributes* parent, bool glyph)
{
    Frame f;
    f.hasParent = parent != nullptr;
    if (parent)
    {
        f.parent = *parent;
    }
    f.transform = transform;
    f.glyph = glyph;
    m_frames.push_back(std::move(f));
}

void ExplodeDraw::drawInFrame(const IGiDrawable& drawable)
{
    drawable.setAttributes(*this);
    drawable.worldDraw(*this);
}

Attributes ExplodeDraw::resolve(const Frame& f) const
{
    Attributes r;
    r.layer = f.set.layer ? f.set.layer : (f.hasParent ? f.parent.layer : nullptr);
    r.lineTypeScale = f.hasParent ? f.parent.lineTypeScale * f.set.lineTypeScale : f.set.lineTypeScale;
    r.color = f.set.color;
    r.width = f.set.width;
    r.lineType = f.set.lineType;
    if (!f.hasParent)
    {
        // 可绘制对象自己的属性原样给出
        return r;
    }
    // ByBlock 取外层；外层是 ByLayer 而所在图层不同时，按外层的图层解析成具体值
    const DmLayer* outerLayer = f.parent.layer;
    const bool otherLayer = outerLayer && r.layer != outerLayer;
    if (r.color.isByBlock())
    {
        r.color = f.parent.color;
        if (otherLayer && r.color.isByLayer())
        {
            r.color = outerLayer->getPen().getColor();
        }
    }
    if (r.width == DM::WidthByBlock)
    {
        r.width = f.parent.width;
        if (otherLayer && r.width == DM::WidthByLayer)
        {
            r.width = outerLayer->getPen().getWidth();
        }
    }
    if (isByBlockLineType(r.lineType))
    {
        r.lineType = f.parent.lineType;
        if (otherLayer && isByLayerLineType(r.lineType))
        {
            r.lineType = outerLayer->getPen().getLineType();
        }
    }
    return r;
}

void ExplodeDraw::addEntityWith(DmEntity* entity, const Attributes& a)
{
    if (m_document)
    {
        // 先归文档（它会设成当前图层、当前画笔），再设成解析出的属性
        entity->setDocument(m_document);
    }
    entity->setLayer(const_cast<DmLayer*>(a.layer));
    DmLineType* lineType = const_cast<DmLineType*>(a.lineType);
    if (!lineType && m_document)
    {
        lineType = m_document->getLineTypeTable()->getLineTypeByBlock();
    }
    if (lineType)
    {
        entity->setPen(DmPen(a.color, a.width, lineType));
    }
    else if (a.color.isByBlock() && a.width == DM::WidthByBlock)
    {
        // 没有文档时取不到随块线型记录：三项都随块就用无效画笔
        entity->setPen(DmPen(DM::FlagInvalid));
    }
    else
    {
        entity->setPen(DmPen(a.color, a.width, DmLineTypeTable::Continuous));
    }
    entity->setLineTypeScale(a.lineTypeScale);
    entity->update();
    entity->calculateBorders();
    m_out.push_back(DmGiExplode::Item{std::unique_ptr<DmEntity>(entity), frame().glyph});
}

void ExplodeDraw::polyline(std::span<const DmVector> points, std::span<const double> bulges,
                           std::span<const GiSegmentWidth> widths, GiPolylineFlags flags)
{
    const std::size_t n = points.size();
    if (frame().screenSpace || n < 2)
    {
        return;
    }
    const bool closed = hasFlag(flags, GiPolylineFlags::Closed);
    const std::size_t segments = closed ? n : n - 1;
    auto bulgeAt = [&bulges](std::size_t i) { return i < bulges.size() ? bulges[i] : 0.0; };
    const GiTransform& m = frame().transform;
    double scale = 1.0;
    const bool similar = m.isSimilarity(&scale);
    bool hasBulge = false;
    for (std::size_t i = 0; i < segments; ++i)
    {
        hasBulge = hasBulge || std::fabs(bulgeAt(i)) >= kBulgeTolerance;
    }
    const bool hasWidth = !widths.empty();

    if (!hasBulge && !hasWidth && !closed && n == 2)
    {
        addLine(m.apply(points[0]), m.apply(points[1]));
        return;
    }
    if (!hasBulge || similar)
    {
        // 仍是一条多段线：顶点变换，凸度在镜像时反号，宽度乘缩放比例（非等比时按面积比例近似）
        const double widthScale = similar ? scale : std::sqrt(std::fabs(m.determinant()));
        const double bulgeSign = m.determinant() >= 0.0 ? 1.0 : -1.0;
        std::vector<DmVector> vertices;
        vertices.reserve(n);
        for (const DmVector& p : points)
        {
            vertices.push_back(m.apply(p));
        }
        std::vector<double> segmentBulges(segments, 0.0);
        std::vector<double> lineWeights(2 * segments, 0.0);
        for (std::size_t i = 0; i < segments; ++i)
        {
            segmentBulges[i] = bulgeAt(i) * bulgeSign;
            if (i < widths.size())
            {
                lineWeights[2 * i] = widths[i].start * widthScale;
                lineWeights[2 * i + 1] = widths[i].end * widthScale;
            }
        }
        addEntity(new DmPolyline(nullptr, PolylineData(vertices, segmentBulges, lineWeights, closed)));
        return;
    }
    // 非等比缩放或错切下圆弧段变成椭圆弧，拆成直线与椭圆弧（宽度做不成实体）
    for (std::size_t i = 0; i < segments; ++i)
    {
        const DmVector& s = points[i];
        const DmVector& e = points[(i + 1) % n];
        const double b = bulgeAt(i);
        if (std::fabs(b) < kBulgeTolerance)
        {
            addLine(m.apply(s), m.apply(e));
            continue;
        }
        DmVector center;
        double radius = 0.0, start = 0.0, sweep = 0.0;
        bulgeArc(s, e, b, center, radius, start, sweep);
        addTransformedEllipse(center, DmVector(radius, 0.0), 1.0, start, start + sweep, false);
    }
}

void ExplodeDraw::circle(const DmVector& center, double radius)
{
    if (!frame().screenSpace)
    {
        addTransformedArc(center, radius, 0.0, 2.0 * M_PI, true);
    }
}

void ExplodeDraw::arc(const DmVector& center, double radius, double startAngle, double sweepAngle)
{
    if (frame().screenSpace)
    {
        return;
    }
    if (sweepAngle < 0.0)
    {
        // 顺时针扫过：换成等价的逆时针
        startAngle += sweepAngle;
        sweepAngle = -sweepAngle;
    }
    addTransformedArc(center, radius, startAngle, sweepAngle, sweepAngle >= kFullSweep);
}

void ExplodeDraw::ellipseArc(const DmVector& center, const DmVector& majorAxis, double ratio, double startParam,
                             double endParam)
{
    if (!frame().screenSpace)
    {
        addTransformedEllipse(center, majorAxis, ratio, startParam, endParam, endParam - startParam >= kFullSweep);
    }
}

void ExplodeDraw::nurbs(const GiNurbs& curve)
{
    if (frame().screenSpace || !curve.isValid())
    {
        return;
    }
    // B 样条对仿射变换不变：变换控制点即可（YiCAD 的样条都是非有理的，不带权重）
    const GiTransform& m = frame().transform;
    if (m_purpose == DmGiExplode::Purpose::Graphics)
    {
        // 代理图形（AutoCAD 的代理图形没有样条）：按控制点包围框的 2×10⁻⁴ 离散，与图形系统的默认容差相同
        DmVector lo(DM_MAXDOUBLE, DM_MAXDOUBLE);
        DmVector hi(-DM_MAXDOUBLE, -DM_MAXDOUBLE);
        for (const DmVector& p : curve.controlPoints)
        {
            lo = DmVector::minimum(lo, p);
            hi = DmVector::maximum(hi, p);
        }
        const double size = std::max(hi.x - lo.x, hi.y - lo.y);
        std::vector<DmVector> points;
        curve.sample(points, size > 0.0 ? size * 2.0e-4 : 1.0e-6);
        for (DmVector& p : points)
        {
            p = m.apply(p);
        }
        if (points.size() >= 2)
        {
            const std::size_t segments = points.size() - 1;
            std::vector<double> lineWeights(2 * segments, 0.0);
            addEntity(new DmPolyline(nullptr, PolylineData(points, std::vector<double>(segments, 0.0), lineWeights,
                                                           false)));
        }
        return;
    }
    std::vector<DmVector> controlPoints;
    controlPoints.reserve(curve.controlPoints.size());
    for (const DmVector& p : curve.controlPoints)
    {
        controlPoints.push_back(m.apply(p));
    }
    SplineData data(curve.degree, curve.closed, ESplineType::eControlPoints);
    data.setControlPoints(controlPoints);
    data.setKnots(curve.knots);
    addEntity(new DmSpline(nullptr, data));
}

void ExplodeDraw::fill(std::span<const GiLoop> loops, GiFillRule)
{
    if (frame().screenSpace || loops.empty())
    {
        return;
    }
    if (m_purpose == DmGiExplode::Purpose::Query)
    {
        // 拾取、捕捉只要边界
        for (const GiLoop& loop : loops)
        {
            if (DmPolyline* pl = makeClosedPolyline(transformLoop(loop)))
            {
                addEntity(pl);
            }
        }
        return;
    }
    if (frame().fill)
    {
        // 图案：按与图形系统同一份算法在边界里切出图案线，切成划线与点（同 AutoCAD 炸开图案填充得到线）。
        // 在当前坐标里切，再把结果变换出去：仿射变换保持直线
        const GiTransform& m = frame().transform;
        std::vector<HatchPatternRun> runs;
        std::vector<std::pair<DmVector, DmVector>> segments;
        std::vector<DmVector> dots;
        for (const GiHatchPatternLine& line : frame().fill->lines)
        {
            runs.clear();
            HatchPatternClipper::clip(loops, line, runs);
            for (const HatchPatternRun& run : runs)
            {
                segments.clear();
                dots.clear();
                HatchPatternClipper::splitRun(run, line.dashes, segments, dots);
                for (const auto& [a, b] : segments)
                {
                    addLine(m.apply(a), m.apply(b));
                }
                for (const DmVector& p : dots)
                {
                    addEntity(new DmPoint(nullptr, PointData(m.apply(p))));
                }
            }
        }
        return;
    }
    std::vector<GiLoop> transformed;
    transformed.reserve(loops.size());
    for (const GiLoop& loop : loops)
    {
        transformed.push_back(transformLoop(loop));
    }
    addSolidFill(transformed);
}

void ExplodeDraw::triangles(std::span<const DmVector> vertices, std::span<const std::uint32_t> indices)
{
    if (frame().screenSpace)
    {
        return;
    }
    const GiTransform& m = frame().transform;
    for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        if (indices[i] >= vertices.size() || indices[i + 1] >= vertices.size() || indices[i + 2] >= vertices.size())
        {
            continue;
        }
        const std::vector<DmVector> corners = {m.apply(vertices[indices[i]]), m.apply(vertices[indices[i + 1]]),
                                               m.apply(vertices[indices[i + 2]])};
        addEntity(new DmSolid(nullptr, SolidData(corners)));
    }
}

void ExplodeDraw::glyphRun(const GiGlyphRun& run)
{
    if (frame().screenSpace || !run.font)
    {
        return;
    }
    // 每个字形如同一次 drawShared：字形里的 ByBlock 取字形串当时的属性
    const Attributes parent = resolve(frame());
    for (const GiGlyph& g : run.glyphs)
    {
        const IGiDrawable* glyph = run.font->glyph(g.code);
        if (!glyph)
        {
            continue;
        }
        pushFrame(frame().transform * g.transform, &parent, true);
        drawInFrame(*glyph);
        m_frames.pop_back();
    }
}

void ExplodeDraw::image(const GiImage& image)
{
    // 只有来自文件的图片做得成实体：内嵌的像素不归 DmImage 管
    if (frame().screenSpace || image.path.isEmpty() || image.width <= 0 || image.height <= 0)
    {
        return;
    }
    const GiTransform& m = frame().transform;
    const DmVector u = m.applyVector(image.u) / image.width;
    const DmVector v = m.applyVector(image.v) / image.height;
    addEntity(new DmImage(nullptr, ImageData(0, m.apply(image.origin), u, v, DmVector(image.width, image.height),
                                        image.path.toStdString(), 50, 50, 0)));
}

void ExplodeDraw::point(const DmVector& position)
{
    if (!frame().screenSpace)
    {
        addEntity(new DmPoint(nullptr, PointData(frame().transform.apply(position))));
    }
}

void ExplodeDraw::ray(const DmVector& base, const DmVector& direction)
{
    if (!frame().screenSpace)
    {
        const GiTransform& m = frame().transform;
        addEntity(new DmRay(nullptr, RayData(m.apply(base), m.applyVector(direction).normalize())));
    }
}

void ExplodeDraw::xline(const DmVector& base, const DmVector& direction)
{
    if (!frame().screenSpace)
    {
        const GiTransform& m = frame().transform;
        addEntity(new DmXline(nullptr, XLineData(m.apply(base), m.applyVector(direction).normalize())));
    }
}

void ExplodeDraw::draw(const IGiDrawable& drawable)
{
    if (m_frames.size() >= kMaxNesting)
    {
        return;
    }
    const Attributes parent = resolve(frame());
    pushFrame(frame().transform, &parent, frame().glyph);
    drawInFrame(drawable);
    m_frames.pop_back();
}

void ExplodeDraw::drawShared(const IGiDrawable& drawable, const GiTransform& transform,
                             const GiByBlockTraits& byBlock)
{
    if (m_frames.size() >= kMaxNesting)
    {
        return;
    }
    // GiByBlockTraits 按调用方的上下文解析：ByLayer 取调用方的图层，ByBlock 取调用方的外层
    Frame caller;
    caller.set.color = byBlock.color;
    caller.set.width = byBlock.lineWeight;
    caller.set.lineType = byBlock.lineType;
    caller.set.layer = frame().set.layer;
    caller.set.lineTypeScale = frame().set.lineTypeScale;
    caller.hasParent = frame().hasParent;
    caller.parent = frame().parent;
    const Attributes parent = resolve(caller);
    const GiTransform placement = frame().transform * transform;
    if (m_purpose == DmGiExplode::Purpose::Explode && !frame().glyph && addBlockReference(drawable, placement, parent))
    {
        return;
    }
    pushFrame(placement, &parent, frame().glyph);
    drawInFrame(drawable);
    m_frames.pop_back();
}

void ExplodeDraw::pushTransform(const GiTransform& transform)
{
    Frame& f = frame();
    f.saved.push_back(f.transform);
    f.transform = f.transform * transform;
}

void ExplodeDraw::popTransform()
{
    Frame& f = frame();
    if (!f.saved.empty())
    {
        f.transform = f.saved.back();
        f.saved.pop_back();
    }
}

void ExplodeDraw::addTransformedArc(const DmVector& center, double radius, double startAngle, double sweep, bool full)
{
    const GiTransform& m = frame().transform;
    double scale = 1.0;
    if (m.isSimilarity(&scale))
    {
        const DmVector c = m.apply(center);
        const double r = radius * scale;
        if (full)
        {
            addEntity(new DmCircle(nullptr, CircleData(c, r)));
            return;
        }
        // 相似变换下仍是圆弧：起始角随旋转；含镜像时方向反转，逆时针的起点是原终点的像
        const double theta = std::atan2(m.b(), m.a());
        const double start = Math2d::correctAngle(m.determinant() >= 0.0 ? startAngle + theta
                                                                         : theta - startAngle - sweep);
        addEntity(new DmArc(nullptr, ArcData(c, DmVector(0.0, 0.0, 1.0), r, start, Math2d::correctAngle(start + sweep))));
        return;
    }
    // 非等比缩放或错切：圆的像是椭圆
    addTransformedEllipse(center, DmVector(radius, 0.0), 1.0, startAngle, startAngle + sweep, full);
}

void ExplodeDraw::addTransformedEllipse(const DmVector& center, const DmVector& majorAxis, double ratio,
                                        double startParam, double endParam, bool closed)
{
    const GiTransform& m = frame().transform;
    // p(t) = c + cos t·u + sin t·v，u、v 是一对共轭半径；换成主轴 a、b：p = c + cos(t - t0)·a + sin(t - t0)·b
    const DmVector u = m.applyVector(majorAxis);
    const DmVector v = m.applyVector(DmVector(-majorAxis.y, majorAxis.x) * ratio);
    double t0 = 0.5 * std::atan2(2.0 * dot(u, v), dot(u, u) - dot(v, v));
    DmVector a = u * std::cos(t0) + v * std::sin(t0);
    DmVector b = u * -std::sin(t0) + v * std::cos(t0);
    if (dot(a, a) < dot(b, b))
    {
        // 换成另一根轴作长轴：t0 加 90°
        t0 += M_PI_2;
        const DmVector major = b;
        b = a * -1.0;
        a = major;
    }
    const double lenA = std::sqrt(dot(a, a));
    if (!(lenA > 0.0))
    {
        return;
    }
    const double newRatio = std::sqrt(dot(b, b)) / lenA;
    const DmVector c = m.apply(center);
    if (closed)
    {
        addEntity(new DmEllipse(nullptr, EllipseData(c, a, DmVector(0.0, 0.0, 1.0), newRatio, true, 0.0, 2.0 * M_PI)));
        return;
    }
    // b 在 a 的逆时针一侧时参数同向；否则（含镜像）参数反向，逆时针的起点是原终点
    const bool sameOrientation = a.x * b.y - a.y * b.x >= 0.0;
    const double s = sameOrientation ? startParam - t0 : t0 - endParam;
    const double e = sameOrientation ? endParam - t0 : t0 - startParam;
    addEntity(new DmEllipse(nullptr, EllipseData(c, a, DmVector(0.0, 0.0, 1.0), newRatio, false, Math2d::correctAngle(s),
                                            Math2d::correctAngle(e))));
}

GiLoop ExplodeDraw::transformLoop(const GiLoop& loop) const
{
    const GiTransform& m = frame().transform;
    GiLoop out;
    const std::size_t n = loop.points.size();
    double scale = 1.0;
    const bool similar = m.isSimilarity(&scale);
    auto bulgeAt = [&loop](std::size_t i) { return i < loop.bulges.size() ? loop.bulges[i] : 0.0; };
    bool hasBulge = false;
    for (std::size_t i = 0; i < n; ++i)
    {
        hasBulge = hasBulge || std::fabs(bulgeAt(i)) >= kBulgeTolerance;
    }
    if (!hasBulge || similar)
    {
        const double sign = m.determinant() >= 0.0 ? 1.0 : -1.0;
        for (std::size_t i = 0; i < n; ++i)
        {
            out.points.push_back(m.apply(loop.points[i]));
            if (hasBulge)
            {
                out.bulges.push_back(bulgeAt(i) * sign);
            }
        }
        return out;
    }
    // 非等比时圆弧段的像是椭圆弧，离散成点（每段至少 8 份，每份不超过 10°）
    for (std::size_t i = 0; i < n; ++i)
    {
        const DmVector& s = loop.points[i];
        const DmVector& e = loop.points[(i + 1) % n];
        out.points.push_back(m.apply(s));
        const double b = bulgeAt(i);
        if (std::fabs(b) < kBulgeTolerance)
        {
            continue;
        }
        DmVector center;
        double radius = 0.0, start = 0.0, sweep = 0.0;
        bulgeArc(s, e, b, center, radius, start, sweep);
        const int count = std::max(8, static_cast<int>(std::ceil(sweep / (M_PI / 18.0))));
        // 凸度为正时从起点逆时针走到终点；为负时 start 是终点的角度，从起点顺时针走
        const double from = b > 0.0 ? start : start + sweep;
        const double step = (b > 0.0 ? sweep : -sweep) / count;
        for (int k = 1; k < count; ++k)
        {
            const double t = from + step * k;
            out.points.push_back(m.apply(center + DmVector(std::cos(t), std::sin(t)) * radius));
        }
    }
    return out;
}

DmPolyline* ExplodeDraw::makeClosedPolyline(const GiLoop& loop) const
{
    const std::size_t n = loop.points.size();
    if (n < 2)
    {
        return nullptr;
    }
    std::vector<double> bulges(n, 0.0);
    for (std::size_t i = 0; i < n && i < loop.bulges.size(); ++i)
    {
        bulges[i] = loop.bulges[i];
    }
    std::vector<double> lineWeights(2 * n, 0.0);
    return new DmPolyline(nullptr, PolylineData(loop.points, bulges, lineWeights, true));
}

void ExplodeDraw::addLoopEdges(DmEntityContainer& boundary, const GiLoop& loop)
{
    const std::size_t n = loop.points.size();
    for (std::size_t i = 0; i < n; ++i)
    {
        const DmVector& s = loop.points[i];
        const DmVector& e = loop.points[(i + 1) % n];
        const double b = i < loop.bulges.size() ? loop.bulges[i] : 0.0;
        if (std::fabs(b) < kBulgeTolerance)
        {
            if (s.distanceTo(e) > 0.0)
            {
                boundary.addEntity(new DmLine(s, e));
            }
            continue;
        }
        DmVector center;
        double radius = 0.0, start = 0.0, sweep = 0.0;
        bulgeArc(s, e, b, center, radius, start, sweep);
        boundary.addEntity(new DmArc(nullptr, ArcData(center, DmVector(0.0, 0.0, 1.0), radius,
                                                      Math2d::correctAngle(start), Math2d::correctAngle(start + sweep))));
    }
}

void ExplodeDraw::addSolidFill(const std::vector<GiLoop>& loops)
{
    // 奇偶规则：被偶数个环包住的环是外环，奇数个是孔洞，孔洞归包住它、深一层的外环。每个外环一个实心填充
    const std::size_t count = loops.size();
    std::vector<int> depth(count, 0);
    for (std::size_t i = 0; i < count; ++i)
    {
        if (loops[i].points.empty())
        {
            continue;
        }
        for (std::size_t j = 0; j < count; ++j)
        {
            if (i != j && loops[j].points.size() >= 3 && insidePolygon(loops[i].points.front(), loops[j].points))
            {
                ++depth[i];
            }
        }
    }
    for (std::size_t outer = 0; outer < count; ++outer)
    {
        if (depth[outer] % 2 != 0 || loops[outer].points.size() < 3)
        {
            continue;
        }
        auto boundary = std::make_shared<DmEntityContainer>(nullptr, true);
        addLoopEdges(*boundary, loops[outer]);
        std::vector<DmEntityContainerPtr> holes;
        for (std::size_t h = 0; h < count; ++h)
        {
            if (depth[h] == depth[outer] + 1 && !loops[h].points.empty() &&
                insidePolygon(loops[h].points.front(), loops[outer].points))
            {
                auto hole = std::make_shared<DmEntityContainer>(nullptr, true);
                addLoopEdges(*hole, loops[h]);
                holes.push_back(hole);
            }
        }
        HatchData hatchData(true, 1.0, 0.0, std::wstring(L"SOLID"));
        hatchData.setBoundary(std::make_shared<DmRegion>(nullptr, RegionData(boundary, holes)));
        addEntity(new DmHatch(nullptr, hatchData));
    }
}

bool ExplodeDraw::addBlockReference(const IGiDrawable& drawable, const GiTransform& placement,
                                    const Attributes& attributes)
{
    // 块参照的变换是 插入点·旋转·缩放·(−基点)：线性部分的两列正交（没有错切）时才表示得出来
    const auto* block = dynamic_cast<const DmBlock*>(&drawable);
    if (!block || !m_document)
    {
        return false;
    }
    const DmVector c0(placement.a(), placement.b());
    const DmVector c1(placement.c(), placement.d());
    const double l0 = std::hypot(c0.x, c0.y);
    const double l1 = std::hypot(c1.x, c1.y);
    if (!(l0 > 0.0) || !(l1 > 0.0) || std::fabs(dot(c0, c1)) > 1.0e-9 * l0 * l1)
    {
        return false;
    }
    const double angle = std::atan2(c0.y, c0.x);
    // 第二列应为 l1·(−sin, cos)；反向即 Y 比例为负（镜像）
    const double sy = (-c1.x * std::sin(angle) + c1.y * std::cos(angle)) >= 0.0 ? l1 : -l1;
    const DmVector insertion = placement.apply(block->getBasePoint());
    DmBlockReferenceData data(block->getName(), insertion, DmVector(l0, sy), Math2d::correctAngle(angle), 1, 1,
                              DmVector(0.0, 0.0), m_document->getBlockTable());
    addEntityWith(new DmBlockReference(nullptr, data), attributes);
    return true;
}
}  // namespace

std::vector<DmGiExplode::Item> DmGiExplode::run(const IGiDrawable& drawable, DmDocument* document, Purpose purpose)
{
    std::vector<Item> out;
    ExplodeDraw(document, purpose, out).run(drawable);
    return out;
}
