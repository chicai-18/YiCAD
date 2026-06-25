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

/// @file UIDoubleSpinBox.h
/// @brief 不显示结尾0的QDoubleSpinBox派生控件

#ifndef UIDOUBLESPINBOX_H
#define UIDOUBLESPINBOX_H

#include <QDoubleSpinBox>

/// @class UIDoubleSpinBox
/// @brief 一种不显示结尾0的SpinBox
class UIDoubleSpinBox : public QDoubleSpinBox
{
	Q_OBJECT
public:
	UIDoubleSpinBox(QWidget* parent = nullptr);
	virtual ~UIDoubleSpinBox() = default;
	UIDoubleSpinBox(const UIDoubleSpinBox&) = delete;
	UIDoubleSpinBox& operator=(const UIDoubleSpinBox&) = delete;

	virtual QString textFromValue(double value) const override;
};
#endif