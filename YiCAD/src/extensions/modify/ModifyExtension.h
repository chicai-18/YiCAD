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

/// @file ModifyExtension.h
/// @brief 修改扩展（src/extensions/modify/）的入口。
///
/// 业务工具化第四步（doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.4 节）把原 src/actions/ 的内置
/// 命令拆进扩展：移动、复制、旋转、缩放、镜像、修剪、延伸、偏移、倒角、圆角、打断、修改
/// 实体属性、分解、反向、删除、复制到图层与多段线节点编辑，倒角、圆角的选项条，以及
/// "绘图"类目里修改面板与多段线面板上的节点按钮都在本目录。
///
/// 宿主固定调用其中两个：Delete 键与手写板橡皮擦删除选择集（ext.modify.delete_no_select），
/// 图层面板的"复制到图层"（ext.modify.copy_to_layer）；没有本扩展时它们什么也不做。
/// 命令行别名仍在 keyconfig.xml，不随命令注册。

#ifndef MODIFYEXTENSION_H
#define MODIFYEXTENSION_H

#include "IExtension.h"

class ModifyExtension : public IExtension
{
public:
    /// @brief 注册修改命令（含倒角、圆角的选项条）与修改面板、多段线面板里的按钮。
    void OnRegister(IExtensionContext& ctx) override;

    std::string_view Id() const override { return "ext.modify"; }
};

#endif  // MODIFYEXTENSION_H
