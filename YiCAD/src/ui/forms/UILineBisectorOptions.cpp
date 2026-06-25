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

/// @file UILineBisectorOptions.cpp
/// @brief 角平分线选项控件实现

#include "UILineBisectorOptions.h"

#include "ActionDrawLineBisector.h"
#include "DmSettings.h"
#include "Math2d.h"
#include "Debug.h"
#include "ui_UILineBisectorOptions.h"

UILineBisectorOptions::UILineBisectorOptions(QWidget* parent, Qt::WindowFlags fl)
	: QWidget(parent, fl)
	, ui(new Ui::Ui_LineBisectorOptions{})
{
	ui->setupUi(this);
}

UILineBisectorOptions::~UILineBisectorOptions()
{
	saveSettings();
}

void UILineBisectorOptions::languageChange()
{
	ui->retranslateUi(this);
}

void UILineBisectorOptions::saveSettings()
{
	DMSETTINGS->beginGroup("/Draw");
	DMSETTINGS->writeEntry("/LineBisectorLength", ui->leLength->text());
	DMSETTINGS->writeEntry("/LineBisectorNumber", ui->sbNumber->text());
	DMSETTINGS->endGroup();
}

void UILineBisectorOptions::setAction(ActionInterface* a, bool update)
{
	if (a && a->getEntityType() == DM::ActionDrawLineBisector)
	{
		action = static_cast<ActionDrawLineBisector*>(a);

		QString sl;
		QString sn;
		if (update)
		{
			sl = QString("%1").arg(action->getLength());
			sn = QString("%1").arg(action->getNumber());
		}
		else
		{
			DMSETTINGS->beginGroup("/Draw");
			sl = DMSETTINGS->readEntry("/LineBisectorLength", "3000.0");
			sn = DMSETTINGS->readEntry("/LineBisectorNumber", "1");
			DMSETTINGS->endGroup();
		}
		ui->leLength->setText(sl);
		ui->sbNumber->setValue(sn.toInt());
	}
	else
	{
		action = nullptr;
	}
}

void UILineBisectorOptions::updateLength(const QString& l)
{
	if (action)
	{
		action->setLength(Math2d::eval(l));
	}
}

void UILineBisectorOptions::updateNumber(int n)
{
	if (action)
	{
		action->setNumber(n);
	}
}
