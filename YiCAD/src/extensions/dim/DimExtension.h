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

/// @file DimExtension.h
/// @brief 标注扩展（src/extensions/dim/）的入口。
///
/// 阶段4任务⑤的第一个领域扩展：标注的交互命令（对齐、线性、半径、直径、
/// 角度、引线、基线、标注样式）、线性标注选项条、标注样式对话框与 Ribbon
/// 按钮都在本目录。标注实体（DmDim*）、标注样式表与它们的持久化仍在内核
/// ——打开含标注的图纸不依赖本扩展，移除本扩展只是不能再新建标注、编辑
/// 标注样式。

#ifndef DIMEXTENSION_H
#define DIMEXTENSION_H

#include "IExtension.h"

class DimExtension : public IExtension
{
public:
    /// @brief 注册标注命令（含命令行别名与选项条）与"绘图/标注"面板里的按钮。
    void OnRegister(IExtensionContext& ctx) override;

    std::string_view Id() const override { return "ext.dim"; }
};

#endif  // DIMEXTENSION_H
