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

/// @file UIActionGroupManager.cpp
/// @brief QActionGroup管理器，统一管理工具栏按钮分组、排序和排他性捕捉模式切换

#include "UIActionGroupManager.h"

#include <QActionGroup>

namespace Sorting
{
	bool byObjectName(QActionGroup* left, QActionGroup* right)
	{
		return left->objectName() < right->objectName();
	}
}

UIActionGroupManager::UIActionGroupManager(QObject* parent)
	: QObject(parent)
	, block(new QActionGroup(this))
	, circle(new QActionGroup(this))
	, curve(new QActionGroup(this))
	, edit(new QActionGroup(this))
	, ellipse(new QActionGroup(this))
	, file(new QActionGroup(this))
	, dimension(new QActionGroup(this))
	, info(new QActionGroup(this))
	, layer(new QActionGroup(this))
	, line(new QActionGroup(this))
	, modify(new QActionGroup(this))
	, options(new QActionGroup(this))
	, other(new QActionGroup(this))
	, polyline(new QActionGroup(this))
	, restriction(new QActionGroup(this))
	, select(new QActionGroup(this))
	, snap(new QActionGroup(this))
	, snap_extras(new QActionGroup(this))
	, view(new QActionGroup(this))
	, widgets(new QActionGroup(this))
{
	block->setObjectName(QObject::tr("Block"));
	circle->setObjectName(QObject::tr("Circle"));
	curve->setObjectName(QObject::tr("Curve"));
	edit->setObjectName(QObject::tr("Edit"));
	ellipse->setObjectName(QObject::tr("Ellipse"));
	file->setObjectName(QObject::tr("File"));
	dimension->setObjectName(QObject::tr("Dimension"));
	info->setObjectName(QObject::tr("Info"));
	layer->setObjectName(QObject::tr("Layer"));
	line->setObjectName(QObject::tr("Line"));
	modify->setObjectName(QObject::tr("Modify"));
	options->setObjectName(QObject::tr("Options"));
	other->setObjectName(QObject::tr("Other"));
	polyline->setObjectName(QObject::tr("Polyline"));
	restriction->setObjectName(QObject::tr("Restriction"));
	select->setObjectName(QObject::tr("Select"));
	snap->setObjectName(QObject::tr("Snap"));
	snap_extras->setObjectName(QObject::tr("Snap Extras"));
	view->setObjectName(QObject::tr("View"));
	widgets->setObjectName(QObject::tr("Widgets"));

	foreach(auto ag, findChildren<QActionGroup*>())
	{
		ag->setExclusive(false);
		if (ag->objectName() != QObject::tr("File"))
		{
			connect(parent, SIGNAL(windowsChanged(bool)), ag, SLOT(setEnabled(bool)));
		}
	}

	foreach(auto ag, toolGroups())
	{
		connect(ag, SIGNAL(triggered(QAction*)), parent, SLOT(relayAction(QAction*)));
	}
}

void UIActionGroupManager::sortGroupsByName(QList<QActionGroup*>& list)
{
	std::sort(list.begin(), list.end(), Sorting::byObjectName);
}

QList<QActionGroup*> UIActionGroupManager::toolGroups()
{
	QList<QActionGroup*> ag_list;
	ag_list << block
		<< circle
		<< curve
		<< ellipse
		<< dimension
		<< info
		<< line
		<< modify
		<< other
		<< polyline
		<< select;

	return ag_list;
}

QMap<QString, QActionGroup*> UIActionGroupManager::allGroups()
{
	QList<QActionGroup*> ag_list = findChildren<QActionGroup*>();
	sortGroupsByName(ag_list);

	QMap<QString, QActionGroup*> ag_map;

	foreach(auto ag, ag_list)
	{
		ag_map[ag->objectName()] = ag;
	}

	return ag_map;
}

void UIActionGroupManager::toggleExclusiveSnapMode(bool state)
{
	auto snap_actions = snap->actions();

	QList<bool> temp_memory;

	foreach(auto action, snap_actions)
	{
		temp_memory << action->isChecked();
		if (action->isChecked())
		{
			action->activate(QAction::Trigger);
			action->setChecked(false);
		}
	}

	snap->setExclusive(state);

	if (!snap_memory.isEmpty())
	{
		for (int i = 0; i < snap_actions.size(); ++i)
		{
			if (snap_memory.at(i) == true)
			{
				snap_actions.at(i)->activate(QAction::Trigger);
			}
		}
	}
	snap_memory = temp_memory;
}

void UIActionGroupManager::toggleTools(bool state)
{
	foreach(auto group, toolGroups())
	{
		foreach(auto action, group->actions())
		{
			action->setDisabled(state);
		}
	}
}
