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

/// @file RhiShaderLibrary.h
/// @brief 读入构建期工具链生成的着色器程序（RENDER_PLAN.md 第 4.7.4 节）
/// @details tools/compile_shaders.py 为每个程序生成 <名字>.<阶段>.spv（Vulkan）与 .glsl（GL 430），
///          以及描述绑定组布局的头文件；这里按设备的着色器形式读文件、建着色器与绑定组布局

#ifndef RHISHADERLIBRARY_H
#define RHISHADERLIBRARY_H

#include <filesystem>
#include <string_view>
#include <vector>

#include "RhiDevice.h"

/// @brief 一个程序的两个阶段
struct RhiProgramShaders
{
    RhiShaderPtr vertex;
    RhiShaderPtr fragment;

    bool isValid() const { return vertex && fragment; }
};

/// @brief 从目录里读程序 name 的两个阶段并创建着色器；文件缺失或编译失败时对应项为空
RhiProgramShaders rhiLoadProgram(RhiDevice& device, const std::filesystem::path& directory, std::string_view name);

/// @brief 按生成头文件里程序的绑定组（按组号排列，空表示不用）创建布局，结果可直接放进 RhiPipelineDesc
std::vector<RhiBindGroupLayoutPtr> rhiCreateBindGroupLayouts(
    RhiDevice& device, std::span<const std::span<const RhiBindGroupLayoutEntry>> bindGroups);

#endif // RHISHADERLIBRARY_H
