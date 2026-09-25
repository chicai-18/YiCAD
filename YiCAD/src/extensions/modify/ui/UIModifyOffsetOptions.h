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

class IExclusiveCommand;
class ModifySingleOffsetCommand;
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
	/// @brief 设置关联的命令
	/// @param c 命令；类型不符时视为没有命令
	/// @param update true 从命令取距离，false 从配置文件取距离并交给命令
	virtual void setCommand(IExclusiveCommand* c, bool update);

	/// @brief 更新偏移距离
	/// @param d 距离字符串
	virtual void updateDist(const QString& d);

protected:
	ModifySingleOffsetCommand* command = nullptr; ///< 命令

protected slots:
	virtual void languageChange();

private:
	void saveSettings();
	std::unique_ptr<Ui::Ui_ModifyOffsetOptions> ui; ///< UI 对象
};

#endif // UIMODIFYOFFSETOPTIONS_H
