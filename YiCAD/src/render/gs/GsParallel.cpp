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

/// @file GsParallel.cpp
/// @brief gsParallelFor 实现

#include "GsParallel.h"

#include <algorithm>
#include <atomic>
#include <exception>
#include <mutex>
#include <thread>
#include <vector>

namespace
{

std::atomic<unsigned> g_maxWorkers{0};

}  // namespace

unsigned gsWorkerCount()
{
    const unsigned hardware = std::max(std::thread::hardware_concurrency(), 1u);
    const unsigned limit = g_maxWorkers.load(std::memory_order_relaxed);
    return limit == 0 ? hardware : std::min(hardware, limit);
}

void gsSetMaxWorkers(unsigned workers)
{
    g_maxWorkers.store(workers, std::memory_order_relaxed);
}

void gsParallelFor(std::size_t count, std::size_t minParallel, const std::function<void(std::size_t, unsigned)>& body)
{
    const unsigned workers =
        static_cast<unsigned>(std::min<std::size_t>(gsWorkerCount(), std::max<std::size_t>(count, 1)));
    if (workers <= 1 || count < minParallel)
    {
        for (std::size_t i = 0; i < count; ++i)
        {
            body(i, 0);
        }
        return;
    }

    std::atomic<std::size_t> next{0};
    std::exception_ptr failure;
    std::mutex failureMutex;
    auto run = [&](unsigned worker) {
        try
        {
            for (std::size_t i = next.fetch_add(1, std::memory_order_relaxed); i < count;
                 i = next.fetch_add(1, std::memory_order_relaxed))
            {
                body(i, worker);
            }
        }
        catch (...)
        {
            const std::lock_guard<std::mutex> lock(failureMutex);
            if (!failure)
            {
                failure = std::current_exception();
            }
            // 别的线程领完剩下的下标就停
            next.store(count, std::memory_order_relaxed);
        }
    };
    std::vector<std::thread> threads;
    threads.reserve(workers - 1);
    for (unsigned w = 1; w < workers; ++w)
    {
        threads.emplace_back(run, w);
    }
    run(0);
    for (std::thread& t : threads)
    {
        t.join();
    }
    if (failure)
    {
        std::rethrow_exception(failure);
    }
}
