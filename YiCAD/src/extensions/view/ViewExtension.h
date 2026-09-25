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

/// @file ViewExtension.h
/// @brief 视图扩展（src/extensions/view/）的入口。
///
/// 业务工具化第四步（doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.4 节）把原 src/actions/ 的内置
/// 命令拆进扩展：放大、缩小（即时命令）。原先的平移模式（临时视图工具）已删除；滚轮缩放、
/// 中键平移属于视图本身（UIView 与导航层 PanZoomTool），不在这里。

#ifndef VIEWEXTENSION_H
#define VIEWEXTENSION_H

#include "IExtension.h"

class ViewExtension : public IExtension
{
public:
    /// @brief 注册缩放命令。
    void OnRegister(IExtensionContext& ctx) override;

    std::string_view Id() const override { return "ext.view"; }
};

#endif  // VIEWEXTENSION_H
