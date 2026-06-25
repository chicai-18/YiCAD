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

/// @file UISplineOptions.h
/// @brief 样条线选项控件（控制点/拟合点共享）

#ifndef UISPLINEOPTIONS_H
#define UISPLINEOPTIONS_H

#include <memory>
#include <QWidget>

class ActionInterface;
class ActionDrawSpline;
namespace Ui {
	class Ui_SplineOptions;
}

/// @brief 样条线选项（控制点/拟合点共享）
class UISplineOptions : public QWidget
{
	Q_OBJECT

public:
	/// @brief 构造函数
	/// @param parent 父窗口指针
	/// @param fl 窗口标志
	UISplineOptions(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

	/// @brief 析构函数
	~UISplineOptions();

public slots:
	/// @brief 设置关联的 Action
	/// @param a Action 接口指针
	/// @param update 是否从 Action 更新 UI
	virtual void setAction(ActionInterface* a, bool update);

	/// @brief 设置闭合状态
	/// @param c 是否闭合
	virtual void setClosed(bool c);

	virtual void undo();

	/// @brief 设置样条阶数
	/// @param deg 阶数字符串
	virtual void setDegree(const QString& deg);

protected:
	ActionInterface* action = nullptr; ///< Action 接口指针

protected slots:
	virtual void languageChange();

private:
	void saveSettings();
	std::unique_ptr<Ui::Ui_SplineOptions> ui; ///< UI 对象
};

#endif // UISPLINEOPTIONS_H
