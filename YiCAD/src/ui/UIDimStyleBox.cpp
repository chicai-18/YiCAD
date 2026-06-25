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

/// @file UIDimStyleBox.cpp
/// @brief 标注样式选择下拉框控件

#include "UIDimStyleBox.h"
#include "DmDimensionStyle.h"
#include "DmDimensionStyleTable.h"

UIDimStyleBox::UIDimStyleBox(QWidget* parent /*= nullptr*/)
	: QComboBox(parent), m_pStyle(nullptr)
	, m_pDimStyleTable(nullptr)
{
}

DmDimensionStyle* UIDimStyleBox::getStyle()
{
	return m_pStyle;
}

void UIDimStyleBox::setStyle(const QString& style)
{
	setCurrentText(style);
	m_pStyle = m_pDimStyleTable->find(style);
	emit styleChanged();
}

void UIDimStyleBox::init(DmDimensionStyleTable* dimStyleTable, const QString& curStyle)
{
	m_pDimStyleTable = dimStyleTable;
	clear();
	QStringList list;
	for (auto& style : *m_pDimStyleTable)
	{
		list.append(style->getName());
	}
	addItems(list);

	connect(this, SIGNAL(currentTextChanged(const QString&)), this, SLOT(slotStyleChanged(const QString&)));
	DmDimensionStyle* pActive = m_pDimStyleTable->getActive();
	if (pActive)
	{
		if (curStyle.isEmpty())
		{
			int idx = indexOf(pActive);
			setCurrentIndex(idx);
			setStyle(pActive->getName());
		}
		else
		{
			int idx = indexOf(curStyle);
			if (idx != -1)
			{
				setCurrentIndex(idx);
				setStyle(curStyle);
			}
		}
	}
}

int UIDimStyleBox::indexOf(DmDimensionStyle* t)
{
    int index = -1;
    int i = 0;
    for (auto style : *m_pDimStyleTable)
    {
        if (style->getName() == t->getName())
        {
            index = i;
            break;
        }
        i++;
    }
    return index;
}

int UIDimStyleBox::indexOf(const QString& name)
{
    int index = -1;
    int i = 0;
    for (auto style : *m_pDimStyleTable)
    {
        if (style->getName() == name)
        {
            index = i;
            break;
        }
        i++;
    }
    return index;
}

void UIDimStyleBox::slotStyleChanged(const QString& style)
{
	m_pStyle = m_pDimStyleTable->find(style);
	emit styleChanged();
}

