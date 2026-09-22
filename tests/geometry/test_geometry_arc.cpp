/// @file test_geometry_arc.cpp
/// @brief DmArc 几何语义的单元测试
///
/// 圆弧的顺逆时针、起止角与法向量三者互相牵制：setClockwise 的契约是
/// 「显示效果不变，只改内部表示」。这条契约没有测试保护时，任何一次重构
/// 都可能把圆弧画反，而且只在特定角度区间暴露。

#include <gtest/gtest.h>

#include <cmath>
#include <memory>

#include "ArcData.h"
#include "DmArc.h"
#include "DmVector.h"
#include "Math2d.h"

namespace
{
constexpr double kPi = 3.14159265358979323846;

std::unique_ptr<DmArc> makeArc(const DmVector& center, double radius,
                               double startAngle, double endAngle)
{
    ArcData data(center, DmVector(0.0, 0.0, 1.0), radius, startAngle, endAngle);
    auto arc = std::make_unique<DmArc>(nullptr, data);
    arc->calculateBorders();
    return arc;
}

void expectPointNear(const DmVector& got, double x, double y, double tol = 1e-6)
{
    EXPECT_NEAR(got.x, x, tol);
    EXPECT_NEAR(got.y, y, tol);
}
}  // namespace

TEST(DmArcTest, 起止点由圆心半径与角度确定)
{
    auto arc = makeArc(DmVector(0.0, 0.0), 10.0, 0.0, kPi / 2.0);

    expectPointNear(arc->getStartpoint(), 10.0, 0.0);
    expectPointNear(arc->getEndpoint(), 0.0, 10.0);
}

TEST(DmArcTest, 圆心偏移时起止点跟随)
{
    auto arc = makeArc(DmVector(5.0, -3.0), 2.0, 0.0, kPi);

    expectPointNear(arc->getStartpoint(), 7.0, -3.0);
    expectPointNear(arc->getEndpoint(), 3.0, -3.0);
}

TEST(DmArcTest, 四分之一圆弧的弧长)
{
    auto arc = makeArc(DmVector(0.0, 0.0), 10.0, 0.0, kPi / 2.0);

    EXPECT_NEAR(arc->getAngleLength(), kPi / 2.0, 1e-9);
    EXPECT_NEAR(arc->getLength(), 10.0 * kPi / 2.0, 1e-6);
}

TEST(DmArcTest, 半圆的中点在圆弧上)
{
    auto arc = makeArc(DmVector(0.0, 0.0), 10.0, 0.0, kPi);

    // 0 到 180 度的中点是 90 度处
    expectPointNear(arc->getMiddlePoint(), 0.0, 10.0);
}

TEST(DmArcTest, 中点到圆心的距离等于半径)
{
    for (double endAngle : {kPi / 3.0, kPi / 2.0, kPi, 1.5 * kPi})
    {
        auto arc = makeArc(DmVector(4.0, 7.0), 3.0, 0.2, endAngle);
        const DmVector mid = arc->getMiddlePoint();
        EXPECT_NEAR(DmVector(4.0, 7.0).distanceTo(mid), 3.0, 1e-6)
            << "endAngle = " << endAngle;
    }
}

TEST(DmArcTest, switchStartEndAngle得到互补的圆弧)
{
    auto arc = makeArc(DmVector(0.0, 0.0), 10.0, 0.0, kPi / 2.0);
    const double originalLength = arc->getAngleLength();

    arc->switchStartEndAngle();

    // 原弧 1/4 圆，补弧应为 3/4 圆
    EXPECT_NEAR(arc->getAngleLength(), 2.0 * kPi - originalLength, 1e-6);
}

TEST(DmArcTest, setClockwise保持绘制出的曲线但交换起止点)
{
    // DmArc.h 的注释说「显示的效果不变」。准确地说：画出来的那条曲线不变
    // （点集、中点、弧长都不变），但参数方向翻转，因此起点与终点互换。
    // 这个区别对绘制无所谓，对「从起点开始延伸/修剪」一类操作却是关键，
    // 所以这里把它明确钉下来。
    auto arc = makeArc(DmVector(0.0, 0.0), 10.0, 0.0, kPi / 2.0);

    const DmVector start = arc->getStartpoint();
    const DmVector end = arc->getEndpoint();
    const DmVector mid = arc->getMiddlePoint();
    const double length = arc->getLength();

    ASSERT_FALSE(arc->isClockwise());
    arc->setClockwise(true);
    EXPECT_TRUE(arc->isClockwise());

    // 曲线本身不变
    expectPointNear(arc->getMiddlePoint(), mid.x, mid.y);
    EXPECT_NEAR(arc->getLength(), length, 1e-6);

    // 起止点互换
    expectPointNear(arc->getStartpoint(), end.x, end.y);
    expectPointNear(arc->getEndpoint(), start.x, start.y);
}

TEST(DmArcTest, setClockwise来回切换是幂等的)
{
    auto arc = makeArc(DmVector(1.0, 2.0), 5.0, 0.3, 2.1);
    const DmVector mid = arc->getMiddlePoint();

    arc->setClockwise(true);
    arc->setClockwise(false);

    EXPECT_FALSE(arc->isClockwise());
    expectPointNear(arc->getMiddlePoint(), mid.x, mid.y);
}

TEST(DmArcTest, 三点定圆弧)
{
    DmArc arc;
    // 单位圆上的三点：0 度、90 度、180 度
    const bool ok = arc.createFrom3P(DmVector(1.0, 0.0),
                                     DmVector(0.0, 1.0),
                                     DmVector(-1.0, 0.0));

    ASSERT_TRUE(ok);
    expectPointNear(arc.getCenter(), 0.0, 0.0);
    EXPECT_NEAR(arc.getRadius(), 1.0, 1e-6);
}

TEST(DmArcTest, 三点共线时无法定圆弧)
{
    DmArc arc;
    const bool ok = arc.createFrom3P(DmVector(0.0, 0.0),
                                     DmVector(1.0, 0.0),
                                     DmVector(2.0, 0.0));

    EXPECT_FALSE(ok);
}

TEST(DmArcTest, clone得到几何相同的独立副本)
{
    auto arc = makeArc(DmVector(3.0, 4.0), 7.0, 0.5, 2.5);

    std::unique_ptr<DmEntity> copy(arc->clone());
    ASSERT_NE(copy, nullptr);

    auto* cloned = dynamic_cast<DmArc*>(copy.get());
    ASSERT_NE(cloned, nullptr);
    EXPECT_NE(cloned, arc.get()) << "clone 必须是新对象";

    expectPointNear(cloned->getCenter(), 3.0, 4.0);
    EXPECT_NEAR(cloned->getRadius(), 7.0, 1e-9);
    EXPECT_NEAR(cloned->getStartAngle(), arc->getStartAngle(), 1e-9);
    EXPECT_NEAR(cloned->getEndAngle(), arc->getEndAngle(), 1e-9);

    // 改副本不应影响原件
    cloned->setRadius(1.0);
    EXPECT_NEAR(arc->getRadius(), 7.0, 1e-9);
}

TEST(DmArcTest, offset改变半径且保持圆心)
{
    auto arc = makeArc(DmVector(0.0, 0.0), 10.0, 0.0, kPi / 2.0);

    // 朝圆外一点偏移 2：半径变大
    const bool ok = arc->offset(DmVector(20.0, 0.0), 2.0);

    ASSERT_TRUE(ok);
    expectPointNear(arc->getCenter(), 0.0, 0.0);
    EXPECT_NEAR(arc->getRadius(), 12.0, 1e-6);
}

TEST(DmArcTest, offsetTwoSides得到内外两条圆弧)
{
    auto arc = makeArc(DmVector(0.0, 0.0), 10.0, 0.0, kPi / 2.0);

    const std::vector<DmEntity*> sides = arc->offsetTwoSides(2.0);

    ASSERT_EQ(sides.size(), 2u);
    std::vector<double> radii;
    for (DmEntity* e : sides)
    {
        auto* a = dynamic_cast<DmArc*>(e);
        ASSERT_NE(a, nullptr);
        radii.push_back(a->getRadius());
        delete e;
    }
    std::sort(radii.begin(), radii.end());
    EXPECT_NEAR(radii[0], 8.0, 1e-6);
    EXPECT_NEAR(radii[1], 12.0, 1e-6);
}
