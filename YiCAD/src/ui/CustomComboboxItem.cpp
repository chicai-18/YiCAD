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

/// @file CustomComboboxItem.cpp
/// @brief 自定义组合框项控件，用于图层列表中的开关、锁定、打印和颜色控制

#include "CustomComboboxItem.h"
#include "DmLayer.h"
#include "DmPen.h"
#include "DmColor.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QToolButton>
#include <QPushButton>
#include <QComboBox>


CustomComboboxItem::CustomComboboxItem(QWidget* parent, DmLayer* layer)
	: QWidget(parent)
	, m_layer(layer)
	, m_pItemData(nullptr)
{
}

CustomComboboxItem::~CustomComboboxItem()
{
    if (m_pItemData)
    {
        delete m_pItemData;
        m_pItemData = nullptr;
    }
}

void CustomComboboxItem::setLayerName(const QString& name)
{
	m_pItemData->setLayerName(name);
}

QString CustomComboboxItem::getLayerName()
{
	return m_pItemData->getLayerName();
}

ComboBoxData* CustomComboboxItem::getData() const
{
	return m_pItemData;
}

void CustomComboboxItem::setData(ComboBoxData* data)
{
	m_pItemData = data;
}

void CustomComboboxItem::setIsOn(const bool isOn)
{
	if (isOn != m_pItemData->isOn)
	{
		m_pItemData->setIsOn(isOn);
	}
}

bool CustomComboboxItem::getIsOn()
{
	return m_pItemData->getIsOn();
}

void CustomComboboxItem::setIsLock(const bool isLock)
{
	if (isLock != m_pItemData->isLock)
	{
		m_pItemData->setIsLock(isLock);
	}
}

bool CustomComboboxItem::getIsLock()
{
	return m_pItemData->isLock;
}

void CustomComboboxItem::setIsPrint(const bool isPrint)
{
	if (isPrint != m_pItemData->isPrint)
	{
		m_pItemData->setIsPrint(isPrint);
	}
}

bool CustomComboboxItem::getIsPrint()
{
	return m_pItemData->getIsPrint();
}

void CustomComboboxItem::setColor(const QColor& color)
{
	m_pItemData->setColor(color);
}

QColor CustomComboboxItem::getColor()
{
	return m_pItemData->getColor();
}

void ComboBoxData::setByData(ComboBoxData* data)
{
	bool isOn = data->getIsOn();
	if (getIsOn() != isOn)
	{
		setIsOn(isOn);
	}

	bool isLock = data->getIsLock();
	if (getIsLock() != isLock)
	{
		setIsLock(isLock);
	}

	bool isPrint = data->getIsPrint();
	if (getIsPrint() != isPrint)
	{
		setIsPrint(isPrint);
	}

	QColor color = data->getColor();
	if (getColor() != color)
	{
		setColor(color);
	}

	QString layerName = data->getLayerName();
	if (getLayerName() != layerName)
	{
		setLayerName(layerName);
	}
}

void ComboBoxData::hide()
{
	if (btnOn)
	{
		btnOn->hide();
	}
	if (btnLock)
	{
		btnLock->hide();
	}
	if (btnPrint)
	{
		btnPrint->hide();
	}
	if (btnColor)
	{
		btnColor->hide();
	}
	if (labelName)
	{
		labelName->hide();
	}
	if (btnDelete)
	{
		btnDelete->hide();
	}
}

void ComboBoxData::show()
{
	if (btnOn)
	{
		btnOn->show();
	}
	if (btnLock)
	{
		btnLock->show();
	}
	if (btnPrint)
	{
		btnPrint->show();
	}
	if (btnColor)
	{
		btnColor->show();
	}
	if (labelName)
	{
		labelName->show();
	}
	if (btnDelete)
	{
		btnDelete->show();
	}
}

void ComboBoxData::setIsOn(const bool isOn)
{
	if (isOn)
	{
		btnOn->setIcon(QIcon(":/ribbon/layer/layer_visible.svg"));
	}
	else
	{
		btnOn->setIcon(QIcon(":ribbon/layer/layer_invisible.svg"));
	}
	this->isOn = isOn;
}

bool ComboBoxData::getIsOn()
{
	return isOn;
}

void ComboBoxData::setIsLock(const bool isLock)
{
	if (isLock)
	{
		btnLock->setIcon(QIcon(":/ribbon/layer/layer_lock.svg"));
	}
	else
	{
		btnLock->setIcon(QIcon(":/ribbon/layer/layer_unlock.svg"));
	}
	this->isLock = isLock;
}

bool ComboBoxData::getIsLock()
{
	return isLock;
}

void ComboBoxData::setIsPrint(const bool isPrint)
{
	if (isPrint)
	{
		btnPrint->setIcon(QIcon(":/ribbon/layer/layer_print.svg"));
	}
	else
	{
		btnPrint->setIcon(QIcon(":/ribbon/layer/layer_unprint.svg"));
	}
	this->isPrint = isPrint;
}

bool ComboBoxData::getIsPrint()
{
	return isPrint;
}

void ComboBoxData::setColor(const QColor& color)
{
	btnColor->setStyleSheet(
		"background:rgb(" + QString::number(color.red()) + ","
		+ QString::number(color.green()) + ","
		+ QString::number(color.blue()) + ");padding: -2px; border-width: 1px; border-style: solid; border-color: black;");
	rgb = QColor(color.red(), color.green(), color.blue());
}

QColor ComboBoxData::getColor()
{
	return rgb;
}

void ComboBoxData::setLayerName(const QString& name)
{
	//labelName->setText(name);
	strName = name;

	QFontMetrics elidfont(labelName->font());
	labelName->setText(elidfont.elidedText(name, Qt::ElideRight, labelName->width()));
	labelName->setToolTip(name);
	//设置名字按钮的样式以及超长字符的省略号
	constexpr int kLabelNameWidth = 75;
	constexpr int kLabelNameHeight = 20;
	labelName->setMinimumSize(kLabelNameWidth, kLabelNameHeight);
	labelName->setMaximumSize(kLabelNameWidth, kLabelNameHeight);
}

QString ComboBoxData::getLayerName()
{
	return strName;
}
