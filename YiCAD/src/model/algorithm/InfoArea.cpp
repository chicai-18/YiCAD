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

/// @file InfoArea.cpp
/// @brief 面积与周长计算类的实现

#include "InfoArea.h"

#include <cmath>
#include <QPolygon>

#include "Math2d.h"
#include "Debug.h"

/// @brief 多边形顶点数阈值，少于该值不构成多边形
constexpr int MIN_POLYGON_VERTICES = 3;

/// @brief 面积计算因子（1/2）
constexpr double AREA_HALF_FACTOR = 0.5;

InfoArea::InfoArea()
	: isCalculationNeeded(true)
{
}

/// @brief 添加点到点列表，自动记录第一个点的Y坐标为基准
/// @param p 点的坐标
void InfoArea::push_back(const DmVector& p)
{
	if (m_thePoints.empty())
	{
		m_dBaseY = p.y;
	}

	m_thePoints.push_back(p);
	isCalculationNeeded = true;
}

/// @brief 移除最后一个点
void InfoArea::pop_back()
{
	m_thePoints.pop_back();
	isCalculationNeeded = true;
}

/// @brief 重置所有点和计算结果
void InfoArea::reset()
{
	m_thePoints.clear();
	m_dArea = 0.0;
	m_dCircumference = 0.0;
}

/// @brief 检查点是否已存在于轮廓中
/// @param p 待检查的点坐标
/// @return true 表示点重复，false 表示点不在轮廓中
bool InfoArea::duplicated(const DmVector& p)
{
	if (m_thePoints.size() < 1)
	{
		return false;
	}

	for (const DmVector& v : m_thePoints)
	{
		if ((v - p).squared() < DM_TOLERANCE2)
		{
			return true;
		}
	}
	return false;
}

/// @brief 计算面积和周长
void InfoArea::calculate()
{
	m_dArea = 0.0;
	m_dCircumference = 0.0;
	if (m_thePoints.size() < MIN_POLYGON_VERTICES)
	{
		return;
	}

	DmVector p1 = m_thePoints.front();
	for (size_t i = 0; i < m_thePoints.size(); ++i)
	{
		DmVector p2 = m_thePoints.at((i + 1) % m_thePoints.size());
		m_dArea += calcSubArea(p1, p2);
		m_dCircumference += p1.distanceTo(p2);
		p1 = p2;
	}

	m_dArea = AREA_HALF_FACTOR * fabs(m_dArea);
	isCalculationNeeded = false;
}

/// @brief 计算QPolygon的面积（静态方法）
/// @param polygon 多边形
/// @return 面积值
double InfoArea::getArea(const QPolygon& polygon)
{
	double ret = 0.0;
	if (polygon.size() < MIN_POLYGON_VERTICES)
	{
		return ret;
	}

	for (int i = 0; i < polygon.size(); ++i)
	{
		const QPoint& p0 = polygon.at(i);
		const QPoint& p1 = polygon.at((i + 1) % polygon.size());
		ret += p0.x() * p1.y() - p0.y() * p1.x();
	}
	return AREA_HALF_FACTOR * fabs(ret);
}

/// @brief 计算子区域面积（梯形面积的一半）
/// @param p1 第一个点
/// @param p2 第二个点
/// @return 子区域面积值
double InfoArea::calcSubArea(const DmVector& p1, const DmVector& p2)
{
	double width = p2.x - p1.x;
	double height = (p1.y - m_dBaseY) + (p2.y - m_dBaseY);
	return width * height;
}

/// @brief 获取计算后的面积（调用方需确保已调用calculate）
/// @return 面积值
double InfoArea::getArea() const
{
	return m_dArea;
}

/// @brief 获取周长，需要时自动触发计算
/// @return 周长值
double InfoArea::getCircumference()
{
	if (isCalculationNeeded)
	{
		calculate();
	}
	return m_dCircumference;
}

/// @brief 获取点数量
/// @return 点数量
int InfoArea::size()
{
	if (isCalculationNeeded)
	{
		calculate();
	}
	return m_thePoints.size();
}

/// @brief 获取指定索引的点
/// @param i 索引
/// @return 点的常量引用
const DmVector& InfoArea::at(const int i) const
{
	return m_thePoints.at(i);
}
