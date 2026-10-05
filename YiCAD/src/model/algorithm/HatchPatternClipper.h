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

/// @file HatchPatternClipper.h
/// @brief 填充图案线在边界里的切分（RENDER_PLAN.md 第 10 节阶段 6）
/// @details 填充（DmHatch，切出供选择、捕捉、炸开的划线）与图形系统（GI 的 setFill，画图）共用这一份，
///          两边切出的图案线一致。边界是 GI 的环（顶点与凸度，圆弧精确），按奇偶规则：
///          每条图案线与全部环的边求交，交点沿线排序后两两成段。顶点正好落在线上时按"在线上算在正侧"
///          计数（半开规则），所以穿过顶点的线只计一次、擦过顶点的线不计

#ifndef HATCHPATTERNCLIPPER_H
#define HATCHPATTERNCLIPPER_H

#include <span>
#include <vector>

#include "DmVector.h"
#include "GiTypes.h"

/// @brief 一条图案线在边界内的一整段
struct HatchPatternRun
{
    DmVector start;       ///< 沿图案线方向在前的一端
    DmVector end;
    double phase = 0.0;   ///< 起点在图案里的位置（图案从这条线经过的 base + k·offset 起算，对周期取模；实线为 0）
};

/// @brief 填充图案线的切分，见文件说明
class HatchPatternClipper
{
public:
    /// @brief 一族图案线超过这么多条就不切（与 AutoCAD 的 HPMAXLINES 的默认值相同）
    static constexpr double kMaxLines = 1000000.0;

    /// @brief 一族图案线在边界环里切出的段，按线的序号、再沿线的方向排列，追加到 runs
    /// @return false：线族不合法（方向为零、位移与方向平行）或条数超过 kMaxLines，什么也不切
    static bool clip(std::span<const GiLoop> loops, const GiHatchPatternLine& line, std::vector<HatchPatternRun>& runs);

    /// @brief 图案的周期（各元素绝对值之和）；实线为 0
    static double period(const std::vector<double>& dashes);

    /// @brief 把一整段按图案切成划线与点
    /// @details 图案的一个周期从 s = -相位 + m·周期 开始；划线与这一段相交的部分成一条线段，落在段内的点成一个点。
    ///          实线或图案无效时整段是一条线段。填充（DmHatch）切出供选择、捕捉的划线实体，自定义实体的默认炸开（DmGiExplode）都用它
    /// @param segments 追加划线的两端
    /// @param dots 追加点
    static void splitRun(const HatchPatternRun& run, const std::vector<double>& dashes,
                         std::vector<std::pair<DmVector, DmVector>>& segments, std::vector<DmVector>& dots);

    /// @brief 相邻两条线的距离（offset 在线的法向上的分量的绝对值）；方向为零时为 0
    static double spacing(const GiHatchPatternLine& line);
};

#endif // HATCHPATTERNCLIPPER_H
