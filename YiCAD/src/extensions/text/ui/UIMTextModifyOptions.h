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

/// @file UIMTextModifyOptions.h
/// @brief 多行文字修改选项控件

#ifndef UIMTEXTMODIFYOPTIONS_H
#define UIMTEXTMODIFYOPTIONS_H

#include "ui_UIMTextModifyOptions.h"

class ModifyMTextCommand;

class UIMTextModifyOptions : public QWidget, public Ui::Ui_MTextModifyOptions
{
	Q_OBJECT

public:
	/// @brief 构造函数
	/// @param parent 父窗口指针
	/// @param fl 窗口标志
	UIMTextModifyOptions(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
	~UIMTextModifyOptions() = default;

public:
	/// @brief 设置关联的属性面板命令
	/// @param c 多行文字属性面板命令
	virtual void setCommand(ModifyMTextCommand* c);

	/// @brief 初始化控件连接和数据
	void init();

	/// @brief 从数据更新 UI 显示
	void updateUIFromData();

protected:
	/// @brief 窗口显示事件
	/// @param ev 显示事件
	void showEvent(QShowEvent* ev) override;

protected slots:
	virtual void languageChange();

private slots:
	void slotEditLineEditingFinished();
	void slotStyleChanged();

protected:
	ModifyMTextCommand* command = nullptr; ///< 多行文字属性面板命令
	bool m_isDlgShow = false; ///< 窗体是否已显示
};

#endif // !UIMTEXTMODIFYOPTIONS_H
