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

/// @file GsCompiler.h
/// @brief 把可绘制对象的 GI 输出编译成 GPU 记录（RENDER_PLAN.md 第 4.3.2 节的"编译"）
///
/// 输入是 worldDraw 的图元（GS 平时重放节点的 GI 流），输出按管线类分好的记录、图元记录、
/// 共享对象（块定义、字形）的引用、无限线与图片。坐标先经模型变换，再减去原点后转 float。
///
/// 属性不在这里解析成颜色值：ByLayer 记成图层序号，块定义与字形里的 ByBlock 记成"取实例记录"，
/// 着色器运行时查表，所以图层、选中这些会变的东西改了不用重新编译（第 4.3.7 节）。解析规则与旧渲染器的
/// 适配器（原先的 GLCacheWorldDraw::resolve，更早是 DmEntity::getPen(true)）相同：嵌套绘制与 drawShared 各开一层，
/// ByBlock 取外层解析后的属性，ByLayer 取所在图层，图层为空取外层的图层。
///
/// 线型的对齐（第 4.5.1 节）：直线、多段线的每一段、圆弧各自是一段居中对齐的虚线；圆、整椭圆、闭合的线串按整周期对齐；
/// 线型生成的多段线与样条连成一条线串；内联图案的线（setLinePattern）按给定的相位周期重复、不做端点对齐。
/// 按填充图案（setFill）画的 fill：图案线由这里在边界里切出（与 DmHatch 共用 HatchPatternClipper），按图案线自己的划线画，
/// 另编一份边界的三角形作线距太密时的替身（第 4.3.10 节）。
/// 小字：字形串另加一条沿基线的细条，字高小于阈值时着色器只画它、不画字形（第 4.3.10 节）。
/// 一个编译器只在一个线程上用；并行编译时每个线程一个（GsModel 的分块编译）。
/// 比例链里的实体线型比例逐层相乘（嵌套绘制），drawShared 调用方的线型比例放在实例记录里；LTSCALE 在着色器里乘。
/// 块参照不把自己的线型比例交给块的内容（DmBlockReference::setAttributes，与 AutoCAD 相同）。
/// 超长的虚线在 double 下分段（第 4.5.5 节）：顶层几何按当时的 LTSCALE 与线型的周期，共享几何只分填充图案线
/// （图案的长度随块缩放，与插入无关）。圆、圆弧在相似变换下是解析的圆弧记录，否则离散成椭圆。
/// 样条、椭圆按弦高容差离散（第 4.3.10 节）：默认是曲线包围框的尺寸乘相对容差；视图放大到弦高在屏幕上超过半个像素时，
/// 图形系统给节点一个更细的容差重新编译（compileNode 的 tolerance），取两者中小的。

#ifndef GSCOMPILER_H
#define GSCOMPILER_H

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include <QString>

#include "DmColor.h"
#include "GiNurbs.h"
#include "GsTypes.h"
#include "IGiDrawable.h"
#include "IGiGeometry.h"
#include "IGiSubEntityTraits.h"

class DmLayer;
class DmLineType;
class QImage;

/// @brief 解析后的颜色：值、随层（图层序号，可以是"实例图层"）、随块（取实例记录）
struct GsColorRef
{
    GsKind kind = GsKind::Value;
    std::uint32_t rgba = gsPackColor(0, 0, 0, 255);
    std::uint16_t layer = kGsLayerNone;

    bool operator==(const GsColorRef&) const = default;
};

/// @brief 解析后的线型：值（线型序号，0 为连续线）、随层、随块
struct GsLineTypeRef
{
    GsKind kind = GsKind::Value;
    std::uint16_t index = 0;
    std::uint16_t layer = kGsLayerNone;

    bool operator==(const GsLineTypeRef&) const = default;
};

/// @brief 解析后的线宽：值（DM::LineWidth 代码）、随层、随块
struct GsLineWeightRef
{
    GsKind kind = GsKind::Value;
    std::int16_t code = 0;
    std::uint16_t layer = kGsLayerNone;

    bool operator==(const GsLineWeightRef&) const = default;
};

/// @brief 一组解析后的属性与所在图层（冻结判断、图层为空的子对象取它）
struct GsAttributes
{
    GsColorRef color;
    GsLineTypeRef lineType;
    GsLineWeightRef lineWeight;
    std::uint16_t layer = kGsLayerNone;

    bool operator==(const GsAttributes&) const = default;
};

/// @brief 一处 drawShared 或字形：共享对象、它在本单元里的变换与它里面 ByBlock 取的属性
struct GsSharedUse
{
    const IGiDrawable* drawable = nullptr;
    GiTransform transform;        ///< 共享对象的定义坐标 -> 本单元坐标（不含本单元的原点）
    GsAttributes byBlock;         ///< 共享对象里 ByBlock 取的属性；layer 是图层为空的图元取的图层
    double lineTypeScale = 1.0;   ///< 调用处的线型比例（块参照的），共享对象里的线型比例另乘它
    bool glyph = false;           ///< 字形串里的字形（字高小于阈值时整个字形不画，第 4.3.10 节）
};

/// @brief 顶层几何里按线型画的线的长度摘要：LTSCALE、线型的图案或图层的线型改了，据此判断要不要重新分段
struct GsRunSummary
{
    GsLineTypeRef lineType;       ///< 解析后的线型：值（非连续线）或随层
    double length = 0.0;          ///< 线长 ÷ 实体线型比例 的最大值（世界长度）
};

/// @brief 一条射线或构造线（double，本单元坐标，不减原点）
struct GsInfiniteLine
{
    DmVector base;
    DmVector direction;
    bool ray = false;
    std::uint32_t prim = 0;       ///< 本单元的图元记录序号
};

/// @brief 一张图片的像素来源：纹理按 key 缓存，没有缓存时调 load 解码
struct GsImageSource
{
    QString key;
    std::function<QImage()> load;
};

/// @brief 一个单元（顶层实体，或块定义、字形）编译的结果
/// @details 记录里的图元记录序号是本单元的下标，上传时加上图元记录区段的起点
struct GsCompiled
{
    std::array<std::vector<GsTexel>, kGsClassCount> records;
    std::vector<GsPrimRecord> prims;
    std::vector<GsSharedUse> shared;
    std::vector<GsInfiniteLine> infinite;
    std::vector<GsImageSource> images;     ///< 与 Image 类的记录一一对应
    bool hasNonUniformUse = false;         ///< 有非相似变换的块参照（图层、线型改了要重新判断要不要展开）
    std::vector<GsRunSummary> runs;        ///< 顶层几何里按线型画的线，每种线型一条
    bool hasPieces = false;                ///< 有按线型分了段的线（分段的相位取决于 LTSCALE 与线型的周期）
    double curveTolerance = 0.0;           ///< 离散的曲线（样条、椭圆）用的最大弦高容差（本单元长度）；没有为 0
    double defaultCurveTolerance = 0.0;    ///< 不给节点容差时会用的最大弦高容差（缩小回去时据此恢复默认）
    bool hasNurbs = false;                 ///< 有样条（重新离散要在后台做；椭圆的离散便宜，直接重编）

    void clear();
    bool empty() const;
};

/// @brief 图案（线型的元素，最多取 12 个）的周期与第一段划线的中点（double）；只有点的图案中点为 0
void gsPatternMetrics(const std::vector<double>& dashes, double& period, double& firstDashCenter);

/// @brief 样条按"包围框的尺寸 × 它"的弦高离散（第 4.3.10 节）：与原先每段转角 3° 的点数相当
constexpr double kGsCurveRelativeTolerance = 2.0e-4;

/// @brief 椭圆默认的分段：整椭圆 120 段（与旧渲染器相同），开放椭圆弧按扫角的比例、至少 10 段。
///        对椭圆来说这正是"包围框 × 相对容差"（弦高与长半轴成正比，段数与大小无关）
constexpr int kGsEllipseSegments = 120;

/// @brief 一条曲线离散用的弦高容差：默认值与节点的容差（大于 0 时）取小的
inline double gsCurveTolerance(double defaultTolerance, double nodeTolerance)
{
    return nodeTolerance > 0.0 && nodeTolerance < defaultTolerance ? nodeTolerance : defaultTolerance;
}

/// @brief 超长的虚线超过这么多个周期时分段（第 4.5.5 节：float 的弧长参数到约 2^14 个周期时相位误差可见）
constexpr double kGsPieceLimitPeriods = 16384.0;
/// @brief 分段时每段的周期数（最后一段另含余下的，不超过 kGsPieceLimitPeriods）
constexpr double kGsPiecePeriods = 8192.0;

/// @brief 编译时向图形系统要的东西
class GsCompileContext
{
public:
    virtual ~GsCompileContext() = default;

    /// @brief 图层的序号；空指针为 kGsLayerNone
    virtual std::uint16_t layerIndex(const DmLayer* layer) = 0;

    /// @brief 线型的序号；连续线（含名为 continuous 的与没有图案的）为 0。ByLayer、ByBlock 不经这里
    virtual std::uint16_t lineTypeIndex(const DmLineType* lineType) = 0;

    /// @brief 样条按弦高容差的离散点（缓存，同样的曲线与容差只离散一次）；可能在多个线程上同时调用
    virtual std::shared_ptr<const std::vector<DmVector>> sampleNurbs(const GiNurbs& curve, double tolerance) = 0;

    /// @brief 顶层实体在 transform 下画共享对象，是否要展开成自己的几何：展开到叶子后有非相似变换、
    ///        且（按 byBlock 解析后）内容里有虚线的
    /// @details 块内虚线不随插入比例变化（D10），非等比插入时弧长没有简单的换算，单独编译世界坐标下的几何（第 4.3.3 节）
    /// @param nonUniform 输出：叶子里有没有非相似变换（不展开时也要记下，图层、线型改了要重新判断）
    virtual bool needsFlatten(const IGiDrawable& drawable, const GiTransform& transform, const GsAttributes& byBlock,
                              bool* nonUniform) = 0;

    /// @brief 内联图案（填充图案线）在线型表里的序号；同样的图案只有一项
    virtual std::uint16_t patternIndex(const std::vector<double>& dashes) = 0;

    /// @brief 超长的线要不要分段：文档模型要；预览等容器模型不分（整体变换可能带缩放，分段的参数对不上）
    virtual bool splitLongRuns() const = 0;

    /// @brief 全局线型比例 LTSCALE（分段时用）
    virtual double globalLineTypeScale() const = 0;

    /// @brief 解析后的线型（值或随层）的周期与第一段划线的中点（图案长度，double；分段时用）
    /// @return 连续线与随块返回 false
    virtual bool lineTypeMetrics(const GsLineTypeRef& lineType, double& period, double& firstDashCenter) = 0;

    /// @brief 字体 font 里字符码 code 的字形（IGiFont::glyph 要求调用方串行化，由这里负责）
    virtual const IGiDrawable* glyph(const IGiFont& font, char32_t code) = 0;

    /// @brief 字形在字形坐标里的横向范围（小字的细条用）
    /// @return 字形没有几何时返回 false
    virtual bool glyphExtent(const IGiDrawable& glyph, double& minX, double& maxX) = 0;
};

/// @brief GI 编译器，见文件说明。一次编译一个单元，可以反复使用
class GsCompiler final : public IGiWorldDraw, public IGiGeometry, public IGiSubEntityTraits
{
public:
    explicit GsCompiler(GsCompileContext& context);

    /// @brief 编译一个顶层实体：先 setAttributes 再 worldDraw；图元记录带它的槽位，顶层的 ByBlock 无处可取
    /// @param origin 分块的原点，记录里的坐标减去它
    /// @param tolerance 样条、椭圆的弦高容差（世界长度）；为 0 时按默认（gsCurveTolerance）
    void compileNode(const IGiDrawable& drawable, std::uint32_t slot, const DmVector& origin, GsCompiled& out,
                     double tolerance = 0.0);

    /// @brief 编译一个共享对象（块定义、字形）：它最外层的 ByBlock 取实例记录，图层为空取实例图层
    /// @param origin 定义坐标里的原点，记录里的坐标减去它
    void compileShared(const IGiDrawable& drawable, const DmVector& origin, GsCompiled& out);

    // IGiWorldDraw
    IGiGeometry& geometry() override { return *this; }
    IGiSubEntityTraits& traits() override { return *this; }
    GiRegenType regenType() const override { return GiRegenType::Display; }
    double deviation() const override;
    bool isDragging() const override { return false; }

    // IGiSubEntityTraits
    void setColor(const DmColor& color) override;
    void setLayer(const DmLayer* layer) override;
    void setLineType(const DmLineType* lineType) override;
    void setLineTypeScale(double scale) override;
    void setLinePattern(const GiLinePattern& pattern) override;
    void setFill(const GiHatchPattern* pattern) override;
    void setLineWeight(DM::LineWidth weight) override;
    void setTransparency(std::uint8_t alpha) override;
    void setSelectionMarker(std::int32_t marker) override;
    void setScreenSpace(const DmVector* anchor) override;

    // IGiGeometry
    void polyline(std::span<const DmVector> points, std::span<const double> bulges,
                  std::span<const GiSegmentWidth> widths, GiPolylineFlags flags) override;
    void circle(const DmVector& center, double radius) override;
    void arc(const DmVector& center, double radius, double startAngle, double sweepAngle) override;
    void ellipseArc(const DmVector& center, const DmVector& majorAxis, double ratio,
                    double startParam, double endParam) override;
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
    /// @brief 一层可绘制对象：它设置的属性、外层解析后的属性与模型变换
    struct Frame
    {
        DmColor color = DmColor(DM::FlagByBlock);
        DM::LineWidth width = DM::WidthByBlock;
        const DmLineType* lineType = nullptr;   ///< 空为 ByBlock
        const DmLayer* layer = nullptr;         ///< 空取外层
        bool hasParent = false;                 ///< 有外层（嵌套绘制）
        bool sharedTop = false;                 ///< 共享对象的最外层：ByBlock 取实例记录，图层为空取实例图层
        GsAttributes parent;                    ///< 外层解析后的属性
        GiTransform transform;                  ///< 本单元坐标 <- 本层坐标
        std::vector<GiTransform> savedTransforms;
        bool resolvedValid = false;
        GsAttributes resolved;                  ///< 本层解析后的属性（属性改了作废）
        double lineTypeScale = 1.0;             ///< 本层设的实体线型比例
        double parentLineTypeScale = 1.0;       ///< 外层的线型比例（逐层相乘）
        bool hasPattern = false;                ///< 设了内联图案（填充图案线）
        GiLinePattern pattern;
        bool hasFill = false;                   ///< 设了填充图案（setFill），之后的 fill 按图案画
        GiHatchPattern fill;
    };

    /// @brief 一条线怎么画：对齐方式与分段（各段的 dash 参数）
    struct RunPlan
    {
        GsDashMode mode = GsDashMode::None;
        bool pattern = false;                   ///< 填充图案线：线型取内联图案
        std::uint16_t patternIndex = 0;
        std::vector<double> cuts;               ///< 段的分界（弧长参数，不含 0 与总长）；空为不分段
        std::vector<std::array<float, 4>> dash; ///< 每段的 dash（不分段时一个）
    };

    void begin(std::uint32_t slot, const DmVector& origin, GsCompiled& out);
    void pushFrame(const GiTransform& transform, const GsAttributes* parent, bool sharedTop, double parentLineTypeScale);
    void drawInFrame(const IGiDrawable& drawable);
    Frame& frame() { return m_frames.back(); }
    const GsAttributes& attributes();
    GsAttributes resolve(const Frame& f);

    /// @brief 本层的线型比例（逐层相乘）
    double lineTypeScale() const;

    /// @brief 一条线（总长 length，弧长参数单位）按线型怎么画：对齐方式 mode（开放或闭合），设了内联图案时为图案线；
    ///        需要时分段并算好各段的参数
    /// @param patternScale 图案长度到弧长参数的比例（填充图案线在变换下不为 1）；负数为按本层的变换取
    RunPlan planRun(GsDashMode mode, double length, double patternScale);

    /// @brief 按当前属性加一条图元记录，返回本单元的序号
    /// @param plan 线的画法；segment 是第几段（plan 不分段时为 0）
    std::uint32_t addPrim(GsDashMode mode, double runLength, std::uint8_t flags);
    std::uint32_t addRunPrim(const RunPlan& plan, std::size_t segment, double segmentLength);
    /// @brief 填充、点、图片共用的图元记录（同一组属性只建一条）
    std::uint32_t fillPrim(std::uint8_t flags);

    /// @brief 本单元坐标的点 -> 记录里的 float（减去原点）
    GsTexel localPoint(const DmVector& p, float z, std::uint32_t prim) const;

    // 各类几何（点已在本单元坐标）
    // patternScale 见 planRun()
    void addStrip(const std::vector<DmVector>& points, bool closed, double patternScale = -1.0);
    void addLine(const DmVector& a, const DmVector& b, double patternScale = -1.0);
    void addArcRecord(const DmVector& center, double radius, double start, double sweep, bool closed,
                      double patternScale = -1.0);
    void addTransformedArc(const DmVector& center, double radius, double start, double sweep, bool full);
    void addTransformedEllipse(const DmVector& center, const DmVector& majorAxis, double ratio,
                               double startParam, double endParam, bool closed);
    void addEllipse(const DmVector& center, const DmVector& majorAxis, double ratio,
                    double startParam, double endParam, bool closed);
    void addBulgeArc(const DmVector& start, const DmVector& end, double bulge);
    void addWideSegment(const DmVector& start, const DmVector& end, double bulge, const GiSegmentWidth& width);
    void addTriangle(const DmVector& a, const DmVector& b, const DmVector& c, std::uint32_t prim);
    /// @brief 环（本层坐标）变换到本单元后剖分成三角形
    void triangulate(std::span<const GiLoop> loops, std::vector<std::array<DmVector, 3>>& triangles);
    /// @brief 按填充图案画 fill：每族图案线切进边界、按图案画，另加一份过密时的替身三角形（第 4.3.10 节）
    void addHatchPattern(std::span<const GiLoop> loops);
    /// @brief 一条填充图案线（本单元坐标）：按图案与相位画（实线族不按图案），图元记录标上线距
    /// @param patternScale 图案长度到本单元长度的比例；spacing 线距（本单元长度）
    void addHatchLine(const DmVector& a, const DmVector& b, const std::vector<double>& dashes, double phase,
                      double patternScale, double spacing);
    /// @brief 字形串的细条：沿基线跨整个字形串，字高小于阈值时代替字形（第 4.3.10 节）
    /// @param glyphs 各字形的几何（与 run.glyphs 一一对应，查不到的为空）
    void addTextBar(const GiGlyphRun& run, const std::vector<const IGiDrawable*>& glyphs);
    void addQuad(const DmVector& a, const DmVector& b, const DmVector& c, const DmVector& d, std::uint32_t prim);

    GsCompileContext& m_context;
    GsCompiled* m_out = nullptr;
    double m_tolerance = 0.0;   ///< 节点的弦高容差（compileNode 的 tolerance）
    std::uint32_t m_slot = kGsNoSlot;
    DmVector m_origin;
    std::vector<Frame> m_frames;
    /// @brief 填充类图元记录的复用：同一组属性与标志只建一条（一个单元里通常只有几组）
    struct FillPrim
    {
        GsAttributes attributes;
        std::uint8_t flags = 0;
        std::uint32_t index = 0;
    };
    std::vector<FillPrim> m_fillPrims;
};

#endif // GSCOMPILER_H
