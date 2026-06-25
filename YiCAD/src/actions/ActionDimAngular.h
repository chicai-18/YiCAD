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


/// @file ActionDimAngular.h
/// @brief 角度标注操作类头文件

#ifndef ACTIONDIMANGULAR_H
#define ACTIONDIMANGULAR_H

#include "ActionDimension.h"
#include "DmLine.h"

struct DmDimAngularData;

/// @brief 角度标注操作类，处理用户绘制角度标注的交互事件
class ActionDimAngular : public ActionDimension
{
	Q_OBJECT

private:
	/// @brief 操作状态枚举
	enum Status
	{
		SetLine1,  ///< 选择第一条线
		SetLine2,  ///< 选择第二条线
		SetPos,    ///< 选择标注弧线位置
		SetText    ///< 在命令行中设置文本标签
	};

public:
	/// @brief 构造函数
	/// @param [in] doc 文档对象
	/// @param [in] docView 文档视图对象
	ActionDimAngular(DmDocument* doc, GuiDocumentView* docView);

	/// @brief 析构函数
	~ActionDimAngular() override;

	/// @brief 重置操作状态
	void reset() override;

	/// @brief 触发创建角度标注
	void trigger() override;

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

	/// @brief 隐藏选项面板
	void hideOptions() override;

	/// @brief 显示选项面板
	void showOptions() override;

	/// @brief 更新鼠标按钮提示
	void updateMouseButtonHints() override;

private:
	DmLine line1;                              ///< 第一条线
	DmLine line2;                              ///< 第二条线
	DmVector center;                           ///< 标注圆弧中心
	std::unique_ptr<DmDimAngularData> edata;   ///< 角度标注数据
	Status lastStatus;                         ///< 进入文本输入前的上一状态
};

#endif // ACTIONDIMANGULAR_H
