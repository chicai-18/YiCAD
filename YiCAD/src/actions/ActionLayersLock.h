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


/// @file ActionLayersLock.h
/// @brief 图层锁定动作类头文件

#ifndef ACTIONLAYERSLOCK_H
#define ACTIONLAYERSLOCK_H

#include "ActionInterface.h"

class DmLayer;

/// @brief 图层锁定/解锁动作类
class ActionLayersLock : public ActionInterface
{
	Q_OBJECT
public:
	/// @brief 构造函数
	/// @param sender 触发动作的按钮对象
	/// @param doc 文档指针
	/// @param docView 文档视图指针
	ActionLayersLock(QObject* sender, DmDocument* doc, GuiDocumentView* docView);

	/// @brief 初始化动作
	/// @param status 状态参数，默认为0
	void init(int status = 0) override;

	/// @brief 触发动作执行
	void trigger() override;

	/// @brief 设置要操作的图层
	/// @param layerName 图层名称
	void setLayer(const QString& layerName);

	/// @brief 设置锁定/解锁状态
	/// @param lock true表示锁定，false表示解锁
	void setToLock(bool lock);

protected:
	DmLayer* layer;       ///< 目标图层指针
	bool     toLock;      ///< 是否锁定
	QObject* theButton;   ///< 触发按钮指针
};

#endif // !ACTIONLAYERSLOCK_H
