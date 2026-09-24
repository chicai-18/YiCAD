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

/// @file UIInsertOptions.cpp
/// @brief 块插入选项控件实现

#include "UIInsertOptions.h"

#include "BlockInsertCommand.h"
#include "DmSettings.h"
#include "Math2d.h"
#include "ui_UIInsertOptions.h"
#include "Debug.h"

UIInsertOptions::UIInsertOptions(QWidget* parent, Qt::WindowFlags fl)
	: QWidget(parent, fl)
	, ui(new Ui::Ui_InsertOptions{})
{
	ui->setupUi(this);
}

/// @brief 析构函数，保存设置后释放资源
UIInsertOptions::~UIInsertOptions()
{
	saveSettings();
}

/// @brief 根据当前语言刷新子控件字符串
void UIInsertOptions::languageChange()
{
	ui->retranslateUi(this);
}

void UIInsertOptions::saveSettings()
{
	DMSETTINGS->beginGroup("/Insert");
	DMSETTINGS->writeEntry("/InsertAngle", ui->leAngle->text());
	DMSETTINGS->writeEntry("/InsertFactor", ui->leFactor->text());
	DMSETTINGS->writeEntry("/InsertColumns", ui->sbColumns->text());
	DMSETTINGS->writeEntry("/InsertRows", ui->sbRows->text());
	DMSETTINGS->writeEntry("/InsertColumnSpacing", ui->leColumnSpacing->text());
	DMSETTINGS->writeEntry("/InsertRowSpacing", ui->leRowSpacing->text());
	DMSETTINGS->endGroup();
}

void UIInsertOptions::setCommand(IExclusiveCommand* c, bool update)
{
	command = dynamic_cast<BlockInsertCommand*>(c);
	if (command)
	{
		QString sAngle;
		QString sFactor;
		QString sColumns;
		QString sRows;
		QString sColumnSpacing;
		QString sRowSpacing;
		if (update)
		{
			sAngle = QString("%1").arg(Math2d::rad2deg(command->angle()));
			sFactor = QString("%1").arg(command->factor());
			sColumns = QString("%1").arg(command->columns());
			sRows = QString("%1").arg(command->rows());
			sColumnSpacing = QString("%1").arg(command->columnSpacing());
			sRowSpacing = QString("%1").arg(command->rowSpacing());
		}
		else
		{
			DMSETTINGS->beginGroup("/Insert");
			sAngle = DMSETTINGS->readEntry("/InsertAngle", "0.0");
			sFactor = DMSETTINGS->readEntry("/InsertFactor", "1.0");
			sColumns = DMSETTINGS->readEntry("/InsertColumns", "1");
			sRows = DMSETTINGS->readEntry("/InsertRows", "1");
			sColumnSpacing = DMSETTINGS->readEntry("/InsertColumnSpacing", "1.0");
			sRowSpacing = DMSETTINGS->readEntry("/InsertRowSpacing", "1.0");
			DMSETTINGS->endGroup();
		}
		ui->leAngle->setText(sAngle);
		ui->leFactor->setText(sFactor);
		ui->sbColumns->setValue(sColumns.toInt());
		ui->sbRows->setValue(sRows.toInt());
		ui->leColumnSpacing->setText(sColumnSpacing);
		ui->leRowSpacing->setText(sRowSpacing);
	}
}

void UIInsertOptions::updateData()
{
	if (command)
	{
		command->setAngle(Math2d::deg2rad(Math2d::eval(ui->leAngle->text())));
		command->setFactor(Math2d::eval(ui->leFactor->text()));
		command->setColumns(ui->sbColumns->value());
		command->setRows(ui->sbRows->value());
		command->setColumnSpacing(Math2d::eval(ui->leColumnSpacing->text()));
		command->setRowSpacing(Math2d::eval(ui->leRowSpacing->text()));
	}
}
