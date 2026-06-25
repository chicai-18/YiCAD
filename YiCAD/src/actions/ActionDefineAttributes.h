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


/// @file ActionDefineAttributes.h
/// @brief 定义属性操作类头文件

#ifndef ACTIONDEFINEATTRIBUTES_H
#define ACTIONDEFINEATTRIBUTES_H

#include "PreviewActionInterface.h"
#include "TextData.h"
#include "AttributeDefinitionData.h"

/// @brief 定义属性操作类，处理用户定义块属性的交互事件
class ActionDefineAttributes : public PreviewActionInterface
{
	Q_OBJECT

public:
	/// @brief 操作状态枚举
	enum Status
	{
		ShowDialog, ///< 显示文本对话框
		SetPos,     ///< 设置插入点位置
		SetSecPos,  ///< 设置对齐/布满文本的第二点
	};

	/// @brief 坐标点集合
	struct Points
	{
		DmVector pos;    ///< 第一点位置
		DmVector secPos; ///< 第二点位置（对齐/布满模式）
	};

public:
	/// @brief 构造函数
	/// @param [in] doc 文档对象
	/// @param [in] docView 文档视图对象
	ActionDefineAttributes(DmDocument* doc, GuiDocumentView* docView);

	/// @brief 析构函数
	~ActionDefineAttributes() = default;

	/// @brief 初始化操作
	/// @param [in] status 初始状态
	void init(int status = 0) override;

	/// @brief 重置操作状态
	void reset();

	/// @brief 触发创建属性定义
	void trigger() override;

	/// @brief 准备预览图形
	void preparePreview();

	/// @brief 鼠标移动事件处理
	/// @param [in] e 鼠标事件
	void mouseMoveEvent(QMouseEvent* e) override;

	/// @brief 鼠标释放事件处理
	/// @param [in] e 鼠标事件
	void mouseReleaseEvent(QMouseEvent* e) override;

	/// @brief 坐标事件处理
	/// @param [in] e 坐标事件
	void coordinateEvent(GuiCoordinateEvent* e) override;

	/// @brief 命令事件处理
	/// @param [in] e 命令事件
	void commandEvent(GuiCommandEvent* e) override;

	/// @brief 获取可用命令列表
	/// @return 命令字符串列表
	QStringList getAvailableCommands() override;

	/// @brief 更新鼠标按钮提示
	void updateMouseButtonHints() override;

	/// @brief 更新鼠标光标样式
	void updateMouseCursor() override;

private:
	/// @brief 对于只需要1点定位的情况，设置对齐及位置坐标
	void setDataWithOnePoint();

private:
	std::unique_ptr<Points> pPoints;                    ///< 坐标点集合
	std::unique_ptr<TextData> textData;                 ///< 文本数据
	std::unique_ptr<AttributeDefinitionData> attrData;  ///< 属性定义数据
	bool textChanged;                                   ///< 文本是否已变更
};

#endif // ACTIONDEFINEATTRIBUTES_H
