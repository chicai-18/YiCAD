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

/// @file UISnapMiddleOptions.cpp
/// @brief 捕捉中点选项控件实现

#include "UISnapMiddleOptions.h"

#include <QVariant>
#include "ui_UISnapMiddleOptions.h"

namespace
{
    constexpr int MIN_MIDDLE_POINTS = 1;
    constexpr int MAX_MIDDLE_POINTS = 99;
    constexpr int DEFAULT_MIDDLE_POINTS = 3;
}

UISnapMiddleOptions::UISnapMiddleOptions(int& i, QWidget* parent, Qt::WindowFlags fl)
	: QWidget(parent, fl)
	, middlePoints(&i)
	, ui(new Ui::Ui_SnapMiddleOptions())
{
	ui->setupUi(this);
}

UISnapMiddleOptions::~UISnapMiddleOptions()
{
	saveSettings();
}

void UISnapMiddleOptions::languageChange()
{
	ui->retranslateUi(this);
}

void UISnapMiddleOptions::saveSettings()
{
	DMSETTINGS->beginGroup("/Snap");
	DMSETTINGS->writeEntry("/MiddlePoints", *middlePoints);
	DMSETTINGS->endGroup();
}

void UISnapMiddleOptions::setMiddlePoints(int& i, bool initial)
{
	middlePoints = &i;
	if (initial)
	{
		DMSETTINGS->beginGroup("/Snap");
		*middlePoints = DMSETTINGS->readNumEntry("/MiddlePoints", DEFAULT_MIDDLE_POINTS);
		if (!(*middlePoints >= MIN_MIDDLE_POINTS && *middlePoints <= MAX_MIDDLE_POINTS))
		{
			*middlePoints = MIN_MIDDLE_POINTS;
			DMSETTINGS->writeEntry("/MiddlePoints", MIN_MIDDLE_POINTS);
		}
		ui->sbMiddlePoints->setValue(*middlePoints);
		DMSETTINGS->endGroup();
	}
	else
	{
		*middlePoints = ui->sbMiddlePoints->value();
	}
}

void UISnapMiddleOptions::updateMiddlePoints()
{
	if (middlePoints)
	{
		*middlePoints = ui->sbMiddlePoints->value();
	}
}

void UISnapMiddleOptions::on_sbMiddlePoints_valueChanged(int i)
{
	if (middlePoints)
	{
		*middlePoints = i;
	}
}
