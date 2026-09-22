/// @file test_persistence_roundtrip.cpp
/// @brief 实体序列化往返测试
///
/// 方案 3.2 把往返测试列为投入产出比最高的一类：写盘再读回、逐字段比对，
/// 一次性锁住整个数据模型的序列化语义。任何一次给实体加字段、调整写入顺序、
/// 或改动 revision 协商逻辑而忘了对称修改的改动，都会在这里立刻暴露。
///
/// 走的是 OutputStream / InputStream 这一对二进制流接口，也就是实体的
/// saveStream / restoreStream 真正使用的那一层，不落盘，因而测试可并行、
/// 无需清理临时文件。
///
/// 没有走 Persistence::dumpToStream / restoreFromStream：那一对接口全仓
/// 没有任何调用点，且 restoreFromStream 违反了 Archive.h 为 ArchiveReader
/// 写明的契约——「nextEntry() 必须在读取第一个条目之前调用一次」，而它
/// 拿到 MinizipNgArchiveReader 后直接取 stream()，读到的是空流。详见本文件
/// 末尾的 DISABLED_ 用例。
///
/// 阶段 5 迁移 Qt 6 时，DXF 代码页相关的编码回归也要靠这一类测试兜底。

#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <sstream>
#include <string>

#include "ArcData.h"
#include "CircleData.h"
#include "DmArc.h"
#include "DmCircle.h"
#include "DmEllipse.h"
#include "DmLine.h"
#include "DmVector.h"
#include "EllipseData.h"
#include "LineData.h"
#include "Persistence.h"
#include "Stream.h"

namespace
{
constexpr double kPi = 3.14159265358979323846;

/// @brief 序列化再反序列化：把 source 写进内存流，读回到 target
/// @param [in] source 待写出的对象
/// @param [out] target 读回的目标对象（须已构造且类型一致）
void roundTrip(const Persistence& source, Persistence& target)
{
    std::stringstream buffer(std::ios::in | std::ios::out | std::ios::binary);

    {
        OutputStream out(buffer);
        source.saveStream(out);
    }

    ASSERT_GT(buffer.tellp(), std::streampos(0)) << "序列化没有产生任何字节";

    buffer.seekg(0, std::ios::beg);
    InputStream in(buffer);
    target.restoreStream(in);
}

void expectVectorEq(const DmVector& got, const DmVector& expected, double tol = 1e-9)
{
    EXPECT_NEAR(got.x, expected.x, tol);
    EXPECT_NEAR(got.y, expected.y, tol);
    EXPECT_NEAR(got.z, expected.z, tol);
}
}  // namespace

// ---------------------------------------------------------------------------
// 直线
// ---------------------------------------------------------------------------

TEST(PersistenceRoundTrip, 直线往返保持端点)
{
    DmLine original(DmVector(1.5, -2.25), DmVector(30.75, 41.125));
    original.calculateBorders();

    DmLine restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));

    expectVectorEq(restored.getStartpoint(), original.getStartpoint());
    expectVectorEq(restored.getEndpoint(), original.getEndpoint());
}

TEST(PersistenceRoundTrip, 直线往返保持包围盒)
{
    DmLine original(DmVector(-10.0, 7.0), DmVector(3.0, -4.0));
    original.calculateBorders();

    DmLine restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));

    expectVectorEq(restored.getMin(), original.getMin(), 1e-6);
    expectVectorEq(restored.getMax(), original.getMax(), 1e-6);
}

// ---------------------------------------------------------------------------
// 圆
// ---------------------------------------------------------------------------

TEST(PersistenceRoundTrip, 圆往返保持圆心与半径)
{
    CircleData data(DmVector(12.0, -34.5), 6.125);
    DmCircle original(nullptr, data);
    original.calculateBorders();

    DmCircle restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));

    expectVectorEq(restored.getCenter(), original.getCenter());
    EXPECT_NEAR(restored.getRadius(), original.getRadius(), 1e-9);
}

// ---------------------------------------------------------------------------
// 圆弧
// ---------------------------------------------------------------------------

TEST(PersistenceRoundTrip, 圆弧往返保持全部几何字段)
{
    ArcData data(DmVector(3.0, 4.0), DmVector(0.0, 0.0, 1.0), 7.5, 0.25, 2.75);
    DmArc original(nullptr, data);
    original.calculateBorders();

    DmArc restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));

    expectVectorEq(restored.getCenter(), original.getCenter());
    EXPECT_NEAR(restored.getRadius(), original.getRadius(), 1e-9);
    EXPECT_NEAR(restored.getStartAngle(), original.getStartAngle(), 1e-9);
    EXPECT_NEAR(restored.getEndAngle(), original.getEndAngle(), 1e-9);
    expectVectorEq(restored.getNormal(), original.getNormal());
    EXPECT_EQ(restored.isClockwise(), original.isClockwise());
}

TEST(PersistenceRoundTrip, 顺时针圆弧往返保持方向)
{
    ArcData data(DmVector(0.0, 0.0), DmVector(0.0, 0.0, 1.0), 5.0, 0.0, kPi / 2.0);
    DmArc original(nullptr, data);
    original.setClockwise(true);
    original.calculateBorders();
    ASSERT_TRUE(original.isClockwise());

    DmArc restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));

    EXPECT_EQ(restored.isClockwise(), true);
    // 方向由法向量承载，读回后几何上必须是同一段弧
    expectVectorEq(restored.getStartpoint(), original.getStartpoint(), 1e-6);
    expectVectorEq(restored.getEndpoint(), original.getEndpoint(), 1e-6);
    expectVectorEq(restored.getMiddlePoint(), original.getMiddlePoint(), 1e-6);
}

TEST(PersistenceRoundTrip, 圆弧往返保持中点即保持了弧段而非补弧)
{
    // 只比对起止点不足以发现「弧和它的补弧」被写反：两者起止点相同。
    ArcData data(DmVector(0.0, 0.0), DmVector(0.0, 0.0, 1.0), 10.0, 0.0, kPi / 2.0);
    DmArc original(nullptr, data);
    original.calculateBorders();

    DmArc restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));

    expectVectorEq(restored.getMiddlePoint(), original.getMiddlePoint(), 1e-6);
    EXPECT_NEAR(restored.getAngleLength(), original.getAngleLength(), 1e-9);
}

// ---------------------------------------------------------------------------
// 椭圆
// ---------------------------------------------------------------------------

TEST(PersistenceRoundTrip, 椭圆往返保持长轴与比率)
{
    EllipseData data(DmVector(1.0, 2.0),    // 圆心
                     DmVector(8.0, 0.0),    // 长轴向量
                     DmVector(0.0, 0.0, 1.0),
                     0.5,                   // 短长轴比
                     true,                  // 闭合
                     0.0, 2.0 * kPi);
    DmEllipse original(nullptr, data);
    original.calculateBorders();

    DmEllipse restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));

    expectVectorEq(restored.getCenter(), original.getCenter());
    EXPECT_NEAR(restored.getRatio(), original.getRatio(), 1e-9);
    expectVectorEq(restored.getMajorP(), original.getMajorP(), 1e-9);
}

// ---------------------------------------------------------------------------
// 数值与压缩级别的边界
// ---------------------------------------------------------------------------

TEST(PersistenceRoundTrip, 同一对象多次写出字节完全相同)
{
    // 序列化必须是确定性的，否则「改动前后文件是否相同」这种最直接的
    // 回归判据就失效了。
    ArcData data(DmVector(3.0, 4.0), DmVector(0.0, 0.0, 1.0), 7.5, 0.25, 2.75);
    DmArc arc(nullptr, data);
    arc.calculateBorders();

    std::string first;
    std::string second;
    for (std::string* slot : {&first, &second})
    {
        std::stringstream buffer(std::ios::in | std::ios::out | std::ios::binary);
        OutputStream out(buffer);
        arc.saveStream(out);
        *slot = buffer.str();
    }

    EXPECT_FALSE(first.empty());
    EXPECT_EQ(first, second);
}

TEST(PersistenceRoundTrip, 极端坐标不丢精度)
{
    // 大图纸上的坐标可以很大，同时半径可以很小；double 的有效位必须全部保住
    const double bigX = 1234567.891011;
    const double smallR = 1e-4;

    CircleData data(DmVector(bigX, -bigX), smallR);
    DmCircle original(nullptr, data);
    original.calculateBorders();

    DmCircle restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));

    // 二进制写出的是 double，往返应当是逐位相同而不只是「接近」
    EXPECT_DOUBLE_EQ(restored.getCenter().x, bigX);
    EXPECT_DOUBLE_EQ(restored.getCenter().y, -bigX);
    EXPECT_DOUBLE_EQ(restored.getRadius(), smallR);
}

TEST(PersistenceRoundTrip, 零半径与零长度不崩溃)
{
    CircleData data(DmVector(0.0, 0.0), 0.0);
    DmCircle original(nullptr, data);
    original.calculateBorders();

    DmCircle restored;
    EXPECT_NO_FATAL_FAILURE(roundTrip(original, restored));
    EXPECT_NEAR(restored.getRadius(), 0.0, 1e-12);
}

TEST(PersistenceRoundTrip, 连续两次往返结果稳定)
{
    // 第二次往返的输出必须与第一次完全一致，否则说明序列化不是幂等的
    ArcData data(DmVector(2.5, -6.25), DmVector(0.0, 0.0, 1.0), 3.75, 0.5, 4.0);
    DmArc original(nullptr, data);
    original.calculateBorders();

    DmArc first;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, first));

    DmArc second;
    ASSERT_NO_FATAL_FAILURE(roundTrip(first, second));

    expectVectorEq(second.getCenter(), first.getCenter(), 0.0);
    EXPECT_DOUBLE_EQ(second.getRadius(), first.getRadius());
    EXPECT_DOUBLE_EQ(second.getStartAngle(), first.getStartAngle());
    EXPECT_DOUBLE_EQ(second.getEndAngle(), first.getEndAngle());
}

// ---------------------------------------------------------------------------
// 已知缺陷：Persistence 的压缩流往返接口不可用
// ---------------------------------------------------------------------------

// Persistence::dumpToStream（Persistence.cpp）把内容写成一个含
// "Persistence.xml" 条目的 zip；restoreFromStream 反过来读：
//
//     MinizipNgArchiveReader archive(stream);
//     XMLReader reader("", archive.stream());
//
// 但 Archive.h 为 ArchiveReader 写明的契约是
// 「nextEntry() 必须在读取第一个条目之前调用一次」，
// 且 stream() 「仅在 nextEntry() 与下一次 nextEntry()/closeEntry() 之间有效」。
// restoreFromStream 跳过了 nextEntry()，于是拿到的是空流，
// XMLReader 报 "No document element found at offset 0" 并抛异常。
//
// 这对接口全仓没有任何调用点，所以缺陷一直没有暴露。产品的文档读写走的是
// FilterOcdIO（kernel/filters/），与这条链路无关。
//
// 修复属于行为变更，不在阶段 0 范围内。两种收尾方式二选一：
// 补上 nextEntry() 让接口可用，或者连同这对死接口一起删掉。
TEST(PersistenceRoundTrip, DISABLED_压缩流往返)
{
    CircleData data(DmVector(1.0, 2.0), 3.0);
    DmCircle original(nullptr, data);
    original.calculateBorders();

    std::stringstream buffer(std::ios::in | std::ios::out | std::ios::binary);
    original.dumpToStream(buffer, 1);
    buffer.seekg(0, std::ios::beg);

    DmCircle restored;
    EXPECT_NO_THROW(restored.restoreFromStream(buffer));
    expectVectorEq(restored.getCenter(), original.getCenter());
    EXPECT_NEAR(restored.getRadius(), original.getRadius(), 1e-9);
}
