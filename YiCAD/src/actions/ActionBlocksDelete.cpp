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


/// @file ActionBlocksDelete.cpp
/// @brief 删除块定义的动作类实现文件

#include "ActionBlocksDelete.h"
#include "DmDocument.h"
#include "DmBlockTable.h"
#include "UIBlockDelete.h"
#include "ApplicationWindow.h"


/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionBlocksDelete::ActionBlocksDelete(DmDocument* doc,
	GuiDocumentView* docView)
	: ActionInterface("Delete Block", doc, docView)
	, m_pDialog(nullptr)
{
}

/// @brief 初始化动作
/// @param status 状态参数，默认为0
void ActionBlocksDelete::init(int status)
{
	ActionInterface::init(status);
	trigger();
}

/// @brief 触发动作执行
void ActionBlocksDelete::trigger()
{
	if (!m_pDialog)
	{
		m_pDialog = new UIBlockDelete(ApplicationWindow::getAppWindow());
	}
	m_pDialog->setBlockTable(pDocument->getBlockTable());
	m_pDialog->setDocument(pDocument);
	m_pDialog->exec();

	finish();
}
