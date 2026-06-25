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


/// @file ActionBlocksCreate.h
/// @brief 从现有实体创建块的动作类头文件

#ifndef ACTIONBLOCKSCREATE_H
#define ACTIONBLOCKSCREATE_H

#include "PreviewActionInterface.h"

/// @brief 从现有实体创建块的动作类
class ActionBlocksCreate : public PreviewActionInterface
{
	Q_OBJECT
public:
	/// @brief 动作状态枚举
	enum EStatus
	{
		eSetReferencePoint, ///< 设置参考点
		eShowDialog         ///< 显示名称对话框
	};

public:
	/// @brief 构造函数
	/// @param doc 文档指针
	/// @param docView 文档视图指针
	ActionBlocksCreate(DmDocument* doc, GuiDocumentView* docView);

	/// @brief 析构函数
	~ActionBlocksCreate() override;

	/// @brief 初始化动作
	/// @param status 状态参数，默认为0
	void init(int status = 0) override;

	/// @brief 检查是否为独占动作
	/// @return true表示独占，false表示非独占
	bool isExclusive() override;

	/// @brief 触发动作执行
	void trigger() override;

	/// @brief 鼠标移动事件处理
	/// @param e 鼠标事件指针
	void mouseMoveEvent(QMouseEvent* e) override;

	/// @brief 鼠标释放事件处理
	/// @param e 鼠标事件指针
	void mouseReleaseEvent(QMouseEvent* e) override;

	/// @brief 坐标事件处理
	/// @param e 坐标事件指针
	void coordinateEvent(GuiCoordinateEvent* e) override;

	/// @brief 更新鼠标按钮提示
	void updateMouseButtonHints() override;

	/// @brief 更新鼠标光标
	void updateMouseCursor() override;

protected:
	std::unique_ptr<DmVector> m_referencePoint; ///< 参考点指针
};

#endif
