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

/// @file DrawExtension.h
/// @brief 绘图扩展（src/extensions/draw/）的入口。
///
/// 业务工具化第四步（doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.4 节）把原 src/actions/ 的内置
/// 命令拆进扩展：直线、多段线、圆弧、圆、椭圆、样条、云线、点与插入图片 30 个交互命令、
/// 它们的选项条与"绘图"类目里的按钮都在本目录。这几类实体的属性对话框也在本扩展
/// （ext.draw.properties，doc/ARCHITECTURE_EVOLUTION_PLAN.md 9.3 节）。实体与持久化仍在内核，
/// 移除本扩展只是不能再新建、也不能再经对话框修改这些图形。
///
/// 命令行别名仍在 keyconfig.xml（以命令 ID 为键，"默认""拼音简写"两组），不随命令注册。

#ifndef DRAWEXTENSION_H
#define DRAWEXTENSION_H

#include "IExtension.h"

class DrawExtension : public IExtension
{
public:
    /// @brief 注册绘图命令（含选项条）与"绘图"类目里直线、曲线、多段线、圆、椭圆、其他面板的按钮。
    void OnRegister(IExtensionContext& ctx) override;

    std::string_view Id() const override { return "ext.draw"; }
};

#endif  // DRAWEXTENSION_H
