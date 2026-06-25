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


/// @file DmRect.cpp
/// @brief DmRect/Area 类的实现，包括矩形几何运算和单元测试

#include<iostream>
#include <QDebug>
#include <cassert>
#include "DmRect.h"

#define INTERT_TEST(s) qDebug()<<"\ntesting " #s; assert(s); qDebug()<<"Passed";

using namespace geo;

Coordinate DmRect::Vector(Coordinate const& p, Coordinate const& q)
{
	return {q.x - p.x, q.y - p.y};
}

DmRect::Area(const Coordinate& coordA, const Coordinate& coordB) 
    : _minP(std::min(coordA.x, coordB.x), std::min(coordA.y, coordB.y))
    , _maxP(std::max(coordA.x, coordB.x), std::max(coordA.y, coordB.y))
{
}

DmRect::Area() 
    : _minP(0., 0.)
    , _maxP(0., 0.) 
{
}

DmRect::Area(const Coordinate& coord, double width, double height)
    : Area(coord, {coord.x + width, coord.y + height})
{
}

const Coordinate& DmRect::minP() const
{
    return _minP;
}

const Coordinate& DmRect::maxP() const
{
    return _maxP;
}

Coordinate DmRect::upperLeftCorner() const
{
  return Coordinate(_minP.x, _maxP.y);
}

Coordinate DmRect::upperRightCorner() const
{
  return _maxP;
}

Coordinate DmRect::lowerRightCorner() const
{
  return Coordinate(_maxP.x, _minP.y);
}

Coordinate DmRect::lowerLeftCorner() const
{
  return _minP;
}

double DmRect::width() const
{
    return _maxP.x - _minP.x;
}

double DmRect::height() const
{
    return _maxP.y - _minP.y;
}

bool DmRect::inArea(const Coordinate& point, double tolerance) const
{
    return (point.x >= _minP.x - tolerance && point.x <= _maxP.x + tolerance && point.y >= _minP.y - tolerance && point.y <= _maxP.y + tolerance);
}

bool DmRect::inArea(const Area& area) const
{
    return _minP.x >= area._minP.x && _minP.y >= area._minP.y && _maxP.x <= area._maxP.x && _maxP.y <= area._maxP.y;
}

bool DmRect::overlaps(const Area& otherArea) const
{
    return intersects(otherArea);
}

short DmRect::numCornersInside(const Area& otherArea) const
{
    short pointsInside = 0;

    if (otherArea.inArea(_minP))
    {
        pointsInside++;
    }

    if (otherArea.inArea(_maxP))
    {
        pointsInside++;
    }

    if (otherArea.inArea(upperLeftCorner()))
    {
        pointsInside++;
    }

    if (otherArea.inArea(lowerRightCorner()))
    {
        pointsInside++;
    }

    return pointsInside;
}

Area DmRect::merge(const Area& other) const
{
    return {{std::min(other.minP().x, this->minP().x), std::min(other.minP().y, this->minP().y)},
               {std::max(other.maxP().x, this->maxP().x), std::max(other.maxP().y, this->maxP().y)}};
}

Area DmRect::merge(const Coordinate& other) const
{
    return {{std::min(other.x, this->minP().x), std::min(other.y, this->minP().y)},
            {std::max(other.x, this->maxP().x), std::max(other.y, this->maxP().y)}};
}

Area DmRect::intersection(const Area& other, double tolerance) const
{
  Area const ret{{std::max(other.minP().x, this->minP().x), std::max(other.minP().y, this->minP().y)},
      {std::min(other.maxP().x, this->maxP().x), std::min(other.maxP().y, this->maxP().y)}};

    if (ret.width() < tolerance || ret.height() < tolerance)
    {
        return {};
    }
    return ret;
}

bool DmRect::intersects(Area const& rhs, double tolerance) const
{
    return maxP().x + tolerance >= rhs.minP().x && maxP().y + tolerance >= rhs.minP().y 
        && rhs.maxP().x + tolerance >= minP().x && rhs.maxP().y + tolerance >= minP().y;
}

Coordinate DmRect::top() const
{
    return Vector(upperLeftCorner(), _maxP);
}

Coordinate DmRect::bottom() const
{
    return Vector(_minP, lowerRightCorner());
}

Coordinate DmRect::left() const
{
    return Vector(_minP, upperLeftCorner());
}

Coordinate DmRect::right() const
{
    return Vector(lowerRightCorner(), _maxP);
}

Area DmRect::increaseBy(double increaseBy) const
{
    return {_minP - increaseBy, _maxP + increaseBy};
}

std::array<Coordinate, 4> DmRect::vertices() const
{
    return {{lowerLeftCorner(), lowerRightCorner(), upperRightCorner(), upperLeftCorner()}};
}

void DmRect::unitTest()
{
	DmRect const rect0{{0., 0.}, {1., 1.}};
	DmRect const rect1{{0.5, 0.5}, {1.5, 1.5}};
	DmRect const rect2{{1.5, 1.5}, {2.5, 2.5}};
	DmRect const rect3{{0.0, 1.}, {1.5, 1.5}};

	//intersects() tests
	INTERT_TEST(rect0.intersects(rect1))
	INTERT_TEST(rect1.intersects(rect0))
	INTERT_TEST(rect1.intersects(rect2))
	INTERT_TEST(rect2.intersects(rect1))

	INTERT_TEST(!rect0.intersects(rect2))
	INTERT_TEST(!rect2.intersects(rect0))

	INTERT_TEST(rect2.intersects(rect3))
	INTERT_TEST(rect3.intersects(rect2))

	// inArea() tests
	INTERT_TEST(rect0.inArea({0.1, 0.1}))
	INTERT_TEST(rect0.inArea({0.5, 0.5}))

	INTERT_TEST(!rect0.inArea({1.1, 1.1}))
	INTERT_TEST(!rect0.inArea({-1.1, -1.1}))
}
