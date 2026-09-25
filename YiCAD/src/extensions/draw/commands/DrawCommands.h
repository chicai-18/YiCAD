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

/// @file DrawCommands.h
/// @brief 绘图扩展（src/extensions/draw/）各命令的工厂
///
/// 命令类多数只在自己的 .cpp 里定义，工厂也定义在那里；扩展入口按命令 ID 登记它们
/// （doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.4 节：原 src/actions/ 的内置命令拆进扩展）。

#ifndef DRAWCOMMANDS_H
#define DRAWCOMMANDS_H

#include <vector>

#include "CommandRegistry.h"

namespace DrawCommands
{
/// @brief 交互命令 ext.draw.arc（DrawArcCommand.cpp）
ExclusiveCommandFactory arc();
/// @brief 交互命令 ext.draw.arc_3p（DrawArcCommand.cpp）
ExclusiveCommandFactory arc3p();
/// @brief 交互命令 ext.draw.arc_tangential（DrawArcTangentialCommand.cpp）
ExclusiveCommandFactory arcTangential();
/// @brief 交互命令 ext.draw.circle（DrawCircleCommands.cpp）
ExclusiveCommandFactory circle();
/// @brief 交互命令 ext.draw.circle_2p（DrawCircleCommands.cpp）
ExclusiveCommandFactory circle2p();
/// @brief 交互命令 ext.draw.circle_3p（DrawCircleCommands.cpp）
ExclusiveCommandFactory circle3p();
/// @brief 交互命令 ext.draw.circle_tan2（DrawCircleTan2Command.cpp）
ExclusiveCommandFactory circleTan2();
/// @brief 交互命令 ext.draw.circle_tan3（DrawCircleTan3Command.cpp）
ExclusiveCommandFactory circleTan3();
/// @brief 交互命令 ext.draw.cloud_line_rectangle（DrawCloudLineCommand.cpp）
ExclusiveCommandFactory cloudLineRectangle();
/// @brief 交互命令 ext.draw.cloud_line_polygon（DrawCloudLineCommand.cpp）
ExclusiveCommandFactory cloudLinePolygon();
/// @brief 交互命令 ext.draw.cloud_line_free（DrawCloudLineCommand.cpp）
ExclusiveCommandFactory cloudLineFree();
/// @brief 交互命令 ext.draw.ellipse_axis（DrawEllipseAxisCommand.cpp）
ExclusiveCommandFactory ellipseAxis();
/// @brief 交互命令 ext.draw.ellipse_arc_axis（DrawEllipseAxisCommand.cpp）
ExclusiveCommandFactory ellipseArcAxis();
/// @brief 交互命令 ext.draw.ellipse_inscribe（DrawEllipseInscribeCommand.cpp）
ExclusiveCommandFactory ellipseInscribe();
/// @brief 交互命令 ext.draw.image（DrawImageCommand.cpp）
ExclusiveCommandFactory image();
/// @brief 交互命令 ext.draw.ray（DrawInfiniteLineCommands.cpp）
ExclusiveCommandFactory ray();
/// @brief 交互命令 ext.draw.xline（DrawInfiniteLineCommands.cpp）
ExclusiveCommandFactory xline();
/// @brief 交互命令 ext.draw.line_bisector（DrawLineBisectorCommand.cpp）
ExclusiveCommandFactory lineBisector();
/// @brief 交互命令 ext.draw.line（DrawLineCommand.cpp）
ExclusiveCommandFactory line();
/// @brief 交互命令 ext.draw.line_free（DrawLineFreeCommand.cpp）
ExclusiveCommandFactory lineFree();
/// @brief 交互命令 ext.draw.line_orth_tan（DrawLineOrthTanCommand.cpp）
ExclusiveCommandFactory lineOrthTan();
/// @brief 交互命令 ext.draw.line_polygon_cen_cor（DrawLinePolygonCommand.cpp）
ExclusiveCommandFactory linePolygonCenCor();
/// @brief 交互命令 ext.draw.line_polygon_cen_tan（DrawLinePolygonCommand.cpp）
ExclusiveCommandFactory linePolygonCenTan();
/// @brief 交互命令 ext.draw.line_rectangle（DrawLineRectangleCommand.cpp）
ExclusiveCommandFactory lineRectangle();
/// @brief 交互命令 ext.draw.line_tangent1（DrawLineTangent1Command.cpp）
ExclusiveCommandFactory lineTangent1();
/// @brief 交互命令 ext.draw.line_tangent2（DrawLineTangent2Command.cpp）
ExclusiveCommandFactory lineTangent2();
/// @brief 交互命令 ext.draw.point（DrawPointCommand.cpp）
ExclusiveCommandFactory point();
/// @brief 交互命令 ext.draw.polyline（DrawPolylineCommand.cpp）
ExclusiveCommandFactory polyline();
/// @brief 交互命令 ext.draw.spline（DrawSplineCommand.cpp）
ExclusiveCommandFactory spline();
/// @brief 交互命令 ext.draw.spline_points（DrawSplineCommand.cpp）
ExclusiveCommandFactory splinePoints();

/// @brief 即时命令 ext.draw.properties：本扩展创建的几类实体的属性对话框（DrawEntityProperties.cpp）
InstantCommand properties();
/// @brief ext.draw.properties 登记为属性编辑命令的实体类型
const std::vector<DM::EntityType>& propertyEntityTypes();
}  // namespace DrawCommands

#endif  // DRAWCOMMANDS_H
