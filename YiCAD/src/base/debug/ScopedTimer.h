/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file ScopedTimer.h
/// @brief 可开关的耗时埋点
///
/// 替代此前散落在热路径上的 std::cout 计时（方案 P11）。三点差别：
///
/// 1. 默认关闭。未开启时 YICAD_SCOPED_TIMER 只做一次原子读，不取时间戳、
///    不格式化、不输出，因此可以留在 paintGL 这种每帧路径上。
/// 2. 用 steady_clock 而非 system_clock。system_clock 会被系统对时和夏令时
///    调整，拿它测时长可能得到负数。
/// 3. 不逐次打印，而是按计数器累计次数、总耗时、最小、最大，需要时一次性
///    汇总。逐帧打印本身就会拖慢被测对象。
///
/// 开启方式（任选其一）：
///   - 环境变量 YICAD_PROFILE=1
///   - 代码里调用 yicad::Profiler::setEnabled(true)
///
/// 汇总结果由 yicad::Profiler::report() 输出到 render 日志分类。
///
/// 用法：
/// @code
///     void GuiDocumentView::paintGL()
///     {
///         YICAD_SCOPED_TIMER(yicad::counters::paintGL());
///         ...
///     }
/// @endcode

#ifndef SCOPEDTIMER_H
#define SCOPEDTIMER_H

#include <atomic>
#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace yicad
{

/// @brief 一个具名耗时计数器。进程内唯一，通过 counters 命名空间获取。
class TimerCounter
{
public:
    explicit TimerCounter(const char* name);

    const char* name() const { return m_name; }

    /// @brief 累加一次采样
    /// @param [in] nanoseconds 本次耗时
    void addSample(long long nanoseconds);

    /// @brief 清空累计值
    void reset();

    long long count() const { return m_count.load(std::memory_order_relaxed); }
    long long totalNs() const { return m_totalNs.load(std::memory_order_relaxed); }
    /// @brief 最小耗时。无采样时为 LLONG_MAX，读之前先判 count() > 0。
    long long minNs() const { return m_minNs.load(std::memory_order_relaxed); }
    long long maxNs() const { return m_maxNs.load(std::memory_order_relaxed); }

    /// @brief 平均耗时（毫秒）。无采样时返回 0。
    double averageMs() const;

private:
    const char* m_name;
    std::atomic<long long> m_count{0};
    std::atomic<long long> m_totalNs{0};
    std::atomic<long long> m_minNs{0};
    std::atomic<long long> m_maxNs{0};
};

/// @brief 埋点总开关与汇总输出
class Profiler
{
public:
    /// @brief 埋点是否开启。热路径上每次调用只是一次 relaxed 原子读。
    static bool isEnabled()
    {
        return s_enabled.load(std::memory_order_relaxed);
    }

    static void setEnabled(bool enabled);

    /// @brief 按环境变量 YICAD_PROFILE 初始化开关。
    ///        由 DmSystem::init 调用一次；重复调用是安全的。
    static void configureFromEnvironment();

    /// @brief 把全部计数器汇总输出到 render 日志分类（Info 级别）
    static void report();

    /// @brief 清空全部计数器
    static void resetAll();

    /// @brief 全部已注册计数器，供测试与基线采集使用
    static std::vector<TimerCounter*> counters();

private:
    friend class TimerCounter;
    static void registerCounter(TimerCounter* counter);

    static std::atomic<bool> s_enabled;
};

/// @brief 作用域计时器。析构时把耗时累加到计数器。
class ScopedTimer
{
public:
    explicit ScopedTimer(TimerCounter& counter)
        : m_counter(counter)
        , m_start(std::chrono::steady_clock::now())
    {
    }

    ~ScopedTimer()
    {
        const auto elapsed = std::chrono::steady_clock::now() - m_start;
        m_counter.addSample(
            std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count());
    }

    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

private:
    TimerCounter& m_counter;
    std::chrono::steady_clock::time_point m_start;
};

/// @brief 方案阶段 0 要求的三个基线埋点位置（见 3.2 节第 3 条）
namespace counters
{
/// @brief GuiDocumentView::paintGL 的帧耗时
TimerCounter& paintGL();
/// @brief Snapper::catchEntity 的拾取耗时
TimerCounter& catchEntity();
/// @brief Selection::selectWindow 的框选耗时
TimerCounter& selectWindow();
/// @brief EntityTable::getNearestVirtualIntersection 的虚拟交点捕捉耗时
TimerCounter& nearestVirtualIntersection();
/// @brief 文档打开耗时
TimerCounter& openDocument();
}  // namespace counters

}  // namespace yicad

/// @brief 在当前作用域埋一个计时点；埋点关闭时不取时间戳。
///
/// 展开为两条语句（一个变量声明加一个 if），因此不能放在没有大括号的
/// 分支里——`if (x) YICAD_SCOPED_TIMER(c);` 是错的。计时器需要活到作用域
/// 结束，没法包进 do-while(0)。正常用法是放在函数体开头。
///
/// 变量名用 __LINE__ 拼接，所以同一行只能写一个。
#define YICAD_SCOPED_TIMER_CAT_INNER(a, b) a##b
#define YICAD_SCOPED_TIMER_CAT(a, b) YICAD_SCOPED_TIMER_CAT_INNER(a, b)
#define YICAD_SCOPED_TIMER(counter)                                              \
    std::optional<::yicad::ScopedTimer> YICAD_SCOPED_TIMER_CAT(_yicadTimer_, __LINE__); \
    if (::yicad::Profiler::isEnabled())                                          \
    {                                                                            \
        YICAD_SCOPED_TIMER_CAT(_yicadTimer_, __LINE__).emplace(counter);         \
    }

#endif  // SCOPEDTIMER_H
