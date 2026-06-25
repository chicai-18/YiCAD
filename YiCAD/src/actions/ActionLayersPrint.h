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


/// @file ActionLayersPrint.h
/// @brief 图层打印动作类头文件

#ifndef ACTIONLAYERSPRINT_H
#define ACTIONLAYERSPRINT_H

#include "ActionInterface.h"

class DmLayer;

/// @brief 图层打印动作类
class ActionLayersPrint : public ActionInterface
{
	Q_OBJECT
public:
	/// @brief 构造函数
	/// @param sender 发送者对象指针
	/// @param doc 文档指针
	/// @param docView 文档视图指针
	ActionLayersPrint(QObject* sender, DmDocument* doc, GuiDocumentView* docView);

	/// @brief 初始化动作
	/// @param status 状态参数，默认为0
	void init(int status = 0) override;

	/// @brief 触发动作
	void trigger() override;

	/// @brief 设置目标图层
	/// @param layerName 图层名称
	void setLayer(const QString& layerName);

	/// @brief 设置是否打印
	/// @param print 是否打印
	void setToPrint(bool print);

protected:
	DmLayer* layer;		///< 目标图层指针
	bool toPrint;		///< 是否打印标志
	QObject* theButton;	///< 触发按钮对象指针
};

#endif //!ACTIONLAYERSPRINT_H
