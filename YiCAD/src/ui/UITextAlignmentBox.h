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

/// @file UITextAlignmentBox.h
/// @brief 文字对齐方式下拉列表

#ifndef UITEXTALIGNMENTBOX_H
#define UITEXTALIGNMENTBOX_H

#include <QComboBox>
#include "TextData.h"

/// @brief 文字对齐方式下拉列表
class UITextAlignmentBox : public QComboBox
{
	Q_OBJECT 
public:
	UITextAlignmentBox(QWidget* parent = nullptr);
	virtual ~UITextAlignmentBox() = default;

	void setAlignment(ETextHorzMode hAlign, ETextVertMode vAlign);
	void getAlignment(ETextHorzMode& hAlign, ETextVertMode& vAlign) const;

	void setAlignment(ETextMode align);
	ETextMode getAlignment() const;
};
#endif	//!UITEXTALIGNMENTBOX_H
