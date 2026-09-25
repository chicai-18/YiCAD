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

/// @file OptionsExtension.h
/// @brief 选项扩展（src/extensions/options/）的入口。
///
/// 业务工具化第三步第⑤批（doc/COMMAND_TOOL_MIGRATION_PLAN.md 5.2 节）：系统设置、
/// 图纸设置两条即时命令与"设置"面板里的两个按钮。两个设置对话框（ui/，连同系统设置里
/// 的"命令设置"对话框）由本扩展直接构造（doc/ARCHITECTURE_EVOLUTION_PLAN.md 9.3 节）；
/// 系统设置改了颜色后经 IExtensionContext::tabDrawWidget() 刷新全部打开的视图。

#ifndef OPTIONSEXTENSION_H
#define OPTIONSEXTENSION_H

#include "IExtension.h"

class OptionsExtension : public IExtension
{
public:
    /// @brief 注册设置命令与"设置"面板里的按钮。
    void OnRegister(IExtensionContext& ctx) override;

    std::string_view Id() const override { return "ext.options"; }
};

#endif  // OPTIONSEXTENSION_H
