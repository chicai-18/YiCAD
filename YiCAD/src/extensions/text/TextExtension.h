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

/// @file TextExtension.h
/// @brief 文字扩展（src/extensions/text/）的入口。
///
/// 业务工具化第三步第⑦批（doc/COMMAND_TOOL_MIGRATION_PLAN.md 5.2 节）：单行文字、多行
/// 文字（新建与双击就地编辑）、多行文字属性面板、文字样式，以及多行文字编辑框
/// （MTextEditWidget 与它的撤销命令）、三个选项条和"绘图/文字"面板里的按钮。文字实体
/// （DmText、DmMText）、文字样式表与持久化仍在内核。
///
/// 宿主与内核经 ID 使用本扩展：选择层双击多行文字启动它登记的编辑命令
/// （CommandRegistry::registerEntityEditor）；选择变化时宿主运行
/// ext.text.selection_changed，单选多行文字时显示属性面板；修改实体属性点了多行文字时
/// 转到 ext.text.modify_mtext。

#ifndef TEXTEXTENSION_H
#define TEXTEXTENSION_H

#include "IExtension.h"

class TextExtension : public IExtension
{
public:
    /// @brief 注册文字命令、多行文字的双击编辑与"绘图/文字"面板里的按钮。
    void OnRegister(IExtensionContext& ctx) override;

    std::string_view Id() const override { return "ext.text"; }
};

#endif  // TEXTEXTENSION_H
