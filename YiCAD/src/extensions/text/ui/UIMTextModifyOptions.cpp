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

/// @file UIMTextModifyOptions.cpp
/// @brief 多行文字修改选项控件实现

#include "UIMTextModifyOptions.h"
#include "ModifyMTextCommand.h"
#include "DmDocument.h"
#include "Math2d.h"

UIMTextModifyOptions::UIMTextModifyOptions(QWidget* parent, Qt::WindowFlags fl)
	: QWidget(parent, fl)
	, m_isDlgShow(false)
	, command(nullptr)
{
	setupUi(this);
}

void UIMTextModifyOptions::setCommand(ModifyMTextCommand* c)
{
	command = c;
	init();
}

void UIMTextModifyOptions::init()
{
	// 文字样式
	connect(cbStyle, SIGNAL(styleChanged()), this, SLOT(slotStyleChanged()));
	DmTextStyleTable* textStyleTable = command->getDocument()->getTextStyleTable();
	cbStyle->init(textStyleTable);

	//其他
	connect(leHeight, SIGNAL(editingFinished()), this, SLOT(slotEditLineEditingFinished()));
	connect(leLineSpaceFactor, SIGNAL(editingFinished()), this, SLOT(slotEditLineEditingFinished()));
	connect(leAngle, SIGNAL(editingFinished()), this, SLOT(slotEditLineEditingFinished()));
	connect(leLineSpace, SIGNAL(editingFinished()), this, SLOT(slotEditLineEditingFinished()));

	updateUIFromData();
}

void UIMTextModifyOptions::updateUIFromData()
{
	cbStyle->setStyle(command->getStyle()->getName());
	leHeight->setText(QString::number(command->getHeight()));
	leLineSpaceFactor->setText(QString::number(command->getLineSpaceFatctor()));
	leLineSpace->setText(QString::number(command->getLineSpace()));
	double angle_deg = Math2d::rad2deg(command->getAngle());
	leAngle->setText(QString::number(angle_deg));
}

void UIMTextModifyOptions::showEvent(QShowEvent* ev)
{
	QWidget::showEvent(ev);
	m_isDlgShow = true;
}

void UIMTextModifyOptions::languageChange()
{
	retranslateUi(this);
}

void UIMTextModifyOptions::slotEditLineEditingFinished()
{
	if (!m_isDlgShow)
	{
		return;
	}

	if (sender() == leHeight)
	{
		double height = leHeight->text().toDouble();
		if (height <= 0.0)
		{
			return;
		}
		command->setHeight(height);
	}
	else if (sender() == leLineSpaceFactor)
	{
		double factor = leLineSpaceFactor->text().toDouble();
		if (factor <= 0.0)
		{
			return;
		}
		command->setLineSpaceFatctor(factor);
		double lineSpace = command->getLineSpace();
		leLineSpace->setText(QString::number(lineSpace));
	}
	else if (sender() == leLineSpace)
	{
		double lineSpace = leLineSpace->text().toDouble();
		command->setLineSpace(lineSpace);
		double factor = command->getLineSpaceFatctor();
		leLineSpaceFactor->setText(QString::number(factor));
	}
	else if (sender() == leAngle)
	{
		double angle_deg = leAngle->text().toDouble();
		command->setAngle(Math2d::deg2rad(angle_deg));
	}
}

void UIMTextModifyOptions::slotStyleChanged()
{
	if (!m_isDlgShow)
	{
		return;
	}
	command->setStyle(cbStyle->getStyle());
}
