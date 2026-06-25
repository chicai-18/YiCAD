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

/// @file InfoArea.h
/// @brief 面积与周长计算类，用于计算多边形区域的面积和周长

#ifndef INFOAREA_H
#define INFOAREA_H

#include <vector>

#include "DmVector.h"

class QPolygon;

/// @brief 面积与周长计算类，用于计算多边形区域的面积和周长
class InfoArea
{
public:
	InfoArea();

	/// @brief 重置所有点和计算结果
	void reset();

	/// @brief 添加点到点列表
	/// @param p 点的坐标
	void push_back(const DmVector& p);

	/// @brief 检查点是否已存在于轮廓中
	/// @param p 待检查的点坐标
	/// @return true 表示点重复，false 表示点不在轮廓中
	bool duplicated(const DmVector& p);

	/// @brief 移除最后一个点
	void pop_back();

	/// @brief 获取计算后的面积
	/// @return 面积值
	double getArea() const;

	/// @brief 获取周长，需要时自动计算
	/// @return 周长值
	double getCircumference();

	/// @brief 获取点数量
	/// @return 点数量
	int size();

	/// @brief 获取指定索引的点
	/// @param i 索引
	/// @return 点的常量引用
	const DmVector& at(const int i) const;

	/// @brief 计算QPolygon的面积（静态方法）
	/// @param polygon 多边形
	/// @return 面积值
	static double getArea(const QPolygon& polygon);

private:
	/// @brief 计算面积和周长
	void calculate();

	/// @brief 计算子区域面积
	/// @param p1 第一个点
	/// @param p2 第二个点
	/// @return 子区域面积
	double calcSubArea(const DmVector& p1, const DmVector& p2);

private:
	std::vector<DmVector> m_thePoints;          ///< 点列表
	double m_dBaseY = 0.0;                       ///< 基准Y坐标
	double m_dArea = 0.0;                        ///< 面积
	double m_dCircumference = 0.0;               ///< 周长
	bool isCalculationNeeded = true;             ///< 是否需要重新计算
};

#endif
