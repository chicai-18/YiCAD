/// @file test_debug_instrumentation.cpp
/// @brief 日志门面与耗时埋点的单元测试
///
/// 这两个设施本身要被产品的热路径调用（paintGL 每帧一次），因此它们的
/// 「关闭时零开销」和「计数正确」两条性质必须有测试保护——一旦退化成
/// 每帧都取时间戳或每帧都格式化字符串，就变回了它们要替代的那个问题。

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "ScopedTimer.h"
#include "YiCadLog.h"

namespace
{
/// @brief 测试专用计数器，避免污染产品计数器的采样
yicad::TimerCounter& testCounter()
{
    static yicad::TimerCounter counter("test.counter");
    return counter;
}

/// @brief 在作用域内临时开启埋点，退出时恢复原状态
class ProfilerGuard
{
public:
    explicit ProfilerGuard(bool enabled)
        : m_previous(yicad::Profiler::isEnabled())
    {
        yicad::Profiler::setEnabled(enabled);
    }

    ~ProfilerGuard() { yicad::Profiler::setEnabled(m_previous); }

private:
    bool m_previous;
};
}  // namespace

// ---------------------------------------------------------------------------
// 耗时埋点
// ---------------------------------------------------------------------------

TEST(ScopedTimerTest, 埋点关闭时不产生采样)
{
    testCounter().reset();
    ProfilerGuard guard(false);

    for (int i = 0; i < 100; ++i)
    {
        YICAD_SCOPED_TIMER(testCounter());
    }

    EXPECT_EQ(testCounter().count(), 0)
        << "埋点关闭时不应有任何采样，否则热路径仍在取时间戳";
}

TEST(ScopedTimerTest, 埋点开启时按次计数)
{
    testCounter().reset();
    ProfilerGuard guard(true);

    for (int i = 0; i < 10; ++i)
    {
        YICAD_SCOPED_TIMER(testCounter());
    }

    EXPECT_EQ(testCounter().count(), 10);
    EXPECT_GE(testCounter().totalNs(), 0);
}

TEST(ScopedTimerTest, 测得的耗时不小于实际睡眠时长)
{
    testCounter().reset();
    ProfilerGuard guard(true);

    {
        YICAD_SCOPED_TIMER(testCounter());
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    ASSERT_EQ(testCounter().count(), 1);
    // steady_clock 不会倒流，所以这里可以放心断言下界
    EXPECT_GE(testCounter().totalNs(), 4 * 1000 * 1000)
        << "测得 " << testCounter().averageMs() << " ms，短于睡眠的 5 ms";
}

TEST(ScopedTimerTest, 耗时永不为负)
{
    // 这正是此前用 system_clock 的隐患：系统对时会让时长变成负数。
    testCounter().reset();
    ProfilerGuard guard(true);

    for (int i = 0; i < 50; ++i)
    {
        YICAD_SCOPED_TIMER(testCounter());
    }

    EXPECT_GE(testCounter().minNs(), 0);
    EXPECT_GE(testCounter().maxNs(), 0);
    EXPECT_GE(testCounter().totalNs(), 0);
}

TEST(ScopedTimerTest, 最小值不大于最大值)
{
    testCounter().reset();
    ProfilerGuard guard(true);

    for (int i = 0; i < 20; ++i)
    {
        YICAD_SCOPED_TIMER(testCounter());
    }

    ASSERT_GT(testCounter().count(), 0);
    EXPECT_LE(testCounter().minNs(), testCounter().maxNs());
    EXPECT_GE(testCounter().averageMs(), 0.0);
}

TEST(ScopedTimerTest, reset清空累计值)
{
    ProfilerGuard guard(true);
    {
        YICAD_SCOPED_TIMER(testCounter());
    }
    ASSERT_GT(testCounter().count(), 0);

    testCounter().reset();

    EXPECT_EQ(testCounter().count(), 0);
    EXPECT_EQ(testCounter().totalNs(), 0);
    EXPECT_DOUBLE_EQ(testCounter().averageMs(), 0.0);
}

TEST(ScopedTimerTest, 同一作用域可以叠多个埋点)
{
    // 宏用 __LINE__ 拼变量名，同一行只能有一个；不同行不应互相冲突。
    testCounter().reset();
    ProfilerGuard guard(true);

    {
        YICAD_SCOPED_TIMER(testCounter());
        YICAD_SCOPED_TIMER(testCounter());
    }

    EXPECT_EQ(testCounter().count(), 2);
}

TEST(ScopedTimerTest, 方案要求的四个基线计数器都已注册)
{
    // 计数器是函数内静态对象，首次取用时构造并登记。
    // Profiler::configureFromEnvironment（由 DmSystem::init 调用）会把它们
    // 全部取一遍，这里再取一次以免测试依赖全局初始化顺序。
    yicad::counters::paintGL();
    yicad::counters::catchEntity();
    yicad::counters::selectWindow();
    yicad::counters::nearestVirtualIntersection();

    const std::vector<yicad::TimerCounter*> all = yicad::Profiler::counters();

    auto has = [&all](const char* name) {
        for (const yicad::TimerCounter* c : all)
        {
            if (std::string(c->name()) == name)
            {
                return true;
            }
        }
        return false;
    };

    EXPECT_TRUE(has("render.paintGL"));
    EXPECT_TRUE(has("snap.catchEntity"));
    EXPECT_TRUE(has("selection.selectWindow"));
    EXPECT_TRUE(has("snap.nearestVirtualIntersection"));
}

TEST(ScopedTimerTest, 渲染方案阶段0的计数器都已注册)
{
    // RENDER_PLAN.md 0.1 步：整图重建、首帧耗时两个耗时计数器，上传字节与绘制调用两个数量计数器
    yicad::counters::regen();
    yicad::counters::frameAfterHighlight();
    yicad::counters::frameAfterSelection();
    yicad::counters::uploadBytes();
    yicad::counters::drawCalls();

    std::vector<std::string> timers;
    for (const yicad::TimerCounter* c : yicad::Profiler::counters())
    {
        timers.emplace_back(c->name());
    }
    std::vector<std::string> values;
    for (const yicad::ValueCounter* c : yicad::Profiler::valueCounters())
    {
        values.emplace_back(c->name());
    }
    auto contains = [](const std::vector<std::string>& names, const char* name) {
        return std::find(names.begin(), names.end(), name) != names.end();
    };

    EXPECT_TRUE(contains(timers, "render.regen"));
    EXPECT_TRUE(contains(timers, "render.frameAfterHighlight"));
    EXPECT_TRUE(contains(timers, "render.frameAfterSelection"));
    EXPECT_TRUE(contains(values, "render.uploadBytes"));
    EXPECT_TRUE(contains(values, "render.drawCalls"));
}

TEST(ScopedTimerTest, 渲染方案阶段1的计数器都已注册)
{
    // RENDER_PLAN.md 第 10 节阶段 1：选择集、高亮集变化后的局部重建，场景底图的重画
    yicad::counters::regenSelection();
    yicad::counters::regenHighlight();
    yicad::counters::scene();

    std::vector<std::string> timers;
    for (const yicad::TimerCounter* c : yicad::Profiler::counters())
    {
        timers.emplace_back(c->name());
    }
    auto contains = [&timers](const char* name) {
        return std::find(timers.begin(), timers.end(), name) != timers.end();
    };

    EXPECT_TRUE(contains("render.regenSelection"));
    EXPECT_TRUE(contains("render.regenHighlight"));
    EXPECT_TRUE(contains("render.scene"));
}

// ---------------------------------------------------------------------------
// 数量计数器
// ---------------------------------------------------------------------------

namespace
{
/// @brief 测试专用数量计数器
yicad::ValueCounter& testValueCounter()
{
    static yicad::ValueCounter counter("test.value");
    return counter;
}
}  // namespace

TEST(ValueCounterTest, 累计次数合计最小最大与平均)
{
    testValueCounter().reset();

    testValueCounter().addSample(10);
    testValueCounter().addSample(0);
    testValueCounter().addSample(50);

    EXPECT_EQ(testValueCounter().count(), 3);
    EXPECT_EQ(testValueCounter().total(), 60);
    EXPECT_EQ(testValueCounter().min(), 0);
    EXPECT_EQ(testValueCounter().max(), 50);
    EXPECT_DOUBLE_EQ(testValueCounter().average(), 20.0);
}

TEST(ValueCounterTest, resetAll也清空数量计数器)
{
    testValueCounter().addSample(7);
    ASSERT_GT(testValueCounter().count(), 0);

    yicad::Profiler::resetAll();

    EXPECT_EQ(testValueCounter().count(), 0);
    EXPECT_EQ(testValueCounter().total(), 0);
    EXPECT_DOUBLE_EQ(testValueCounter().average(), 0.0);
}

TEST(ValueCounterTest, 汇总文本列出有采样的数量计数器)
{
    yicad::Profiler::resetAll();
    testValueCounter().addSample(1234);

    const std::string text = yicad::Profiler::summary();

    EXPECT_NE(text.find("test.value"), std::string::npos) << text;
    EXPECT_NE(text.find("1234"), std::string::npos) << text;
    // 没有采样的计数器不出现在汇总里
    EXPECT_EQ(text.find("render.drawCalls"), std::string::npos) << text;
}

// ---------------------------------------------------------------------------
// 日志门面
// ---------------------------------------------------------------------------

TEST(YiCadLogTest, 默认级别为Warning)
{
    // 默认必须严到不会在热路径上打印，否则就又回到了每帧输出。
    yicad::LogCategory category("test.default");

    EXPECT_EQ(category.level(), yicad::LogLevel::Warning);
    EXPECT_TRUE(category.isEnabled(yicad::LogLevel::Critical));
    EXPECT_TRUE(category.isEnabled(yicad::LogLevel::Error));
    EXPECT_TRUE(category.isEnabled(yicad::LogLevel::Warning));
    EXPECT_FALSE(category.isEnabled(yicad::LogLevel::Info));
    EXPECT_FALSE(category.isEnabled(yicad::LogLevel::Debug));
    EXPECT_FALSE(category.isEnabled(yicad::LogLevel::Trace));
}

TEST(YiCadLogTest, setLevel放宽过滤)
{
    yicad::LogCategory category("test.setlevel");

    category.setLevel(yicad::LogLevel::Debug);

    EXPECT_TRUE(category.isEnabled(yicad::LogLevel::Info));
    EXPECT_TRUE(category.isEnabled(yicad::LogLevel::Debug));
    EXPECT_FALSE(category.isEnabled(yicad::LogLevel::Trace));
}

TEST(YiCadLogTest, Off级别关闭全部输出)
{
    yicad::LogCategory category("test.off", yicad::LogLevel::Off);

    EXPECT_FALSE(category.isEnabled(yicad::LogLevel::Critical));
    EXPECT_FALSE(category.isEnabled(yicad::LogLevel::Warning));
    EXPECT_FALSE(category.isEnabled(yicad::LogLevel::Trace));
    // Off 本身也不是一个可输出的级别
    EXPECT_FALSE(category.isEnabled(yicad::LogLevel::Off));
}

TEST(YiCadLogTest, 过滤不通过时右侧不求值)
{
    // 这是宏最关键的性质：写日志的那一行在关闭时必须零成本，
    // 包括不调用参数里的函数。
    yicad::LogCategory category("test.lazy", yicad::LogLevel::Warning);
    int evaluations = 0;
    auto expensive = [&evaluations]() {
        ++evaluations;
        return 42;
    };

    YICAD_LOG(category, yicad::LogLevel::Debug) << expensive();

    EXPECT_EQ(evaluations, 0) << "被过滤掉的日志仍然求值了参数";

    YICAD_LOG(category, yicad::LogLevel::Error) << expensive();

    EXPECT_EQ(evaluations, 1) << "放行的日志应当求值一次";
}

TEST(YiCadLogTest, 内置分类都已注册且命名稳定)
{
    EXPECT_STREQ(yicad::log::render().name(), "render");
    EXPECT_STREQ(yicad::log::snap().name(), "snap");
    EXPECT_STREQ(yicad::log::selection().name(), "selection");
    EXPECT_STREQ(yicad::log::persistence().name(), "persistence");
    EXPECT_STREQ(yicad::log::plugin().name(), "plugin");
}
