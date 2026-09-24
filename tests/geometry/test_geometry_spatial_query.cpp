/// @file test_geometry_spatial_query.cpp
/// @brief 走空间搜索树的两处查询的单元测试（P10）
///
/// Selection::selectWindow 与 EntityTable::getNearestVirtualIntersection
/// 原先遍历全部实体，改为在 SpacialSearchTree 上取候选（框选）与做最近邻
/// 查询（虚拟交点）。除针对性用例外，另有随机用例把结果与原全量扫描的算法
/// 逐一比对，锁定"只换查询方式、不改语义"；唯一的语义差异（圆弧圆心落在
/// 包围盒外）单独用一个用例写明。
///
/// 虚拟交点的两条既有语义直接决定了下面用例的构造方式，先写在这里：
/// - "最近"按 DmEntity::getDistanceToPoint 计，它取到曲线距离与到
///   getCenter() 距离中的较小者，圆、圆弧、椭圆的圆心也算近点；
/// - 与构造线求交用 onEntities=true，Information::getIntersection 会先要求
///   两者包围盒相交，而临时构造线的包围盒只是查询点到"查询点+单位方向"这一
///   小段。因此只有包围盒覆盖查询点附近的实体才可能求出交点，其余情况返回
///   查询点本身。
///
/// 实体用 EntityTable::add_direct 直接放进表：默认构造的 DmDocument 没有
/// 完整的应用上下文，走 add() 的 Cmd 路径会崩溃。

#include <gtest/gtest.h>

#include <cmath>
#include <random>
#include <set>
#include <vector>

#include "ArcData.h"
#include "CircleData.h"
#include "DmArc.h"
#include "DmCircle.h"
#include "DmConstructionLine.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "DmLine.h"
#include "DmVector.h"
#include "EntityTable.h"
#include "Information.h"
#include "Selection.h"
#include "SpacialSearchTree.h"

namespace
{
constexpr double kPi = 3.14159265358979323846;

DmLine* addLine(DmDocument& doc, const DmVector& a, const DmVector& b)
{
    auto* line = new DmLine(a, b);
    line->calculateBorders();
    EXPECT_TRUE(doc.getEntityTable()->add_direct(line));
    return line;
}

DmCircle* addCircle(DmDocument& doc, const DmVector& center, double radius)
{
    auto* circle = new DmCircle(nullptr, CircleData(center, radius));
    circle->calculateBorders();
    EXPECT_TRUE(doc.getEntityTable()->add_direct(circle));
    return circle;
}

DmArc* addArc(DmDocument& doc, const DmVector& center, double radius, double startAngle, double endAngle)
{
    auto* arc = new DmArc(nullptr, ArcData(center, DmVector(0.0, 0.0, 1.0), radius, startAngle, endAngle));
    arc->calculateBorders();
    EXPECT_TRUE(doc.getEntityTable()->add_direct(arc));
    return arc;
}

std::set<DmEntity*> selectedEntities(DmDocument& doc)
{
    std::set<DmEntity*> result;
    for (auto e : *doc.getEntityTable())
    {
        if (e->isSelected())
        {
            result.insert(e);
        }
    }
    return result;
}

/// @brief 原全量扫描下交叉框选对无子实体、非三角形/Solid 实体的判定
bool crossWindowHit(DmEntity* e, const DmVector& v1, const DmVector& v2)
{
    if (e->isInWindow(v1, v2))
    {
        return true;
    }
    DmEntityContainer edges;
    edges.addRectangle(v1, v2);
    for (auto edge : edges)
    {
        if (Information::getIntersection(e, edge, true).hasValid())
        {
            return true;
        }
    }
    return false;
}

/// @brief 原实现：对渲染容器全量求最近实体，再与构造线求交（调用前须 updateContainer）
DmVector bruteForceVirtualIntersection(EntityTable* table, const DmVector& coord, double angle)
{
    DmEntity* nearest = table->getEntityContainer()->getNearestEntity(coord, nullptr, DM::ResolveAllButTextImage);
    if (!nearest)
    {
        return coord;
    }
    DmVector direction;
    direction.set(angle);
    DmConstructionLine line(nullptr, DmConstructionLineData(coord, coord + direction));
    DmVectorSolutions sol = Information::getIntersection(nearest, &line, true);
    if (sol.getVector().empty())
    {
        return coord;
    }
    return sol.getClosest(coord, nullptr, nullptr);
}

void expectPoint(const DmVector& actual, double x, double y)
{
    EXPECT_NEAR(actual.x, x, 1e-9);
    EXPECT_NEAR(actual.y, y, 1e-9);
}
}  // namespace

TEST(SpatialQueryTest, 窗选只选完全落在窗口内的实体)
{
    DmDocument doc;
    DmLine* inside = addLine(doc, DmVector(1.0, 1.0), DmVector(2.0, 2.0));
    addLine(doc, DmVector(1.0, 1.0), DmVector(10.0, 1.0));      // 跨出窗口
    addLine(doc, DmVector(100.0, 100.0), DmVector(101.0, 101.0));  // 远离窗口

    Selection(&doc, nullptr).selectWindow(DmVector(0.0, 0.0), DmVector(5.0, 5.0), true, false);

    EXPECT_EQ(selectedEntities(doc), std::set<DmEntity*>{inside});
}

TEST(SpatialQueryTest, 交叉选按几何相交而非包围盒重叠判定)
{
    DmDocument doc;
    DmLine* inside = addLine(doc, DmVector(1.0, 1.0), DmVector(2.0, 2.0));
    DmLine* crossing = addLine(doc, DmVector(1.0, 1.0), DmVector(10.0, 1.0));
    // 包围盒 (4,4)-(10,10) 与窗口的右上角重叠，但线段 x+y=14 不经过窗口。
    addLine(doc, DmVector(4.0, 10.0), DmVector(10.0, 4.0));
    addLine(doc, DmVector(100.0, 100.0), DmVector(101.0, 101.0));

    Selection(&doc, nullptr).selectWindow(DmVector(5.0, 5.0), DmVector(0.0, 0.0), true, true);

    EXPECT_EQ(selectedEntities(doc), (std::set<DmEntity*>{inside, crossing}));
}

TEST(SpatialQueryTest, 框选跳过不可见实体且可反选)
{
    DmDocument doc;
    DmLine* visible = addLine(doc, DmVector(1.0, 1.0), DmVector(2.0, 2.0));
    DmLine* hidden = addLine(doc, DmVector(2.0, 2.0), DmVector(3.0, 3.0));
    hidden->setVisible(false);

    Selection selection(&doc, nullptr);
    selection.selectWindow(DmVector(0.0, 0.0), DmVector(5.0, 5.0), true, false);
    EXPECT_EQ(selectedEntities(doc), std::set<DmEntity*>{visible});

    selection.selectWindow(DmVector(0.0, 0.0), DmVector(5.0, 5.0), false, false);
    EXPECT_TRUE(selectedEntities(doc).empty());
}

TEST(SpatialQueryTest, 盖住全部实体的框选与局部框选结果一致)
{
    // 窗口盖住全部实体的包围框时 selectWindow 改为顺序遍历实体表，
    // 两条路径对每个实体的判断相同。
    DmDocument doc;
    EntityTable* table = doc.getEntityTable();
    DmVector allMin, allMax;
    EXPECT_FALSE(table->getSearchBounds(allMin, allMax));

    DmLine* a = addLine(doc, DmVector(1.0, 1.0), DmVector(2.0, 2.0));
    DmLine* b = addLine(doc, DmVector(-5.0, 3.0), DmVector(8.0, 3.0));
    DmCircle* c = addCircle(doc, DmVector(20.0, -4.0), 2.0);
    DmLine* hidden = addLine(doc, DmVector(0.0, 0.0), DmVector(1.0, 0.0));
    hidden->setVisible(false);

    ASSERT_TRUE(table->getSearchBounds(allMin, allMax));
    expectPoint(allMin, -5.0, -6.0);
    expectPoint(allMax, 22.0, 3.0);

    Selection selection(&doc, nullptr);
    // 恰好等于全部包围框的窗口（盖住全部）与只差一点的窗口（走树），窗选结果一致。
    selection.selectWindow(allMin, allMax, true, false);
    EXPECT_EQ(selectedEntities(doc), (std::set<DmEntity*>{a, b, c}));
    selection.selectAll(false);
    selection.selectWindow(allMin, allMax - DmVector(0.5, 0.0), true, false);
    EXPECT_EQ(selectedEntities(doc), (std::set<DmEntity*>{a, b}));

    // 交叉选：盖住全部时同样跳过不可见实体。
    selection.selectAll(false);
    selection.selectWindow(DmVector(100.0, 100.0), DmVector(-100.0, -100.0), true, true);
    EXPECT_EQ(selectedEntities(doc), (std::set<DmEntity*>{a, b, c}));
}

TEST(SpatialQueryTest, 重复插入同一实体只更新不重复)
{
    // EntityTable::searchEntities 依赖"树中每个实体只有一份"跳过去重。
    DmDocument doc;
    DmLine* line = addLine(doc, DmVector(0.0, 0.0), DmVector(1.0, 1.0));

    SpacialSearchTree tree;
    tree.insert(line);
    line->move(DmVector(10.0, 0.0));
    tree.insert(line);

    std::vector<DmEntity*> found;
    tree.search(DmVector(-100.0, -100.0), DmVector(100.0, 100.0), found);
    EXPECT_EQ(found, std::vector<DmEntity*>{line});

    // 第二次插入更新了包围框：旧位置查不到，新位置查得到。
    found.clear();
    tree.search(DmVector(0.0, 0.0), DmVector(1.0, 1.0), found);
    EXPECT_TRUE(found.empty());

    // 一次 remove 就移除干净，不留残余条目。
    tree.remove(line);
    found.clear();
    tree.search(DmVector(-100.0, -100.0), DmVector(100.0, 100.0), found);
    EXPECT_TRUE(found.empty());
}

TEST(SpatialQueryTest, 虚拟交点的最近实体不限搜索半径)
{
    DmDocument doc;
    EntityTable* table = doc.getEntityTable();

    // 空表：原样返回查询点。
    expectPoint(table->getNearestVirtualIntersection(DmVector(0.0, 0.0), kPi / 2.0, nullptr), 0.0, 0.0);

    // 唯一的实体离查询点约 141，照样被找到：竖直构造线 x=0 交 y=x+200 于 (0,200)。
    addLine(doc, DmVector(-300.0, -100.0), DmVector(100.0, 300.0));
    double dist = -1.0;
    expectPoint(table->getNearestVirtualIntersection(DmVector(0.0, 0.0), kPi / 2.0, &dist), 0.0, 200.0);
    EXPECT_NEAR(dist, 200.0, 1e-9);

    // 更近的 y=x+20（距离约 14）胜出。
    addLine(doc, DmVector(-50.0, -30.0), DmVector(50.0, 70.0));
    expectPoint(table->getNearestVirtualIntersection(DmVector(0.0, 0.0), kPi / 2.0, nullptr), 0.0, 20.0);
}

TEST(SpatialQueryTest, 虚拟交点按精确距离而非包围盒距离取最近)
{
    DmDocument doc;
    EntityTable* table = doc.getEntityTable();

    // 对角线 y=x 的包围盒包含查询点 (50,-40)（包围盒距离为 0），精确距离约 63.6；
    // 短线 (45,-50)-(55,-20) 精确距离约 1.6。水平构造线与两者的交点分别是
    // (-40,-40) 与 (48.33,-40)。
    addLine(doc, DmVector(-100.0, -100.0), DmVector(100.0, 100.0));
    addLine(doc, DmVector(45.0, -50.0), DmVector(55.0, -20.0));

    expectPoint(table->getNearestVirtualIntersection(DmVector(50.0, -40.0), 0.0, nullptr), 45.0 + 10.0 / 3.0, -40.0);
}

TEST(SpatialQueryTest, 虚拟交点跳过不可见实体)
{
    DmDocument doc;
    EntityTable* table = doc.getEntityTable();

    // 最近的 y=x+5 不可见，取次近的 y=x+20。
    DmLine* hidden = addLine(doc, DmVector(-10.0, -5.0), DmVector(10.0, 15.0));
    hidden->setVisible(false);
    addLine(doc, DmVector(-50.0, -30.0), DmVector(50.0, 70.0));

    expectPoint(table->getNearestVirtualIntersection(DmVector(0.0, 0.0), kPi / 2.0, nullptr), 0.0, 20.0);
}

TEST(SpatialQueryTest, 圆心在包围盒外的圆弧不再凭圆心成为最近实体)
{
    // 与原全量扫描唯一的语义差异：最近邻按空间搜索树里的包围盒剪枝，而
    // getDistanceToPoint 把圆心也算作近点；圆弧（含多段线、块参照里的圆弧段）
    // 的圆心可能落在包围盒之外，这时凭圆心算出的距离不再被考虑。点选
    // （Snapper::catchEntity）早已按包围盒取候选，对这类圆心同样不可达。
    DmDocument doc;
    EntityTable* table = doc.getEntityTable();

    // 圆心在原点、半径 100、80°~100° 的圆弧，包围盒在 y≈98.5~100。
    addArc(doc, DmVector(0.0, 0.0), 100.0, 80.0 * kPi / 180.0, 100.0 * kPi / 180.0);
    addLine(doc, DmVector(-50.0, -30.0), DmVector(50.0, 70.0));
    table->updateContainer();

    // 原实现：圆弧凭圆心（距离 0）成为最近实体，而它的包围盒与查询点处的
    // 构造线不相交，求不出交点，返回查询点本身。
    expectPoint(bruteForceVirtualIntersection(table, DmVector(0.0, 0.0), kPi / 2.0), 0.0, 0.0);
    // 现实现：取精确距离约 14 的 y=x+20，交于 (0,20)。
    expectPoint(table->getNearestVirtualIntersection(DmVector(0.0, 0.0), kPi / 2.0, nullptr), 0.0, 20.0);
}

TEST(SpatialQueryTest, 随机图纸上与原全量扫描结果一致)
{
    DmDocument doc;
    EntityTable* table = doc.getEntityTable();
    std::vector<DmEntity*> all;

    std::mt19937 rng(20260924);
    std::uniform_real_distribution<double> coord(-1000.0, 1000.0);
    std::uniform_real_distribution<double> extent(1.0, 80.0);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    std::uniform_real_distribution<double> jitter(-0.3, 0.3);
    std::uniform_real_distribution<double> angle(0.0, 2.0 * kPi);
    // 逐个取随机数：同一表达式里多次调用的求值顺序未指定。
    auto randomPoint = [&]()
    {
        double x = coord(rng);
        double y = coord(rng);
        return DmVector(x, y);
    };
    auto randomOffset = [&](double shiftY)
    {
        double dx = extent(rng);
        double dy = extent(rng) + shiftY;
        return DmVector(dx, dy);
    };

    // 只用直线与整圆：它们的圆心（若有）都在包围盒内，树上的剪枝是精确的。
    std::vector<DmLine*> lines;
    for (int i = 0; i < 400; ++i)
    {
        DmVector base = randomPoint();
        if (i % 4 == 0)
        {
            all.push_back(addCircle(doc, base, extent(rng)));
        }
        else
        {
            DmLine* line = addLine(doc, base, base + randomOffset(-40.0));
            lines.push_back(line);
            all.push_back(line);
        }
    }
    // 原实现读的是渲染容器，要先按实体表刷新一次。
    table->updateContainer();

    // 最近邻本身：在同样的实体上建一棵树，与渲染容器上的全量求最近逐一比对。
    SpacialSearchTree tree;
    for (auto e : all)
    {
        tree.insert(e);
    }
    for (int i = 0; i < 300; ++i)
    {
        DmVector q = randomPoint();
        DmEntity* expected = table->getEntityContainer()->getNearestEntity(q, nullptr, DM::ResolveAllButTextImage);
        const DmEntity* actual = tree.nearest(q, [&q](DmEntity* e)
        {
            DmEntity* sub = nullptr;
            return e->getDistanceToPoint(q, &sub, DM::ResolveAllButTextImage);
        });
        EXPECT_EQ(actual, expected) << "query " << i;
    }

    // 端到端：查询点取在随机直线附近，使构造线与实体包围盒相交、真能求出交点。
    int nonTrivial = 0;
    for (int i = 0; i < 200; ++i)
    {
        DmLine* line = lines[static_cast<size_t>(unit(rng) * (lines.size() - 1))];
        double t = unit(rng);
        double jx = jitter(rng);
        double jy = jitter(rng);
        DmVector q = line->getStartpoint() + (line->getEndpoint() - line->getStartpoint()) * t + DmVector(jx, jy);
        double a = angle(rng);
        DmVector expected = bruteForceVirtualIntersection(table, q, a);
        DmVector actual = table->getNearestVirtualIntersection(q, a, nullptr);
        EXPECT_NEAR(actual.x, expected.x, 1e-9) << "query " << i;
        EXPECT_NEAR(actual.y, expected.y, 1e-9) << "query " << i;
        if (expected.distanceTo(q) > 1e-9)
        {
            ++nonTrivial;
        }
    }
    // 防止比对退化成"两边都原样返回查询点"。
    EXPECT_GT(nonTrivial, 100);

    Selection selection(&doc, nullptr);
    for (int i = 0; i < 100; ++i)
    {
        DmVector v1 = randomPoint();
        DmVector v2 = v1 + randomOffset(0.0) * 5.0;
        if (i % 5 == 0)
        {
            // 盖住全部实体：selectWindow 走顺序遍历实体表的路径（窗选、交叉选各半）。
            v1 = DmVector(-2000.0, -2000.0);
            v2 = DmVector(2000.0, 2000.0);
        }
        bool cross = (i % 2 == 0);

        std::set<DmEntity*> expected;
        for (auto e : all)
        {
            if (cross ? crossWindowHit(e, v1, v2) : e->isInWindow(v1, v2))
            {
                expected.insert(e);
            }
        }

        selection.selectAll(false);
        selection.selectWindow(v1, v2, true, cross);
        EXPECT_EQ(selectedEntities(doc), expected) << "window " << i << (cross ? " cross" : " window");
    }
}
