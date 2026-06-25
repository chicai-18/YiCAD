/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


/// @file ActionBlockInsertPrepare.cpp
/// @brief 块插入准备动作类实现文件

#include <QDialog>
#include "ActionBlockInsertPrepare.h"
#include "UIBlockListWidget.h"
#include "ApplicationWindow.h"
#include "DmDocument.h"

/// @brief 对话框垂直居中位置系数
constexpr double DIALOG_CENTER_RATIO = 0.5;


/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionBlockInsertPrepare::ActionBlockInsertPrepare(DmDocument* doc,
	GuiDocumentView* docView)
	: ActionInterface("Block Insert Prepare", doc, docView)
	, m_pBlockBack(nullptr)
	, m_pBlockWidget(nullptr)
{
	actionType = DM::ActionBlockInsertPrepare;
}

/// @brief 析构函数
ActionBlockInsertPrepare::~ActionBlockInsertPrepare()
{
	if (m_pBlockBack)
	{
		m_pBlockBack->hide();
	}
}

/// @brief 初始化动作
/// @param status 状态参数，默认为0
void ActionBlockInsertPrepare::init(int status)
{
	ActionInterface::init(status);
	ApplicationWindow* appWin = ApplicationWindow::getAppWindow();
	m_pBlockBack = new QDialog(appWin);
	m_pBlockBack->setWindowTitle(QObject::tr("Block List"));
	m_pBlockBack->setMinimumWidth(300);
	m_pBlockBack->setMaximumWidth(300);
	m_pBlockBack->setMinimumHeight(600);
	m_pBlockBack->setMaximumHeight(600);

	// 计算对话框位置：右侧居中
	int xPos = appWin->width() - m_pBlockBack->width();
	int yPos = (appWin->height() - m_pBlockBack->height()) * DIALOG_CENTER_RATIO;
	m_pBlockBack->move(xPos, yPos);

	m_pBlockWidget = new UIBlockListWidget(
		appWin->getActionHandler(), m_pBlockBack, "Block");
	m_pBlockWidget->setBlockList(pDocument->getBlockTable());
	m_pBlockWidget->resize(m_pBlockBack->size());
	m_pBlockBack->show();
}

/// @brief 检查是否可以中断
/// @return false表示不可以中断
bool ActionBlockInsertPrepare::canBeInterrupt()
{
	return false;
}

/// @brief 检查是否为独占动作
/// @return true表示独占
bool ActionBlockInsertPrepare::isExclusive()
{
	return true;
}

/// @brief 鼠标移动事件处理
/// @param e 鼠标事件指针
void ActionBlockInsertPrepare::mouseMoveEvent(QMouseEvent* e)
{
	// 这样才能绘制鼠标，否则显示像卡顿的效果
	deleteSnapper();
}

/// @brief 鼠标释放事件处理
/// @param e 鼠标事件指针
void ActionBlockInsertPrepare::mouseReleaseEvent(QMouseEvent* e)
{
	finish();
}
