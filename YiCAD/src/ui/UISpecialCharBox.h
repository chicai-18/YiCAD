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

/// @file UISpecialCharBox.h
/// @brief 特殊符号列表框

#ifndef UISPECIALCHARBOX_H
#define UISPECIALCHARBOX_H

#include <QComboBox>

/// @brief 特殊符号列表框，发送可显示的QString
class UISpecialCharBox : public QComboBox
{
	Q_OBJECT
public:
	UISpecialCharBox(QWidget* parent = nullptr);
	void init();

signals:
	void charActivated(const QString& text);

private slots:
	void slotTextActivated(const QString& text);
};
#endif //!UISPECIALCHARBOX_H