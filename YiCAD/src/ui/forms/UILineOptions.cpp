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

/// @file UILineOptions.cpp
/// @brief 两点画线交互设置实现

#include "UILineOptions.h"

#include "ActionDrawLine.h"
#include "ui_UILineOptions.h"
#include "Debug.h"

UILineOptions::UILineOptions(QWidget* parent, Qt::WindowFlags fl)
	: QWidget(parent, fl)
	, ui(new Ui::Ui_LineOptions())
{
	ui->setupUi(this);
}

UILineOptions::~UILineOptions() = default;

void UILineOptions::languageChange()
{
	ui->retranslateUi(this);
}

void UILineOptions::setAction(ActionInterface* a)
{
	if (a && a->getEntityType() == DM::ActionDrawLine)
	{
		action = static_cast<ActionDrawLine*>(a);
	}
	else
	{
		action = nullptr;
	}
}

void UILineOptions::close()
{
	if (action)
	{
		action->close();
	}
}

void UILineOptions::undo()
{
	if (action)
	{
		action->undo();
	}
}

void UILineOptions::redo()
{
	if (action)
	{
		action->redo();
	}
}
