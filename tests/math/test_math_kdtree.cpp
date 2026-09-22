/// @file test_math_kdtree.cpp
/// @brief KDTree 的单元测试
///
/// KDTree 用于点集的邻域查询。测试用「与暴力扫描比对」的方式验证，
/// 这样即使后续换掉内部的分割策略，期望值也不需要跟着改。

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "KDTree.h"

namespace
{
using Tree = KDTree<KDTreePoint>;

KDTreePoint makePoint(double x, double y, double z = 0.0)
{
    KDTreePoint p;
    p.x = x;
    p.y = y;
    p.z = z;
    return p;
}

/// @brief 暴力求出落在以 center 为心、半径 radius 的立方体内的点索引
std::vector<size_t> bruteForceCube(const std::vector<KDTreePoint>& pts,
                                   const KDTreePoint& center, double radius)
{
    std::vector<size_t> out;
    for (size_t i = 0; i < pts.size(); ++i)
    {
        const bool inside = std::fabs(pts[i].x - center.x) <= radius &&
                            std::fabs(pts[i].y - center.y) <= radius &&
                            std::fabs(pts[i].z - center.z) <= radius;
        if (inside)
        {
            out.push_back(i);
        }
    }
    return out;
}

std::vector<size_t> sorted(std::vector<size_t> v)
{
    std::sort(v.begin(), v.end());
    return v;
}
}  // namespace

TEST(KDTreeTest, 空点集不崩溃且查不到)
{
    Tree tree;
    tree.buildTree({});

    EXPECT_TRUE(tree.getPointsWithinCube(makePoint(0.0, 0.0), 10.0).empty());
}

TEST(KDTreeTest, 单点点集)
{
    const std::vector<KDTreePoint> pts = {makePoint(1.0, 2.0, 3.0)};
    Tree tree(pts);

    EXPECT_EQ(tree.getPointsWithinCube(makePoint(1.0, 2.0, 3.0), 0.1).size(), 1u);
    EXPECT_TRUE(tree.getPointsWithinCube(makePoint(100.0, 0.0), 1.0).empty());
}

TEST(KDTreeTest, 立方体查询结果与暴力扫描一致)
{
    // 确定性网格，覆盖多层分割
    std::vector<KDTreePoint> pts;
    for (int i = 0; i < 12; ++i)
    {
        for (int j = 0; j < 12; ++j)
        {
            pts.push_back(makePoint(i * 1.0, j * 1.0, 0.0));
        }
    }

    Tree tree(pts);

    struct Query
    {
        double x, y, radius;
    };
    const Query queries[] = {
        {5.0, 5.0, 2.0},
        {0.0, 0.0, 1.5},
        {11.0, 11.0, 3.0},
        {5.5, 5.5, 0.4},   // 落在网格空隙，应当查不到
        {5.0, 5.0, 100.0}, // 覆盖全体
    };

    for (const Query& q : queries)
    {
        const KDTreePoint center = makePoint(q.x, q.y);
        const std::vector<size_t> expected = bruteForceCube(pts, center, q.radius);
        const std::vector<size_t> got = tree.getPointsWithinCube(center, q.radius);

        EXPECT_EQ(sorted(got), sorted(expected))
            << "查询中心 (" << q.x << ", " << q.y << ") 半径 " << q.radius;
    }
}

TEST(KDTreeTest, 中序遍历访问到全部点)
{
    std::vector<KDTreePoint> pts;
    for (int i = 0; i < 25; ++i)
    {
        pts.push_back(makePoint(i * 1.0, (24 - i) * 1.0));
    }

    Tree tree(pts);

    size_t visited = 0;
    tree.inorderTraversal([&visited](const KDTreePoint&) { ++visited; });

    EXPECT_EQ(visited, pts.size());
}

TEST(KDTreeTest, 重复点不会丢失)
{
    const std::vector<KDTreePoint> pts = {
        makePoint(1.0, 1.0), makePoint(1.0, 1.0), makePoint(1.0, 1.0),
        makePoint(9.0, 9.0),
    };
    Tree tree(pts);

    EXPECT_EQ(tree.getPointsWithinCube(makePoint(1.0, 1.0), 0.01).size(), 3u);
}
