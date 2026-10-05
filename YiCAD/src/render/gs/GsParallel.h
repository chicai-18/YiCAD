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

/// @file GsParallel.h
/// @brief 图形系统的并行执行（RENDER_PLAN.md 第 4.3.11 节：worldDraw 与编译在多个线程上并行）
/// @details 用标准库的线程：每次调用开若干个 std::thread，连同当前线程按原子计数领取下标。
///          打开 50 万实体的图纸时整图重建要几秒，开线程的开销（每次几十微秒）可以忽略；
///          工作量小时（改几个实体）在当前线程上顺序执行

#ifndef GSPARALLEL_H
#define GSPARALLEL_H

#include <cstddef>
#include <functional>

/// @brief 并行执行用的线程数（含当前线程）：硬件线程数，或 gsSetMaxWorkers() 设的上限
unsigned gsWorkerCount();

/// @brief 限制并行的线程数（含当前线程），0 为不限（硬件线程数）；测试用 1 对照顺序执行的结果
void gsSetMaxWorkers(unsigned workers);

/// @brief 对 [0, count) 的每个下标执行 body(下标, 线程序号)，线程序号在 [0, gsWorkerCount()) 里，当前线程为 0
/// @details count 小于 minParallel 时全在当前线程上顺序执行。body 抛出的第一个异常在全部线程结束后重新抛出
void gsParallelFor(std::size_t count, std::size_t minParallel, const std::function<void(std::size_t, unsigned)>& body);

#endif // GSPARALLEL_H
