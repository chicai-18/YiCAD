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

/// @file UIColorBox.cpp
/// @brief 颜色选择下拉框控件，支持随层/随块颜色、预设颜色表、自定义颜色和历史颜色记录

#include "UIColorBox.h"
#include "Datamodel.h"
#include <QColorDialog>
#include <QPainter>
#include <QPixmap>
#include "DmColor.h"

#define MAX_HISTORY_COLOR_COUNT 8

// Default Constructor. You must call init manually if you choose to use this constructor.
UIColorBox::UIColorBox(QWidget* parent, const char* name)
	: QComboBox(parent)
	, m_pCurrentColor(new DmColor())
	, m_isShowByLayer(false)
    , m_isChangingByCode(false)
{
	setObjectName(name);
	setEditable(false);
}

UIColorBox::~UIColorBox() 
{
}

void UIColorBox::init(bool m_isShowByLayer)
{
    this->m_isShowByLayer = m_isShowByLayer;
	initItems();
	setMinimumWidth(100);

	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(slotColorChanged(int)));

	if (m_isShowByLayer)
	{
		setCurrentIndex(0);
	}
	else 
	{
		setCurrentIndex(findData(QColor(Qt::black))); // default to Qt::black
	}
}

void UIColorBox::initItems()
{
    clear();
    if (m_isShowByLayer)
    {
        addItem(QIcon(":/ribbon/color/color_white.svg"), tr("By Layer"));
        addItem(QIcon(":/ribbon/color/color_white.svg"), tr("By Block"));
    }

    addItem(QIcon(":/ribbon/color/color_custom.svg"), tr("Custom"));

    QString red(tr("Red"));
    addColor(Qt::red, red);
    m_iColorIndexStart = findText(red); // keep the starting point of static colors
    addColor(Qt::darkRed, tr("Dark Red"));
    addColor(Qt::yellow, tr("Yellow"));
    addColor(Qt::darkYellow, tr("Dark Yellow"));
    addColor(Qt::green, tr("Green"));
    addColor(Qt::darkGreen, tr("Dark Green"));
    addColor(Qt::cyan, tr("Cyan"));
    addColor(Qt::darkCyan, tr("Dark Cyan"));
    addColor(Qt::blue, tr("Blue"));
    addColor(Qt::darkBlue, tr("Dark Blue"));
    addColor(Qt::magenta, tr("Magenta"));
    addColor(Qt::darkMagenta, tr("Dark Magenta"));
    addColor(Qt::white, tr("White"));
    addColor(Qt::gray, tr("Gray"));
    addColor(Qt::darkGray, tr("Dark Gray"));
    addColor(Qt::lightGray, tr("Light Gray"));

    for (auto historyColor : m_historyColors)
    {
        QColor c(historyColor.red(), historyColor.green(),historyColor.blue());
        QString text = QString("%1,%2,%3").arg(historyColor.red()).arg(historyColor.green()).arg(historyColor.blue());
        addColor(c, text);
    }
}

void UIColorBox::addColorToHistory(const DmColor& c)
{
    if (m_historyColors.size() >= MAX_HISTORY_COLOR_COUNT)
    {
        m_historyColors.erase(m_historyColors.begin());
    }
    m_historyColors.emplace_back(c);
}

int UIColorBox::indexOfColor(const DmColor& c)
{
    int cIndex;

    if (c.isByLayer() && m_isShowByLayer)
    {
        cIndex = 0;
    }
    else if (c.isByBlock() && m_isShowByLayer)
    {
        cIndex = 1;
    }
    else
    {
        bool found = false;
        for (cIndex = m_iColorIndexStart; cIndex < count() - 1; cIndex++)
        {
            QColor q(itemData(cIndex).value<QColor>());
            if ((q.red() == c.red()) &&
                (q.green() == c.green()) &&
                (q.blue() == c.blue()))
            {
                found = true;
                break;
            }
        }
        if (found)
        {
            return cIndex;
        }
        else
        {
            return -1;
        }
    }
    return cIndex;
}

void UIColorBox::setColor(const DmColor& color) 
{
    *m_pCurrentColor = color;
    int index = indexOfColor(color);
    if (index == -1)
    {
        addColorToHistory(color);
        index = indexOfColor(color);
        m_isChangingByCode = true;
        initItems();
        setCurrentIndex(index);
        m_isChangingByCode = false;
    }
    else
    {
        m_isChangingByCode = true;
        setCurrentIndex(index);
        m_isChangingByCode = false;
    }

}

void UIColorBox::addColor(QColor color, QString text)
{
    QPixmap pixmap(":/ribbon/color/color_white.svg");
    int w = pixmap.width();
    int h = pixmap.height();
    QPainter painter(&pixmap);
    painter.fillRect(1, 1, w - 2, h - 2, color);
    addItem(QIcon(pixmap), text, color);
}


void UIColorBox::addColor(Qt::GlobalColor color, QString text)
{
    addColor(QColor(color), text);
}

void UIColorBox::slotColorChanged(int index) 
{
    if (m_isChangingByCode)
    {
        return;
    }
	m_pCurrentColor->resetFlags();

	if (m_isShowByLayer) 
	{
        if (index == 0)
        {
            *m_pCurrentColor = DmColor(DM::FlagByLayer);
            emit colorChanged(*m_pCurrentColor);
            return;
        }
        else if (index == 1)
        {
            *m_pCurrentColor = DmColor(DM::FlagByBlock);
            emit colorChanged(*m_pCurrentColor);
            return;
        }
	}

    // 自定义颜色
	if (itemText(index) == tr("Custom"))
	{
		QColor initColor(m_pCurrentColor->red(), m_pCurrentColor->green(), m_pCurrentColor->blue(), m_pCurrentColor->alpha());
		QColor selectedColor = QColorDialog::getColor(initColor, this);
        //确认，将颜色添加到历史并置为当前选择
		if (selectedColor.isValid())
		{
			m_pCurrentColor->setRgb(selectedColor.red(), selectedColor.green(), selectedColor.blue());
			m_pCurrentColor->setAlpha(selectedColor.alpha());
            addColorToHistory(*m_pCurrentColor);
            m_isChangingByCode = true;
            initItems();
            setCurrentIndex(count() - 1);
            m_isChangingByCode = false;
            emit colorChanged(*m_pCurrentColor);
		}
        //取消，当前选择设置为原来的颜色
        else
        {
            m_isChangingByCode = true;
            int index = indexOfColor(*m_pCurrentColor);
            setCurrentIndex(index);
            m_isChangingByCode = false;
        }
	}
    // 其他颜色
	else if (index >= m_iColorIndexStart)
	{
		if (index < count())
		{
			QVariant q0 = itemData(index);
			QColor color;
			if (q0 != QVariant::Invalid)
			{
				color = QColor(itemData(index).value<QColor>());
			}
			else
			{
				color = QColor(Qt::black);
			}
			m_pCurrentColor->setRgba(color.red(), color.green(), color.blue(), color.alpha());
            emit colorChanged(*m_pCurrentColor);
		}
	}
}

DmColor UIColorBox::getColor() const
{
	return *m_pCurrentColor;
}
