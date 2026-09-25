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

/// @file MeasureExtension.h
/// @brief 查询扩展（src/extensions/measure/）的入口。
///
/// 业务工具化第四步（doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.4 节）把原 src/actions/ 的内置
/// 命令拆进扩展：查询距离、角度、面积、选中实体总长与选中实体信息，以及"绘图"类目里
/// 测量面板的按钮都在本目录。命令行别名仍在 keyconfig.xml，不随命令注册。

#ifndef MEASUREEXTENSION_H
#define MEASUREEXTENSION_H

#include "IExtension.h"

class MeasureExtension : public IExtension
{
public:
    /// @brief 注册查询命令与测量面板里的按钮。
    void OnRegister(IExtensionContext& ctx) override;

    std::string_view Id() const override { return "ext.measure"; }
};

#endif  // MEASUREEXTENSION_H
