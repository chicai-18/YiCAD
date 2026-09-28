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

/// @file GLCacheWorldDraw.h
/// @brief 旧渲染器的 GI 适配器：实体经 worldDraw 输出的图元转成 GLCachePainter 的顶点格式
///
/// RENDER_PLAN.md 第 2.3 步。旧渲染器不再按实体类型分支、不再展平子实体，而是让每个实体经 GI 描述自己，
/// 由这个适配器把图元离散成原先实体里存的那种 GL 顶点（圆弧、椭圆、样条在这里离散，算法与原先的
/// updateVertices 相同），按画笔与缓存类型放进 GLCachePainter。第 4 阶段 GS 取代旧渲染器时随之删除。
///
/// 属性的解析与原先 DmEntity::getPen(true) 沿父实体的解析相同：嵌套绘制与 drawShared 各开一层，
/// ByBlock 取外层解析后的属性，ByLayer 取所在图层（图层为空取外层的图层）。

#ifndef GLCACHEWORLDDRAW_H
#define GLCACHEWORLDDRAW_H

#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "DmColor.h"
#include "GLCache.h"
#include "IGiDrawable.h"
#include "IGiGeometry.h"
#include "GiNurbs.h"
#include "IGiSubEntityTraits.h"

class DmEntity;
class DmLayer;
class DmLineType;

namespace opengl
{
class GLCachePainter;
}

/// @brief 把 GI 图元写进 GLCachePainter 的一个缓存组（普通、选中或高亮）
class GLCacheWorldDraw final : public IGiWorldDraw, public IGiGeometry, public IGiSubEntityTraits
{
public:
    /// @brief 样条离散结果的缓存，由调用方跨整图重建保留
    /// @details 离散一条样条要递归求 B 样条基函数，比画其他图元贵两个数量级（中图纸 2 千条样条约 0.6 秒）；
    ///          原先离散结果缓存在 DmSpline 里。按曲线内容（次数、闭合、节点、控制点、权重）查找，同样的曲线只离散一次。
    ///          整图重建时先 beginSweep()，重建中取过的记为用到，endSweep() 丢掉这一轮没用到的；
    ///          两次整图重建之间取的（局部重建的选中组、高亮组）不丢，下一轮整图重建再判断
    class NurbsSamples
    {
    public:
        /// @brief 曲线的离散点：有同样的曲线就直接给出，否则离散后存下
        /// @return 引用在下一次调用 sample() 或 endSweep() 之前有效
        const std::vector<DmVector>& sample(const GiNurbs& curve);

        /// @brief 整图重建开始：全部记为没用到
        void beginSweep();

        /// @brief 整图重建结束：丢掉 beginSweep() 之后没取过的
        void endSweep();

    private:
        /// @brief 一条曲线与它的离散点
        struct Entry
        {
            GiNurbs curve;
            std::vector<DmVector> points;
            bool used = false;
        };

        std::unordered_map<std::size_t, std::vector<Entry>> m_entries;  ///< 按曲线内容的哈希分桶
    };

    /// @param painter 写入的画笔缓存
    /// @param group 写入的缓存组
    /// @param nurbsSamples 样条离散结果的缓存
    GLCacheWorldDraw(opengl::GLCachePainter& painter, opengl::CacheGroupType group, NurbsSamples& nurbsSamples);

    /// @brief 画一个顶层实体：先按实体自身的属性设置，再 worldDraw；顶层的 ByBlock 无处可取，保持原样
    void drawEntity(const DmEntity& entity);

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
    /// @brief 解析后的属性，即原先 DmPen 的三项加图层
    struct Resolved
    {
        DmColor color;
        DM::LineWidth width = DM::Width00;
        const DmLineType* lineType = nullptr;
        const DmLayer* layer = nullptr;
    };

    /// @brief 一层可绘制对象：它自己设置的属性、外层解析后的属性与当前模型变换
    struct Frame
    {
        DmColor color;                              ///< 本层设置的颜色（可为 ByLayer、ByBlock）
        DM::LineWidth width = DM::WidthByBlock;     ///< 本层设置的线宽
        const DmLineType* lineType = nullptr;       ///< 本层设置的线型；空为 ByBlock
        const DmLayer* layer = nullptr;             ///< 本层设置的图层；空取外层的图层
        bool hasParent = false;                     ///< 是否有外层（顶层实体没有，ByBlock 无处可取）
        Resolved parent;                            ///< 外层解析后的属性，ByBlock 取它
        GiTransform transform;                      ///< 当前模型变换
        std::vector<GiTransform> savedTransforms;   ///< pushTransform 之前的变换
        int penId = -1;                             ///< 解析出的画笔，属性改变后作废
    };

    /// @brief 开一层：外层为当前层（没有当前层时为顶层）；本层属性初值为全 ByBlock、图层为空
    void pushFrame(const GiTransform& transform, const Resolved* parent);

    /// @brief 按当前层画一个可绘制对象：setAttributes 后 worldDraw
    void drawInFrame(const IGiDrawable& drawable);

    /// @brief 解析一层的属性，规则同 DmEntity::getPen(true)
    static Resolved resolve(const Frame& frame);

    /// @brief 当前层的画笔 ID；第一次用到某支画笔时按原先 DmCachePainter 的规则设置它的线宽、虚线与颜色
    int penId();

    /// @brief 当前层的属性改了，画笔要重新解析
    void invalidatePen();

    /// @brief 当前层
    Frame& frame() { return m_frames.back(); }

    // 各类图元离散后写入缓存；点已经过模型变换
    void addLine(const DmVector& p0, const DmVector& p1);
    void addArc(const DmVector& center, double radius, double startAngle, double endAngle);
    void addCircle(const DmVector& center, double radius);
    void addEllipse(const DmVector& center, const DmVector& majorAxis, double ratio,
                    double startParam, double endParam, bool closed);
    void addLineStrip(const std::vector<DmVector>& points, bool closed);
    void addSolid(const std::vector<DmVector>& corners);

    /// @brief 模型空间的圆弧（圆心、半径、逆时针从 startAngle 转过 sweepAngle）经当前变换画出：
    ///        相似变换下仍是圆弧，否则是椭圆弧
    void addTransformedArc(const DmVector& center, double radius, double startAngle, double sweepAngle, bool full);

    /// @brief 模型空间的椭圆弧经当前变换画出
    void addTransformedEllipse(const DmVector& center, const DmVector& majorAxis, double ratio,
                               double startParam, double endParam, bool closed);

    /// @brief 多段线的一段带凸度的圆弧：与原先 DmPolyline 生成的 DmArc 相同
    void addBulgeArc(const DmVector& start, const DmVector& end, double bulge);

    /// @brief 多段线的一段带宽度的线段或圆弧：与原先 DmPolyline 生成的 DmSolid 相同
    void addWideSegment(const DmVector& start, const DmVector& end, double bulge, const GiSegmentWidth& width);

    opengl::GLCachePainter& m_painter;
    opengl::CacheGroupType m_group;
    NurbsSamples& m_nurbsSamples;
    std::vector<Frame> m_frames;
    std::unordered_set<int> m_configuredPens;   ///< 这一轮已设置过的画笔
};

#endif // GLCACHEWORLDDRAW_H
