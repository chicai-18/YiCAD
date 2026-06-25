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

/// @file UIDimzerosBox.h
/// @brief 尺寸零抑制选择下拉框控件

#ifndef UIDIMZEROSBOX_H
#define UIDIMZEROSBOX_H

#include <QComboBox>
#include <QStandardItemModel>
#include <QListView>

// DimZin value is mixed integer and bit flag value
// inches and feet are integer values, removal of left and right zeros are flags
// 0: removes 0' & 0"
// 1: draw 0' & 0"
// 2: removes 0"
// 4: removes 0'
// bit 3 set (4) remove 0 to the left
// bit 4 set (8) removes 0's to the right
// DimAZin value is integer or bit flag value
// 0: draw all
// 1: remove 0 to the left
// 2: removes 0's to the right
// 3: removes all zeros

class UIDimzerosBox : public QComboBox
{
	Q_OBJECT

public:
	explicit UIDimzerosBox(QWidget* parent = nullptr);
	~UIDimzerosBox();
	void setLinear();
	void setData(int i);
	int getData();
private:
	int convertDimZin(int v, bool toIdx);

private:
	QStandardItemModel*		m_pModel = nullptr;
	QListView*				m_pView = nullptr;
	bool					m_isDimLine = false;
};

#endif
