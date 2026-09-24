/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file DimCommands.h
/// @brief 标注扩展的交互命令：对齐、线性、半径、直径、角度、引线、基线，取代原
///        ActionDimAligned/Linear/Radial/Diametric/Angular/Leader/Baseline
///
/// 命令只负责装配工具与选项条；各自的工具在同名的 .cpp 里，从原 Action 机械改写而来。

#ifndef DIMCOMMANDS_H
#define DIMCOMMANDS_H

#include <memory>

#include <QCoreApplication>

#include "DimensionCommand.h"

/// @brief 对齐标注 ext.dim.aligned
class DimAlignedCommand : public DimensionCommand
{
    Q_DECLARE_TR_FUNCTIONS(DimAlignedCommand)

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 线性标注 ext.dim.linear；选项条（UIDimLinearOptions）改标注线角度
class DimLinearCommand : public DimensionCommand
{
    Q_DECLARE_TR_FUNCTIONS(DimLinearCommand)

public:
    /// @brief 标注线角度（弧度）；记在工具的标注数据上，每放一个标注复位为 0（与原 Action 一致）
    double angle() const;
    void setAngle(double a);
    /// @brief 放下一个标注后让选项条重新读取（原 reset() 里的 requestOptions）
    void refreshOptions();

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 半径标注 ext.dim.radial
class DimRadialCommand : public DimensionCommand
{
    Q_DECLARE_TR_FUNCTIONS(DimRadialCommand)

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 直径标注 ext.dim.diametric
class DimDiametricCommand : public DimensionCommand
{
    Q_DECLARE_TR_FUNCTIONS(DimDiametricCommand)

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 角度标注 ext.dim.angular
class DimAngularCommand : public DimensionCommand
{
    Q_DECLARE_TR_FUNCTIONS(DimAngularCommand)

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 基线标注 ext.dim.baseline
class DimBaselineCommand : public DimensionCommand
{
    Q_DECLARE_TR_FUNCTIONS(DimBaselineCommand)

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 引线 ext.dim.leader（原 ActionDimLeader 不是标注 Action，没有标注数据与选项条）
class DimLeaderCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DimLeaderCommand)

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

#endif  // DIMCOMMANDS_H
