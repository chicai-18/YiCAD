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

/// @file UISnapDistOptions.cpp
/// @brief 捕捉距离选项控件实现

#include "UISnapDistOptions.h"

#include <QVariant>
#include "Math2d.h"
#include "ui_UISnapDistOptions.h"

/// @brief 构造捕捉距离选项控件
/// @param parent 父窗口指针
/// @param fl 窗口标志
UISnapDistOptions::UISnapDistOptions(QWidget* parent, Qt::WindowFlags fl)
	: QWidget(parent, fl)
	, ui(new Ui::Ui_SnapDistOptions{})
{
	ui->setupUi(this);
}

/// @brief 析构函数，保存设置后释放资源
UISnapDistOptions::~UISnapDistOptions()
{
	saveSettings();
}

/// @brief 根据当前语言刷新子控件字符串
void UISnapDistOptions::languageChange()
{
	ui->retranslateUi(this);
}

void UISnapDistOptions::saveSettings()
{
	DMSETTINGS->beginGroup("/Snap");
	DMSETTINGS->writeEntry("/Distance", ui->leDist->text());
	DMSETTINGS->endGroup();
}

void UISnapDistOptions::setDist(double& d, bool initial)
{
	dist = &d;
	if (initial)
	{
		DMSETTINGS->beginGroup("/Snap");
		QString r = DMSETTINGS->readEntry("/Distance", "1.0");
		DMSETTINGS->endGroup();

		ui->leDist->setText(r);
		*dist = r.toDouble();
	}
	else
	{
		*dist = ui->leDist->text().toDouble();
	}
}

void UISnapDistOptions::updateDist(const QString& d)
{
	if (dist)
	{
		*dist = Math2d::eval(d, 1.0);
	}
}
