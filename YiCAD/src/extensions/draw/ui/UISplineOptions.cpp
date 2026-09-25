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

/// @file UISplineOptions.cpp
/// @brief 样条线选项控件实现

#include "UISplineOptions.h"

#include "DrawSplineCommand.h"
#include "DmSettings.h"
#include "ui_UISplineOptions.h"
#include "Debug.h"

namespace
{
    constexpr int DEFAULT_SPLINE_DEGREE = 3;
}

/// @brief 构造样条线选项控件
/// @param parent 父窗口指针
/// @param fl 窗口标志
UISplineOptions::UISplineOptions(QWidget* parent, Qt::WindowFlags fl)
	: QWidget(parent, fl)
	, ui(new Ui::Ui_SplineOptions{})
{
	ui->setupUi(this);
}

/// @brief 析构函数，保存设置后释放资源
UISplineOptions::~UISplineOptions()
{
	saveSettings();
}

/// @brief 根据当前语言刷新子控件字符串
void UISplineOptions::languageChange()
{
	ui->retranslateUi(this);
}

void UISplineOptions::saveSettings()
{
	DMSETTINGS->beginGroup("/Draw");
	if (command)
	{
		if (dynamic_cast<DrawSplineCommand*>(command))
		{
			DMSETTINGS->writeEntry("/SplineDegree", ui->cbDegree->currentText().toInt());
		}
		DMSETTINGS->writeEntry("/SplineClosed", (int)ui->cbClosed->isChecked());
	}
	DMSETTINGS->endGroup();
}

void UISplineOptions::setCommand(IExclusiveCommand* c, bool update)
{
	command = dynamic_cast<SplineCommand*>(c);
	if (!command)
	{
		return;
	}

	// 控制点样条有阶数，拟合点样条没有
	auto* spline = dynamic_cast<DrawSplineCommand*>(command);
	int degree = DEFAULT_SPLINE_DEGREE;
	bool closed = false;

	if (update)
	{
		if (spline)
		{
			degree = spline->getDegree();
		}
		closed = command->isClosed();
	}
	else
	{
		DMSETTINGS->beginGroup("/Draw");
		if (spline)
		{
			degree = DMSETTINGS->readNumEntry("/SplineDegree", DEFAULT_SPLINE_DEGREE);
			spline->setDegree(degree);
		}
		closed = DMSETTINGS->readNumEntry("/SplineClosed", 0);
		command->setClosed(closed);
		DMSETTINGS->endGroup();
	}
	if (spline)
	{
		ui->cbDegree->setCurrentIndex(ui->cbDegree->findText(QString::number(degree)));
		ui->lDegree->show();
		ui->cbDegree->show();
	}
	else
	{
		ui->lDegree->hide();
		ui->cbDegree->hide();
	}
	ui->cbClosed->setChecked(closed);
}

void UISplineOptions::setClosed(bool c)
{
	if (command)
	{
		command->setClosed(c);
	}
}

void UISplineOptions::undo()
{
	if (command)
	{
		command->undo();
	}
}

void UISplineOptions::setDegree(const QString& deg)
{
	if (auto* spline = dynamic_cast<DrawSplineCommand*>(command))
	{
		spline->setDegree(deg.toInt());
	}
}
