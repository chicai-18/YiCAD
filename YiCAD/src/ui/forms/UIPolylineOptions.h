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

/// @file UIPolylineOptions.h
/// @brief 多段线选项控件

#ifndef UIPOLYLINEOPTIONS_H
#define UIPOLYLINEOPTIONS_H

#include <memory>
#include <QWidget>

class ActionInterface;
class ActionDrawPolyline;
namespace Ui
{
	class Ui_PolylineOptions;
}

/// @brief 多段线小工具条
class UIPolylineOptions : public QWidget
{
	Q_OBJECT

public:
	/// @brief 构造函数
	/// @param parent 父窗口指针
	/// @param fl 窗口标志
	UIPolylineOptions(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

	/// @brief 析构函数
	~UIPolylineOptions();

public slots:
	/// @brief 设置关联的 Action
	/// @param a Action 接口指针
	/// @param update 是否从 Action 更新 UI
	virtual void setAction(ActionInterface* a, bool update);
	virtual void close();
	virtual void undo();
	virtual void updateRadius(const QString& s);
	virtual void updateAngle(const QString& s);
	virtual void updateDirection(bool);
	virtual void updateMode(int m);

	void updateStartLineWeight();
	void updateEndLineWeight();

protected:
	ActionDrawPolyline* action = nullptr; ///< 多段线 Action 指针

protected slots:
	virtual void languageChange();

private:
	void destroy();
	std::unique_ptr<Ui::Ui_PolylineOptions> ui; ///< UI 对象
};

#endif // UIPOLYLINEOPTIONS_H
