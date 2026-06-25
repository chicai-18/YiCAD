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

/// @file UILinePolygonOptions.h
/// @brief 多边形选项控件

#ifndef UILINEPOLYGONOPTIONS_H
#define UILINEPOLYGONOPTIONS_H

#include <memory>
#include <QWidget>

class ActionInterface;
class ActionDrawLinePolygonCenCor;
namespace Ui {
	class Ui_LinePolygonOptions;
}

class UILinePolygonOptions : public QWidget
{
	Q_OBJECT

public:
	/// @brief 构造函数
	/// @param parent 父窗口指针
	/// @param fl 窗口标志
	UILinePolygonOptions(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

	/// @brief 析构函数
	~UILinePolygonOptions();

public slots:
	/// @brief 设置关联的 Action
	/// @param a Action 接口指针
	/// @param update 是否从 Action 更新 UI
	virtual void setAction(ActionInterface* a, bool update);

	/// @brief 更新多边形边数
	/// @param n 边数
	virtual void updateNumber(int n);

protected:
	ActionDrawLinePolygonCenCor* action = nullptr; ///< 多边形 Action 指针

protected slots:
	virtual void languageChange();

private:
	void saveSettings();
	std::unique_ptr<Ui::Ui_LinePolygonOptions> ui; ///< UI 对象
};

#endif // UILINEPOLYGONOPTIONS_H
