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

/// @file GLCacheWorldDraw.cpp
/// @brief GLCacheWorldDraw 实现。各类图元的顶点生成原在各实体的 updateVertices 里，算法（含 float 运算）原样搬来

#include "GLCacheWorldDraw.h"

#include <algorithm>
#include <cmath>

#include <QByteArrayView>
#include <QDateTime>
#include <QFileInfo>
#include <QHash>
#include <QImage>

#include "ConstrainedDelaunayTriangulation.h"
#include "DmEntity.h"
#include "DmLayer.h"
#include "DmLineType.h"
#include "DmPenList.h"
#include "GLCachePainter.h"
#include "GeometryMethods.h"
#include "GiNurbs.h"
#include "IGiFont.h"
#include "Math2d.h"

namespace
{

constexpr int kFloatsPerVertex = 5;         ///< 线类顶点：(x, y, z, 弧长参数, 总长)
constexpr double kBulgeTolerance = 1.0e-5;  ///< 与 DmPolyline 相同：凸度小于它的段是直线
constexpr double kDeviation = 1.0e-3;       ///< 实体自行离散时的弦高容差；旧渲染器的缓存与视图无关，取固定值
constexpr double kFullSweep = 2.0 * M_PI - 1.0e-9;  ///< 扫角达到它即为整圆、整椭圆

double dot(const DmVector& a, const DmVector& b)
{
    return a.x * b.x + a.y * b.y;
}

/// @brief 图片来源，纹理按它缓存（GLImageTextureCache）：有文件时是文件的绝对路径、修改时间与大小，
///        文件在磁盘上改了就是新的来源；没有文件时是内嵌像素的尺寸与内容哈希
QString imageSource(const GiImage& image)
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

/// @brief 多段线凸度段的圆弧：圆心、半径与逆时针的起止角，同原先 DmPolyline 生成的 DmArc 的"翻正"角度
void bulgeArc(const DmVector& start, const DmVector& end, double bulge,
              DmVector& center, double& radius, double& startAngle, double& endAngle)
{
    DmVector normal(0.0, 0.0, 1.0);
    double a0 = 0.0;
    double a1 = 0.0;
    GeometryMethods::getArcInfo(start, end, bulge, center, radius, a0, a1, normal);
    if (normal.z < 0.0)
    {
        // DmArc::getStartAngleNormal()、getEndAngleNormal()：顺时针圆弧的法向朝 -Z
        startAngle = Math2d::correctAngle(M_PI - a1);
        endAngle = Math2d::correctAngle(M_PI - a0);
    }
    else
    {
        startAngle = a0;
        endAngle = a1;
    }
}

/// @brief 圆弧上从 startAngle 逆时针到 endAngle 的点（不含起点与终点），每 6° 一段，同 DmArc::getPoints
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

}  // namespace

GLCacheWorldDraw::GLCacheWorldDraw(opengl::GLCachePainter& painter, opengl::CacheGroupType group,
                                   NurbsSamples& nurbsSamples)
    : m_painter(painter)
    , m_group(group)
    , m_nurbsSamples(nurbsSamples)
{
}

void GLCacheWorldDraw::drawEntity(const DmEntity& entity)
{
    pushFrame(GiTransform(), nullptr);
    drawInFrame(entity);
    m_frames.pop_back();
}

double GLCacheWorldDraw::deviation() const
{
    return kDeviation;
}

// ---------------------------------------------------------------------------
// 属性
// ---------------------------------------------------------------------------

void GLCacheWorldDraw::setColor(const DmColor& color)
{
    frame().color = color;
    invalidatePen();
}

void GLCacheWorldDraw::setLayer(const DmLayer* layer)
{
    frame().layer = layer;
    invalidatePen();
}

void GLCacheWorldDraw::setLineType(const DmLineType* lineType)
{
    frame().lineType = lineType;
    invalidatePen();
}

void GLCacheWorldDraw::setLineWeight(DM::LineWidth weight)
{
    frame().width = weight;
    invalidatePen();
}

// 旧渲染器没有实体线型比例、内联图案、透明度、子实体标记与屏幕空间图元，这几项不起作用
void GLCacheWorldDraw::setLineTypeScale(double)
{
}

void GLCacheWorldDraw::setLinePattern(const GiLinePattern&)
{
}

void GLCacheWorldDraw::setTransparency(std::uint8_t)
{
}

void GLCacheWorldDraw::setSelectionMarker(std::int32_t)
{
}

void GLCacheWorldDraw::setScreenSpace(const DmVector*)
{
}

void GLCacheWorldDraw::pushFrame(const GiTransform& transform, const Resolved* parent)
{
    Frame f;
    f.color = DmColor(DM::FlagByBlock);
    f.width = DM::WidthByBlock;
    f.lineType = nullptr;
    f.layer = nullptr;
    f.hasParent = parent != nullptr;
    if (parent)
    {
        f.parent = *parent;
    }
    f.transform = transform;
    m_frames.push_back(std::move(f));
}

void GLCacheWorldDraw::drawInFrame(const IGiDrawable& drawable)
{
    drawable.setAttributes(*this);
    drawable.worldDraw(*this);
}

GLCacheWorldDraw::Resolved GLCacheWorldDraw::resolve(const Frame& f)
{
    // 与 DmEntity::getPen(true) 相同：ByBlock 取外层解析后的属性，再按图层解析 ByLayer；
    // 图层为空取外层的图层（getLayer(true)）。顶层没有外层，ByBlock 保持原样
    Resolved r;
    r.layer = f.layer ? f.layer : (f.hasParent ? f.parent.layer : nullptr);

    DmColor color = f.color;
    if (color.isByBlock() && f.hasParent)
    {
        color = f.parent.color;
    }
    if (color.isByLayer() && r.layer)
    {
        color = r.layer->getPen().getColor();
    }

    DM::LineWidth width = f.width;
    if (width == DM::WidthByBlock && f.hasParent)
    {
        width = f.parent.width;
    }
    if (width == DM::WidthByLayer && r.layer)
    {
        width = r.layer->getPen().getWidth();
    }

    const DmLineType* lineType = f.lineType ? f.lineType : DmLineTypeTable::ByBlock;
    if (lineType == DmLineTypeTable::ByBlock && f.hasParent)
    {
        lineType = f.parent.lineType;
    }
    if (lineType == DmLineTypeTable::ByLayer && r.layer)
    {
        lineType = r.layer->getPen().getLineType();
    }

    r.color = color;
    r.width = width;
    r.lineType = lineType;
    return r;
}

void GLCacheWorldDraw::invalidatePen()
{
    frame().penId = -1;
}

int GLCacheWorldDraw::penId()
{
    Frame& f = frame();
    if (f.penId >= 0)
    {
        return f.penId;
    }
    const Resolved r = resolve(f);
    // DmPenList 与 DmPen 用非 const 的线型指针，只作画笔的键，不改线型
    DmPen* pen = DMPENLIST->request(r.color, r.width, const_cast<DmLineType*>(r.lineType));
    const int id = DMPENLIST->getPenId(*pen);
    if (m_configuredPens.insert(id).second)
    {
        // 原 DmCachePainter::cacheEntity：线宽按 width × 0.05 像素、至少 1 像素；非连续线型设虚线；
        // RGB 全 0 画成白色（P12，第 5 阶段改为 ACI 7 的语义）
        constexpr double kMinLineWidth = 1.0;
        m_painter.lineWidth(id, std::max(pen->getWidth() * 0.05, kMinLineWidth));
        DmLineType* lineType = pen->getLineType();
        if (lineType)
        {
            const QString name = lineType->getLineTypeName();
            if (name != "continuous" && name != "ByLayer" && name != "ByBlock")
            {
                m_painter.setDash(id, lineType->getLineTypeData().data(), static_cast<int>(lineType->getNum()));
            }
        }
        const DmColor& c = pen->getColor();
        if (c.red() + c.green() + c.blue() == 0)
        {
            m_painter.setColor(id, 255, 255, 255, 255);
        }
        else
        {
            m_painter.setColor(id, c.red(), c.green(), c.blue(), c.alpha());
        }
    }
    f.penId = id;
    return id;
}

// ---------------------------------------------------------------------------
// 嵌套、共享与变换
// ---------------------------------------------------------------------------

void GLCacheWorldDraw::draw(const IGiDrawable& drawable)
{
    const Resolved parent = resolve(frame());
    pushFrame(frame().transform, &parent);
    drawInFrame(drawable);
    m_frames.pop_back();
}

void GLCacheWorldDraw::drawShared(const IGiDrawable& drawable, const GiTransform& transform,
                                  const GiByBlockTraits& byBlock)
{
    // GiByBlockTraits 按调用方的上下文解析：ByLayer 取调用方的图层，ByBlock 取调用方的外层
    Frame caller;
    caller.color = byBlock.color;
    caller.width = byBlock.lineWeight;
    caller.lineType = byBlock.lineType;
    caller.layer = frame().layer;
    caller.hasParent = frame().hasParent;
    caller.parent = frame().parent;
    const Resolved parent = resolve(caller);
    pushFrame(frame().transform * transform, &parent);
    drawInFrame(drawable);
    m_frames.pop_back();
}

void GLCacheWorldDraw::pushTransform(const GiTransform& transform)
{
    Frame& f = frame();
    f.savedTransforms.push_back(f.transform);
    f.transform = f.transform * transform;
}

void GLCacheWorldDraw::popTransform()
{
    Frame& f = frame();
    if (!f.savedTransforms.empty())
    {
        f.transform = f.savedTransforms.back();
        f.savedTransforms.pop_back();
    }
}

void GLCacheWorldDraw::glyphRun(const GiGlyphRun& run)
{
    if (!run.font)
    {
        return;
    }
    // 每个字形如同一次 drawShared：字形里的 ByBlock 取字形串当时的属性
    const Resolved parent = resolve(frame());
    for (const GiGlyph& g : run.glyphs)
    {
        const IGiDrawable* glyph = run.font->glyph(g.code);
        if (!glyph)
        {
            continue;
        }
        pushFrame(frame().transform * g.transform, &parent);
        drawInFrame(*glyph);
        m_frames.pop_back();
    }
}

// ---------------------------------------------------------------------------
// 图元
// ---------------------------------------------------------------------------

void GLCacheWorldDraw::polyline(std::span<const DmVector> points, std::span<const double> bulges,
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
                // 负凸度的圆弧按"翻正"角度是从终点到起点
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
        addLineStrip(strip, closed);
        return;
    }

    // 每段单独：与原先 DmPolyline 生成的直线、圆弧、带宽度的 DmSolid 相同
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

void GLCacheWorldDraw::circle(const DmVector& center, double radius)
{
    addTransformedArc(center, radius, 0.0, 2.0 * M_PI, true);
}

void GLCacheWorldDraw::arc(const DmVector& center, double radius, double startAngle, double sweepAngle)
{
    if (sweepAngle < 0.0)
    {
        // 顺时针扫过：换成等价的逆时针
        startAngle += sweepAngle;
        sweepAngle = -sweepAngle;
    }
    addTransformedArc(center, radius, startAngle, sweepAngle, false);
}

void GLCacheWorldDraw::ellipseArc(const DmVector& center, const DmVector& majorAxis, double ratio,
                                  double startParam, double endParam)
{
    const bool closed = endParam - startParam >= kFullSweep;
    addTransformedEllipse(center, majorAxis, ratio, startParam, endParam, closed);
}

void GLCacheWorldDraw::nurbs(const GiNurbs& curve)
{
    // B 样条对仿射变换不变：变换控制点后离散，与原先先变换样条、再离散相同
    GiNurbs transformed = curve;
    const GiTransform& m = frame().transform;
    for (DmVector& p : transformed.controlPoints)
    {
        p = m.apply(p);
    }
    addLineStrip(m_nurbsSamples.sample(transformed), transformed.closed);
}

void GLCacheWorldDraw::fill(std::span<const GiLoop> loops, GiFillRule)
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
    std::vector<float> vertices;
    vertices.reserve(tris.size() * 9);
    for (const auto& tri : tris)
    {
        for (const DmVector& p : tri)
        {
            vertices.emplace_back(p.x);
            vertices.emplace_back(p.y);
            vertices.emplace_back(0.0f);
        }
    }
    m_painter.addTriangle(penId(), m_group, vertices);
}

void GLCacheWorldDraw::triangles(std::span<const DmVector> vertices, std::span<const std::uint32_t> indices)
{
    const GiTransform& m = frame().transform;
    std::vector<float> data;
    data.reserve(indices.size() * 3);
    for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        if (indices[i] >= vertices.size() || indices[i + 1] >= vertices.size() || indices[i + 2] >= vertices.size())
        {
            continue;
        }
        for (std::size_t k = 0; k < 3; ++k)
        {
            const DmVector p = m.apply(vertices[indices[i + k]]);
            data.emplace_back(p.x);
            data.emplace_back(p.y);
            data.emplace_back(0.0f);
        }
    }
    if (!data.empty())
    {
        m_painter.addTriangle(penId(), m_group, data);
    }
}

void GLCacheWorldDraw::image(const GiImage& image)
{
    const GiTransform& m = frame().transform;
    // 角点次序与原先 DmImage::getCorners() 相同：左下、右下、右上、左上
    const DmVector c0 = m.apply(image.origin);
    const DmVector c1 = m.apply(image.origin + image.u);
    const DmVector c2 = m.apply(image.origin + image.v + image.u);
    const DmVector c3 = m.apply(image.origin + image.v);

    std::vector<float> vertices;
    vertices.reserve(5 * 4);
    vertices.insert(vertices.end(), { (float)c0.x, (float)c0.y, 0.0f, 0.0f, 0.0f });
    vertices.insert(vertices.end(), { (float)c1.x, (float)c1.y, 0.0f, 1.0f, 0.0f });
    vertices.insert(vertices.end(), { (float)c2.x, (float)c2.y, 0.0f, 1.0f, 1.0f });
    vertices.insert(vertices.end(), { (float)c3.x, (float)c3.y, 0.0f, 0.0f, 1.0f });

    // 纹理按图片来源缓存，缓存重建时复用，只有新的来源才解码、上传（RENDER_PLAN.md 1.2 步）
    const QString path = image.path;
    const QImage* pixels = image.pixels;
    m_painter.addImage(penId(), m_group, vertices, imageSource(image), [path, pixels]() {
        if (!path.isEmpty())
        {
            return QImage(path);
        }
        return pixels ? QImage(*pixels) : QImage();
    });
}

void GLCacheWorldDraw::point(const DmVector& position)
{
    const DmVector p = frame().transform.apply(position);
    m_painter.addPoint(penId(), m_group, p.x, p.y);
}

void GLCacheWorldDraw::ray(const DmVector& base, const DmVector& direction)
{
    const GiTransform& m = frame().transform;
    const DmVector b = m.apply(base);
    const DmVector d = m.applyVector(direction);
    m_painter.addRay(penId(), m_group, b.x, b.y, d.x, d.y);
}

void GLCacheWorldDraw::xline(const DmVector& base, const DmVector& direction)
{
    const GiTransform& m = frame().transform;
    const DmVector b = m.apply(base);
    const DmVector d = m.applyVector(direction);
    m_painter.addXLine(penId(), m_group, b.x, b.y, d.x, d.y);
}

// ---------------------------------------------------------------------------
// 变换后的圆弧、椭圆、多段线段
// ---------------------------------------------------------------------------

void GLCacheWorldDraw::addTransformedArc(const DmVector& center, double radius, double startAngle,
                                         double sweepAngle, bool full)
{
    const GiTransform& m = frame().transform;
    double scale = 1.0;
    if (m.isSimilarity(&scale))
    {
        const DmVector c = m.apply(center);
        const double r = radius * scale;
        if (full)
        {
            addCircle(c, r);
            return;
        }
        // 相似变换下仍是圆弧：旋转平移起始角；含镜像时方向反转，逆时针起点是原终点的像
        const double theta = std::atan2(m.b(), m.a());
        const double start = m.determinant() >= 0.0 ? startAngle + theta : theta - startAngle - sweepAngle;
        const double s = Math2d::correctAngle(start);
        addArc(c, r, s, Math2d::correctAngle(s + sweepAngle));
        return;
    }
    // 非等比缩放或错切：圆的像是椭圆
    addTransformedEllipse(center, DmVector(radius, 0.0), 1.0, startAngle, startAngle + sweepAngle, full);
}

void GLCacheWorldDraw::addTransformedEllipse(const DmVector& center, const DmVector& majorAxis, double ratio,
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
        // 换成另一根轴作长轴：t0 加 90°
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
    // b 在 a 的逆时针一侧时参数同向；否则（含镜像）参数反向，逆时针的起点是原终点
    const bool sameOrientation = a.x * b.y - a.y * b.x >= 0.0;
    const double s = sameOrientation ? startParam - t0 : t0 - endParam;
    const double e = sameOrientation ? endParam - t0 : t0 - startParam;
    addEllipse(m.apply(center), a, newRatio, s, e, closed);
}

void GLCacheWorldDraw::addBulgeArc(const DmVector& start, const DmVector& end, double bulge)
{
    const GiTransform& m = frame().transform;
    double scale = 1.0;
    if (m.isSimilarity(&scale))
    {
        // 与原先相同：端点先变换，再由凸度求圆弧；镜像时凸度反号
        const double b = m.determinant() >= 0.0 ? bulge : -bulge;
        DmVector center;
        double radius = 0.0, a0 = 0.0, a1 = 0.0;
        bulgeArc(m.apply(start), m.apply(end), b, center, radius, a0, a1);
        addArc(center, radius, a0, a1);
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

void GLCacheWorldDraw::addWideSegment(const DmVector& startPt, const DmVector& endPt, double bulge,
                                      const GiSegmentWidth& width)
{
    const GiTransform& m = frame().transform;
    const double startWeight = width.start;
    const double endWeight = width.end;
    if (std::fabs(bulge) < kBulgeTolerance)
    {
        DmVector dir = (startPt - endPt).normalize();
        DmVector vDir = DmVector(dir).rotate(M_PI_2);
        auto pt1 = startPt + vDir * startWeight * 0.5;
        auto pt2 = startPt - vDir * startWeight * 0.5;
        auto pt3 = endPt + vDir * endWeight * 0.5;
        auto pt4 = endPt - vDir * endWeight * 0.5;
        // 按多边形顺序给出四个角：原先按 pt1、pt2、pt3、pt4（Z 字顺序）当扇形画，每段缺四分之一
        addSolid({ m.apply(pt1), m.apply(pt2), m.apply(pt4), m.apply(pt3) });
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

    double weightDelta = endWeight - startWeight;
    float delta = std::abs(end - start);
    float angle = 0.0f;

    DmVector lastPt1(true);
    DmVector lastPt2(true);
    DmVector curPt1(true);
    DmVector curPt2(true);
    double curWeight = 0.0;

    bool first = true;
    for (int i = 0; i <= DM_CURVE_VERTEXS; i++)
    {
        if (normal.z > 0)
        {
            angle = start + (((float)i) / DM_CURVE_VERTEXS) * (delta);
        }
        else
        {
            angle = -start + M_PI - (((float)i) / DM_CURVE_VERTEXS) * (delta);
        }

        curWeight = startWeight + (((float)i) / DM_CURVE_VERTEXS) * (weightDelta);
        curPt1.x = center.x + (radius - curWeight / 2.0) * cos(angle);
        curPt1.y = center.y + (radius - curWeight / 2.0) * sin(angle);
        curPt2.x = center.x + (radius + curWeight / 2.0) * cos(angle);
        curPt2.y = center.y + (radius + curWeight / 2.0) * sin(angle);

        if (first)
        {
            lastPt1 = curPt1;
            lastPt2 = curPt2;
            first = false;
        }
        else
        {
            addSolid({ m.apply(lastPt1), m.apply(lastPt2), m.apply(curPt2), m.apply(curPt1) });
            lastPt1 = curPt1;
            lastPt2 = curPt2;
        }
    }
}

// ---------------------------------------------------------------------------
// 顶点生成：原各实体的 updateVertices
// ---------------------------------------------------------------------------

void GLCacheWorldDraw::addLine(const DmVector& p0, const DmVector& p1)
{
    // 原 DmLine::updateVertices
    std::vector<float> vertexes;
    vertexes.reserve(10);
    float x0 = p0.x;
    float y0 = p0.y;
    float x1 = p1.x;
    float y1 = p1.y;
    float dist = p0.distanceTo(p1);

    vertexes.emplace_back(x0);
    vertexes.emplace_back(y0);
    vertexes.emplace_back(0.0f);
    vertexes.emplace_back(0.0f);
    vertexes.emplace_back(dist);

    vertexes.emplace_back(x1);
    vertexes.emplace_back(y1);
    vertexes.emplace_back(0.0f);
    vertexes.emplace_back(dist);
    vertexes.emplace_back(dist);

    m_painter.addLine(penId(), m_group, vertexes, kFloatsPerVertex);
}

void GLCacheWorldDraw::addArc(const DmVector& center, double radiusD, double startAngleD, double endAngleD)
{
    // 原 DmArc::updateVertices：起止角为逆时针的"翻正"角度
    float startAngle = (float)startAngleD;
    float endAngle = (float)endAngleD;
    float radius = (float)radiusD;
    float x0 = (float)center.x;
    float y0 = (float)center.y;

    std::vector<float> vertexs;
    constexpr float _2pi = M_PI * 2.0f;
    float delta_angle = endAngle - startAngle;
    if (delta_angle < 0.0f)
    {
        delta_angle += _2pi;
    }
    float factor = delta_angle / _2pi;
    int segment_count = (int)std::ceil(factor * CIRCLE_SEGMENT_COUNT);
    segment_count = std::max(2, segment_count);

    vertexs.reserve((segment_count + 3) * 5);
    float ang_delta = delta_angle / segment_count;
    float total_length = delta_angle * radius;
    float angle = startAngle;
    for (int i = 0; i < segment_count; i++)
    {
        if (i == 0)
        {
            // 针对GL_LINE_STRIP_ADJACENCY的起始坐标
            vertexs.emplace_back(x0 + radius * std::cos(angle + ang_delta));
            vertexs.emplace_back(y0 + radius * std::sin(angle + ang_delta));
            vertexs.emplace_back(0.0f);
            vertexs.emplace_back(0.0f);
            vertexs.emplace_back(total_length);

            // 起始点
            vertexs.emplace_back(x0 + radius * std::cos(angle));
            vertexs.emplace_back(y0 + radius * std::sin(angle));
            vertexs.emplace_back(0.0f);
            vertexs.emplace_back(0.0f);
            vertexs.emplace_back(total_length);
        }
        angle += ang_delta;
        vertexs.emplace_back((x0 + radius * std::cos(angle)));
        vertexs.emplace_back((y0 + radius * std::sin(angle)));
        vertexs.emplace_back(0.0f);
        vertexs.emplace_back(radius * (angle - startAngle));
        vertexs.emplace_back(total_length);

        if (i == segment_count - 1)
        {
            // 针对GL_LINE_STRIP_ADJACENCY的终止坐标
            vertexs.emplace_back((x0 + radius * std::cos(angle - ang_delta)));
            vertexs.emplace_back((y0 + radius * std::sin(angle - ang_delta)));
            vertexs.emplace_back(0.0f);
            vertexs.emplace_back(total_length);
            vertexs.emplace_back(total_length);
        }
    }

    m_painter.addArc(penId(), m_group, vertexs, kFloatsPerVertex);
}

void GLCacheWorldDraw::addCircle(const DmVector& center, double radiusD)
{
    // 原 DmCircle::updateVertices
    float radius = (float)radiusD;
    float x0 = (float)center.x;
    float y0 = (float)center.y;

    std::vector<float> vertexs;
    vertexs.reserve((CIRCLE_SEGMENT_COUNT + 3) * 5);
    constexpr float _2pi = M_PI * 2.0f;
    constexpr float ang_delta = _2pi / CIRCLE_SEGMENT_COUNT;
    float total_length = _2pi * radius;
    float angle = 0.0f;
    for (int i = 0; i < CIRCLE_SEGMENT_COUNT; i++)
    {
        if (i == 0)
        {
            // 针对GL_LINE_STRIP_ADJACENCY的起始坐标
            vertexs.emplace_back((x0 + radius * std::cos(-ang_delta)));
            vertexs.emplace_back((y0 + radius * std::sin(-ang_delta)));
            vertexs.emplace_back(0.0f);
            vertexs.emplace_back(0.0f);
            vertexs.emplace_back(total_length);

            // 起始点
            vertexs.emplace_back(x0 + radius);
            vertexs.emplace_back(y0);
            vertexs.emplace_back(0.0f);
            vertexs.emplace_back(0.0f);
            vertexs.emplace_back(total_length);
        }
        angle += ang_delta;
        vertexs.emplace_back((x0 + radius * std::cos(angle)));
        vertexs.emplace_back((y0 + radius * std::sin(angle)));
        vertexs.emplace_back(0.0f);
        vertexs.emplace_back(radius * angle);
        vertexs.emplace_back(total_length);

        if (i == CIRCLE_SEGMENT_COUNT - 1)
        {
            // 针对GL_LINE_STRIP_ADJACENCY的终止坐标
            vertexs.emplace_back((x0 + radius * std::cos(ang_delta)));
            vertexs.emplace_back((y0 + radius * std::sin(ang_delta)));
            vertexs.emplace_back(0.0f);
            vertexs.emplace_back(total_length);
            vertexs.emplace_back(total_length);
        }
    }

    m_painter.addCircle(penId(), m_group, vertexs, kFloatsPerVertex);
}

void GLCacheWorldDraw::addEllipse(const DmVector& center, const DmVector& majorAxis, double ratioD,
                                  double startParamD, double endParamD, bool closed)
{
    // 原 DmEllipse::updateVertices：开放椭圆弧的起止参数为逆时针的"翻正"参数，整椭圆从参数 0 起
    std::vector<float> vertexs;
    float majorx = (float)majorAxis.x;
    float majory = (float)majorAxis.y;
    float ratio = (float)ratioD;
    float centerx = (float)center.x;
    float centery = (float)center.y;

    float radius = std::sqrt(majorx * majorx + majory * majory);
    float majorLen = std::sqrt(majorx * majorx + majory * majory);
    if (majorLen <= 0.0f)
    {
        return;
    }
    float cosa = majorx / majorLen;
    float sina = majory / majorLen;

    auto func = [cosa, sina, radius, ratio, centerx, centery](float ea, float& x, float& y)
    {
        float tx = radius * std::cos(ea); // parametric equation
        float ty = ratio * radius * std::sin(ea);
        x = (tx * cosa - ty * sina) + centerx; // first rotate then shift origin
        y = (tx * sina + ty * cosa) + centery;
    };

    // 不闭合
    if (!closed)
    {
        float startParam = (float)Math2d::correctAngle(startParamD);
        float endParam = (float)Math2d::correctAngle(endParamD);

        constexpr float _2pi = M_PI * 2.0f;
        float delta_angle = endParam - startParam;
        if (delta_angle <= 0.0f)
        {
            delta_angle += _2pi;
        }
        float factor = delta_angle / _2pi;
        int segment_count = (int)std::ceil(factor * ELLIPSE_SEGMENT_COUNT);
        constexpr int kMinSegmentCount = 10;
        segment_count = std::max(kMinSegmentCount, segment_count);
        vertexs.reserve((segment_count + 3) * 5);

        float ang_delta = delta_angle / segment_count;
        float param = startParam;
        float total_length = 0.0f;

        float tempx = 0.0f, tempy = 0.0f;
        float para = 0.0f;
        float lastX = 0.0f, lastY = 0.0f;

        for (int i = 0; i < segment_count; i++)
        {
            if (i == 0)
            {
                func(param + ang_delta, tempx, tempy);
                // 针对GL_LINE_STRIP_ADJACENCY的起始坐标
                vertexs.emplace_back(tempx);
                vertexs.emplace_back(tempy);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(total_length);

                // 起始点
                func(param, tempx, tempy);
                vertexs.emplace_back(tempx);
                vertexs.emplace_back(tempy);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(total_length);
                lastX = tempx;
                lastY = tempy;
            }
            param += ang_delta;
            func(param, tempx, tempy);
            // 同原先的 glm::distance：float 下开方
            const float dx = tempx - lastX;
            const float dy = tempy - lastY;
            para += std::sqrt(dx * dx + dy * dy);
            lastX = tempx;
            lastY = tempy;
            vertexs.emplace_back(tempx);
            vertexs.emplace_back(tempy);
            vertexs.emplace_back(0.0f);
            vertexs.emplace_back(para);
            vertexs.emplace_back(total_length);
            if (i == segment_count - 1)
            {
                // 针对GL_LINE_STRIP_ADJACENCY的终止坐标
                func(param - ang_delta, tempx, tempy);
                vertexs.emplace_back(tempx);
                vertexs.emplace_back(tempy);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(para);
                vertexs.emplace_back(total_length);
            }
        }
        //设置总长
        for (int i = 0; i < segment_count + 3; i++)
        {
            vertexs.at(5 * i + 4) = para;
        }
        m_painter.addEllipse(penId(), m_group, vertexs, kFloatsPerVertex);
    }
    //闭合
    else
    {
        vertexs.reserve((ELLIPSE_SEGMENT_COUNT + 3) * 5);
        constexpr float _2pi = M_PI * 2.0f;
        constexpr float ang_delta = _2pi / ELLIPSE_SEGMENT_COUNT;
        float param = 0.0f;
        float total_length = 0.0f;

        float tempx = 0.0f, tempy = 0.0f;
        float para = 0.0f;
        float lastX = 0.0f, lastY = 0.0f;
        for (int i = 0; i < ELLIPSE_SEGMENT_COUNT; i++)
        {
            if (i == 0)
            {
                func(-ang_delta, tempx, tempy);
                // 针对GL_LINE_STRIP_ADJACENCY的起始坐标
                vertexs.emplace_back(tempx);
                vertexs.emplace_back(tempy);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(total_length);

                // 起始点
                func(0.0f, tempx, tempy);
                vertexs.emplace_back(tempx);
                vertexs.emplace_back(tempy);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(total_length);
                lastX = tempx;
                lastY = tempy;
            }
            param += ang_delta;
            func(param, tempx, tempy);
            // 同原先的 glm::distance：float 下开方
            const float dx = tempx - lastX;
            const float dy = tempy - lastY;
            para += std::sqrt(dx * dx + dy * dy);
            lastX = tempx;
            lastY = tempy;
            vertexs.emplace_back(tempx);
            vertexs.emplace_back(tempy);
            vertexs.emplace_back(0.0f);
            vertexs.emplace_back(para);
            vertexs.emplace_back(total_length);

            if (i == ELLIPSE_SEGMENT_COUNT - 1)
            {
                // 针对GL_LINE_STRIP_ADJACENCY的终止坐标
                func(ang_delta, tempx, tempy);
                vertexs.emplace_back(tempx);
                vertexs.emplace_back(tempy);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(para);
                vertexs.emplace_back(total_length);
            }
        }
        //设置总长
        for (int i = 0; i < ELLIPSE_SEGMENT_COUNT + 3; i++)
        {
            vertexs.at(5 * i + 4) = para;
        }
        m_painter.addEllipseClosed(penId(), m_group, vertexs, kFloatsPerVertex);
    }
}

void GLCacheWorldDraw::addLineStrip(const std::vector<DmVector>& pts, bool closed)
{
    // 原 DmLineStrip::updateVertices
    if (pts.empty())
    {
        return;
    }
    // 去除重复点
    std::vector<DmVector> new_pts;
    float lastX = 0.0f;
    float lastY = 0.0f;
    float curX = 0.0f;
    float curY = 0.0f;
    bool isFirst = true;
    for (int i = 0; i < (int)pts.size(); i++)
    {
        if (isFirst)
        {
            lastX = (float)pts.at(0).x;
            lastY = (float)pts.at(0).y;
            new_pts.emplace_back(pts.at(0));
            isFirst = false;
        }
        else
        {
            curX = (float)pts.at(i).x;
            curY = (float)pts.at(i).y;
            if (curX - lastX == 0.0f && curY - lastY == 0.0f)
            {
                continue;
            }
            new_pts.emplace_back(pts.at(i));
            lastX = curX;
            lastY = curY;
        }
    }
    // 如果闭合，最后的点不能与第一个点重复
    if (closed)
    {
        float firstX = (float)pts.at(0).x;
        float firstY = (float)pts.at(0).y;
        if (firstX - lastX == 0.0f && firstY - lastY == 0.0f)
        {
            new_pts.erase(new_pts.end() - 1);
        }
    }
    if (new_pts.size() < 2)
    {
        return;
    }
    int pointCount = (int)new_pts.size();
    std::vector<float> vertexs;
    // 闭合
    if (closed)
    {
        vertexs.reserve((pointCount + 3) * 5);
        float total_length = 0.0f;
        float lastX_v = 0.0f;
        float lastY_v = 0.0f;
        float curX_v = 0.0f;
        float curY_v = 0.0f;
        float para = 0.0f;
        for (int i = 0; i < pointCount; i++)
        {
            curX_v = new_pts.at(i).x;
            curY_v = new_pts.at(i).y;
            if (i == 0)
            {
                // 针对GL_LINE_STRIP_ADJACENCY的起始坐标
                vertexs.emplace_back(new_pts.back().x);
                vertexs.emplace_back(new_pts.back().y);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(total_length);
            }
            else
            {
                para += DmVector(lastX_v, lastY_v).distanceTo(DmVector(curX_v, curY_v));
            }

            vertexs.emplace_back(curX_v);
            vertexs.emplace_back(curY_v);
            vertexs.emplace_back(0.0f);
            vertexs.emplace_back(para);
            vertexs.emplace_back(total_length);
            lastX_v = curX_v;
            lastY_v = curY_v;
            if (i == pointCount - 1)
            {
                // 最后一段的终点
                vertexs.emplace_back(new_pts.front().x);
                vertexs.emplace_back(new_pts.front().y);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(para);
                vertexs.emplace_back(total_length);

                // 针对GL_LINE_STRIP_ADJACENCY的终止坐标
                vertexs.emplace_back(new_pts.at(1).x);
                vertexs.emplace_back(new_pts.at(1).y);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(para);
                vertexs.emplace_back(total_length);
            }
        }
        // 设置总长
        for (int i = 0; i < pointCount + 3; i++)
        {
            vertexs.at(i * 5 + 4) = para;
        }
        m_painter.addSplineClosed(penId(), m_group, vertexs, kFloatsPerVertex);
    }
    // 不闭合
    else
    {
        vertexs.reserve((pointCount + 2) * 5);
        float total_length = 0.0f;
        float lastX_v = 0.0f;
        float lastY_v = 0.0f;
        float curX_v = 0.0f;
        float curY_v = 0.0f;
        float para = 0.0f;
        for (int i = 0; i < pointCount; i++)
        {
            curX_v = new_pts.at(i).x;
            curY_v = new_pts.at(i).y;
            if (i == 0)
            {
                // 针对GL_LINE_STRIP_ADJACENCY的起始坐标
                vertexs.emplace_back(new_pts.at(1).x);
                vertexs.emplace_back(new_pts.at(1).y);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(total_length);
            }
            else
            {
                para += DmVector(lastX_v, lastY_v).distanceTo(DmVector(curX_v, curY_v));
            }

            vertexs.emplace_back(curX_v);
            vertexs.emplace_back(curY_v);
            vertexs.emplace_back(0.0f);
            vertexs.emplace_back(para);
            vertexs.emplace_back(total_length);
            lastX_v = curX_v;
            lastY_v = curY_v;
            if (i == pointCount - 1)
            {
                // 针对GL_LINE_STRIP_ADJACENCY的终止坐标
                vertexs.emplace_back(new_pts.at(pointCount - 2).x);
                vertexs.emplace_back(new_pts.at(pointCount - 2).y);
                vertexs.emplace_back(0.0f);
                vertexs.emplace_back(para);
                vertexs.emplace_back(total_length);
            }
        }
        // 设置总长
        for (int i = 0; i < pointCount + 2; i++)
        {
            vertexs.at(i * 5 + 4) = para;
        }
        m_painter.addSpline(penId(), m_group, vertexs, kFloatsPerVertex);
    }
}

void GLCacheWorldDraw::addSolid(const std::vector<DmVector>& corners)
{
    // 原 DmCachePainter 画 DmSolid：角点按扇形画
    std::vector<double> xy;
    xy.reserve(corners.size() * 2);
    for (const DmVector& v : corners)
    {
        xy.emplace_back(v.x);
        xy.emplace_back(v.y);
    }
    m_painter.addSolid(penId(), m_group, static_cast<int>(corners.size() * 2), xy.data());
}

// ---------------------------------------------------------------------------
// 样条离散结果的缓存
// ---------------------------------------------------------------------------

namespace
{

/// @brief FNV-1a，按字节累加
void hashBytes(std::size_t& h, const void* data, std::size_t size)
{
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t i = 0; i < size; ++i)
    {
        h ^= bytes[i];
        h *= 1099511628211ull;
    }
}

std::size_t hashOf(const GiNurbs& curve)
{
    std::size_t h = 14695981039346656037ull;
    hashBytes(h, &curve.degree, sizeof(curve.degree));
    hashBytes(h, &curve.closed, sizeof(curve.closed));
    for (double k : curve.knots)
    {
        hashBytes(h, &k, sizeof(k));
    }
    for (const DmVector& p : curve.controlPoints)
    {
        hashBytes(h, &p.x, sizeof(p.x));
        hashBytes(h, &p.y, sizeof(p.y));
    }
    for (double w : curve.weights)
    {
        hashBytes(h, &w, sizeof(w));
    }
    return h;
}

/// @brief 内容完全相同（不用容差：离散结果要与重新离散的一模一样）
bool sameCurve(const GiNurbs& a, const GiNurbs& b)
{
    if (a.degree != b.degree || a.closed != b.closed || a.knots != b.knots || a.weights != b.weights ||
        a.controlPoints.size() != b.controlPoints.size())
    {
        return false;
    }
    for (std::size_t i = 0; i < a.controlPoints.size(); ++i)
    {
        if (a.controlPoints[i].x != b.controlPoints[i].x || a.controlPoints[i].y != b.controlPoints[i].y)
        {
            return false;
        }
    }
    return true;
}

}  // namespace

const std::vector<DmVector>& GLCacheWorldDraw::NurbsSamples::sample(const GiNurbs& curve)
{
    std::vector<Entry>& bucket = m_entries[hashOf(curve)];
    for (Entry& entry : bucket)
    {
        if (sameCurve(entry.curve, curve))
        {
            entry.used = true;
            return entry.points;
        }
    }
    Entry entry;
    entry.curve = curve;
    curve.sample(entry.points);
    entry.used = true;
    bucket.push_back(std::move(entry));
    return bucket.back().points;
}

void GLCacheWorldDraw::NurbsSamples::beginSweep()
{
    for (auto& [hash, bucket] : m_entries)
    {
        for (Entry& entry : bucket)
        {
            entry.used = false;
        }
    }
}

void GLCacheWorldDraw::NurbsSamples::endSweep()
{
    for (auto it = m_entries.begin(); it != m_entries.end();)
    {
        std::vector<Entry>& bucket = it->second;
        bucket.erase(std::remove_if(bucket.begin(), bucket.end(), [](const Entry& e) { return !e.used; }),
                     bucket.end());
        it = bucket.empty() ? m_entries.erase(it) : std::next(it);
    }
}
