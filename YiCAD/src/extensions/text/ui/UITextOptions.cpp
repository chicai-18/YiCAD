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

/// @file UITextOptions.cpp
/// @brief 文字选项控件实现

#include "UITextOptions.h"

#include "DrawTextCommand.h"
#include "Math2d.h"
#include "ui_UITextOptions.h"
#include "Debug.h"

UITextOptions::UITextOptions(QWidget* parent, Qt::WindowFlags fl)
	: QWidget(parent, fl)
	, ui(new Ui::Ui_TextOptions{})
{
	ui->setupUi(this);
}

/// @brief 析构函数（默认实现）
UITextOptions::~UITextOptions() = default;

/// @brief 根据当前语言刷新子控件字符串
void UITextOptions::languageChange()
{
	ui->retranslateUi(this);
}

void UITextOptions::setCommand(IExclusiveCommand* c, bool update)
{
	command = dynamic_cast<DrawTextCommand*>(c);
	if (command)
	{
		QString st;
		QString sa;
		if (update)
		{
			st = command->text();
			sa = QString("%1").arg(Math2d::rad2deg(command->angle()));
		}
		else
		{
			st = "";
			sa = "0.0";
		}

		ui->teText->setText(st);
		ui->leAngle->setText(sa);
	}
}

void UITextOptions::updateText()
{
	if (command)
	{
		command->setText(ui->teText->toPlainText());
	}
}

void UITextOptions::updateAngle()
{
	if (command)
	{
		command->setAngle(Math2d::deg2rad(Math2d::eval(ui->leAngle->text())));
	}
}
