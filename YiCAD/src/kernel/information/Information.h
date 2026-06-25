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

/// @file Information.h
/// @brief 实体信息查询类，提供交点计算、可修剪判断、维度判断等功能

#ifndef INFORMATION_H
#define INFORMATION_H

#include "Datamodel.h"

class DmEllipse;
class DmEntity;
class DmEntityContainer;
class DmVector;
class DmVectorSolutions;
class DmArc;
class DmCircle;
class DmLine;

/// @brief 实体信息查询类
/// @details 提供交点计算、可修剪判断、维度实体判断等功能
class Information
{
public:
	/// @brief 构造函数
	/// @param entityContainer 实体容器引用
	Information(DmEntityContainer& entityContainer);

	/// @brief 判断实体类型是否为标注类型
	/// @param type 实体类型
	/// @return true 是标注实体，false 不是
	static bool isDimension(DM::EntityType type);

	/// @brief 判断单个实体是否可修剪
	/// @param e 实体指针
	/// @return true 可修剪，false 不可修剪
	static bool isTrimmable(DmEntity* e);

	/// @brief 判断两个实体是否可互相修剪
	/// @param e1 实体1指针
	/// @param e2 实体2指针
	/// @return true 可互相修剪，false 不可
	static bool isTrimmable(DmEntity* e1, DmEntity* e2);

	/// @brief 计算两实体的交点
	/// @details 支持直线、圆、多段线、圆弧、椭圆、椭圆弧、过点样条
	/// @param e1 实体1指针
	/// @param e2 实体2指针
	/// @param onEntities true仅返回同时在两实体上的交点，false返回曲线方程的交点
	/// @return 交点集合
	static DmVectorSolutions getIntersection(DmEntity const* e1, DmEntity const* e2, bool onEntities = false);

	/// @brief 计算多段线与实体的交点
	/// @param e1 实体1指针
	/// @param e2 实体2指针
	/// @param onEntities 是否限定在实体上
	/// @return 交点集合
	static DmVectorSolutions getIntersectionPolyline(DmEntity const* e1, DmEntity const* e2, bool onEntities = false);

	/// @brief 计算两直线的交点
	/// @param e1 直线1指针
	/// @param e2 直线2指针
	/// @return 交点集合
	static DmVectorSolutions getIntersectionLineLine(DmLine* e1, DmLine* e2);

	/// @brief 计算直线与圆弧的交点
	/// @param line 直线指针
	/// @param arc 圆弧指针
	/// @return 交点集合
	static DmVectorSolutions getIntersectionLineArc(DmLine* line, DmArc* arc);

	/// @brief 计算两圆/圆弧的交点
	/// @details 圆弧当作圆处理，所以圆弧得到的交点不一定在圆弧上
	/// @param e1 实体1指针
	/// @param e2 实体2指针
	/// @return 相切返回1个交点，相交返回2个，不相交返回0个
	static DmVectorSolutions getIntersectionArcArc(DmEntity const* e1, DmEntity const* e2);

	/// @brief 计算两椭圆的交点
	/// @param e1 椭圆1指针
	/// @param e2 椭圆2指针
	/// @return 交点集合
	static DmVectorSolutions getIntersectionEllipseEllipse(DmEllipse const* e1, DmEllipse const* e2);

	/// @brief 计算圆弧与椭圆的交点
	/// @param e1 圆弧指针
	/// @param e2 椭圆指针
	/// @return 交点集合
	static DmVectorSolutions getIntersectionArcEllipse(DmArc* e1, DmEllipse* e2);

	/// @brief 计算圆与椭圆的交点
	/// @param e1 圆指针
	/// @param e2 椭圆指针
	/// @return 交点集合
	static DmVectorSolutions getIntersectionCircleEllipse(DmCircle* e1, DmEllipse* e2);

	/// @brief 计算直线与椭圆的交点
	/// @param line 直线指针
	/// @param ellipse 椭圆指针
	/// @return 交点集合
	static DmVectorSolutions getIntersectionEllipseLine(DmLine* line, DmEllipse* ellipse);

	/// @brief 从4条直线构造四边形
	/// @param container 包含4条直线的实体容器
	/// @return 按角度排序的四边形顶点集合
	static DmVectorSolutions createQuadrilateral(const DmEntityContainer& container);

private:
	DmEntityContainer* container = nullptr; ///< 实体容器指针
};

#endif
