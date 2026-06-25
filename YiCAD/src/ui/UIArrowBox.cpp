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

/// @file UIArrowBox.cpp
/// @brief 箭头样式选择下拉框控件，用于标注中箭头类型的可视化选择

#include "UIArrowBox.h"
#include "DmSettings.h"
UIArrowBox::UIArrowBox(QWidget* parent /*= nullptr*/)
	:QComboBox(parent),m_curArrowType(DM::ArrowType::ClosedFilled)
{
	init();
}

DM::ArrowType UIArrowBox::getArrowType() const
{
	return m_curArrowType;
}

void UIArrowBox::setArrowType(DM::ArrowType arrowType)
{
	int idx = static_cast<int>(arrowType);
	setCurrentIndex(idx);
	m_curArrowType = arrowType;
	emit arrowChanged(arrowType);
}

void UIArrowBox::init()
{
	addItem(QIcon(":/ribbon/arrows/closed_filled.svg"), tr("Closed filled"));
	addItem(QIcon(":/ribbon/arrows/closed_blank.svg"), tr("Closed blank"));
	addItem(QIcon(":/ribbon/arrows/closed.svg"), tr("Closed"));
	addItem(QIcon(":/ribbon/arrows/dot.svg"), tr("Dot"));
	addItem(QIcon(":/ribbon/arrows/architectural_tick.svg"), tr("Architectural tick"));
	addItem(QIcon(":/ribbon/arrows/oblique.svg"), tr("Oblique"));
	addItem(QIcon(":/ribbon/arrows/open.svg"), tr("Open"));
	addItem(QIcon(":/ribbon/arrows/origin_indicator.svg"), tr("Origin indicator"));
	addItem(QIcon(":/ribbon/arrows/origin_indicator2.svg"), tr("Origin indicator2"));
	addItem(QIcon(":/ribbon/arrows/right_angle.svg"), tr("Right angle"));
	addItem(QIcon(":/ribbon/arrows/open30.svg"), tr("Open30"));
	addItem(QIcon(":/ribbon/arrows/dot_small.svg"), tr("Dot small"));
	addItem(QIcon(":/ribbon/arrows/dot_blank.svg"), tr("Dot blank"));
	addItem(QIcon(":/ribbon/arrows/dot_small_blank.svg"), tr("Dot small blank"));
	addItem(QIcon(":/ribbon/arrows/box.svg"), tr("Box"));
	addItem(QIcon(":/ribbon/arrows/box_filled.svg"), tr("Box filled"));
	addItem(QIcon(":/ribbon/arrows/datum_triangle.svg"), tr("Datum triangle"));
	addItem(QIcon(":/ribbon/arrows/datum_triangle_filled.svg"), tr("Datum triangle filled"));
	addItem(QIcon(":/ribbon/arrows/integral.svg"), tr("Integral"));
	//addItem(QIcon(":/ribbon/arrows/none.svg"), tr("None"));

	connect(this, SIGNAL(activated(int)), this, SLOT(slotArrowChanged(int)));
	setCurrentIndex(0);
}

void UIArrowBox::slotArrowChanged(int index)
{
	m_curArrowType = static_cast<DM::ArrowType>(index);
	emit arrowChanged(m_curArrowType);
}

