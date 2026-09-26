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

/// @file UISnapMiddleOptions.h
/// @brief 捕捉中点选项控件

#ifndef UISNAPMIDDLEOPTIONS_H
#define UISNAPMIDDLEOPTIONS_H

#include <memory>
#include <QWidget>

namespace Ui
{
	class Ui_SnapMiddleOptions;
}

class UISnapMiddleOptions : public QWidget
{
	Q_OBJECT

public:
	/// @brief 构造函数
	/// @param i 中点数量引用
	/// @param parent 父窗口指针
	/// @param fl 窗口标志
	UISnapMiddleOptions(int& i, QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

	/// @brief 析构函数
	~UISnapMiddleOptions();

public slots:
	/// @brief 设置中点数量
	/// @param i 中点数量引用
	/// @param initial 是否从设置初始化
	virtual void setMiddlePoints(int& i, bool initial = true);

	/// @brief 从中点 SpinBox 更新中点数量
	virtual void updateMiddlePoints();

	virtual void languageChange();

	/// @brief SpinBox 值变更槽
	/// @param arg1 新值
	void on_sbMiddlePoints_valueChanged(int arg1);

private:
	void saveSettings();

protected:
	int* middlePoints = nullptr; ///< 中点数量指针

private:
	std::unique_ptr<Ui::Ui_SnapMiddleOptions> ui; ///< UI 对象
};

#endif // UISNAPMIDDLEOPTIONS_H
