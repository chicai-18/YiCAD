/// @file test_graphics_transform.cpp
/// @brief GiTransform：构造、合成、逆变换与相似变换的判断；GiNurbs 的离散

#include <gtest/gtest.h>

#include <cmath>

#include "GiNurbs.h"
#include "GiTransform.h"

namespace
{

void expectNear(const DmVector& actual, const DmVector& expected, double tol = 1.0e-12)
{
    EXPECT_NEAR(actual.x, expected.x, tol);
    EXPECT_NEAR(actual.y, expected.y, tol);
}

}  // namespace

TEST(GiTransformTest, 平移旋转缩放作用在点上)
{
    expectNear(GiTransform::translation(DmVector(2.0, -1.0)).apply(DmVector(1.0, 1.0)), DmVector(3.0, 0.0));
    expectNear(GiTransform::rotation(M_PI_2).apply(DmVector(1.0, 0.0)), DmVector(0.0, 1.0));
    expectNear(GiTransform::rotation(M_PI_2, DmVector(1.0, 1.0)).apply(DmVector(2.0, 1.0)), DmVector(1.0, 2.0));
    expectNear(GiTransform::scaling(DmVector(2.0, 3.0), DmVector(1.0, 1.0)).apply(DmVector(2.0, 2.0)),
               DmVector(3.0, 4.0));
    // 向量不受平移影响
    expectNear(GiTransform::translation(DmVector(5.0, 5.0)).applyVector(DmVector(1.0, 2.0)), DmVector(1.0, 2.0));
}

TEST(GiTransformTest, 镜像与DmVector的mirror一致)
{
    const DmVector a(1.0, 2.0);
    const DmVector b(4.0, 3.0);
    for (const DmVector& p : { DmVector(0.0, 0.0), DmVector(3.0, -1.0), DmVector(-2.5, 7.0) })
    {
        DmVector expected = p;
        expected.mirror(a, b);
        expectNear(GiTransform::mirroring(a, b).apply(p), expected, 1.0e-9);
    }
    EXPECT_LT(GiTransform::mirroring(a, b).determinant(), 0.0);
    EXPECT_TRUE(GiTransform::mirroring(a, a).isIdentity());
}

TEST(GiTransformTest, 合成先做右边再做左边)
{
    const GiTransform t = GiTransform::translation(DmVector(10.0, 0.0));
    const GiTransform r = GiTransform::rotation(M_PI_2);
    // 先旋转再平移
    expectNear((t * r).apply(DmVector(1.0, 0.0)), DmVector(10.0, 1.0));
    // 先平移再旋转
    expectNear((r * t).apply(DmVector(1.0, 0.0)), DmVector(0.0, 11.0));
}

TEST(GiTransformTest, 逆变换)
{
    const GiTransform m = GiTransform::translation(DmVector(3.0, -2.0)) * GiTransform::rotation(0.7) *
                          GiTransform(2.0, 0.5, -0.3, 1.5, 0.0, 0.0);
    const DmVector p(1.25, -4.5);
    expectNear(m.inverse().apply(m.apply(p)), p, 1.0e-12);
    EXPECT_TRUE((m.inverse() * m).isIdentity(1.0e-12));
}

TEST(GiTransformTest, 相似变换)
{
    double scale = 0.0;
    EXPECT_TRUE((GiTransform::rotation(0.3) * GiTransform::scaling(DmVector(2.0, 2.0))).isSimilarity(&scale));
    EXPECT_NEAR(scale, 2.0, 1.0e-12);
    // 镜像也是相似变换，比例取正
    EXPECT_TRUE(GiTransform::scaling(DmVector(-3.0, 3.0)).isSimilarity(&scale));
    EXPECT_NEAR(scale, 3.0, 1.0e-12);
    // 非等比缩放与错切不是
    EXPECT_FALSE(GiTransform::scaling(DmVector(2.0, 1.0)).isSimilarity());
    EXPECT_FALSE(GiTransform(1.0, 0.0, 0.5, 1.0, 0.0, 0.0).isSimilarity());
}

TEST(GiNurbsTest, 一次样条离散成控制点)
{
    GiNurbs curve;
    curve.degree = 1;
    curve.controlPoints = { DmVector(0.0, 0.0), DmVector(1.0, 1.0), DmVector(2.0, 0.0) };
    curve.knots = { 0.0, 0.0, 0.5, 1.0, 1.0 };
    ASSERT_TRUE(curve.isValid());
    std::vector<DmVector> pts;
    curve.sample(pts);
    ASSERT_EQ(pts.size(), 3u);
    expectNear(pts[0], DmVector(0.0, 0.0));
    expectNear(pts[1], DmVector(1.0, 1.0));
    expectNear(pts[2], DmVector(2.0, 0.0));
}

TEST(GiNurbsTest, 二次样条过首末控制点且点在曲线上)
{
    GiNurbs curve;
    curve.degree = 2;
    curve.controlPoints = { DmVector(0.0, 0.0), DmVector(1.0, 2.0), DmVector(2.0, 0.0) };
    curve.knots = { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 };
    std::vector<DmVector> pts;
    curve.sample(pts);
    ASSERT_GE(pts.size(), 10u);
    expectNear(pts.front(), DmVector(0.0, 0.0));
    expectNear(pts.back(), DmVector(2.0, 0.0));
    // 这条二次贝塞尔曲线是 y = 2x - x²
    for (const DmVector& p : pts)
    {
        EXPECT_NEAR(p.y, 2.0 * p.x - p.x * p.x, 1.0e-9);
    }
}

TEST(GiNurbsTest, 无效样条不输出点)
{
    GiNurbs curve;
    curve.degree = 3;
    curve.controlPoints = { DmVector(0.0, 0.0), DmVector(1.0, 1.0) };
    EXPECT_FALSE(curve.isValid());
    std::vector<DmVector> pts;
    curve.sample(pts);
    EXPECT_TRUE(pts.empty());
}
