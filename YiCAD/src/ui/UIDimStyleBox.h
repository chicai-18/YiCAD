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

/// @file UIDimStyleBox.h
/// @brief 标注样式选择下拉框控件

#ifndef UIDIMSTYLEBOX_H
#define UIDIMSTYLEBOX_H

#include <QComboBox>

class DmDimensionStyle;
class DmDimensionStyleTable;

class UIDimStyleBox : public QComboBox
{
	Q_OBJECT
public:
	UIDimStyleBox(QWidget* parent = nullptr);
	virtual ~UIDimStyleBox() = default;

public:
	DmDimensionStyle* getStyle();
	void setStyle(const QString& style);

	void init(DmDimensionStyleTable* styleList, const QString& curStyle);
    int indexOf(DmDimensionStyle* t);
    int indexOf(const QString& name);

private slots:
	void slotStyleChanged(const QString& text);
signals:
	void styleChanged();
private:
	DmDimensionStyle*		m_pStyle = nullptr;
	DmDimensionStyleTable*	m_pDimStyleTable = nullptr;
};
#endif 