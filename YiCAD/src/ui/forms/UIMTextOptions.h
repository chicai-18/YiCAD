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

/// @file UIMTextOptions.h
/// @brief 多行文字选项控件

#ifndef UIMTEXTOPTIONS_H
#define UIMTEXTOPTIONS_H

#include "ui_UIMTextOptions.h"
#include "EntityDataDef.h"
#include "DmMTextParagraph.h"

class ActionDrawMText;
class ActionDrawMTextContext;
class ActionInterface;

class UIMTextOptions : public QWidget, public Ui::Ui_MTextOptions
{
	Q_OBJECT

public:
	/// @brief 构造函数
	/// @param parent 父窗口指针
	/// @param fl 窗口标志
	UIMTextOptions(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
	~UIMTextOptions() = default;

public:
	/// @brief 设置关联的 Action
	/// @param a Action 接口指针
	/// @param update 是否从 Action 更新 UI
	virtual void setAction(ActionInterface* a, bool update);

	/// @brief 初始化控件连接和数据
	void init();

	/// @brief 询问用户是否确认修改文字样式
	/// @return 用户确认结果
	static bool queryChangeItem();

protected:
	/// @brief 窗口显示事件
	/// @param ev 显示事件
	void showEvent(QShowEvent* ev);

	/// @brief 窗口离开事件
	/// @param event 离开事件
	virtual void leaveEvent(QEvent* event);

protected slots:
	virtual void languageChange();

private slots:
	void updateUIFromData();
	void slotUpdateUndoUI(bool undoable, bool redoable);
	void slotEditLineEditingFinished();
	void slotStyleChanged();
	void slotCboFontChanged(const QString& font);
	void slotColorChanged(const DmColor& color);
	void slotCboSymbolActivated(const QString& symbol);
	void slotBtnToggled(bool checked);
	void slotJustifyChanged(const QString& text);
	void slotParaAlignmentChanged(const QString& text);
	void slotCaseChange();
	void slotSpinBoxValueChanged(double d);

private:
	void initFonts();
	QString justifyToTranslateString(EMTextMode mode);
	EMTextMode translateStringToJustify(const QString& str);
	QString alignmentToTranslateString(DmMTextParagraph::Alignment align);
	DmMTextParagraph::Alignment translateStringToAlignment(const QString& str);

protected:
	ActionDrawMText* action = nullptr;          ///< 画多行文字 Action 指针
	ActionDrawMTextContext* context = nullptr;  ///< 多行文字上下文指针
	bool m_isDlgShow = false;                   ///< 窗体是否已显示
	bool m_isChangingStyle = false;             ///< 是否在修改文字样式，修改文字样式时可能会同步修改"粗体""倾斜"，为了避免多次触发修改添加该变量
	bool m_isChangingSpinBoxValue = false;      ///< 是否正在修改SpinBox，避免在更新到UI时又触发更新到文字
	bool m_isChangingComboBoxValue = false;     ///< 是否正在修改ComboBox，避免在更新到UI时又触发更新到文字
};

#endif // !UIMTEXTOPTIONS_H
