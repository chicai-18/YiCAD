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

/// @file UILinePolygonOptions.cpp
/// @brief 多边形选项控件实现

#include "UILinePolygonOptions.h"

#include "DrawLinePolygonCommand.h"
#include "DmSettings.h"
#include "ui_UILinePolygonOptions.h"
#include "Debug.h"

/// @brief 构造多边形选项控件
/// @param parent 父窗口指针
/// @param fl 窗口标志
UILinePolygonOptions::UILinePolygonOptions(QWidget* parent, Qt::WindowFlags fl)
	: QWidget(parent, fl)
	, ui(new Ui::Ui_LinePolygonOptions{})
{
	ui->setupUi(this);
}

/// @brief 析构函数，保存设置后释放资源
UILinePolygonOptions::~UILinePolygonOptions()
{
	saveSettings();
}

/// @brief 根据当前语言刷新子控件字符串
void UILinePolygonOptions::languageChange()
{
	ui->retranslateUi(this);
}

void UILinePolygonOptions::saveSettings()
{
	DMSETTINGS->beginGroup("/Draw");
	DMSETTINGS->writeEntry("/LinePolygonNumber", ui->sbNumber->text());
	DMSETTINGS->endGroup();
}

void UILinePolygonOptions::setCommand(IExclusiveCommand* c, bool update)
{
	command = dynamic_cast<LinePolygonCommand*>(c);
	if (command)
	{
		QString sn;
		if (update)
		{
			sn = QString("%1").arg(command->getNumber());
		}
		else
		{
			DMSETTINGS->beginGroup("/Draw");
			sn = DMSETTINGS->readEntry("/LinePolygonNumber", "3");
			DMSETTINGS->endGroup();
		}
		ui->sbNumber->setValue(sn.toInt());
	}
}

void UILinePolygonOptions::updateNumber(int n)
{
	if (command)
	{
		command->setNumber(n);
	}
}
