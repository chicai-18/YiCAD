/// @file test_geometry_bounds.cpp
/// @brief 几何实体包围盒的单元测试
///
/// 包围盒是 R 树粗筛的输入（见方案 1.3 节）。它一旦算小了，拾取和框选就会
/// 漏掉实体，而这种错误在界面上表现为「偶尔点不中」，极难定位。圆弧的包围盒
/// 尤其容易错——正确答案不是起止点的外接矩形，而要考虑圆弧跨越的象限极值点。

#include <gtest/gtest.h>

#include <cmath>
#include <memory>

#include "ArcData.h"
#include "CircleData.h"
#include "DmArc.h"
#include "DmCircle.h"
#include "DmLine.h"
#include "DmVector.h"
#include "Math2d.h"

namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kEps = 1e-9;

void expectBounds(const DmEntity& e, double minX, double minY, double maxX, double maxY,
                  double tol = kEps)
{
    const DmVector mn = e.getMin();
    const DmVector mx = e.getMax();
    EXPECT_NEAR(mn.x, minX, tol) << "min.x";
    EXPECT_NEAR(mn.y, minY, tol) << "min.y";
    EXPECT_NEAR(mx.x, maxX, tol) << "max.x";
    EXPECT_NEAR(mx.y, maxY, tol) << "max.y";
}

std::unique_ptr<DmArc> makeArc(const DmVector& center, double radius,
                               double startAngle, double endAngle)
{
    ArcData data(center, DmVector(0.0, 0.0, 1.0), radius, startAngle, endAngle);
    auto arc = std::make_unique<DmArc>(nullptr, data);
    arc->calculateBorders();
    return arc;
}
}  // namespace

TEST(GeometryBounds, 直线包围盒是两端点的外接矩形)
{
    DmLine line(DmVector(1.0, 5.0), DmVector(7.0, 2.0));
    line.calculateBorders();

    expectBounds(line, 1.0, 2.0, 7.0, 5.0);
}

TEST(GeometryBounds, 直线端点顺序不影响包围盒)
{
    DmLine a(DmVector(1.0, 5.0), DmVector(7.0, 2.0));
    DmLine b(DmVector(7.0, 2.0), DmVector(1.0, 5.0));
    a.calculateBorders();
    b.calculateBorders();

    EXPECT_NEAR(a.getMin().x, b.getMin().x, kEps);
    EXPECT_NEAR(a.getMin().y, b.getMin().y, kEps);
    EXPECT_NEAR(a.getMax().x, b.getMax().x, kEps);
    EXPECT_NEAR(a.getMax().y, b.getMax().y, kEps);
}

TEST(GeometryBounds, 圆包围盒是外接正方形)
{
    CircleData data(DmVector(3.0, -2.0), 5.0);
    DmCircle circle(nullptr, data);
    circle.calculateBorders();

    expectBounds(circle, -2.0, -7.0, 8.0, 3.0);
}

TEST(GeometryBounds, 第一象限圆弧的包围盒不含未跨越的极值点)
{
    // 圆心原点、半径 10，从 0 度到 90 度。
    // 正确答案是 [0,10] x [0,10]，而不是整个圆的 [-10,10]^2。
    auto arc = makeArc(DmVector(0.0, 0.0), 10.0, 0.0, kPi / 2.0);

    expectBounds(*arc, 0.0, 0.0, 10.0, 10.0, 1e-6);
}

TEST(GeometryBounds, 跨越九十度极值点的圆弧包围盒包含该极值)
{
    // 从 45 度到 135 度：跨过了 y 最大的点 (0, 10)。
    // x 范围由两端点决定，y 上界必须是 10。
    auto arc = makeArc(DmVector(0.0, 0.0), 10.0, kPi / 4.0, 3.0 * kPi / 4.0);

    const double diag = 10.0 * std::sqrt(0.5);  // 约 7.0711
    expectBounds(*arc, -diag, diag, diag, 10.0, 1e-6);
}

TEST(GeometryBounds, 接近整圆的圆弧包围盒接近外接正方形)
{
    // 从 0 度到 350 度，跨过了 90、180、270 三个极值点
    auto arc = makeArc(DmVector(0.0, 0.0), 10.0, 0.0, Math2d::deg2rad(350.0));

    const DmVector mn = arc->getMin();
    const DmVector mx = arc->getMax();
    EXPECT_NEAR(mn.x, -10.0, 1e-6);
    EXPECT_NEAR(mn.y, -10.0, 1e-6);
    EXPECT_NEAR(mx.x, 10.0, 1e-6);
    EXPECT_NEAR(mx.y, 10.0, 1e-6);
}

TEST(GeometryBounds, 圆弧包围盒随圆心平移)
{
    auto atOrigin = makeArc(DmVector(0.0, 0.0), 4.0, 0.0, kPi / 2.0);
    auto moved = makeArc(DmVector(100.0, -50.0), 4.0, 0.0, kPi / 2.0);

    EXPECT_NEAR(moved->getMin().x - atOrigin->getMin().x, 100.0, 1e-6);
    EXPECT_NEAR(moved->getMin().y - atOrigin->getMin().y, -50.0, 1e-6);
    EXPECT_NEAR(moved->getMax().x - atOrigin->getMax().x, 100.0, 1e-6);
    EXPECT_NEAR(moved->getMax().y - atOrigin->getMax().y, -50.0, 1e-6);
}
