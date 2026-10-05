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

/// @file test_graphics_custom.cpp
/// @brief 自定义实体的宿主默认实现（RENDER_PLAN.md 第 7.2 步）：worldDraw 的图元做成基本实体（DmGiExplode），
///        包围框、拾取、捕捉、求交、炸开基于它；代理实体按代理图形画

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>

#include "DmArc.h"
#include "DmBlock.h"
#include "DmBlockReference.h"
#include "DmBlockTable.h"
#include "DmCustomEntity.h"
#include "DmCustomEntityRegistry.h"
#include "DmDocument.h"
#include "DmEllipse.h"
#include "DmGiExplode.h"
#include "DmHatch.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "DmLine.h"
#include "DmLineTypeTable.h"
#include "DmPolyline.h"
#include "DmProxyEntity.h"
#include "DmRegion.h"
#include "DmSolid.h"
#include "GiStream.h"
#include "GiTextDump.h"
#include "IGiGeometry.h"
#include "IGiSubEntityTraits.h"
#include "Information.h"
#include "support/SamplePipeEntity.h"

namespace
{
constexpr double kTol = 1e-9;
constexpr double kDeg = M_PI / 180.0;

void expectNear(const DmVector& got, const DmVector& expected, double tol = kTol)
{
    EXPECT_NEAR(got.x, expected.x, tol) << "(" << got.x << "," << got.y << ")";
    EXPECT_NEAR(got.y, expected.y, tol) << "(" << got.x << "," << got.y << ")";
}

/// @brief 用一个函数描述几何的可绘制对象
struct FunctionDrawable : IGiDrawable
{
    std::function<void(IGiSubEntityTraits&)> attributes;
    std::function<void(IGiWorldDraw&)> draw;

    void setAttributes(IGiSubEntityTraits& traits) const override
    {
        if (attributes)
        {
            attributes(traits);
        }
    }
    void worldDraw(IGiWorldDraw& wd) const override { draw(wd); }
};

/// @brief 按类型数
std::map<DM::EntityType, int> countTypes(const std::vector<DmGiExplode::Item>& items)
{
    std::map<DM::EntityType, int> counts;
    for (const DmGiExplode::Item& item : items)
    {
        ++counts[item.entity->getEntityType()];
    }
    return counts;
}

/// @brief 初始化示例实体的类型（类名取 MetaType 的类型名；不登记进注册表）
void ensureSampleType()
{
    DmCustomEntityRegistry::describe<SamplePipeEntity>(DmProxyFlags::None, QString());
}

/// @brief (0,0)-(10,0)、管径 2 的管道，绿色实线
SamplePipeEntity makePipe()
{
    ensureSampleType();
    SamplePipeEntity pipe({DmVector(0, 0), DmVector(10, 0)}, 2.0);
    pipe.setPen(DmPen(DmColor(0, 255, 0), DM::Width00, DmLineTypeTable::Continuous));
    pipe.update();
    return pipe;
}
}  // namespace

TEST(GiExplodeTest, 管道的图元做成直线圆弧与SOLID)
{
    const SamplePipeEntity pipe = makePipe();
    const auto items = DmGiExplode::run(pipe, nullptr, DmGiExplode::Purpose::Query);
    const auto counts = countTypes(items);
    // 中心线与两条边线（两点的多段线做成直线）、两个端头、箭头的两个三角形
    EXPECT_EQ(counts.at(DM::EntityLine), 3);
    EXPECT_EQ(counts.at(DM::EntityArc), 2);
    EXPECT_EQ(counts.at(DM::EntitySolid), 2);
    EXPECT_EQ(items.size(), 7u);
    // 属性：实体自己的画笔原样给出，箭头逐图元覆盖成红色
    for (const DmGiExplode::Item& item : items)
    {
        const DmColor expected = item.entity->getEntityType() == DM::EntitySolid ? DmColor(255, 0, 0)
                                                                                  : DmColor(0, 255, 0);
        EXPECT_EQ(item.entity->getPen(false).getColor(), expected);
        EXPECT_FALSE(item.fromGlyph);
    }
}

TEST(GiExplodeTest, 非等比缩放下圆变椭圆凸度段变椭圆弧)
{
    FunctionDrawable drawable;
    drawable.draw = [](IGiWorldDraw& wd)
    {
        wd.geometry().pushTransform(GiTransform::scaling(DmVector(2.0, 1.0)));
        wd.geometry().circle(DmVector(1, 0), 1.0);
        const DmVector pts[2] = {DmVector(0, 0), DmVector(2, 0)};
        const double bulges[1] = {1.0};
        wd.geometry().polyline(pts, bulges, {}, GiPolylineFlags::None);
        wd.geometry().popTransform();
    };
    const auto items = DmGiExplode::run(drawable, nullptr, DmGiExplode::Purpose::Query);
    ASSERT_EQ(items.size(), 2u);
    auto* ellipse = dynamic_cast<DmEllipse*>(items[0].entity.get());
    ASSERT_NE(ellipse, nullptr);
    expectNear(ellipse->getCenter(), DmVector(2, 0));
    EXPECT_NEAR(ellipse->getMajorP().magnitude(), 2.0, kTol);
    EXPECT_NEAR(ellipse->getRatio(), 0.5, kTol);
    EXPECT_TRUE(ellipse->isClosed());
    // 凸度 1 是从 (0,0) 逆时针到 (2,0) 的下半圆；X 方向放大 2 倍后是从 (0,0) 到 (4,0) 的半个椭圆
    auto* half = dynamic_cast<DmEllipse*>(items[1].entity.get());
    ASSERT_NE(half, nullptr);
    expectNear(half->getCenter(), DmVector(2, 0));
    const DmVector a = half->getStartpoint();
    const DmVector b = half->getEndpoint();
    EXPECT_TRUE((a.distanceTo(DmVector(0, 0)) < 1e-6 && b.distanceTo(DmVector(4, 0)) < 1e-6) ||
                (a.distanceTo(DmVector(4, 0)) < 1e-6 && b.distanceTo(DmVector(0, 0)) < 1e-6));
    EXPECT_NEAR(half->getNearestPointOnEntity(DmVector(2, -5), true).y, -1.0, 1e-6) << "是下半个";
}

TEST(GiExplodeTest, 含镜像的相似变换下圆弧仍是圆弧且端点对应)
{
    const GiTransform m = GiTransform::translation(DmVector(5, 5)) * GiTransform::rotation(30.0 * kDeg) *
                          GiTransform::mirroring(DmVector(0, 0), DmVector(1, 0)) * GiTransform::scaling(DmVector(3, 3));
    FunctionDrawable drawable;
    drawable.draw = [&m](IGiWorldDraw& wd)
    {
        wd.geometry().pushTransform(m);
        wd.geometry().arc(DmVector(0, 0), 1.0, 10.0 * kDeg, 70.0 * kDeg);
        wd.geometry().popTransform();
    };
    const auto items = DmGiExplode::run(drawable, nullptr, DmGiExplode::Purpose::Query);
    ASSERT_EQ(items.size(), 1u);
    auto* arc = dynamic_cast<DmArc*>(items[0].entity.get());
    ASSERT_NE(arc, nullptr);
    expectNear(arc->getCenter(), DmVector(5, 5));
    EXPECT_NEAR(arc->getRadius(), 3.0, kTol);
    const DmVector s = m.apply(DmVector(std::cos(10.0 * kDeg), std::sin(10.0 * kDeg)));
    const DmVector e = m.apply(DmVector(std::cos(80.0 * kDeg), std::sin(80.0 * kDeg)));
    // 镜像后方向反转：逆时针的起点是原终点的像
    expectNear(arc->getStartpoint(), e, 1e-9);
    expectNear(arc->getEndpoint(), s, 1e-9);
}

TEST(GiExplodeTest, 嵌套绘制的随块取外层且外层随层时按外层图层解析)
{
    DmDocument doc;
    auto addLayer = [&doc](const QString& name, const DmColor& color)
    {
        auto* layer = new DmLayer(name);
        layer->setDocument(&doc);
        DmPen pen = layer->getPen();
        pen.setColor(color);
        layer->setPen(pen);
        doc.getLayerTable()->add_direct(layer);
        return layer;
    };
    DmLayer* outer = addLayer(QStringLiteral("外"), DmColor(0, 0, 255));
    DmLayer* inner = addLayer(QStringLiteral("内"), DmColor(255, 255, 0));

    FunctionDrawable child;
    child.attributes = [inner](IGiSubEntityTraits& t)
    {
        t.setLayer(inner);
        t.setColor(DmColor(DM::FlagByBlock));
    };
    child.draw = [](IGiWorldDraw& wd)
    {
        const DmVector pts[2] = {DmVector(0, 0), DmVector(1, 0)};
        wd.geometry().polyline(pts, {}, {}, GiPolylineFlags::None);
    };
    FunctionDrawable parent;
    parent.attributes = [outer](IGiSubEntityTraits& t)
    {
        t.setLayer(outer);
        t.setColor(DmColor(DM::FlagByLayer));
        t.setLineTypeScale(2.0);
    };
    parent.draw = [&child](IGiWorldDraw& wd)
    {
        wd.traits().setLineTypeScale(1.0);
        wd.geometry().draw(child);
        const DmVector pts[2] = {DmVector(0, 1), DmVector(1, 1)};
        wd.geometry().polyline(pts, {}, {}, GiPolylineFlags::None);
    };
    const auto items = DmGiExplode::run(parent, &doc, DmGiExplode::Purpose::Explode);
    ASSERT_EQ(items.size(), 2u);
    // 子对象在"内"图层上，随块取外层的随层，外层的图层是"外"：解析成"外"的蓝色
    EXPECT_EQ(items[0].entity->getLayer(false), inner);
    EXPECT_EQ(items[0].entity->getPen(false).getColor(), DmColor(0, 0, 255));
    // 外层自己的图元：随层原样给出
    EXPECT_EQ(items[1].entity->getLayer(false), outer);
    EXPECT_TRUE(items[1].entity->getPen(false).getColor().isByLayer());
    // 线型比例：外层 setAttributes 设 2、worldDraw 里改成 1 之后才画子对象
    EXPECT_DOUBLE_EQ(items[0].entity->getLineTypeScale(), 1.0);
    EXPECT_DOUBLE_EQ(items[1].entity->getLineTypeScale(), 1.0);
}

TEST(GiExplodeTest, 炸开时块能表示成块参照就做成块参照否则展开)
{
    DmDocument doc;
    auto* block = new DmBlock(&doc, DmBlockData(QStringLiteral("B"), DmVector(1.0, 1.0), false));
    doc.getBlockTable()->add_direct(block);
    block->getEntityTable().add_direct(new DmLine(DmVector(1.0, 1.0), DmVector(2.0, 1.0)));

    // 插入点 (10,0)、比例 (2,-3)、旋转 30°
    const GiTransform insert = GiTransform::translation(DmVector(10, 0)) * GiTransform::rotation(30.0 * kDeg) *
                               GiTransform::scaling(DmVector(2, -3)) * GiTransform::translation(DmVector(-1, -1));
    FunctionDrawable drawable;
    GiTransform placement = insert;
    drawable.draw = [&](IGiWorldDraw& wd) { wd.geometry().drawShared(*block, placement, GiByBlockTraits()); };

    auto items = DmGiExplode::run(drawable, &doc, DmGiExplode::Purpose::Explode);
    ASSERT_EQ(items.size(), 1u);
    auto* reference = dynamic_cast<DmBlockReference*>(items[0].entity.get());
    ASSERT_NE(reference, nullptr);
    const DmBlockReferenceData data = reference->getData();
    EXPECT_EQ(data.name, QStringLiteral("B"));
    expectNear(data.insertionPoint, DmVector(10, 0));
    EXPECT_NEAR(data.scaleFactor.x, 2.0, kTol);
    EXPECT_NEAR(data.scaleFactor.y, -3.0, kTol);
    EXPECT_NEAR(data.angle, 30.0 * kDeg, kTol);

    // 拾取、捕捉用的结果总是展开块的内容
    items = DmGiExplode::run(drawable, &doc, DmGiExplode::Purpose::Query);
    ASSERT_EQ(items.size(), 1u);
    ASSERT_EQ(items[0].entity->getEntityType(), DM::EntityLine);
    expectNear(items[0].entity->getStartpoint(), insert.apply(DmVector(1, 1)));

    // 有错切的块参照表示不出来：炸开也展开
    placement = GiTransform(1.0, 0.0, 0.5, 1.0, 0.0, 0.0);
    items = DmGiExplode::run(drawable, &doc, DmGiExplode::Purpose::Explode);
    ASSERT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0].entity->getEntityType(), DM::EntityLine);
}

TEST(GiExplodeTest, 实心填充炸开成带孔的实心填充拾取只取边界)
{
    DmDocument doc;
    FunctionDrawable drawable;
    drawable.draw = [](IGiWorldDraw& wd)
    {
        GiLoop outer;
        outer.points = {DmVector(0, 0), DmVector(10, 0), DmVector(10, 10), DmVector(0, 10)};
        GiLoop hole;
        hole.points = {DmVector(4, 4), DmVector(6, 4), DmVector(6, 6), DmVector(4, 6)};
        const GiLoop loops[2] = {outer, hole};
        wd.geometry().fill(loops, GiFillRule::EvenOdd);
    };
    auto items = DmGiExplode::run(drawable, &doc, DmGiExplode::Purpose::Explode);
    ASSERT_EQ(items.size(), 1u);
    auto* hatch = dynamic_cast<DmHatch*>(items[0].entity.get());
    ASSERT_NE(hatch, nullptr);
    EXPECT_TRUE(hatch->isSolid());
    ASSERT_NE(hatch->getBoundary(), nullptr);
    EXPECT_EQ(hatch->getBoundary()->getData().getHoles().size(), 1u);

    items = DmGiExplode::run(drawable, &doc, DmGiExplode::Purpose::Query);
    ASSERT_EQ(items.size(), 2u);
    for (const DmGiExplode::Item& item : items)
    {
        auto* polyline = dynamic_cast<DmPolyline*>(item.entity.get());
        ASSERT_NE(polyline, nullptr);
        EXPECT_TRUE(polyline->isClosed());
    }
}

TEST(GiExplodeTest, 图案填充炸开成切好的划线)
{
    FunctionDrawable drawable;
    drawable.draw = [](IGiWorldDraw& wd)
    {
        GiHatchPattern pattern;
        GiHatchPatternLine line;
        line.base = DmVector(0, 0.5);
        line.direction = DmVector(1, 0);
        line.offset = DmVector(0, 1);
        pattern.lines.push_back(line);
        wd.traits().setFill(&pattern);
        GiLoop square;
        square.points = {DmVector(0, 0), DmVector(10, 0), DmVector(10, 3), DmVector(0, 3)};
        wd.geometry().fill(std::span<const GiLoop>(&square, 1), GiFillRule::EvenOdd);
    };
    const auto items = DmGiExplode::run(drawable, nullptr, DmGiExplode::Purpose::Explode);
    // 线距 1、从 y = 0.5 起：y = 0.5、1.5、2.5 三条，各贯穿整个宽度
    ASSERT_EQ(items.size(), 3u);
    for (const DmGiExplode::Item& item : items)
    {
        ASSERT_EQ(item.entity->getEntityType(), DM::EntityLine);
        EXPECT_NEAR(item.entity->getStartpoint().distanceTo(item.entity->getEndpoint()), 10.0, 1e-9);
    }
}

TEST(CustomEntityDefaultTest, 包围框含端头)
{
    const SamplePipeEntity pipe = makePipe();
    expectNear(pipe.getMin(), DmVector(-1, -1));
    expectNear(pipe.getMax(), DmVector(11, 1));
    EXPECT_EQ(pipe.getEntityType(), DM::EntityCustom);
    EXPECT_EQ(pipe.className(), QStringLiteral("ext.sample.Pipe"));
}

TEST(CustomEntityDefaultTest, 捕捉与拾取按基本实体)
{
    const SamplePipeEntity pipe = makePipe();
    double dist = 0.0;
    expectNear(pipe.getNearestEndpoint(DmVector(10.2, 0.9), &dist), DmVector(10, 1));
    EXPECT_NEAR(dist, std::hypot(0.2, 0.1), 1e-9);
    expectNear(pipe.getNearestCenter(DmVector(10.5, 0.2)), DmVector(10, 0));
    expectNear(pipe.getNearestMiddle(DmVector(5.2, 1.1)), DmVector(5, 1));
    expectNear(pipe.getNearestPointOnEntity(DmVector(3, 0.7)), DmVector(3, 1));
    EXPECT_NEAR(pipe.getDistanceToPoint(DmVector(3, 0.7)), 0.3, 1e-9);
    EXPECT_TRUE(pipe.isPointOnEntity(DmVector(3, 1)));
    // y 取 0.6 而不是 0.5：0.5 正好是箭头最上面的顶点的高度，碰上 isPtInside 的缺陷（见下面的 DISABLED_ 用例）
    EXPECT_FALSE(pipe.isPointOnEntity(DmVector(3, 0.6)));
    // 拾取总是交出本实体（与块参照相同）
    DmEntity* picked = nullptr;
    pipe.getDistanceToPoint(DmVector(3, 0.7), &picked, DM::ResolveAll);
    EXPECT_EQ(picked, &pipe);
    // 夹点由示例实体自己实现：两个折点与管径夹点
    EXPECT_EQ(pipe.getRefPoints().size(), 3u);
}

// 缺陷（本阶段范围外，发现于上一例）：GeometryMethods::isPtInside（base/geometry/GeometryMethods.cpp:601-608 的
// isXRayCrossLine）按"交点与边的终点重合不算相交"去重，水平射线正好擦过多边形最上或最下的顶点时，以它为起点的边算一次、
// 以它为终点的边不算，合计奇数次，点被判成在多边形里。DmSolid::getNearestPointOnEntity 据此把距离给 0，所以在 SOLID
// 最高（最低）顶点左边、同一高度处点击会拾取到这个 SOLID；自定义实体的默认拾取用 SOLID 表示三角形，同样受影响。
// 修好后本例应当通过
TEST(CustomEntityDefaultTest, DISABLED_SOLID的射线擦过顶点时不算在里面)
{
    DmSolid solid(nullptr, SolidData({DmVector(6, 0), DmVector(4, 0.5), DmVector(4.5, 0)}));
    EXPECT_GT(solid.getDistanceToPoint(DmVector(3, 0.5)), 0.9);
}

TEST(CustomEntityDefaultTest, 与直线求交按基本实体)
{
    const SamplePipeEntity pipe = makePipe();
    DmLine line(DmVector(8, -3), DmVector(8, 3));
    const DmVectorSolutions sol = Information::getIntersection(&line, &pipe, true);
    ASSERT_EQ(sol.size(), 3u);
    std::vector<double> ys;
    for (const DmVector& p : sol)
    {
        EXPECT_NEAR(p.x, 8.0, 1e-9);
        ys.push_back(p.y);
    }
    std::sort(ys.begin(), ys.end());
    EXPECT_NEAR(ys[0], -1.0, 1e-9);
    EXPECT_NEAR(ys[1], 0.0, 1e-9);
    EXPECT_NEAR(ys[2], 1.0, 1e-9);
    // 另一个方向（自定义实体在前）结果相同
    EXPECT_EQ(Information::getIntersection(&pipe, &line, true).size(), 3u);
    // 交叉选用的子实体
    EXPECT_EQ(pipe.getSubEntities().size(), 7u);
}

TEST(CustomEntityDefaultTest, 炸开按图元做成基本实体)
{
    const SamplePipeEntity pipe = makePipe();
    std::vector<DmEntity*> parts = pipe.explode();
    EXPECT_EQ(parts.size(), 7u);
    for (DmEntity* part : parts)
    {
        EXPECT_EQ(part->getParent(), nullptr);
        delete part;
    }
}

TEST(CustomEntityDefaultTest, 按仿射变换改动实体)
{
    // 旋转、非等比缩放、镜像与平移的组合：拆成实体自己的旋转、镜像、缩放、平移后，折点落在仿射变换的像上
    const GiTransform m = GiTransform::translation(DmVector(5, -2)) * GiTransform::rotation(0.7) *
                          GiTransform::scaling(DmVector(2.0, 0.5)) * GiTransform::rotation(0.3) *
                          GiTransform::mirroring(DmVector(0, 0), DmVector(1, 1));
    ensureSampleType();
    SamplePipeEntity pipe({DmVector(1, 2), DmVector(4, -1), DmVector(-3, 5)}, 1.0);
    const std::vector<DmVector> before = pipe.vertices();
    pipe.transformBy(m);
    ASSERT_EQ(pipe.vertices().size(), before.size());
    for (std::size_t i = 0; i < before.size(); ++i)
    {
        expectNear(pipe.vertices()[i], m.apply(before[i]), 1e-9);
    }
}

TEST(CustomEntityDefaultTest, 代理按代理图形画与原实体相同)
{
    const SamplePipeEntity pipe = makePipe();
    DmProxyEntity proxy(pipe.className(), DmProxyFlags::Transform);
    proxy.setPen(pipe.getPen(false));
    proxy.setProxyGraphics(pipe.proxyGraphics());
    proxy.update();
    EXPECT_EQ(giDump(proxy), giDump(pipe));
    expectNear(proxy.getMin(), pipe.getMin());
    expectNear(proxy.getMax(), pipe.getMax());

    // 允许变换时记下累计变换：图形与包围框跟着动
    proxy.move(DmVector(100, 0));
    expectNear(proxy.getMin(), pipe.getMin() + DmVector(100, 0));
    expectNear(proxy.getNearestEndpoint(DmVector(110.1, 1.1)), DmVector(110, 1));
}
