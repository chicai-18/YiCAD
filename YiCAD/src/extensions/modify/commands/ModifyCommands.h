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

/// @file ModifyCommands.h
/// @brief 修改扩展（src/extensions/modify/）各命令的工厂
///
/// 命令类多数只在自己的 .cpp 里定义，工厂也定义在那里；扩展入口按命令 ID 登记它们
/// （doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.4 节：原 src/actions/ 的内置命令拆进扩展）。

#ifndef MODIFYCOMMANDS_H
#define MODIFYCOMMANDS_H

#include "CommandRegistry.h"

namespace ModifyCommands
{
/// @brief 交互命令 ext.modify.copy_to_layer（CopyToLayerCommand.cpp）
ExclusiveCommandFactory copyToLayer();
/// @brief 交互命令 ext.modify.bevel（ModifyBevelCommand.cpp）
ExclusiveCommandFactory bevel();
/// @brief 交互命令 ext.modify.copy（ModifyCopyCommand.cpp）
ExclusiveCommandFactory copy();
/// @brief 交互命令 ext.modify.cut（ModifyCutCommands.cpp）
ExclusiveCommandFactory cut();
/// @brief 交互命令 ext.modify.cut_2p（ModifyCutCommands.cpp）
ExclusiveCommandFactory cut2p();
/// @brief 交互命令 ext.modify.delete（ModifyDeleteCommand.cpp）
ExclusiveCommandFactory remove();
/// @brief 即时命令 ext.modify.delete_no_select（ModifyDeleteCommand.cpp）
InstantCommand deleteSelection();
/// @brief 交互命令 ext.modify.entity（ModifyEntityCommand.cpp）
ExclusiveCommandFactory entity();
/// @brief 交互命令 ext.modify.explode（ModifyExplodeCommand.cpp）
ExclusiveCommandFactory explode();
/// @brief 交互命令 ext.modify.extend（ModifyExtendCommand.cpp）
ExclusiveCommandFactory extend();
/// @brief 交互命令 ext.modify.mirror（ModifyMirrorCommand.cpp）
ExclusiveCommandFactory mirror();
/// @brief 交互命令 ext.modify.move（ModifyMoveCommand.cpp）
ExclusiveCommandFactory move();
/// @brief 交互命令 ext.modify.reverse（ModifyReverseCommand.cpp）
ExclusiveCommandFactory reverse();
/// @brief 交互命令 ext.modify.rotate（ModifyRotateCommand.cpp）
ExclusiveCommandFactory rotate();
/// @brief 交互命令 ext.modify.round（ModifyRoundCommand.cpp）
ExclusiveCommandFactory round();
/// @brief 交互命令 ext.modify.scale（ModifyScaleCommand.cpp）
ExclusiveCommandFactory scale();
/// @brief 交互命令 ext.modify.single_offset（ModifySingleOffsetCommand.cpp）
ExclusiveCommandFactory singleOffset();
/// @brief 交互命令 ext.modify.trim（ModifyTrimCommand.cpp）
ExclusiveCommandFactory trim();
/// @brief 交互命令 ext.modify.polyline_add（PolylineEditCommands.cpp）
ExclusiveCommandFactory polylineAdd();
/// @brief 交互命令 ext.modify.polyline_append（PolylineEditCommands.cpp）
ExclusiveCommandFactory polylineAppend();
/// @brief 交互命令 ext.modify.polyline_del（PolylineEditCommands.cpp）
ExclusiveCommandFactory polylineDel();
}  // namespace ModifyCommands

#endif  // MODIFYCOMMANDS_H
