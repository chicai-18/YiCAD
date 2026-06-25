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

/// @file UIWidthBox.cpp
/// @brief 线宽选择下拉框控件，支持ISO标准线宽及随层/随块选项

#include "UIWidthBox.h"
#include "Debug.h"
#include <QKeyEvent>

UIWidthBox::UIWidthBox(QWidget* parent, const char* name)
	: QComboBox(parent)
	, m_isShowByLayer(false)
{
	setObjectName(name);
}

DM::LineWidth UIWidthBox::getWidth() const 
{
	return m_currentWidth;
}

void UIWidthBox::init(bool m_isShowByLayer)
{
	this->m_isShowByLayer = m_isShowByLayer;

	if (m_isShowByLayer) 
	{
		addItem(QIcon(":/ribbon/line_width/width00.svg"), tr("By Layer"));
		addItem(QIcon(":/ribbon/line_width/width00.svg"), tr("By Block"));
	}
	addItem(QIcon(":/ribbon/line_width/width01.svg"), tr("Default"));
	addItem(QIcon(":/ribbon/line_width/width01.svg"), tr("0.00mm"));
	addItem(QIcon(":/ribbon/line_width/width01.svg"), tr("0.05mm"));
	addItem(QIcon(":/ribbon/line_width/width01.svg"), tr("0.09mm"));
	addItem(QIcon(":/ribbon/line_width/width01.svg"), tr("0.13mm (ISO)"));
	addItem(QIcon(":/ribbon/line_width/width01.svg"), tr("0.15mm"));
	addItem(QIcon(":/ribbon/line_width/width01.svg"), tr("0.18mm (ISO)"));
	addItem(QIcon(":/ribbon/line_width/width01.svg"), tr("0.20mm"));
	addItem(QIcon(":/ribbon/line_width/width01.svg"), tr("0.25mm (ISO)"));
	addItem(QIcon(":/ribbon/line_width/width01.svg"), tr("0.30mm"));
	addItem(QIcon(":/ribbon/line_width/width03.svg"), tr("0.35mm (ISO)"));
	addItem(QIcon(":/ribbon/line_width/width03.svg"), tr("0.40mm"));
	addItem(QIcon(":/ribbon/line_width/width04.svg"), tr("0.50mm (ISO)"));
	addItem(QIcon(":/ribbon/line_width/width05.svg"), tr("0.53mm"));
	addItem(QIcon(":/ribbon/line_width/width05.svg"), tr("0.60mm"));
	addItem(QIcon(":/ribbon/line_width/width06.svg"), tr("0.70mm (ISO)"));
	addItem(QIcon(":/ribbon/line_width/width07.svg"), tr("0.80mm"));
	addItem(QIcon(":/ribbon/line_width/width08.svg"), tr("0.90mm"));
	addItem(QIcon(":/ribbon/line_width/width09.svg"), tr("1.00mm (ISO)"));
	addItem(QIcon(":/ribbon/line_width/width10.svg"), tr("1.06mm"));
	addItem(QIcon(":/ribbon/line_width/width10.svg"), tr("1.20mm"));
	addItem(QIcon(":/ribbon/line_width/width12.svg"), tr("1.40mm (ISO)"));
	addItem(QIcon(":/ribbon/line_width/width12.svg"), tr("1.58mm"));
	addItem(QIcon(":/ribbon/line_width/width12.svg"), tr("2.00mm (ISO)"));
	addItem(QIcon(":/ribbon/line_width/width12.svg"), tr("2.11mm"));
	setMinimumWidth(150);

	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(slotWidthChanged(int)));

	setCurrentIndex(0);
}

void UIWidthBox::mousePressEvent(QMouseEvent* e)
{
    m_userChoose = true;
    QComboBox::mousePressEvent(e);
}

void UIWidthBox::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down)
    {
        m_userChoose = true;
    }
    QComboBox::keyPressEvent(e);
}

void UIWidthBox::setWidth(DM::LineWidth w) 
{
	int offset = m_isShowByLayer ? 2 : 0;

	switch (w) 
	{
	case DM::WidthByLayer:
		if (m_isShowByLayer) 
		{
			setCurrentIndex(0);
		}
		break;
	case DM::WidthByBlock:
		if (m_isShowByLayer) 
		{
			setCurrentIndex(1);
		}
		break;
	case DM::WidthDefault:
		setCurrentIndex(0 + offset);
		break;
	case DM::Width00:
		setCurrentIndex(1 + offset);
		break;
	case DM::Width01:
		setCurrentIndex(2 + offset);
		break;
	case DM::Width02:
		setCurrentIndex(3 + offset);
		break;
	case DM::Width03:
		setCurrentIndex(4 + offset);
		break;
	case DM::Width04:
		setCurrentIndex(5 + offset);
		break;
	case DM::Width05:
		setCurrentIndex(6 + offset);
		break;
	case DM::Width06:
		setCurrentIndex(7 + offset);
		break;
	case DM::Width07:
		setCurrentIndex(8 + offset);
		break;
	case DM::Width08:
		setCurrentIndex(9 + offset);
		break;
	case DM::Width09:
		setCurrentIndex(10 + offset);
		break;
	case DM::Width10:
		setCurrentIndex(11 + offset);
		break;
	case DM::Width11:
		setCurrentIndex(12 + offset);
		break;
	case DM::Width12:
		setCurrentIndex(13 + offset);
		break;
	case DM::Width13:
		setCurrentIndex(14 + offset);
		break;
	case DM::Width14:
		setCurrentIndex(15 + offset);
		break;
	case DM::Width15:
		setCurrentIndex(16 + offset);
		break;
	case DM::Width16:
		setCurrentIndex(17 + offset);
		break;
	case DM::Width17:
		setCurrentIndex(18 + offset);
		break;
	case DM::Width18:
		setCurrentIndex(19 + offset);
		break;
	case DM::Width19:
		setCurrentIndex(20 + offset);
		break;
	case DM::Width20:
		setCurrentIndex(21 + offset);
		break;
	case DM::Width21:
		setCurrentIndex(22 + offset);
		break;
	case DM::Width22:
		setCurrentIndex(23 + offset);
		break;
	case DM::Width23:
		setCurrentIndex(24 + offset);
		break;
	default:
		break;
	}
}

void UIWidthBox::slotWidthChanged(int index) 
{
	if (m_isShowByLayer)
	{
		if (index == 0)
		{
			m_currentWidth = DM::WidthByLayer;
		}
		else if (index == 1)
		{
			m_currentWidth = DM::WidthByBlock;
		}
	}

    switch (index - ((int)m_isShowByLayer * 2))
    {
    case 0:
        m_currentWidth = DM::WidthDefault;
        break;
    case 1:
        m_currentWidth = DM::Width00;
        break;
    case 2:
        m_currentWidth = DM::Width01;
        break;
    case 3:
        m_currentWidth = DM::Width02;
        break;
    case 4:
        m_currentWidth = DM::Width03;
        break;
    case 5:
        m_currentWidth = DM::Width04;
        break;
    case 6:
        m_currentWidth = DM::Width05;
        break;
    case 7:
        m_currentWidth = DM::Width06;
        break;
    case 8:
        m_currentWidth = DM::Width07;
        break;
    case 9:
        m_currentWidth = DM::Width08;
        break;
    case 10:
        m_currentWidth = DM::Width09;
        break;
    case 11:
        m_currentWidth = DM::Width10;
        break;
    case 12:
        m_currentWidth = DM::Width11;
        break;
    case 13:
        m_currentWidth = DM::Width12;
        break;
    case 14:
        m_currentWidth = DM::Width13;
        break;
    case 15:
        m_currentWidth = DM::Width14;
        break;
    case 16:
        m_currentWidth = DM::Width15;
        break;
    case 17:
        m_currentWidth = DM::Width16;
        break;
    case 18:
        m_currentWidth = DM::Width17;
        break;
    case 19:
        m_currentWidth = DM::Width18;
        break;
    case 20:
        m_currentWidth = DM::Width19;
        break;
    case 21:
        m_currentWidth = DM::Width20;
        break;
    case 22:
        m_currentWidth = DM::Width21;
        break;
    case 23:
        m_currentWidth = DM::Width22;
        break;
    case 24:
        m_currentWidth = DM::Width23;
        break;
    default:
        break;
    }

    if (!m_userChoose)
    {
        return;
    }

    // 仅用户UI上操作才触发widthChanged信号
	emit widthChanged(m_currentWidth);
    m_userChoose = false;
}
