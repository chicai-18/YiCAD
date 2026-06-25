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


/// @file ActionDimBaseline.h
/// @brief 基线标注操作类头文件

#ifndef ACTIONDIMBASELINE_H
#define ACTIONDIMBASELINE_H

#include "ActionDimension.h"

class DmDimension;
class DmDimAligned;
class DmDimLinear;
class DmDimAngular;

/// @brief 基线标注操作类，处理用户绘制基线标注的交互事件
class ActionDimBaseline : public ActionDimension
{
	Q_OBJECT

public:
	/// @brief 操作状态枚举
	enum Status
	{
		SelectDim, ///< 选择原标注
		SetPos     ///< 设置定位点
	};

public:
	/// @brief 构造函数
	/// @param [in] doc 文档对象
	/// @param [in] docView 文档视图对象
	ActionDimBaseline(DmDocument* doc, GuiDocumentView* docView);

	/// @brief 析构函数
	~ActionDimBaseline() = default;

	/// @brief 重置操作状态
	void reset() override;

	/// @brief 触发创建基线标注
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

	/// @brief 更新鼠标按钮提示
	void updateMouseButtonHints() override;

private:
	/// @brief 为选择的对齐标注创建基线标注
	/// @param [in] pos 另外一个定位点
	/// @return 新创建的线性标注，失败返回nullptr
	DmDimLinear* createDimForAligned(DmVector pos);

	/// @brief 为选择的线性标注创建基线标注
	/// @param [in] pos 另外一个定位点
	/// @return 新创建的线性标注，失败返回nullptr
	DmDimLinear* createDimForLinear(DmVector pos);

	/// @brief 为选择的角度标注创建基线标注
	/// @param [in] pos 另外一个定位点
	/// @return 新创建的角度标注（暂未实现）
	DmDimAngular* createDimForAngular(DmVector pos);

protected:
	DmDimension* selectedDim;  ///< 选择的原标注
	DmEntity* newEnt;          ///< 新添加的标注
	int addNum;                ///< 第几次添加
};

#endif // ACTIONDIMBASELINE_H
