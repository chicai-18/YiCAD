/// @file test_geometry_intersection.cpp
/// @brief 实体求交的单元测试
///
/// Information::getIntersection 是修剪、延伸、倒角、虚拟交点捕捉的共同底座，
/// 内部按实体类型组合分派到几十条分支。这里覆盖最常用的几条，并特别验证
/// onEntities 参数的语义——它决定返回的是「曲线方程的交点」还是
/// 「同时落在两个实体有限段上的交点」，这两者在圆弧与线段上差别很大。

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include "ArcData.h"
#include "CircleData.h"
#include "DmArc.h"
#include "DmCircle.h"
#include "DmLine.h"
#include "DmVector.h"
#include "Information.h"

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

std::unique_ptr<DmCircle> makeCircle(const DmVector& center, double radius)
{
    CircleData data(center, radius);
    auto circle = std::make_unique<DmCircle>(nullptr, data);
    circle->calculateBorders();
    return circle;
}

std::unique_ptr<DmLine> makeLine(const DmVector& a, const DmVector& b)
{
    auto line = std::make_unique<DmLine>(a, b);
    line->calculateBorders();
    return line;
}

/// @brief 解集中是否存在与 (x, y) 在容差内重合的点
bool hasPoint(const DmVectorSolutions& sols, double x, double y, double tol = 1e-6)
{
    for (size_t i = 0; i < sols.size(); ++i)
    {
        const DmVector& v = sols[i];
        if (std::fabs(v.x - x) < tol && std::fabs(v.y - y) < tol)
        {
            return true;
        }
    }
    return false;
}
}  // namespace

// ---------------------------------------------------------------------------
// 线 / 线
// ---------------------------------------------------------------------------

TEST(GeometryIntersection, 两条相交线段有一个交点)
{
    auto h = makeLine(DmVector(-10.0, 0.0), DmVector(10.0, 0.0));
    auto v = makeLine(DmVector(0.0, -10.0), DmVector(0.0, 10.0));

    const DmVectorSolutions sols = Information::getIntersection(h.get(), v.get(), true);

    ASSERT_EQ(sols.size(), 1u);
    EXPECT_TRUE(hasPoint(sols, 0.0, 0.0));
}

TEST(GeometryIntersection, 平行线段没有交点)
{
    auto a = makeLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    auto b = makeLine(DmVector(0.0, 5.0), DmVector(10.0, 5.0));

    const DmVectorSolutions sols = Information::getIntersection(a.get(), b.get(), true);

    EXPECT_FALSE(sols.hasValid());
}

TEST(GeometryIntersection, 延长线相交时onEntities区分有限段与无限直线)
{
    // 两条线段本身不相交，但延长线交于 (0, 0)
    auto a = makeLine(DmVector(1.0, 0.0), DmVector(10.0, 0.0));
    auto b = makeLine(DmVector(0.0, 1.0), DmVector(0.0, 10.0));

    // onEntities = false：求的是直线方程的交点，应当有解
    const DmVectorSolutions unbounded =
        Information::getIntersection(a.get(), b.get(), false);
    ASSERT_EQ(unbounded.size(), 1u);
    EXPECT_TRUE(hasPoint(unbounded, 0.0, 0.0));

    // onEntities = true：交点必须落在两条线段上，应当无解
    const DmVectorSolutions bounded =
        Information::getIntersection(a.get(), b.get(), true);
    EXPECT_FALSE(bounded.hasValid());
}

// ---------------------------------------------------------------------------
// 线 / 圆
// ---------------------------------------------------------------------------

TEST(GeometryIntersection, 割线与圆有两个交点)
{
    auto circle = makeCircle(DmVector(0.0, 0.0), 5.0);
    auto line = makeLine(DmVector(-10.0, 0.0), DmVector(10.0, 0.0));

    const DmVectorSolutions sols =
        Information::getIntersection(line.get(), circle.get(), true);

    ASSERT_EQ(sols.size(), 2u);
    EXPECT_TRUE(hasPoint(sols, -5.0, 0.0));
    EXPECT_TRUE(hasPoint(sols, 5.0, 0.0));
}

// 已知缺口：精确相切的直线与圆求不出切点。
//
// Information.cpp:398 的 getIntersectionLineArc 里有完整的相切处理
// （比较圆心到直线的距离与半径，命中则返回单个解并 setTangent(true)），
// 但该函数是死代码——全仓只有它自己的定义和声明，没有任何调用点。
//
// 实际走的是 Information.cpp 的分派：线与圆不满足 isArc(e1) && isArc(e2)，
// 因而落到通用二次曲线路径 Quadratic::getIntersection，那条路径在判别式
// 恰好为零时给不出重根。
//
// 影响：对圆/圆弧的精确切点做修剪、延伸或交点捕捉时会失败。稍微偏一点
// （相割或相离）都正常，所以日常不易察觉。
//
// 修复涉及求交分派逻辑，属于行为变更，不在阶段 0 范围内。先把缺口钉住。
TEST(GeometryIntersection, DISABLED_切线与圆只有一个交点)
{
    auto circle = makeCircle(DmVector(0.0, 0.0), 5.0);
    // y = 5 与圆相切于 (0, 5)
    auto line = makeLine(DmVector(-10.0, 5.0), DmVector(10.0, 5.0));

    const DmVectorSolutions sols =
        Information::getIntersection(line.get(), circle.get(), true);

    ASSERT_GE(sols.size(), 1u);
    EXPECT_TRUE(hasPoint(sols, 0.0, 5.0, 1e-5));
}

TEST(GeometryIntersection, 近似相切的割线仍能求出两个交点)
{
    // 与上面的 DISABLED_ 用例对照：把直线往圆心挪一点点，从相切变成相割，
    // 通用二次曲线路径就能正常给出两个解。这说明缺口只在精确相切处。
    auto circle = makeCircle(DmVector(0.0, 0.0), 5.0);
    auto line = makeLine(DmVector(-10.0, 4.99), DmVector(10.0, 4.99));

    const DmVectorSolutions sols =
        Information::getIntersection(line.get(), circle.get(), true);

    ASSERT_EQ(sols.size(), 2u);
    const double x = std::sqrt(25.0 - 4.99 * 4.99);
    EXPECT_TRUE(hasPoint(sols, x, 4.99, 1e-5));
    EXPECT_TRUE(hasPoint(sols, -x, 4.99, 1e-5));
}

TEST(GeometryIntersection, 相离的线与圆没有交点)
{
    auto circle = makeCircle(DmVector(0.0, 0.0), 5.0);
    auto line = makeLine(DmVector(-10.0, 20.0), DmVector(10.0, 20.0));

    const DmVectorSolutions sols =
        Information::getIntersection(line.get(), circle.get(), true);

    EXPECT_FALSE(sols.hasValid());
}

// ---------------------------------------------------------------------------
// 圆 / 圆
// ---------------------------------------------------------------------------

TEST(GeometryIntersection, 两圆相交于两点)
{
    // 圆心距 8，半径均为 5：交点在 x = 4, y = ±3
    auto a = makeCircle(DmVector(0.0, 0.0), 5.0);
    auto b = makeCircle(DmVector(8.0, 0.0), 5.0);

    const DmVectorSolutions sols = Information::getIntersection(a.get(), b.get(), true);

    ASSERT_EQ(sols.size(), 2u);
    EXPECT_TRUE(hasPoint(sols, 4.0, 3.0, 1e-5));
    EXPECT_TRUE(hasPoint(sols, 4.0, -3.0, 1e-5));
}

TEST(GeometryIntersection, 相离两圆没有交点)
{
    auto a = makeCircle(DmVector(0.0, 0.0), 1.0);
    auto b = makeCircle(DmVector(100.0, 0.0), 1.0);

    const DmVectorSolutions sols = Information::getIntersection(a.get(), b.get(), true);

    EXPECT_FALSE(sols.hasValid());
}

TEST(GeometryIntersection, 内含两圆没有交点)
{
    auto outer = makeCircle(DmVector(0.0, 0.0), 10.0);
    auto inner = makeCircle(DmVector(0.0, 0.0), 3.0);

    const DmVectorSolutions sols =
        Information::getIntersection(outer.get(), inner.get(), true);

    EXPECT_FALSE(sols.hasValid());
}

// ---------------------------------------------------------------------------
// 圆弧的有限段语义
// ---------------------------------------------------------------------------

TEST(GeometryIntersection, 圆弧求交时onEntities剔除不在弧段上的解)
{
    // 只取上半圆（0 到 180 度）的圆弧，与竖直线 x = 0 求交。
    // 整圆会给出 (0, 5) 和 (0, -5) 两个解，但 (0, -5) 不在这段弧上。
    auto arc = makeArc(DmVector(0.0, 0.0), 5.0, 0.0, kPi);
    auto line = makeLine(DmVector(0.0, -10.0), DmVector(0.0, 10.0));

    const DmVectorSolutions bounded =
        Information::getIntersection(line.get(), arc.get(), true);

    ASSERT_EQ(bounded.size(), 1u);
    EXPECT_TRUE(hasPoint(bounded, 0.0, 5.0, 1e-5));
    EXPECT_FALSE(hasPoint(bounded, 0.0, -5.0, 1e-5));
}

TEST(GeometryIntersection, 圆弧求交时不限定在实体上会给出整圆的解)
{
    auto arc = makeArc(DmVector(0.0, 0.0), 5.0, 0.0, kPi);
    auto line = makeLine(DmVector(0.0, -10.0), DmVector(0.0, 10.0));

    const DmVectorSolutions unbounded =
        Information::getIntersection(line.get(), arc.get(), false);

    EXPECT_EQ(unbounded.size(), 2u);
    EXPECT_TRUE(hasPoint(unbounded, 0.0, 5.0, 1e-5));
    EXPECT_TRUE(hasPoint(unbounded, 0.0, -5.0, 1e-5));
}

TEST(GeometryIntersection, 求交结果与实体顺序无关)
{
    auto circle = makeCircle(DmVector(0.0, 0.0), 5.0);
    auto line = makeLine(DmVector(-10.0, 0.0), DmVector(10.0, 0.0));

    const DmVectorSolutions ab =
        Information::getIntersection(line.get(), circle.get(), true);
    const DmVectorSolutions ba =
        Information::getIntersection(circle.get(), line.get(), true);

    ASSERT_EQ(ab.size(), ba.size());
    for (size_t i = 0; i < ab.size(); ++i)
    {
        EXPECT_TRUE(hasPoint(ba, ab[i].x, ab[i].y))
            << "顺序交换后丢了交点 (" << ab[i].x << ", " << ab[i].y << ")";
    }
}

TEST(GeometryIntersection, 空指针不崩溃)
{
    auto line = makeLine(DmVector(0.0, 0.0), DmVector(1.0, 1.0));

    EXPECT_NO_THROW({
        const DmVectorSolutions sols =
            Information::getIntersection(line.get(), nullptr, true);
        EXPECT_FALSE(sols.hasValid());
    });
}
