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

/// @file Selection.h
/// @brief 实体选择类，提供单选、全选、窗口选择、图层选择等功能

#ifndef SELECTION_H
#define SELECTION_H

#include "DmDocument.h"
#include "GuiDocumentView.h"

/// @brief 选择文档实体的类
class Selection
{
public:
	/// @brief 构造函数
	/// @param doc 文档指针
	/// @param docView 文档视图指针，可为空
	Selection(DmDocument* doc, GuiDocumentView* docView = nullptr);

	/// @brief 切换单个实体的选中状态
	/// @param e 实体指针
	void selectSingle(DmEntity* e);

	/// @brief 选中/取消选中所有实体
	/// @param select true为选中，false为取消选中
	void selectAll(bool select = true);

	/// @brief 根据窗口选择实体
	/// @param v1 窗口角点1
	/// @param v2 窗口角点2
	/// @param select true为选择，false为取消选择
	/// @param cross true为从右到左选择（有相交即选择），false为从左到右选择（完全框住才选择）
	/// @param entityTypeList 限定实体类型列表
	void selectWindow(const DmVector& v1, const DmVector& v2, bool select = true, bool cross = false, std::list<DM::EntityType> const& entityTypeList = {});

	/// @brief 选中/取消选中指定图层的所有实体
	/// @param layerName 图层名称
	/// @param select true为选中，false为取消选中
	void selectLayer(const QString& layerName, bool select = true);

	/// @brief 取消选中指定图层的所有实体
	/// @param layerName 图层名称
	void deselectLayer(QString& layerName);

protected:
	DmDocument* pDocument = nullptr;       ///< 关联的文档
	GuiDocumentView* docView = nullptr;    ///< 关联的文档视图
};

#endif
