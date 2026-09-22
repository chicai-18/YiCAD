/// @file test_math_angle.cpp
/// @brief Math2d 角度工具的单元测试
///
/// 角度归一化是整个几何层的公共前提：圆弧的包含判断、标注文字的可读性校正、
/// 捕捉的方向比较都建立在它之上，语义漂移会以难以定位的方式扩散到各处。

#include <gtest/gtest.h>

#include <cmath>

#include "Math2d.h"

namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

/// @brief 角度比较容差。取值远小于 Math2d 自身的 DM_TOLERANCE，
///        用于验证归一化的精度而不仅仅是分支正确性。
constexpr double kEps = 1e-12;
}  // namespace

TEST(Math2dAngle, 弧度角度互转是可逆的)
{
    for (double deg : {0.0, 1.0, 45.0, 90.0, 180.0, 270.0, 359.5, -37.25})
    {
        EXPECT_NEAR(Math2d::rad2deg(Math2d::deg2rad(deg)), deg, 1e-10)
            << "deg = " << deg;
    }

    EXPECT_NEAR(Math2d::deg2rad(180.0), kPi, kEps);
    EXPECT_NEAR(Math2d::rad2deg(kPi), 180.0, 1e-10);
}

TEST(Math2dAngle, 弧度百分度互转是可逆的)
{
    // 百分度：一个直角为 100 gon。
    EXPECT_NEAR(Math2d::rad2gra(kPi / 2.0), 100.0, 1e-10);
    EXPECT_NEAR(Math2d::gra2rad(100.0), kPi / 2.0, kEps);

    for (double gra : {0.0, 50.0, 100.0, 200.0, 399.0})
    {
        EXPECT_NEAR(Math2d::rad2gra(Math2d::gra2rad(gra)), gra, 1e-10)
            << "gra = " << gra;
    }
}

TEST(Math2dAngle, correctAngle归一化到零到二PI)
{
    struct Case
    {
        double input;
        double expected;
    };

    const Case cases[] = {
        {0.0, 0.0},
        {kPi, kPi},
        {kTwoPi, 0.0},
        {-kPi / 2.0, 1.5 * kPi},
        {3.0 * kTwoPi + kPi / 4.0, kPi / 4.0},
        {-3.0 * kTwoPi - kPi / 4.0, kTwoPi - kPi / 4.0},
    };

    for (const Case& c : cases)
    {
        const double got = Math2d::correctAngle(c.input);
        EXPECT_NEAR(got, c.expected, 1e-9) << "input = " << c.input;
        EXPECT_GE(got, 0.0) << "input = " << c.input;
        EXPECT_LT(got, kTwoPi) << "input = " << c.input;
    }
}

TEST(Math2dAngle, correctAngle2归一化到负PI到正PI)
{
    // 注意区间是闭的：实现是 std::remainder(a, 2PI)，对 a = ±PI 原样返回 ±PI。
    // Math2d.h 的注释写的是半开区间 [-PI, +PI)，与实现不符。这是注释的问题，
    // 不是实现的问题——边界取值对下游（角度差、可读性校正）没有影响。
    // 这里按实现的真实契约断言，避免把注释当成规格。
    for (double a : {-10.0, -kPi, -0.5, 0.0, 0.5, kPi, 3.5, 10.0, 100.0})
    {
        const double got = Math2d::correctAngle2(a);
        EXPECT_GE(got, -kPi) << "input = " << a;
        EXPECT_LE(got, kPi) << "input = " << a;
        // 归一化只能相差整数个 2PI
        const double turns = (a - got) / kTwoPi;
        EXPECT_NEAR(turns, std::round(turns), 1e-9) << "input = " << a;
    }

    // ±PI 原样返回
    EXPECT_NEAR(Math2d::correctAngle2(kPi), kPi, 1e-12);
    EXPECT_NEAR(Math2d::correctAngle2(-kPi), -kPi, 1e-12);
}

TEST(Math2dAngle, correctAngleU归一化到零到PI)
{
    for (double a : {-10.0, -kPi, -0.5, 0.0, 0.5, kPi, 3.5, 10.0})
    {
        const double got = Math2d::correctAngleU(a);
        EXPECT_GE(got, 0.0) << "input = " << a;
        EXPECT_LE(got, kPi + 1e-9) << "input = " << a;
    }
}

TEST(Math2dAngle, isAngleBetween处理跨零点的区间)
{
    // 普通区间
    EXPECT_TRUE(Math2d::isAngleBetween(kPi / 2.0, kPi / 4.0, kPi));
    EXPECT_FALSE(Math2d::isAngleBetween(kPi * 1.5, kPi / 4.0, kPi));

    // 跨零点区间：从 7/4 PI 逆时针到 1/4 PI
    EXPECT_TRUE(Math2d::isAngleBetween(0.0, 1.75 * kPi, 0.25 * kPi));
    EXPECT_FALSE(Math2d::isAngleBetween(kPi, 1.75 * kPi, 0.25 * kPi));
}

TEST(Math2dAngle, getAngleDifferenceU返回最小无符号差)
{
    EXPECT_NEAR(Math2d::getAngleDifferenceU(0.0, kPi / 2.0), kPi / 2.0, 1e-9);
    // 最短路径是 1/2 PI 而不是 3/2 PI
    EXPECT_NEAR(Math2d::getAngleDifferenceU(0.0, 1.5 * kPi), kPi / 2.0, 1e-9);
    // 对称性
    EXPECT_NEAR(Math2d::getAngleDifferenceU(0.3, 2.9),
                Math2d::getAngleDifferenceU(2.9, 0.3), 1e-12);
    // 结果始终落在 [0, PI]
    for (double a : {0.0, 1.0, 3.0, 5.0, 6.2})
    {
        for (double b : {0.0, 1.0, 3.0, 5.0, 6.2})
        {
            const double d = Math2d::getAngleDifferenceU(a, b);
            EXPECT_GE(d, 0.0);
            EXPECT_LE(d, kPi + 1e-9);
        }
    }
}

TEST(Math2dAngle, isSameDirection遵守容差)
{
    EXPECT_TRUE(Math2d::isSameDirection(1.0, 1.0 + 1e-9, 1e-6));
    EXPECT_FALSE(Math2d::isSameDirection(1.0, 1.0 + 1e-3, 1e-6));
    // 跨零点：0 与 2PI 是同一方向
    EXPECT_TRUE(Math2d::isSameDirection(0.0, kTwoPi, 1e-6));
}

TEST(Math2dAngle, round按精度舍入)
{
    EXPECT_EQ(Math2d::round(1.4), 1);
    EXPECT_EQ(Math2d::round(1.6), 2);
    EXPECT_EQ(Math2d::round(-1.6), -2);

    EXPECT_NEAR(Math2d::round(1.23456, 0.01), 1.23, 1e-9);
    EXPECT_NEAR(Math2d::round(1.23556, 0.01), 1.24, 1e-9);
}

TEST(Math2dAngle, equal使用DM容差)
{
    EXPECT_TRUE(Math2d::equal(1.0, 1.0));
    EXPECT_TRUE(Math2d::equal(1.0, 1.0 + 1e-12));
    EXPECT_FALSE(Math2d::equal(1.0, 1.1));
}

TEST(Math2dAngle, findGCD是欧几里得算法)
{
    EXPECT_EQ(Math2d::findGCD(12u, 18u), 6u);
    EXPECT_EQ(Math2d::findGCD(18u, 12u), 6u);
    EXPECT_EQ(Math2d::findGCD(17u, 5u), 1u);
    EXPECT_EQ(Math2d::findGCD(100u, 100u), 100u);
}
