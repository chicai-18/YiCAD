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

/// @file UILineBisectorOptions.h
/// @brief 角平分线选项控件

#ifndef UILINEBISECTOROPTIONS_H
#define UILINEBISECTOROPTIONS_H

#include <memory>
#include <QWidget>

class ActionInterface;
class ActionDrawLineBisector;
namespace Ui {
	class Ui_LineBisectorOptions;
}

class UILineBisectorOptions : public QWidget
{
	Q_OBJECT

public:
	/// @brief 构造函数
	/// @param parent 父窗口指针
	/// @param fl 窗口标志
	UILineBisectorOptions(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

	/// @brief 析构函数
	~UILineBisectorOptions();

public slots:
	/// @brief 设置关联的 Action
	/// @param a Action 接口指针
	/// @param update 是否从 Action 更新 UI
	virtual void setAction(ActionInterface* a, bool update);

	/// @brief 更新角平分线长度
	/// @param l 长度字符串
	virtual void updateLength(const QString& l);

	/// @brief 更新角平分线数量
	/// @param n 数量
	virtual void updateNumber(int n);

protected:
	ActionDrawLineBisector* action = nullptr; ///< 角平分线 Action 指针

protected slots:
	virtual void languageChange();

private:
	void saveSettings();
	std::unique_ptr<Ui::Ui_LineBisectorOptions> ui; ///< UI 对象
};

#endif // UILINEBISECTOROPTIONS_H
