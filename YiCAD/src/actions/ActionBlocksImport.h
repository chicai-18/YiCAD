/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


/// @file ActionBlocksImport.h
/// @brief 从外部文件导入块定义到当前文档的动作类头文件

#ifndef ACTIONBLOCKSIMPORT_H
#define ACTIONBLOCKSIMPORT_H

#include "ActionInterface.h"

class DmEntity;

/// @brief 从外部文件导入块定义到当前文档
class ActionBlocksImport : public ActionInterface
{
	Q_OBJECT
public:
	/// @brief 构造函数
	/// @param doc 文档指针
	/// @param docView 文档视图指针
	ActionBlocksImport(DmDocument* doc, GuiDocumentView* docView);

	/// @brief 初始化动作
	/// @param status 状态参数，默认为0
	void init(int status = 0) override;

	/// @brief 触发动作执行
	void trigger() override;

	/// @brief 递归重定向实体的图层、文字样式、标注样式到目标文档
	/// @param entity 待处理的实体
	void repointEntityStyles(DmEntity* entity);

	/// @brief 检查是否为独占动作
	/// @return false表示非独占
	bool isExclusive() override { return false; }

	/// @brief 检查是否可以中断
	/// @return true表示可以中断
	bool canBeInterrupt() override { return true; }
};

#endif
