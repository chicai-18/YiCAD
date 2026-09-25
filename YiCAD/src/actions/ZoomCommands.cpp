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

/// @file ZoomCommands.cpp
/// @brief 放大、缩小：即时命令 zoom.in、zoom.out，取代原 ActionZoomIn
///
/// 以视图中心为基点缩放。原 ActionZoomIn 是视图 Action（isViewAction），不打断
/// 任何命令，所以注册为 InstantInterrupt::KeepAll。滚轮缩放不经命令，由 UIView
/// 直接调用视图的缩放。

#include "CommandRegistry.h"
#include "IDocumentView.h"

namespace
{
/// @brief 缩放因子
constexpr double ZOOM_FACTOR = 1.25;

/// @brief 视图工具类即时命令的附加信息：不打断任何命令
CommandInfo viewCommandInfo()
{
    CommandInfo info;
    info.instantInterrupt = InstantInterrupt::KeepAll;
    return info;
}

const bool g_registeredIn = CommandRegistry::instance().registerInstantCommand(
    QStringLiteral("zoom.in"),
    [](const CommandContext& ctx)
    {
        if (ctx.view)
        {
            ctx.view->zoomIn(ZOOM_FACTOR);
        }
    },
    viewCommandInfo());

const bool g_registeredOut = CommandRegistry::instance().registerInstantCommand(
    QStringLiteral("zoom.out"),
    [](const CommandContext& ctx)
    {
        if (ctx.view)
        {
            ctx.view->zoomOut(ZOOM_FACTOR);
        }
    },
    viewCommandInfo());
}  // namespace
