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

/// @file UIModifyOffsetOptions.cpp
/// @brief 偏移修改选项控件实现

#include "UIModifyOffsetOptions.h"

#include "DmSettings.h"
#include "ui_UIModifyOffsetOptions.h"
#include "Math2d.h"

UIModifyOffsetOptions::UIModifyOffsetOptions(QWidget* parent, Qt::WindowFlags fl)
	: QWidget(parent, fl)
	, ui(new Ui::Ui_ModifyOffsetOptions{})
{
	ui->setupUi(this);
}

UIModifyOffsetOptions::~UIModifyOffsetOptions()
{
	saveSettings();
}

void UIModifyOffsetOptions::languageChange()
{
	ui->retranslateUi(this);
}

void UIModifyOffsetOptions::saveSettings()
{
	DMSETTINGS->beginGroup("/Draw");
	DMSETTINGS->writeEntry("/ModifyOffsetDistance", ui->leDist->text());
	DMSETTINGS->endGroup();
}

void UIModifyOffsetOptions::setDist(double& d, bool initial)
{
	dist = &d;
	bool ok = false;
	if (initial)
	{
		DMSETTINGS->beginGroup("/Draw");
		QString r = DMSETTINGS->readEntry("/ModifyOffsetDistance", "1.0");
		DMSETTINGS->endGroup();

		ui->leDist->setText(r);
		*dist = Math2d::eval(r, &ok);
		if (!ok)
		{
			*dist = 1.;
		}
	}
	else
	{
		*dist = Math2d::eval(ui->leDist->text(), &ok);
		if (!ok)
		{
			*dist = 1.;
		}
	}
}

void UIModifyOffsetOptions::updateDist(const QString& d)
{
	if (dist)
	{
		*dist = Math2d::eval(d);
	}
}
