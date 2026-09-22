/// @file test_math_solver.cpp
/// @brief 多项式与线性方程组求解器的单元测试
///
/// 这些求解器是实体求交的底层依赖：圆与圆、圆与椭圆、椭圆与椭圆的交点最终
/// 都归结到二次/四次方程。用「构造已知根 -> 展开成系数 -> 求解 -> 比对根」
/// 的方式验证，避免把实现里的舍入策略当成期望值写死。

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "Math2d.h"
#include "QuarticEquation.h"

namespace
{
/// @brief 判断 roots 中是否存在与 expected 在容差内相等的元素
bool containsRoot(const std::vector<double>& roots, double expected, double tol = 1e-6)
{
    return std::any_of(roots.begin(), roots.end(),
                       [&](double r) { return std::fabs(r - expected) < tol; });
}

/// @brief 把首一多项式在 x 处求值，coeffs 为除首项外的系数（降幂）
double evalMonic(const std::vector<double>& coeffs, double x)
{
    double v = 1.0;  // 首项系数恒为 1
    for (double c : coeffs)
    {
        v = v * x + c;
    }
    return v;
}
}  // namespace

// ---------------------------------------------------------------------------
// 一元二次：x^2 + ce[0] x + ce[1] = 0
// ---------------------------------------------------------------------------

TEST(Math2dSolver, 二次方程求出两个相异实根)
{
    // (x - 2)(x + 3) = x^2 + x - 6
    const std::vector<double> roots = Math2d::quadraticSolver({1.0, -6.0});

    ASSERT_EQ(roots.size(), 2u);
    EXPECT_TRUE(containsRoot(roots, 2.0));
    EXPECT_TRUE(containsRoot(roots, -3.0));
}

TEST(Math2dSolver, 二次方程重根只返回一个)
{
    // (x - 5)^2 = x^2 - 10x + 25
    const std::vector<double> roots = Math2d::quadraticSolver({-10.0, 25.0});

    ASSERT_FALSE(roots.empty());
    for (double r : roots)
    {
        EXPECT_NEAR(r, 5.0, 1e-6);
    }
}

TEST(Math2dSolver, 二次方程无实根时返回空)
{
    // x^2 + 1 = 0
    EXPECT_TRUE(Math2d::quadraticSolver({0.0, 1.0}).empty());
}

TEST(Math2dSolver, 二次方程系数个数不符时返回空)
{
    EXPECT_TRUE(Math2d::quadraticSolver({}).empty());
    EXPECT_TRUE(Math2d::quadraticSolver({1.0}).empty());
    EXPECT_TRUE(Math2d::quadraticSolver({1.0, 2.0, 3.0}).empty());
}

TEST(Math2dSolver, 二次方程在根相差悬殊时不丢精度)
{
    // (x - 1e8)(x - 1e-8)：朴素求根公式在这里会因抵消丢掉小根，
    // 实现用的是 Vieta 形式，应当两个根都准确。
    const double big = 1e8;
    const double small = 1e-8;
    const std::vector<double> roots =
        Math2d::quadraticSolver({-(big + small), big * small});

    ASSERT_EQ(roots.size(), 2u);
    EXPECT_TRUE(containsRoot(roots, big, big * 1e-9));
    EXPECT_TRUE(containsRoot(roots, small, small * 1e-6));
}

// ---------------------------------------------------------------------------
// 一元三次：x^3 + ce[0] x^2 + ce[1] x + ce[2] = 0
// ---------------------------------------------------------------------------

TEST(Math2dSolver, 三次方程求出三个实根)
{
    // (x - 1)(x - 2)(x - 3) = x^3 - 6x^2 + 11x - 6
    const std::vector<double> ce = {-6.0, 11.0, -6.0};
    const std::vector<double> roots = Math2d::cubicSolver(ce);

    ASSERT_EQ(roots.size(), 3u);
    EXPECT_TRUE(containsRoot(roots, 1.0));
    EXPECT_TRUE(containsRoot(roots, 2.0));
    EXPECT_TRUE(containsRoot(roots, 3.0));

    for (double r : roots)
    {
        EXPECT_NEAR(evalMonic(ce, r), 0.0, 1e-6);
    }
}

// 已知缺陷：单实根分支（判别式 > 0）在 q > 0 时取错了辅助二次方程的根，
// 导致对负数取立方根，结果是 NaN/inf 而不是实根。
//
//   Math2d.cpp:400  u = (q <= 0) ? pow(r[0], 1./3) : -pow(-r[1], 1./3);
//
// quadraticSolver 先压入绝对值较大的那个根：对 x^3 + x + 1 = 0（p=1, q=1）
// 辅助方程为 t^2 + t - 1/27 = 0，r[0] ≈ -1.036（负）、r[1] ≈ +0.036（正）。
// q > 0 的分支取 -pow(-r[1], 1/3)，底数为负，std::pow 返回 NaN。
// 取 r[0] 才能得到正底数。
//
// 同一段还有第二个问题：r.size() == 0 时只往 cerr 打一行，随后仍然索引
// r[0]/r[1]，是一次越界读。
//
// 该函数经 quarticSolver -> simultaneousQuadraticSolver* 被
// Information.cpp:621（椭圆求交）与 ActionDrawLineTangent2.cpp:392（切线）
// 使用，是产品可达路径。
//
// 修复属于行为变更，不在阶段 0 范围内（阶段 0 只建地基、不改产品语义）。
// 这条测试先以 DISABLED_ 形式把缺陷钉在代码里；修复后去掉前缀即可。
TEST(Math2dSolver, DISABLED_三次方程单实根情形)
{
    // x^3 + x + 1 = 0 只有一个实根，约为 -0.6823278
    const std::vector<double> ce = {0.0, 1.0, 1.0};
    const std::vector<double> roots = Math2d::cubicSolver(ce);

    ASSERT_EQ(roots.size(), 1u);
    EXPECT_NEAR(roots.front(), -0.6823278038280193, 1e-9);
    EXPECT_NEAR(evalMonic(ce, roots.front()), 0.0, 1e-9);
}

TEST(Math2dSolver, 三次方程三实根情形数值正确)
{
    // 判别式 < 0 的分支（三个实根）工作正常，与上面的 DISABLED_ 用例对照，
    // 可以确认缺陷只在单实根分支。
    // (x+4)(x-1)(x-3) = x^3 - 13x + 12
    const std::vector<double> ce = {0.0, -13.0, 12.0};
    const std::vector<double> roots = Math2d::cubicSolver(ce);

    ASSERT_EQ(roots.size(), 3u);
    for (double r : roots)
    {
        EXPECT_TRUE(std::isfinite(r)) << "求出了非有限根 " << r;
        EXPECT_NEAR(evalMonic(ce, r), 0.0, 1e-6);
    }
    EXPECT_TRUE(containsRoot(roots, -4.0));
    EXPECT_TRUE(containsRoot(roots, 1.0));
    EXPECT_TRUE(containsRoot(roots, 3.0));
}

TEST(Math2dSolver, 三次方程退化为纯立方时正确)
{
    // |p| 很小的快捷分支：x^3 - 8 = 0
    const std::vector<double> ce = {0.0, 0.0, -8.0};
    const std::vector<double> roots = Math2d::cubicSolver(ce);

    ASSERT_EQ(roots.size(), 1u);
    EXPECT_NEAR(roots.front(), 2.0, 1e-9);
}

// ---------------------------------------------------------------------------
// 一元四次：x^4 + ce[0] x^3 + ce[1] x^2 + ce[2] x + ce[3] = 0
// ---------------------------------------------------------------------------

TEST(Math2dSolver, 四次方程求出四个实根)
{
    // (x+2)(x+1)(x-1)(x-2) = x^4 - 5x^2 + 4
    const std::vector<double> ce = {0.0, -5.0, 0.0, 4.0};
    const std::vector<double> roots = Math2d::quarticSolver(ce);

    ASSERT_EQ(roots.size(), 4u);
    for (double expected : {-2.0, -1.0, 1.0, 2.0})
    {
        EXPECT_TRUE(containsRoot(roots, expected)) << "缺少根 " << expected;
    }
    for (double r : roots)
    {
        EXPECT_NEAR(evalMonic(ce, r), 0.0, 1e-6);
    }
}

TEST(Math2dSolver, 四次方程退化为双二次时仍求出实根)
{
    // (x^2 - 4)(x^2 + 1) = x^4 - 3x^2 - 4，实根只有 ±2
    const std::vector<double> ce = {0.0, -3.0, 0.0, -4.0};
    const std::vector<double> roots = Math2d::quarticSolver(ce);

    ASSERT_EQ(roots.size(), 2u);
    EXPECT_TRUE(containsRoot(roots, 2.0));
    EXPECT_TRUE(containsRoot(roots, -2.0));
}

TEST(QuarticEquationTest, 费拉里法求出全部实根)
{
    // 2(x-1)(x-2)(x-3)(x-4) 展开：2x^4 - 20x^3 + 70x^2 - 100x + 48
    QuarticEquation eq(2.0, -20.0, 70.0, -100.0, 48.0);
    const std::vector<double> roots = eq.getResults();

    ASSERT_EQ(roots.size(), 4u);
    for (double expected : {1.0, 2.0, 3.0, 4.0})
    {
        EXPECT_TRUE(containsRoot(roots, expected, 1e-5))
            << "缺少根 " << expected;
    }
}

// ---------------------------------------------------------------------------
// 线性方程组
// ---------------------------------------------------------------------------

TEST(Math2dSolver, 线性方程组求出唯一解)
{
    //  2x + 1y = 5
    //  1x - 1y = 1
    //  解: x = 2, y = 1
    const std::vector<std::vector<double>> m = {
        {2.0, 1.0, 5.0},
        {1.0, -1.0, 1.0},
    };
    std::vector<double> sn;

    ASSERT_TRUE(Math2d::linearSolver(m, sn));
    ASSERT_EQ(sn.size(), 2u);
    EXPECT_NEAR(sn[0], 2.0, 1e-9);
    EXPECT_NEAR(sn[1], 1.0, 1e-9);
}

TEST(Math2dSolver, 线性方程组奇异时报告无唯一解)
{
    //  1x + 1y = 2
    //  2x + 2y = 4   （与第一行线性相关）
    const std::vector<std::vector<double>> m = {
        {1.0, 1.0, 2.0},
        {2.0, 2.0, 4.0},
    };
    std::vector<double> sn;

    EXPECT_FALSE(Math2d::linearSolver(m, sn));
}
