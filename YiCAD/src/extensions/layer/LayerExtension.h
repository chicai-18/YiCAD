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

/// @file LayerExtension.h
/// @brief 图层扩展（src/extensions/layer/）的入口。
///
/// 业务工具化第三步第⑤批（doc/COMMAND_TOOL_MIGRATION_PLAN.md 5.2 节）：图层的
/// 激活、新建、改名、删除、颜色、显示/隐藏、锁定、打印与全部显示/解锁等即时命令。
/// "图层"面板（图层下拉框与其下一排按钮）仍由宿主构造，按钮按 ID 启动这里的命令；
/// 下拉框每行按钮上记有图层名（ComboBoxData::tagButtons），命令由触发它的按钮
/// 找到要操作的图层，不访问主窗口。

#ifndef LAYEREXTENSION_H
#define LAYEREXTENSION_H

#include "IExtension.h"

class LayerExtension : public IExtension
{
public:
    /// @brief 注册图层命令。
    void OnRegister(IExtensionContext& ctx) override;

    std::string_view Id() const override { return "ext.layer"; }
};

#endif  // LAYEREXTENSION_H
