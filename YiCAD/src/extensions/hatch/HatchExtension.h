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

/// @file HatchExtension.h
/// @brief 填充扩展（src/extensions/hatch/）的入口。
///
/// 业务工具化第三步第⑧批（doc/COMMAND_TOOL_MIGRATION_PLAN.md 5.2 节）：填充命令与"绘图/
/// 其他"面板里的按钮。填充实体（DmHatch）、图案与持久化仍在内核。填充对话框（UIDlgHatch）
/// 在本扩展的 ui/ 里，新建填充时由命令直接构造，修改填充时由登记的属性编辑命令
/// ext.hatch.properties 构造（doc/ARCHITECTURE_EVOLUTION_PLAN.md 9.3 节）。

#ifndef HATCHEXTENSION_H
#define HATCHEXTENSION_H

#include "IExtension.h"

class HatchExtension : public IExtension
{
public:
    /// @brief 注册填充命令、填充的属性编辑命令与"绘图/其他"面板里的按钮。
    void OnRegister(IExtensionContext& ctx) override;

    std::string_view Id() const override { return "ext.hatch"; }
};

#endif  // HATCHEXTENSION_H
