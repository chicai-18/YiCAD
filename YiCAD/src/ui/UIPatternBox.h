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

/// @file UIPatternBox.h
/// @brief 填充图案选择下拉列表框

#ifndef UIPATTERNBOX_H
#define UIPATTERNBOX_H

#include <QComboBox>

class DmPattern;

/// @brief 填充图案选择下拉列表框
class UIPatternBox : public QComboBox 
{
	Q_OBJECT

public:
	UIPatternBox(QWidget* parent = nullptr);
	virtual ~UIPatternBox();

	DmPattern* getPattern();

	void setPattern(const QString& pName);

	void init();

private slots:
	void slotPatternChanged(int index);

signals:
	void patternChanged();

private:
	DmPattern* m_pCurrentPattern = nullptr;
};

#endif
