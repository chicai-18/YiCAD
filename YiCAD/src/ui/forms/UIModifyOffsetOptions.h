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

/// @file UIModifyOffsetOptions.h
/// @brief 偏移修改选项控件

#ifndef UIMODIFYOFFSETOPTIONS_H
#define UIMODIFYOFFSETOPTIONS_H

#include <memory>
#include <QWidget>

class ActionInterface;
namespace Ui {
	class Ui_ModifyOffsetOptions;
}

class UIModifyOffsetOptions : public QWidget
{
	Q_OBJECT

public:
	/// @brief 构造函数
	/// @param parent 父窗口指针
	/// @param fl 窗口标志
	UIModifyOffsetOptions(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

	/// @brief 析构函数
	~UIModifyOffsetOptions();

public slots:
	/// @brief 更新偏移距离
	/// @param d 距离字符串
	virtual void updateDist(const QString& d);

	/// @brief 设置偏移距离
	/// @param d 距离引用
	/// @param initial 是否从设置初始化
	virtual void setDist(double& d, bool initial = true);

protected:
	double* dist = nullptr; ///< 距离指针

protected slots:
	virtual void languageChange();

private:
	void saveSettings();
	std::unique_ptr<Ui::Ui_ModifyOffsetOptions> ui; ///< UI 对象
};

#endif // UIMODIFYOFFSETOPTIONS_H
