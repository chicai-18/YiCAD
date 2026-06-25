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

/// @file UIWidgetPen.cpp
/// @brief 画笔属性控件实现

#include "UIWidgetPen.h"

#include <QVariant>

#include "UIColorBox.h"
#include "UILineTypeBox.h"

UIWidgetPen::UIWidgetPen(QWidget* parent, Qt::WindowFlags fl)
	: QWidget(parent, fl)
{
	setupUi(this);
}

UIWidgetPen::~UIWidgetPen()
{
	// no need to delete child widgets, Qt does it all for us
}

void UIWidgetPen::setPen(DmPen pen, bool showByLayer, const QString& title)
{
	cbColor->init(showByLayer);
	cbWidth->init(showByLayer);
	cbLineType->init(showByLayer);

	cbColor->setColor(pen.getColor());
	cbWidth->setWidth(pen.getWidth());
	cbLineType->setLineType(pen.getLineType());

	if (!title.isEmpty())
	{
		bgPen->setTitle(title);
	}
}

DmPen UIWidgetPen::getPen()
{
	DmPen pen;

	pen.setColor(cbColor->getColor());
	pen.setWidth(cbWidth->getWidth());
	pen.setLineType(cbLineType->getLineType());

	return pen;
}

void UIWidgetPen::languageChange()
{
	retranslateUi(this);
}
