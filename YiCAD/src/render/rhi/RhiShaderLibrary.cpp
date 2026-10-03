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

/// @file RhiShaderLibrary.cpp
/// @brief 读入构建期工具链生成的着色器程序

#include "RhiShaderLibrary.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>

#include "YiCadLog.h"

namespace
{
/// @brief 路径的 UTF-8 形式，写日志用（path::string() 在 Windows 上按本地代码页转换，遇到中文会抛异常）
std::string utf8(const std::filesystem::path& path)
{
    const std::u8string text = path.u8string();
    return std::string(text.begin(), text.end());
}

std::vector<std::byte> readFile(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return {};
    }
    std::vector<char> chars((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    std::vector<std::byte> bytes(chars.size());
    std::transform(chars.begin(), chars.end(), bytes.begin(), [](char c) { return static_cast<std::byte>(c); });
    return bytes;
}

RhiShaderPtr loadStage(RhiDevice& device, const std::filesystem::path& directory, std::string_view name,
                       RhiShaderStage stage, const char* suffix)
{
    const char* extension = device.caps().shaderLanguage == RhiShaderLanguage::SpirV ? ".spv" : ".glsl";
    const std::string fileName = std::string(name) + "." + suffix + extension;
    const std::vector<std::byte> code = readFile(directory / fileName);
    if (code.empty())
    {
        YICAD_LOG(yicad::log::render(), yicad::LogLevel::Warning)
            << "读不到着色器 " << utf8(directory / fileName);
        return nullptr;
    }
    RhiShaderDesc desc;
    desc.stage = stage;
    desc.code = code;
    desc.debugName = fileName;
    return device.createShader(desc);
}
}  // namespace

RhiProgramShaders rhiLoadProgram(RhiDevice& device, const std::filesystem::path& directory, std::string_view name)
{
    RhiProgramShaders program;
    program.vertex = loadStage(device, directory, name, RhiShaderStage::Vertex, "vert");
    program.fragment = loadStage(device, directory, name, RhiShaderStage::Fragment, "frag");
    return program;
}

std::vector<RhiBindGroupLayoutPtr> rhiCreateBindGroupLayouts(
    RhiDevice& device, std::span<const std::span<const RhiBindGroupLayoutEntry>> bindGroups)
{
    std::vector<RhiBindGroupLayoutPtr> layouts;
    layouts.reserve(bindGroups.size());
    for (const std::span<const RhiBindGroupLayoutEntry>& entries : bindGroups)
    {
        if (entries.empty())
        {
            layouts.push_back(nullptr);
            continue;
        }
        RhiBindGroupLayoutDesc desc;
        desc.entries = entries;
        layouts.push_back(device.createBindGroupLayout(desc));
    }
    return layouts;
}
