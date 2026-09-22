/// @file test_math_vector.cpp
/// @brief DmVector 的单元测试
///
/// DmVector 是整个数据模型的坐标原语，被 146 个文件间接使用。它的
/// valid 标志语义（默认构造为无效）尤其容易在重构中被改掉，这里一并锁住。

#include <gtest/gtest.h>

#include <cmath>

#include "DmVector.h"

namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kEps = 1e-9;

void expectVectorNear(const DmVector& got, double x, double y, double z = 0.0,
                      double tol = kEps)
{
    EXPECT_NEAR(got.x, x, tol);
    EXPECT_NEAR(got.y, y, tol);
    EXPECT_NEAR(got.z, z, tol);
}
}  // namespace

TEST(DmVectorTest, 默认构造是无效向量)
{
    const DmVector v;
    EXPECT_FALSE(static_cast<bool>(v));
    EXPECT_FALSE(v.valid);
    expectVectorNear(v, 0.0, 0.0, 0.0);
}

TEST(DmVectorTest, 坐标构造是有效向量)
{
    const DmVector v(3.0, 4.0);
    EXPECT_TRUE(static_cast<bool>(v));
    EXPECT_TRUE(v.valid);
    expectVectorNear(v, 3.0, 4.0, 0.0);
}

TEST(DmVectorTest, magnitude与distanceTo一致)
{
    const DmVector v(3.0, 4.0);
    EXPECT_NEAR(v.magnitude(), 5.0, kEps);
    EXPECT_NEAR(v.squared(), 25.0, kEps);

    const DmVector origin(0.0, 0.0);
    EXPECT_NEAR(origin.distanceTo(v), 5.0, kEps);
    EXPECT_NEAR(origin.squaredTo(v), 25.0, kEps);
}

TEST(DmVectorTest, normalize得到单位向量且保持方向)
{
    const DmVector v(3.0, 4.0);
    const DmVector n = v.normalize();

    EXPECT_NEAR(n.magnitude(), 1.0, kEps);
    EXPECT_NEAR(n.angle(), v.angle(), kEps);
}

TEST(DmVectorTest, 极坐标构造与angle互逆)
{
    for (double angle : {0.0, kPi / 6.0, kPi / 2.0, kPi, 1.75 * kPi})
    {
        const DmVector v = DmVector::polar(2.0, angle);
        EXPECT_NEAR(v.magnitude(), 2.0, kEps) << "angle = " << angle;
        // angle() 归一到 [0, 2PI)，与输入相差整数个 2PI
        const double diff = std::fabs(v.angle() - angle);
        EXPECT_TRUE(diff < 1e-9 || std::fabs(diff - 2.0 * kPi) < 1e-9)
            << "angle = " << angle << " got " << v.angle();
    }
}

TEST(DmVectorTest, 绕原点旋转九十度)
{
    DmVector v(1.0, 0.0);
    v.rotate(kPi / 2.0);
    expectVectorNear(v, 0.0, 1.0);
}

TEST(DmVectorTest, 绕指定中心旋转一百八十度)
{
    DmVector v(3.0, 1.0);
    v.rotate(DmVector(1.0, 1.0), kPi);
    expectVectorNear(v, -1.0, 1.0);
}

TEST(DmVectorTest, 旋转四次九十度回到原点)
{
    DmVector v(2.0, -5.0);
    const DmVector original = v;
    for (int i = 0; i < 4; ++i)
    {
        v.rotate(kPi / 2.0);
    }
    expectVectorNear(v, original.x, original.y, original.z, 1e-9);
}

TEST(DmVectorTest, 沿Y轴镜像)
{
    DmVector v(3.0, 2.0);
    // 镜像轴为 Y 轴（(0,0) 到 (0,1)）
    v.mirror(DmVector(0.0, 0.0), DmVector(0.0, 1.0));
    expectVectorNear(v, -3.0, 2.0);
}

TEST(DmVectorTest, 镜像两次回到原位)
{
    DmVector v(3.0, 2.0);
    const DmVector original = v;
    v.mirror(DmVector(0.0, 0.0), DmVector(1.0, 1.0));
    v.mirror(DmVector(0.0, 0.0), DmVector(1.0, 1.0));
    expectVectorNear(v, original.x, original.y, original.z);
}

TEST(DmVectorTest, 平移与缩放)
{
    DmVector v(1.0, 2.0);
    v.move(DmVector(3.0, -1.0));
    expectVectorNear(v, 4.0, 1.0);

    DmVector s(2.0, 3.0);
    s.scale(2.0);
    expectVectorNear(s, 4.0, 6.0);
}

TEST(DmVectorTest, 点积与叉积)
{
    const DmVector a(1.0, 0.0, 0.0);
    const DmVector b(0.0, 1.0, 0.0);

    EXPECT_NEAR(DmVector::dotP(a, b), 0.0, kEps);
    EXPECT_NEAR(a.dotP(a), 1.0, kEps);

    const DmVector c = DmVector::crossP(a, b);
    expectVectorNear(c, 0.0, 0.0, 1.0);
}

TEST(DmVectorTest, lerp在两端点之间插值)
{
    const DmVector a(0.0, 0.0);
    const DmVector b(10.0, 20.0);

    expectVectorNear(a.lerp(b, 0.0), 0.0, 0.0);
    expectVectorNear(a.lerp(b, 1.0), 10.0, 20.0);
    expectVectorNear(a.lerp(b, 0.5), 5.0, 10.0);
}

TEST(DmVectorTest, minimum与maximum逐分量取值)
{
    const DmVector a(1.0, 8.0, 3.0);
    const DmVector b(5.0, 2.0, 7.0);

    expectVectorNear(DmVector::minimum(a, b), 1.0, 2.0, 3.0);
    expectVectorNear(DmVector::maximum(a, b), 5.0, 8.0, 7.0);
}

TEST(DmVectorTest, isInWindowOrdered判断点是否落在矩形内)
{
    const DmVector low(0.0, 0.0);
    const DmVector high(10.0, 10.0);

    EXPECT_TRUE(DmVector(5.0, 5.0).isInWindowOrdered(low, high));
    EXPECT_TRUE(DmVector(0.0, 0.0).isInWindowOrdered(low, high));
    EXPECT_FALSE(DmVector(11.0, 5.0).isInWindowOrdered(low, high));
    EXPECT_FALSE(DmVector(5.0, -1.0).isInWindowOrdered(low, high));
}

TEST(DmVectorTest, isInWindow不要求角点有序)
{
    // 两个角点顺序颠倒，结果应当与有序时一致
    EXPECT_TRUE(DmVector(5.0, 5.0).isInWindow(DmVector(10.0, 10.0), DmVector(0.0, 0.0)));
    EXPECT_FALSE(DmVector(15.0, 5.0).isInWindow(DmVector(10.0, 10.0), DmVector(0.0, 0.0)));
}

TEST(DmVectorTest, 算术运算符)
{
    const DmVector a(1.0, 2.0, 3.0);
    const DmVector b(4.0, 5.0, 6.0);

    expectVectorNear(a + b, 5.0, 7.0, 9.0);
    expectVectorNear(b - a, 3.0, 3.0, 3.0);
    expectVectorNear(a * 2.0, 2.0, 4.0, 6.0);
    expectVectorNear(b / 2.0, 2.0, 2.5, 3.0);
    expectVectorNear(-a, -1.0, -2.0, -3.0);
}

TEST(DmVectorTest, 相等比较)
{
    EXPECT_EQ(DmVector(1.0, 2.0), DmVector(1.0, 2.0));
    EXPECT_NE(DmVector(1.0, 2.0), DmVector(1.0, 2.5));
}

TEST(DmVectorTest, getDimension按索引取分量)
{
    const DmVector v(7.0, 8.0, 9.0);
    EXPECT_NEAR(v.getDimension(0), 7.0, kEps);
    EXPECT_NEAR(v.getDimension(1), 8.0, kEps);
    EXPECT_NEAR(v.getDimension(2), 9.0, kEps);
}

TEST(DmVectorTest, angleTo给出两点连线的方位角)
{
    const DmVector origin(0.0, 0.0);
    EXPECT_NEAR(origin.angleTo(DmVector(1.0, 0.0)), 0.0, kEps);
    EXPECT_NEAR(origin.angleTo(DmVector(0.0, 1.0)), kPi / 2.0, kEps);
    EXPECT_NEAR(origin.angleTo(DmVector(-1.0, 0.0)), kPi, kEps);
}

TEST(DmVectorTest, flipXY交换XY分量并丢弃Z)
{
    // 实现是 return DmVector(y, x)，z 分量不被保留而是归零。
    // DmVector.h 的注释「switch x,y for all vectors」没有说明这一点。
    // YiCAD 是二维 CAD，z 基本不参与运算，因此这里按实现的真实行为断言；
    // 若将来 flipXY 被用在带 z 的场景，这条测试会提醒改动者先确认语义。
    const DmVector v(1.0, 2.0, 3.0);
    const DmVector flipped = v.flipXY();

    EXPECT_NEAR(flipped.x, 2.0, kEps);
    EXPECT_NEAR(flipped.y, 1.0, kEps);
    EXPECT_NEAR(flipped.z, 0.0, kEps) << "flipXY 当前会丢弃 z 分量";
}
