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

/// @file Selection.cpp
/// @brief 实体选择功能实现，提供单选、全选、窗口选择、图层选择

#include "Selection.h"
#include "ScopedTimer.h"

#include "DmEntity.h"
#include "DmDocument.h"
#include "DmLayer.h"
#include "EntityTable.h"

Selection::Selection(DmDocument* doc)
{
	this->pDocument = doc;
}

/// @brief 切换单个实体的选中状态
/// @param e 实体指针
void Selection::selectSingle(DmEntity* e)
{
	if (e && (!(e->getLayer() && e->getLayer()->isLocked())))
	{
		e->toggleSelected();

		if (pDocument)
		{
			pDocument->notifyDocumentModified();
			pDocument->requestRedraw();
		}
	}
}

/// @brief 选中/取消选中所有实体
/// @param select true为选中，false为取消选中
void Selection::selectAll(bool select)
{
	auto table = pDocument->getEntityTable();
	for (auto e : *table)
	{
		if (e->isVisible())
		{
			e->setSelected(select);
		}
	}

	pDocument->notifyDocumentModified();
	pDocument->requestRedraw();
}

/// @brief 根据窗口选择实体
/// @param v1 窗口角点1
/// @param v2 窗口角点2
/// @param select true为选择，false为取消选择
/// @param cross true为从右到左选择（有相交即选择），false为从左到右选择（完全框住才选择）
/// @param entityTypeList 限定实体类型列表
void Selection::selectWindow(const DmVector& v1, const DmVector& v2, bool select, bool cross, std::list<DM::EntityType> const& entityTypeList)
{
	// 框选耗时埋点，默认关闭，见 ScopedTimer.h。
	YICAD_SCOPED_TIMER(yicad::counters::selectWindow());

	// 几何判断由实体表的矩形查询完成，这里只置位
	EntityTable* table = pDocument->getEntityTable();
	std::vector<DmEntity*> hits = cross ? table->entitiesCrossingRect(v1, v2, entityTypeList)
		: table->entitiesInsideRect(v1, v2, entityTypeList);
	for (auto e : hits)
	{
		e->setSelected(select);
	}

	pDocument->notifyDocumentModified();
	pDocument->requestRedraw();
}

/// @brief 选中/取消选中指定图层的所有实体
/// @param layerName 图层名称
/// @param select true为选中，false为取消选中
void Selection::selectLayer(const QString& layerName, bool select)
{
	auto table = pDocument->getEntityTable();
	for (auto en : *table)
	{
		if (en && en->isVisible() && en->isSelected() != select && (!(en->getLayer() && en->getLayer()->isLocked())))
		{
			DmLayer* l = en->getLayer(true);

			if (l && l->getName() == layerName)
			{
				en->setSelected(select);
			}
		}
	}

	pDocument->notifyDocumentModified();
	pDocument->requestRedraw();
}

/// @brief 取消选中指定图层的所有实体
/// @param layerName 图层名称
void Selection::deselectLayer(QString& layerName)
{
	selectLayer(layerName, false);
}
