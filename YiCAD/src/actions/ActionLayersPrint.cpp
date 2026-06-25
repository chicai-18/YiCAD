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


/// @file ActionLayersPrint.cpp
/// @brief 图层打印动作类实现文件

#include "ActionLayersPrint.h"
#include "DmLayer.h"
#include "DmDocument.h"
#include "ApplicationWindow.h"
#include "CustomComboboxItem.h"
#include "GuiDocumentView.h"
#include "SARibbonComboBox.h"
#include "MDIWindow.h"
#include "Transaction.h"

/// @brief 构造函数
/// @param sender 发送者对象指针
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionLayersPrint::ActionLayersPrint(QObject* sender, DmDocument* doc, GuiDocumentView* docView)
	: ActionInterface("Print Layer", doc, docView)
	, layer(nullptr)
	, toPrint(false)
	, theButton(sender)
{
}

/// @brief 初始化动作
/// @param status 状态参数，默认为0
void ActionLayersPrint::init(int status)
{
	ActionInterface::init(status);
	trigger();
}

/// @brief 触发动作
void ActionLayersPrint::trigger()
{
	ApplicationWindow* appWin = ApplicationWindow::getAppWindow();
	auto cbxItems = appWin->getLayerComboboxItems();

	// 按钮触发的action，获得按钮所在的图层，判断是否需要打印
	if (theButton && nullptr == layer)
	{
		// 当前图层按钮触发
		ComboBoxData* currentData = appWin->getCurrentLayerItem();
		if (currentData->btnPrint == theButton)
		{
			setLayer(currentData->getLayerName());
			setToPrint(!currentData->isPrint);
		}
		// 图层下拉列表按钮触发
		else
		{
			for (auto& item : cbxItems)
			{
				auto data = item->getData();
				if (data->btnPrint == theButton)
				{
					setLayer(data->getLayerName());
					setToPrint(!data->isPrint);
				}
			}
		}
	}

	// 设置图层打印状态
	if (layer)
	{
		Transaction t(tr("Print Layer").toStdString(), pDocument);
		t.start();
		pDocument->getLayerTable()->startModify(layer);
		layer->setPrint(toPrint);
		t.commit();
	}
	finish();
}

/// @brief 设置目标图层
/// @param layerName 图层名称
void ActionLayersPrint::setLayer(const QString& layerName)
{
	layer = pDocument->getLayerTable()->find(layerName);
}

/// @brief 设置是否打印
/// @param print 是否打印
void ActionLayersPrint::setToPrint(bool print)
{
	toPrint = print;
}
