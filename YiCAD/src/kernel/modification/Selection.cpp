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

#include "DmLine.h"
#include "Information.h"
#include "DmPolyline.h"
#include "DmEntity.h"
#include "DmDocument.h"
#include "DmLayer.h"
#include "DmSolid.h"
#include "DmTriangle.h"

Selection::Selection(DmDocument* doc, GuiDocumentView* docView)
{
	this->pDocument = doc;
	this->docView = docView;
}

/// @brief 切换单个实体的选中状态
/// @param e 实体指针
void Selection::selectSingle(DmEntity* e)
{
	if (e && (!(e->getLayer() && e->getLayer()->isLocked())))
	{
		e->toggleSelected();

		if (docView)
		{
			docView->specifyDocumentModified();
			docView->redraw();
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

	if (docView)
	{
		docView->specifyDocumentModified();
		docView->redraw();
	}
}

/// @brief 根据窗口选择实体
/// @param v1 窗口角点1
/// @param v2 窗口角点2
/// @param select true为选择，false为取消选择
/// @param cross true为从右到左选择（有相交即选择），false为从左到右选择（完全框住才选择）
/// @param entityTypeList 限定实体类型列表
void Selection::selectWindow(const DmVector& v1, const DmVector& v2, bool select, bool cross, std::list<DM::EntityType> const& entityTypeList)
{
	DmVector min(std::min(v1.x, v2.x), std::min(v1.y, v2.y));
	DmVector max(std::max(v1.x, v2.x), std::max(v1.y, v2.y));

	std::list<DM::EntityType>::size_type typeSize = entityTypeList.size();
	bool included = false;
	for (auto e : *pDocument->getEntityTable())
	{
		included = false;
		if (typeSize != 0)
		{
			if (std::find(entityTypeList.begin(), entityTypeList.end(), e->getEntityType()) == entityTypeList.end())
			{
				continue;
			}
		}

		if (!e->isVisible())
		{
			continue;
		}

		// 先用顶层实体包围盒做一次粗过滤，避免全量跑几何相交判断。
		if (e->getMax().x < min.x || e->getMin().x > max.x
			|| e->getMax().y < min.y || e->getMin().y > max.y)
		{
			continue;
		}

		// 完全包含
		if (e->isInWindow(v1, v2))
		{
			included = true;
		}
		// 从右往左选
		else if (cross)
		{
			DmEntityContainer l;
			l.addRectangle(v1, v2);
			DmVectorSolutions sol;

			auto subEntities = e->getSubEntities();
			// 直线，圆弧，Solid，样条线等基本实体
			if (subEntities.size() == 0)
			{
				if (e->getEntityType() == DM::EntityTriangle)
				{
					included = static_cast<DmTriangle*>(e)->isInCrossWindow(v1, v2);
				}
				else if (e->getEntityType() == DM::EntitySolid)
				{
					included = static_cast<DmSolid*>(e)->isInCrossWindow(v1, v2);
				}
				else
				{
					for (auto line : l)
					{
						sol = Information::getIntersection(e, line, true);
						if (sol.hasValid())
						{
							included = true;
							break;
						}
					}
				}
			}
			// 文字等复杂实体，判断子实体是否相交
			else
			{
				for (auto subEnt : subEntities)
				{
					if (subEnt->isInWindow(v1, v2))
					{
						included = true;
					}
					else if (subEnt->getEntityType() == DM::EntityTriangle)
					{
						included = static_cast<DmTriangle*>(subEnt)->isInCrossWindow(v1, v2);
					}
					else if (subEnt->getEntityType() == DM::EntitySolid)
					{
						included = static_cast<DmSolid*>(subEnt)->isInCrossWindow(v1, v2);
					}
					else
					{
						for (auto line : l)
						{
							sol = Information::getIntersection(subEnt, line, true);
							if (sol.hasValid())
							{
								included = true;
								break;
							}
						}
					}

					if (included)
					{
						break;
					}
				}
			}
		}
		else
		{
			// 非交叉模式，不在窗口内则不选中
		}

		if (included)
		{
			e->setSelected(select);
		}
	}

	if (docView)
	{
		docView->specifyDocumentModified();
		docView->redraw();
	}
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

	if (docView)
	{
		docView->specifyDocumentModified();
		docView->redraw();
	}
}

/// @brief 取消选中指定图层的所有实体
/// @param layerName 图层名称
void Selection::deselectLayer(QString& layerName)
{
	selectLayer(layerName, false);
}
