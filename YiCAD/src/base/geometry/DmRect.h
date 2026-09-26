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


/// @file DmRect.h
/// @brief 二维矩形区域类，提供面积判断、合并、相交等几何运算

#ifndef DMRECT_H
#define DMRECT_H
#include <array>

#include "DmVector.h"

namespace geo
{
typedef DmVector Coordinate;

/// @brief 描述二维矩形窗口或区域的类
class Area
	{
	public:
		Area();
		/// @brief Create a new Area. The coordinates coordA and coordB will be ordered so that minP will always be < maxP
		///	The coordinates are not allowed to describe a volume
		/// @param CoordA First coordinate of an area
		/// @param CoordB Second coordinate of an area
		Area(const Coordinate& coordA, const Coordinate& coordB);

		/// @brief  given at a coordinate with a given width and height
		explicit Area(const Coordinate& coord, double width, double height);

		// Return the smallest corner (closest to (0,0,0) )
		const Coordinate& minP() const;
		// Return the highest corner
		const Coordinate& maxP() const;

		/// @brief topLeftCorner return the upperLeftCorner coordinates _minP is considered lowerLeft, _maxP is the upperRight
		///	@return {_minP.x, _maxP.y}
		Coordinate upperLeftCorner() const;
		Coordinate upperRightCorner() const;

		/// @brief lowerRightCorner return the lowerRight coordinates _minP is considered lowerLeft, _maxP is the upperRigh
		/// @return {_maxP.x, _minP.y}
		Coordinate lowerLeftCorner() const;
		Coordinate lowerRightCorner() const;

		double width() const;
		double height() const;

		/// @brief Test of a specific point lies within an area
		///	@param point Point to test against
		/// @return boolean true of the point is within the area
		bool inArea(const Coordinate& point, double tolerance = 0.) const;

		/// @brief test if this object's fit's fully in area
		bool inArea(const Area& area) const;

		/// @brief returns true if any overlap is happening between the two area's, even if otherArea fit's within this area
		bool overlaps(const Area& otherArea) const;

		/// @brief count the number of corners this object has in otherArea
		short numCornersInside(const Area& otherArea) const;

		/// @brief two area's and expand if required to largest containing area
		Area merge(const Area& other) const;

		/// @brief two area's and expand if required to largest containing area
		Area merge(const Coordinate& other) const;

		/// @brief two area's and expand if required to largest containing area
		/// @param tolerance, tolerance to detect zero size intersection
		Area intersection(const Area& other, double tolerance = 0.) const;

		/// @brief intersects whether two rectangular area
		///	if the closest distance between two Areas is smaller than tolerance, they are considered to have intersection
		/// @param rhs the other rect
		/// @return true if closest distance is smaller than or equal to tolerance
		bool intersects(Area const& rhs, double tolerance = 0.) const;

		Coordinate top() const;
		Coordinate bottom() const;
		Coordinate left() const;
		Coordinate right() const;

		// Increase the area on each side by increaseBy
		Area increaseBy(double increaseBy) const;

		/// @brief vertices generate vertices of the rectangular area
		///	@return array of vertices by the order {ll, lr, ur, rl} starting with lower-left corner, i.e. _minP
		std::array<Coordinate, 4> vertices() const;

		static void unitTest();

	private:
		static Coordinate Vector(Coordinate const& p, Coordinate const& q);

	private:
		Coordinate _minP;
		Coordinate _maxP;
	};
}

using DmRect = geo::Area;

#endif  // DMRECT_H
