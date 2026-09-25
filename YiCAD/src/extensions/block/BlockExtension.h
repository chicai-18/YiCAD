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

/// @file BlockExtension.h
/// @brief 块扩展（src/extensions/block/）的入口。
///
/// 业务工具化第三步第⑥批（doc/COMMAND_TOOL_MIGRATION_PLAN.md 5.2 节）：创建块、插入块
/// （块列表加放置的两阶段命令）、编辑块与块编辑模式、定义属性、块的删除/保存/另存为/
/// 导入，以及"绘图/块"面板里的按钮。块实体（DmBlock、DmBlockReference）、块表、块编辑
/// 的文档状态与持久化仍在内核。块相关的对话框与选项条在本扩展的 ui/ 里，由命令直接构造
/// （doc/ARCHITECTURE_EVOLUTION_PLAN.md 9.3 节）；块参照与属性定义的属性对话框见第④批。
///
/// 宿主在撤销/重做使文档回到块编辑状态时按 ID 运行 ext.block.reenter_edit 恢复编辑模式。

#ifndef BLOCKEXTENSION_H
#define BLOCKEXTENSION_H

#include "IExtension.h"

class BlockExtension : public IExtension
{
public:
    /// @brief 注册块命令与"绘图/块"面板里的按钮。
    void OnRegister(IExtensionContext& ctx) override;

    std::string_view Id() const override { return "ext.block"; }
};

#endif  // BLOCKEXTENSION_H
