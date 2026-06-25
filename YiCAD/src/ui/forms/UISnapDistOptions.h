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

/// @file UISnapDistOptions.h
/// @brief 捕捉距离选项控件

#ifndef UISNAPDISTOPTIONS_H
#define UISNAPDISTOPTIONS_H

#include <memory>
#include <QWidget>

namespace Ui {
	class Ui_SnapDistOptions;
}

class UISnapDistOptions : public QWidget
{
	Q_OBJECT

public:
	/// @brief 构造函数
	/// @param parent 父窗口指针
	/// @param fl 窗口标志
	UISnapDistOptions(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

	/// @brief 析构函数
	~UISnapDistOptions();

public slots:
	/// @brief 设置距离参数
	/// @param d 距离引用
	/// @param initial 是否从设置初始化
	virtual void setDist(double& d, bool initial = true);

	/// @brief 更新距离
	/// @param d 距离字符串
	virtual void updateDist(const QString& d);

protected:
	double* dist = nullptr; ///< 距离指针

protected slots:
	virtual void languageChange();

private:
	void saveSettings();
	std::unique_ptr<Ui::Ui_SnapDistOptions> ui; ///< UI 对象
};

#endif // UISNAPDISTOPTIONS_H
