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

#include "DmLine.h"
#include "Information.h"
#include "DmPolyline.h"
#include "DmEntity.h"
#include "DmDocument.h"
#include "DmLayer.h"
#include "DmSolid.h"
#include "DmTriangle.h"

Selection::Selection(DmDocument* doc, IDocumentView* docView)
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

namespace
{
/// @brief 交叉选（从右往左）时判断实体是否与窗口相交
/// @details 基本实体直接与窗口四条边求交；文字等有子实体的复杂实体逐个判断子实体。
///          放在 selectWindow 之外：逐实体的快速判断保持短小，只有需要时才进入这里。
bool crossesWindow(DmEntity* e, const DmVector& v1, const DmVector& v2)
{
	bool included = false;
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
	return included;
}
}  // namespace

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

	DmVector min(std::min(v1.x, v2.x), std::min(v1.y, v2.y));
	DmVector max(std::max(v1.x, v2.x), std::max(v1.y, v2.y));

	std::list<DM::EntityType>::size_type typeSize = entityTypeList.size();

	// 判断单个顶层实体是否被框中，被框中则设置选中状态
	auto selectIfHit = [&](DmEntity* e)
	{
		if (typeSize != 0)
		{
			if (std::find(entityTypeList.begin(), entityTypeList.end(), e->getEntityType()) == entityTypeList.end())
			{
				return;
			}
		}

		if (!e->isVisible() || e->isErased())
		{
			return;
		}

		// 先用顶层实体包围盒做一次粗过滤，避免全量跑几何相交判断。
		if (e->getMax().x < min.x || e->getMin().x > max.x
			|| e->getMax().y < min.y || e->getMin().y > max.y)
		{
			return;
		}

		// 完全包含；从右往左选时与窗口相交也算
		if (e->isInWindow(v1, v2) || (cross && crossesWindow(e, v1, v2)))
		{
			e->setSelected(select);
		}
	};

	// 候选实体的取法（P10）：窗口盖住全部实体的包围框时每个实体都会命中，
	// 顺序遍历实体表最快；否则用空间搜索树只取包围盒与窗口重叠的顶层实体，
	// 取法与点选（Snapper::catchEntity 的 ResolveNone 分支）相同。两条路径对每个
	// 实体做同样的判断，结果一致，只是快慢不同。
	EntityTable* table = pDocument->getEntityTable();
	DmVector allMin, allMax;
	bool coversAll = table->getSearchBounds(allMin, allMax)
		&& min.x <= allMin.x && min.y <= allMin.y && max.x >= allMax.x && max.y >= allMax.y;
	if (coversAll)
	{
		for (auto e : *table)
		{
			selectIfHit(e);
		}
	}
	else
	{
		// 可见性留给 selectIfHit 判断，不让 searchEntities 再过滤一遍
		std::vector<DmEntity*> candidates;
		pDocument->searchEntities(min, max, candidates, false, false);
		for (auto e : candidates)
		{
			selectIfHit(e);
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
