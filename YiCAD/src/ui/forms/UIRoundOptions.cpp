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

/// @file UIRoundOptions.cpp
/// @brief 倒圆角选项控件实现

#include "UIRoundOptions.h"

#include "ActionModifyRound.h"
#include "DmSettings.h"
#include "Math2d.h"
#include "ui_UIRoundOptions.h"
#include "Debug.h"

UIRoundOptions::UIRoundOptions(QWidget* parent, Qt::WindowFlags fl)
	: QWidget(parent, fl)
	, ui(new Ui::Ui_RoundOptions{})
{
	ui->setupUi(this);
}

/// @brief 析构函数，保存设置后释放资源
UIRoundOptions::~UIRoundOptions()
{
	saveSettings();
}

/// @brief 根据当前语言刷新子控件字符串
void UIRoundOptions::languageChange()
{
	ui->retranslateUi(this);
}

void UIRoundOptions::saveSettings()
{
	DMSETTINGS->beginGroup("/Modify");
	DMSETTINGS->writeEntry("/RoundRadius", ui->leRadius->text());
	DMSETTINGS->writeEntry("/RoundTrim", (int)ui->cbTrim->isChecked());
	DMSETTINGS->endGroup();
}

void UIRoundOptions::setAction(ActionInterface* a, bool update)
{
	if (a && a->getEntityType() == DM::ActionModifyRound)
	{
		action = static_cast<ActionModifyRound*>(a);

		QString sr;
		QString st;
		if (update)
		{
			sr = QString("%1").arg(action->getRadius());
			st = QString("%1").arg((int)action->isTrimOn());
		}
		else
		{
			DMSETTINGS->beginGroup("/Modify");
			sr = DMSETTINGS->readEntry("/RoundRadius", "1.0");
			st = DMSETTINGS->readEntry("/RoundTrim", "1");
			DMSETTINGS->endGroup();
		}
		ui->leRadius->setText(sr);
		ui->cbTrim->setChecked(st == "1");
	}
	else
	{
		action = nullptr;
	}
}

void UIRoundOptions::updateData()
{
	if (action)
	{
		action->setTrim(ui->cbTrim->isChecked());
		action->setRadius(Math2d::eval(ui->leRadius->text()));
	}
}
