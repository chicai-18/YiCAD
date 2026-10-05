/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/// @file DemoPipe.h
/// @brief 示例实体"管道"：插件自定义实体的写法（ABI v4，RENDER_PLAN.md 第 8.4 步）

#ifndef YICAD_DEMO_PIPE_H
#define YICAD_DEMO_PIPE_H

#include "YiCadPluginSdk.h"

#include <cstdint>
#include <span>
#include <vector>

namespace demo
{

/// @brief 管道的数据：一串折点与管径
struct PipeData
{
    std::vector<YiCadPoint2d> vertices;
    double diameter = 1.0;
};

/// @brief 实体类"管道"（com.yicad.demo.Pipe）
/// @details 画法：沿折点的中心线（文档里有 CENTER 线型时用它）；每段两侧距中心线管径一半的边线；
///          两端朝外的半圆端头；第一段中点一个指向第二个折点的红色实心箭头，外侧标注 "DN管径"。
///          夹点：每个折点一个，起点处垂直于第一段、距中心线管径一半处一个（拖它改管径）。
///          提供捕捉（折点为端点、每段中点、中心线上的最近点）与炸开（直线、多段线、圆弧、实心填充与文字）。
class PipeClass final : public yicad::plugin::EntityClass<PipeData>
{
public:
    static constexpr const char* ClassName = "com.yicad.demo.Pipe";
    static constexpr uint32_t Version = 1;

    /// @brief 编码：折点数、折点、管径（小端序）
    static std::vector<uint8_t> encodeData(const PipeData& data);
    /// @brief 解码；数据不完整、多余、折点少于 2 个或管径不为正时抛异常
    static PipeData decodeData(std::span<const uint8_t> bytes);

    PipeData decode(std::span<const uint8_t> bytes) const override { return decodeData(bytes); }
    std::vector<uint8_t> encode(const PipeData& data) const override { return encodeData(data); }
    void worldDraw(const PipeData& data, const yicad::plugin::Gi& gi) const override;
    YiCadExtents2d extents(const PipeData& data) const override;
    void transform(PipeData& data, const YiCadMatrix2d& matrix) const override;

    std::vector<YiCadPoint2d> grips(const PipeData& data) const override;
    void moveGrips(PipeData& data, std::span<const uint32_t> indices, YiCadVector2d offset) const override;
    std::vector<YiCadPoint2d> snapPoints(const PipeData& data, uint32_t snapMode,
        YiCadPoint2d pick) const override;
    bool explode(const PipeData& data, const yicad::plugin::ImportContainer& out) const override;
};

} // namespace demo

#endif
