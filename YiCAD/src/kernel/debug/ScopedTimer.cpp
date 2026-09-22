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

/// @file ScopedTimer.cpp
/// @brief 耗时埋点实现

#include "ScopedTimer.h"

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <limits>
#include <mutex>
#include <sstream>

#include "YiCadLog.h"

namespace yicad
{
namespace
{
std::vector<TimerCounter*>& registry()
{
    static std::vector<TimerCounter*> instance;
    return instance;
}

std::mutex& registryMutex()
{
    static std::mutex instance;
    return instance;
}

/// @brief 原子地把 target 更新为 min(target, value)
void atomicMin(std::atomic<long long>& target, long long value)
{
    long long current = target.load(std::memory_order_relaxed);
    while (value < current &&
           !target.compare_exchange_weak(current, value, std::memory_order_relaxed))
    {
        // current 已被 compare_exchange_weak 更新为最新值，继续比较
    }
}

/// @brief 原子地把 target 更新为 max(target, value)
void atomicMax(std::atomic<long long>& target, long long value)
{
    long long current = target.load(std::memory_order_relaxed);
    while (value > current &&
           !target.compare_exchange_weak(current, value, std::memory_order_relaxed))
    {
    }
}

double toMs(long long nanoseconds)
{
    return static_cast<double>(nanoseconds) / 1.0e6;
}
}  // namespace

std::atomic<bool> Profiler::s_enabled{false};

// ---------------------------------------------------------------------------
// TimerCounter
// ---------------------------------------------------------------------------

TimerCounter::TimerCounter(const char* name)
    : m_name(name)
{
    m_minNs.store(std::numeric_limits<long long>::max(), std::memory_order_relaxed);
    Profiler::registerCounter(this);
}

void TimerCounter::addSample(long long nanoseconds)
{
    m_count.fetch_add(1, std::memory_order_relaxed);
    m_totalNs.fetch_add(nanoseconds, std::memory_order_relaxed);
    atomicMin(m_minNs, nanoseconds);
    atomicMax(m_maxNs, nanoseconds);
}

void TimerCounter::reset()
{
    m_count.store(0, std::memory_order_relaxed);
    m_totalNs.store(0, std::memory_order_relaxed);
    m_minNs.store(std::numeric_limits<long long>::max(), std::memory_order_relaxed);
    m_maxNs.store(0, std::memory_order_relaxed);
}

double TimerCounter::averageMs() const
{
    const long long n = count();
    if (n <= 0)
    {
        return 0.0;
    }
    return toMs(totalNs()) / static_cast<double>(n);
}

// ---------------------------------------------------------------------------
// Profiler
// ---------------------------------------------------------------------------

void Profiler::registerCounter(TimerCounter* counter)
{
    std::lock_guard<std::mutex> guard(registryMutex());
    registry().push_back(counter);
}

void Profiler::setEnabled(bool enabled)
{
    s_enabled.store(enabled, std::memory_order_relaxed);
}

void Profiler::configureFromEnvironment()
{
    // 计数器是函数内静态对象，首次取用时才构造并登记。先全部取一遍，
    // 这样 report() 即使在某条路径一次都没走到时也能把它列出来
    // （「这一项没有采样」和「这一项不存在」是两回事）。
    counters::paintGL();
    counters::catchEntity();
    counters::selectWindow();
    counters::nearestVirtualIntersection();
    counters::openDocument();

    const char* value = std::getenv("YICAD_PROFILE");
    if (value == nullptr || *value == '\0')
    {
        return;
    }

    const bool on = (std::strcmp(value, "0") != 0);
    setEnabled(on);

    if (on)
    {
        // 埋点开着却看不到输出会让人以为埋点没生效，这里顺带把 render
        // 分类提到 Info，保证 report() 的汇总能打出来。
        log::render().setLevel(LogLevel::Info);
    }
}

std::vector<TimerCounter*> Profiler::counters()
{
    std::lock_guard<std::mutex> guard(registryMutex());
    return registry();
}

void Profiler::resetAll()
{
    for (TimerCounter* counter : counters())
    {
        counter->reset();
    }
}

void Profiler::report()
{
    const std::vector<TimerCounter*> all = counters();

    std::ostringstream out;
    out << "耗时埋点汇总\n";
    out << std::left << std::setw(32) << "计数器"
        << std::right << std::setw(10) << "次数"
        << std::setw(12) << "平均(ms)"
        << std::setw(12) << "最小(ms)"
        << std::setw(12) << "最大(ms)"
        << std::setw(14) << "合计(ms)" << "\n";

    bool any = false;
    for (const TimerCounter* counter : all)
    {
        if (counter->count() <= 0)
        {
            continue;
        }
        any = true;
        out << std::left << std::setw(32) << counter->name()
            << std::right << std::setw(10) << counter->count()
            << std::setw(12) << std::fixed << std::setprecision(3) << counter->averageMs()
            << std::setw(12) << toMs(counter->minNs())
            << std::setw(12) << toMs(counter->maxNs())
            << std::setw(14) << toMs(counter->totalNs()) << "\n";
    }

    if (!any)
    {
        out << "（无采样；用 YICAD_PROFILE=1 开启埋点）";
    }

    YICAD_LOG(log::render(), LogLevel::Info) << out.str();
}

// ---------------------------------------------------------------------------
// 内置计数器
// ---------------------------------------------------------------------------

namespace counters
{
TimerCounter& paintGL()
{
    static TimerCounter counter("render.paintGL");
    return counter;
}

TimerCounter& catchEntity()
{
    static TimerCounter counter("snap.catchEntity");
    return counter;
}

TimerCounter& selectWindow()
{
    static TimerCounter counter("selection.selectWindow");
    return counter;
}

TimerCounter& nearestVirtualIntersection()
{
    static TimerCounter counter("snap.nearestVirtualIntersection");
    return counter;
}

TimerCounter& openDocument()
{
    static TimerCounter counter("document.open");
    return counter;
}
}  // namespace counters

}  // namespace yicad
