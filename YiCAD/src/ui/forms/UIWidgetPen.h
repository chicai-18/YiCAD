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

/// @file UIWidgetPen.h
/// @brief 画笔属性控件（颜色、线宽、线型）

#ifndef UIWIDGETPEN_H
#define UIWIDGETPEN_H

#include "ui_UIWidgetPen.h"
#include "DmPen.h"

class UIWidgetPen : public QWidget, public Ui::UIWidgetPen
{
	Q_OBJECT

public:
	/// @brief 构造函数
	/// @param parent 父窗口指针
	/// @param fl 窗口标志
	UIWidgetPen(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

	/// @brief 析构函数
	~UIWidgetPen();

public slots:
	/// @brief 设置画笔属性
	/// @param pen 画笔对象
	/// @param showByLayer 是否显示随层选项
	/// @param title 标题
	virtual void setPen(DmPen pen, bool showByLayer, const QString& title);

	/// @brief 获取当前画笔属性
	/// @return 画笔对象
	virtual DmPen getPen();

protected slots:
	virtual void languageChange();

};

#endif // UIWIDGETPEN_H
