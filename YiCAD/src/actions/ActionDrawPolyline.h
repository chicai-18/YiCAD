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


/// @file ActionDrawPolyline.h
/// @brief 多段线绘制交互动作头文件，处理用户鼠标和键盘事件以绘制多段线

#ifndef ACTIONDRAWPOLYLINE_H
#define ACTIONDRAWPOLYLINE_H

#include "PreviewActionInterface.h"
#include "DmArc.h"
#include "DmLine.h"
#include "DmPolyline.h"

class DmDocument;

/// @brief 多段线绘制过程中的点数据
struct ADPPoints
{
	ArcData arc_data;
	DmPolyline* polyline; ///< 当前正在绘制的多段线实体

	DmVector point; ///< 上一个点
	DmVector start; ///< 起始点，用于闭合功能

	DmVector mouse; ///< 当前鼠标位置

	QList<DmVector> history{}; ///< 点历史记录，用于撤销

	QList<double> bHistory{}; ///< 凸度历史记录，用于撤销
};

/// @brief 多段线绘制交互动作类，处理用户鼠标和键盘事件以绘制多段线
class ActionDrawPolyline : public PreviewActionInterface
{
	Q_OBJECT

public:
	/// @brief 动作状态枚举
	enum Status
	{
		SetStartpoint, ///< 设置起始点
		SetNextPoint   ///< 设置下一个点
	};

	/// @brief 分段模式枚举
	enum SegmentMode
	{
		Line = 0,       ///< 直线
		Tangential = 1, ///< 相切
		TanRad = 2,     ///< 正切半径
		Ang = 3,        ///< 角度
	};

public:
	/// @brief 构造函数
	/// @param doc 文档对象指针
	/// @param docView 文档视图指针
	ActionDrawPolyline(DmDocument* doc, GuiDocumentView* docView);

	/// @brief 析构函数
	~ActionDrawPolyline() override;

	/// @brief 重置绘制状态
	void reset();

	/// @brief 初始化动作状态
	/// @param status 初始状态值，默认为0
	void init(int status = 0) override;

	/// @brief 触发动作完成，将多段线添加到文档
	void trigger() override;

	/// @brief 鼠标移动事件处理
	/// @param e 鼠标事件指针
	void mouseMoveEvent(QMouseEvent* e) override;

	/// @brief 鼠标释放事件处理
	/// @param e 鼠标事件指针
	void mouseReleaseEvent(QMouseEvent* e) override;

	/// @brief 坐标输入事件处理
	/// @param e 坐标事件指针
	void coordinateEvent(GuiCoordinateEvent* e) override;

	/// @brief 命令输入事件处理
	/// @param e 命令事件指针
	void commandEvent(GuiCommandEvent* e) override;

	/// @brief 获取可用命令列表
	/// @return 可用命令的字符串列表
	QStringList getAvailableCommands() override;

	/// @brief 显示选项对话框
	void showOptions() override;

	/// @brief 隐藏选项对话框
	void hideOptions() override;

	/// @brief 更新鼠标按钮提示文本
	void updateMouseButtonHints() override;

	/// @brief 更新鼠标光标样式
	void updateMouseCursor() override;

	/// @brief 闭合多段线
	void close();

	/// @brief 撤销上一个点
	void undo();

	/// @brief 设置分段模式
	/// @param m 分段模式
	void setMode(SegmentMode m);

	/// @brief 获取当前分段模式
	/// @return 当前分段模式
	int getMode() const;

	/// @brief 设置起始线宽
	/// @param start 起始线宽
	void setStartWeight(const double& start);

	/// @brief 获取起始线宽
	/// @return 起始线宽
	double getStartWeight() const;

	/// @brief 设置终止线宽
	/// @param end 终止线宽
	void setEndWeight(const double& end);

	/// @brief 获取终止线宽
	/// @return 终止线宽
	double getEndWeight() const;

	/// @brief 设置圆弧半径
	/// @param r 半径值
	void setRadius(double r);

	/// @brief 获取圆弧半径
	/// @return 半径值
	double getRadius() const;

	/// @brief 设置圆弧角度
	/// @param a 角度值
	void setAngle(double a);

	/// @brief 获取圆弧角度
	/// @return 角度值
	double getAngle() const;

	/// @brief 设置是否逆时针
	/// @param ccw 是否逆时针
	void setCCW(bool ccw);

	/// @brief 获取是否逆时针
	/// @return 是否逆时针
	bool isCCW() const;

	/// @brief 根据鼠标位置计算凸度
	/// @param mouse 鼠标位置
	/// @return 计算得到的凸度值
	double solveBulge(const DmVector& mouse);

private:
	/// @brief 添加下一个点
	/// @param mouse 鼠标位置
	/// @param bulge 凸度
	/// @param startWeight 起始线宽
	/// @param endWeight 终止线宽
	void addNextPoint(const DmVector& mouse,
	                  const double bulge,
	                  const double startWeight,
	                  const double endWeight);

	/// @brief 获得最后一段子实体（圆弧、直线或带线宽的圆弧直线）的切线方向
	/// @return 最后一段的切线方向角度
	double getLastPartDirection() const;

protected:
	double      m_startWeight; ///< 起始线宽
	double      m_endWeight;   ///< 终止线宽
	double      m_dRadius;     ///< 圆弧半径
	double      m_dAngle;      ///< 圆弧角度
	SegmentMode m_Mode;        ///< 当前分段模式
	bool        m_bCCW;        ///< 是否为逆时针
	DmDocument* m_pDocument;   ///< 文档对象指针

	std::unique_ptr<ADPPoints> pPoints; ///< 多段线绘制点数据
};

#endif
