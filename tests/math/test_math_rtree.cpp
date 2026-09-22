/// @file test_math_rtree.cpp
/// @brief RTree 空间索引的单元测试
///
/// R 树是拾取与捕捉的性能基础（见方案 1.3 节）。阶段 9.1 要把
/// Selection::selectWindow 与 EntityTable::getNearestVirtualIntersection
/// 也改成走 R 树候选集，改之前先把 R 树本身的语义锁住。

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "RTree.h"

namespace
{
/// 二维、以 int 为载荷、double 为坐标的 R 树，与 SpacialSearchTree 的用法一致。
using TestTree = RTree<int, double, 2, double>;

/// @brief 把 AABB 插入树
void insertBox(TestTree& tree, int id, double minX, double minY, double maxX, double maxY)
{
    const double mn[2] = {minX, minY};
    const double mx[2] = {maxX, maxY};
    tree.Insert(mn, mx, id);
}

/// @brief 查询与给定 AABB 相交的全部载荷，结果升序排列
std::vector<int> search(const TestTree& tree, double minX, double minY, double maxX, double maxY)
{
    const double mn[2] = {minX, minY};
    const double mx[2] = {maxX, maxY};
    std::vector<int> hits;
    tree.Search(mn, mx, [&hits](const int& id) {
        hits.push_back(id);
        return true;  // 继续遍历
    });
    std::sort(hits.begin(), hits.end());
    return hits;
}
}  // namespace

TEST(RTreeTest, 空树查不到任何东西)
{
    TestTree tree;
    EXPECT_EQ(tree.Count(), 0);
    EXPECT_TRUE(search(tree, -1e9, -1e9, 1e9, 1e9).empty());
}

TEST(RTreeTest, 插入后可按范围查到)
{
    TestTree tree;
    insertBox(tree, 1, 0.0, 0.0, 1.0, 1.0);
    insertBox(tree, 2, 5.0, 5.0, 6.0, 6.0);
    insertBox(tree, 3, 10.0, 10.0, 11.0, 11.0);

    EXPECT_EQ(tree.Count(), 3);

    EXPECT_EQ(search(tree, -1.0, -1.0, 2.0, 2.0), std::vector<int>({1}));
    EXPECT_EQ(search(tree, 4.0, 4.0, 7.0, 7.0), std::vector<int>({2}));
    EXPECT_EQ(search(tree, -100.0, -100.0, 100.0, 100.0), std::vector<int>({1, 2, 3}));
}

TEST(RTreeTest, 查询窗口不命中时返回空)
{
    TestTree tree;
    insertBox(tree, 1, 0.0, 0.0, 1.0, 1.0);

    EXPECT_TRUE(search(tree, 100.0, 100.0, 200.0, 200.0).empty());
}

TEST(RTreeTest, 部分重叠也算命中)
{
    TestTree tree;
    insertBox(tree, 1, 0.0, 0.0, 10.0, 10.0);

    // 查询窗口只与 AABB 的一角重叠
    EXPECT_EQ(search(tree, 9.0, 9.0, 20.0, 20.0), std::vector<int>({1}));
    // 完全包含在 AABB 内部的小窗口同样命中
    EXPECT_EQ(search(tree, 4.0, 4.0, 5.0, 5.0), std::vector<int>({1}));
}

TEST(RTreeTest, 回调返回false可提前终止遍历)
{
    TestTree tree;
    for (int i = 0; i < 50; ++i)
    {
        insertBox(tree, i, i * 1.0, 0.0, i * 1.0 + 0.5, 1.0);
    }

    int visited = 0;
    const double mn[2] = {-1.0, -1.0};
    const double mx[2] = {1000.0, 1000.0};
    tree.Search(mn, mx, [&visited](const int&) {
        ++visited;
        return false;  // 第一次命中就停
    });

    EXPECT_EQ(visited, 1);
}

TEST(RTreeTest, 删除后查不到)
{
    TestTree tree;
    insertBox(tree, 1, 0.0, 0.0, 1.0, 1.0);
    insertBox(tree, 2, 5.0, 5.0, 6.0, 6.0);
    ASSERT_EQ(tree.Count(), 2);

    const double mn[2] = {0.0, 0.0};
    const double mx[2] = {1.0, 1.0};
    tree.Remove(mn, mx, 1);

    EXPECT_EQ(tree.Count(), 1);
    EXPECT_EQ(search(tree, -100.0, -100.0, 100.0, 100.0), std::vector<int>({2}));
}

TEST(RTreeTest, RemoveAll清空整棵树)
{
    TestTree tree;
    for (int i = 0; i < 20; ++i)
    {
        insertBox(tree, i, i * 2.0, 0.0, i * 2.0 + 1.0, 1.0);
    }
    ASSERT_EQ(tree.Count(), 20);

    tree.RemoveAll();

    EXPECT_EQ(tree.Count(), 0);
    EXPECT_TRUE(search(tree, -1e9, -1e9, 1e9, 1e9).empty());
}

TEST(RTreeTest, 大量插入后查询结果与暴力扫描一致)
{
    // 分裂逻辑只有在数据量超过 MAXNODES 时才会反复触发，这里用一个
    // 确定性的网格把树撑到多层，再与暴力扫描比对。
    TestTree tree;
    struct Box
    {
        int id;
        double minX, minY, maxX, maxY;
    };
    std::vector<Box> boxes;

    int id = 0;
    for (int gx = 0; gx < 30; ++gx)
    {
        for (int gy = 0; gy < 30; ++gy)
        {
            Box b{id++, gx * 3.0, gy * 3.0, gx * 3.0 + 1.0, gy * 3.0 + 1.0};
            boxes.push_back(b);
            insertBox(tree, b.id, b.minX, b.minY, b.maxX, b.maxY);
        }
    }
    ASSERT_EQ(tree.Count(), 900);

    const double qMinX = 10.0, qMinY = 10.0, qMaxX = 40.0, qMaxY = 40.0;

    std::vector<int> expected;
    for (const Box& b : boxes)
    {
        const bool overlaps = b.minX <= qMaxX && b.maxX >= qMinX &&
                              b.minY <= qMaxY && b.maxY >= qMinY;
        if (overlaps)
        {
            expected.push_back(b.id);
        }
    }
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(search(tree, qMinX, qMinY, qMaxX, qMaxY), expected);
    EXPECT_FALSE(expected.empty()) << "测试数据有误：查询窗口应当命中若干项";
}
