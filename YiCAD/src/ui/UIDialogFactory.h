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

/// @file UIDialogFactory.h
/// @brief GuiDialogFactoryInterface 的 Qt 实现：通用提示、选项条的摆放、状态栏与命令行反馈，
///        以及内核经它取的活动文档与文件读写
///
/// 业务对话框不在这里：由扩展直接构造（doc/ARCHITECTURE_EVOLUTION_PLAN.md 9.3 节）。

#ifndef UIDIALOGFACTORY_H
#define UIDIALOGFACTORY_H
#include <QPointer>
#include "CommandRegistry.h"
#include "GuiDialogFactoryInterface.h"

class UISnapMiddleOptions;
class QWidget;

class UIBottomWindow;
class UICommandWidget;
class DmDocument;
class DmVector;

#define UIDIALOGFACTORY (GuiDialogFactory::instance()->getFactoryObject())

/// @brief GuiDialogFactoryInterface 的 Qt 实现，由主窗口创建并装进 GuiDialogFactory
class UIDialogFactory: public GuiDialogFactoryInterface
{
public:
	UIDialogFactory(QWidget* parent, QWidget* ow);
	~UIDialogFactory() override;
public:
	// Links this dialog factory to a coordinate widget.
	void setBottomWidget(UIBottomWindow* bw) override;

	// Links this dialog factory to a command widget.
	void setCommandWidget(UICommandWidget* command) override;

	void requestWarningDialog(const QString& warning) override;
	bool requestConfirmDialog(const QString& title, const QString& message) override;
	QString requestUntitledDocumentName(DmDocument* document) override;

	DialogAnswer requestYesNoCancelDialog(const QString& title, const QString& message) override;
	void requestCommandOptions(IExclusiveCommand* command, bool on, bool update = false) override;
	void requestEditModeOptions(const std::function<QWidget*(QWidget*)>& build, bool on) override;

protected:
	// Links factory to a widget that can host tool options.
	void setOptionWidget(QWidget* ow);

	/// @brief 在选项条容器里显示/隐藏一个选项条（命令的或编辑模式的）
	/// @param slot 这类选项条当前显示的控件；先删除它，打开时换成新构造的
	/// @param build 在给定的选项条容器里构造控件
	/// @param height 选项条容器的高度（CommandInfo::commandOptionsHeight）
	void showOptions(QPointer<QWidget>& slot, const std::function<QWidget*(QWidget*)>& build, bool on, int height = 23);

public:
	void requestSnapMiddleOptions(int& middlePoints, bool on) override;

	void updateCoordinateWidget(const DmVector& abs, const DmVector& rel, bool updateFormat=false) override;
	/// @brief updateMouseWidget Called when an action has a mouse hint.
	///	@param left mouse hint for left button
	/// @param right mouse hint for right button
	void updateMouseWidget(const QString& left=QString(), const QString& right=QString()) override;
	/// @brief 更新选中的实体数量
	void updateSelectionWidget(int num) override;
	void commandMessage(const QString& message) override;

	static QString extToFormat(const QString& ext);

protected:
	QWidget*						parent = nullptr;						///< Pointer to the widget which can host dialogs
	QWidget*						optionWidget = nullptr;				///< Pointer to the widget which can host individual tool options
	UICommandWidget*				m_pCommandWidget = nullptr;			///< Pointer to the command line widget
	UIBottomWindow*                 bottomWidget = nullptr;

private:
	// pointers to snap option widgets
	UISnapMiddleOptions*			m_pSnapMiddleOptions = nullptr;
	QPointer<QWidget>				m_pRegisteredOptions;					///< 当前显示的命令选项条
	QPointer<QWidget>				m_pEditModeOptions;						///< 当前显示的编辑模式选项条
};

#endif
