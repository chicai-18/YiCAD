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

/// @file BuiltinExtensions.cpp
/// @brief 随程序发布的进程内扩展的注册表

#include "BuiltinExtensions.h"

#include <memory>

#include "ExtensionManager.h"

// 进程内扩展（src/extensions/<扩展>/，构建系统按目录自动建库）。移除一个扩展：
// 删除其目录、这里的 #include，以及 registerBuiltinExtensions() 里的 Register 一行。
#include "AIExtension.h"
#include "BlockExtension.h"
#include "DimExtension.h"
#include "DrawExtension.h"
#include "EditExtension.h"
#include "FileExtension.h"
#include "HatchExtension.h"
#include "LayerExtension.h"
#include "MeasureExtension.h"
#include "ModifyExtension.h"
#include "OptionsExtension.h"
#include "TextExtension.h"
#include "ViewExtension.h"

void registerBuiltinExtensions(ExtensionManager& manager)
{
	// 文件、图层、选项排在前面：选项扩展的两个按钮要排在其它扩展的设置页入口之前（与迁移前一致）
	manager.Register(std::make_unique<FileExtension>());
	manager.Register(std::make_unique<LayerExtension>());
	manager.Register(std::make_unique<OptionsExtension>());
	// 原内置命令（迁移计划 9.4 节）：绘图排在修改之前（多段线面板里节点按钮在云线之后），
	// 也排在填充之前（其他面板里插入图片在填充之前）
	manager.Register(std::make_unique<DrawExtension>());
	manager.Register(std::make_unique<ModifyExtension>());
	manager.Register(std::make_unique<MeasureExtension>());
	manager.Register(std::make_unique<EditExtension>());
	manager.Register(std::make_unique<ViewExtension>());
	manager.Register(std::make_unique<AIExtension>());
	manager.Register(std::make_unique<DimExtension>());
	manager.Register(std::make_unique<BlockExtension>());
	manager.Register(std::make_unique<TextExtension>());
	manager.Register(std::make_unique<HatchExtension>());
}
