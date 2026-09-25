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

/// @file EditExtension.h
/// @brief 编辑扩展（src/extensions/edit/）的入口。
///
/// 业务工具化第四步（doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.4 节）把原 src/actions/ 的内置
/// 命令拆进扩展：复制到剪贴板、剪切、粘贴，撤销与重做。它们没有 Ribbon 按钮，入口是宿主的
/// 快速访问栏与 Ctrl+X/C/V、Ctrl+Z/Y，宿主按 ID 启动；没有本扩展时这些入口什么也不做。

#ifndef EDITEXTENSION_H
#define EDITEXTENSION_H

#include "IExtension.h"

class EditExtension : public IExtension
{
public:
    /// @brief 注册剪贴板命令与撤销、重做。
    void OnRegister(IExtensionContext& ctx) override;

    std::string_view Id() const override { return "ext.edit"; }
};

#endif  // EDITEXTENSION_H
