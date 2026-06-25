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


/// @file DmVector.cpp
/// @brief DmVector/DmVectorSolutions 向量类的实现

#include "DmVector.h"

#include <iostream>
#include <cmath>
#include <algorithm>

#include "Debug.h"
#include "Math2d.h"
#include "DmRect.h"
#include "KDTree.h"

DmVector DmVector::NormalX = DmVector(1.0, 0.0, 0.0);
DmVector DmVector::NormalNegativeX = DmVector(-1.0, 0.0, 0.0);
DmVector DmVector::NormalY = DmVector(0.0, 1.0, 0.0);
DmVector DmVector::NormalNegativeY = DmVector(0.0, -1.0, 0.0);
DmVector DmVector::NormalZ = DmVector(0.0, 0.0, 1.0);
DmVector DmVector::NormalNegativeZ = DmVector(0.0, 0.0, -1.0);

// Constructor for a point with given coordinates.
DmVector::DmVector(double vx, double vy, double vz)
	: x(vx)
	, y(vy)
	, z(vz)
	, valid(true)
{
}

// Constructor for a unit m_vectors with given angle
DmVector::DmVector(double angle)
	: x(cos(angle))
	, y(sin(angle))
	, valid(true)
{
}

/// Constructor for a point with given valid flag.
///	@param valid true: a valid m_vectors with default coordinates is created. false: an invalid m_vectors is created
DmVector::DmVector(bool valid)
	: valid(valid)
{
}

DmVector::operator bool() const
{
	return valid;
}

// Sets to a unit m_vectors by the direction angle
void DmVector::set(double angle)
{
	x = cos(angle);
	y = sin(angle);
	z = 0.;
	valid = true;
}

// Sets a new position for the m_vectors.
void DmVector::set(double vx, double vy, double vz)
{
	x = vx;
	y = vy;
	z = vz;
	valid = true;
}

// Sets a new position for the m_vectors in polar coordinates.
void DmVector::setPolar(double radius, double angle)
{
	x = radius * cos(angle);
	y = radius * sin(angle);
	z = 0.0;
	valid = true;
}

DmVector DmVector::polar(double rho, double theta)
{
	return DmVector(rho * cos(theta), rho * sin(theta), 0.);
}

double DmVector::angle() const
{
	return Math2d::correctAngle(atan2(y, x));
}

double DmVector::angleTo(const DmVector& v) const
{
	if (!valid || !v.valid)
	{
		return 0.0;
	}
	return (v - (*this)).angle();
}

double DmVector::angleToDir(const DmVector& dir) const
{
	double angle2 = dir.angle();
	double angle1 = angle();
	return Math2d::correctAngle(angle2 - angle1);
}

double DmVector::magnitude() const
{
	double ret(0.0);
	if (valid)
	{
		ret = hypot(hypot(x, y), z);
	}
	return ret;
}

/// @return square of m_vectors length
double DmVector::squared() const
{
	if (valid)
	{
		return x * x + y * y + z * z;
	}
	return DM_MAXDOUBLE;
}

/// @return square of m_vectors length
double DmVector::squaredTo(const DmVector& v1) const
{
	if (valid && v1.valid)
	{
		return  (*this - v1).squared();
	}
	return DM_MAXDOUBLE;
}

DmVector DmVector::lerp(const DmVector& v, double t) const
{
	return { x + (v.x - x) * t, y + (v.y - y) * t };
}

DmVector DmVector::normalize() const
{
	return (*this) / (this->magnitude());
}

/// @return The distance between this and the given coordinate.
double DmVector::distanceTo(const DmVector& v) const
{
	if (!valid || !v.valid)
	{
		return DM_MAXDOUBLE;
	}
	else
	{
		return (*this - v).magnitude();
	}
}

/// @return true is this m_vectors is within the given range.
bool DmVector::isInWindow(const DmVector& firstCorner, const DmVector& secondCorner) const
{
	if (!valid)
	{
		return false;
	}
	return DmRect{ firstCorner, secondCorner }.inArea(*this);
}

/// @return true is this m_vectors is within the given range of ordered vectors
bool DmVector::isInWindowOrdered(const DmVector& vLow,
	const DmVector& vHigh) const
{
	if (!valid)
	{
		return false;
	}
	return (x >= vLow.x && x <= vHigh.x && y >= vLow.y && y <= vHigh.y);
}

// move to the closest integer point
DmVector DmVector::toInteger()
{
	x = rint(x);
	y = rint(y);
	return *this;
}

// Moves this m_vectors by the given offset. Equal to the operator +=.
DmVector DmVector::move(const DmVector& offset)
{
	*this += offset;
	return *this;
}

// Rotates this m_vectors around 0/0 by the given angle.
DmVector DmVector::rotate(double ang)
{
	rotate(DmVector{ ang });
	return *this;
}

// Rotates this m_vectors around 0/0 by the given m_vectors
// if the m_vectors is a unit, then, it's the same as rotating around 0/0 by the angle of the m_vectors
DmVector DmVector::rotate(const DmVector& angleVector)
{
	double x0 = x * angleVector.x - y * angleVector.y;
	y = x * angleVector.y + y * angleVector.x;
	x = x0;

	return *this;
}

// Rotates this m_vectors around the given center by the given angle.
DmVector DmVector::rotate(const DmVector& center, double ang)
{
	*this = center + (*this - center).rotate(ang);
	return *this;
}
DmVector DmVector::rotate(const DmVector& center, const DmVector& angleVector)
{
	*this = center + (*this - center).rotate(angleVector);
	return *this;
}

// Scales this m_vectors by the given factors with 0/0 as center.
DmVector DmVector::scale(double factor)
{
	x *= factor;
	y *= factor;
	return *this;
}

// Scales this m_vectors by the given factors with 0/0 as center.
DmVector DmVector::scale(const DmVector& factor)
{
	x *= factor.x;
	y *= factor.y;
	return *this;
}

DmVector DmVector::scale(const DmVector& factor) const
{
	return DmVector(x * factor.x, y * factor.y);
}

// Scales this m_vectors by the given factors with the given center.
DmVector DmVector::scale(const DmVector& center, const DmVector& factor)
{
	*this = center + (*this - center).scale(factor);
	return *this;
}

// Mirrors this m_vectors at the given axis, defined by two points on axis.
DmVector DmVector::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
	DmVector direction(axisPoint2 - axisPoint1);
	double a = direction.squared();
	DmVector ret(false);
	if (a < DM_TOLERANCE2)
	{
		return ret;
	}
	ret = axisPoint1 + direction * dotP(*this - axisPoint1, direction) / a; // projection point
	*this = ret + ret - *this;

	return *this;
}

std::ostream& operator << (std::ostream& os, const DmVector& v)
{
	if (v.valid)
	{
		os << v.x << "/" << v.y << "/" << v.z;
	}
	else
	{
		os << "invalid m_vectors";
	}
	return os;
}

// binary + operator.
DmVector DmVector::operator + (const DmVector& v) const
{
	return DmVector(x + v.x, y + v.y, z + v.z);
}

// binary - operator.
DmVector DmVector::operator - (const DmVector& v) const
{
	return DmVector(x - v.x, y - v.y, z - v.z);
}

DmVector DmVector::operator + (double d) const
{
	return DmVector(x + d, y + d, z + d);
}

DmVector DmVector::operator - (double d) const
{
	return DmVector(x - d, y - d, z - d);
}

DmVector DmVector::operator * (const DmVector& v) const
{
	return DmVector(x * v.x, y * v.y, z * v.z);
}

DmVector DmVector::operator / (const DmVector& v) const
{
	if (fabs(v.x) > DM_TOLERANCE && fabs(v.y) > DM_TOLERANCE)
	{
		return DmVector(x / v.x, y / v.y, std::isnormal(v.z) ? z / v.z : z);
	}

	return *this;
}

// binary * operator.
DmVector DmVector::operator * (double s) const
{
	return DmVector(x * s, y * s, z * s);
}

// binary/operator.
DmVector DmVector::operator / (double s) const
{
	if (fabs(s) > DM_TOLERANCE)
	{
		return DmVector(x / s, y / s, z / s);
	}
	return *this;
}

// unary - operator.
DmVector DmVector::operator - () const
{
	return DmVector(-x, -y, -z);
}

// Scalarproduct (dot product).
double DmVector::dotP(const DmVector& v1) const
{
	return x * v1.x + y * v1.y;
}

double DmVector::dotP(const DmVector& v1, const DmVector& v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

double DmVector::getDimension(int dim) const
{
    switch (dim) {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        default:
            return std::numeric_limits<double>::quiet_NaN();
    }
}

/// @brief Get position of \p pos in line \p start -> \p end, as a factor of line length.
///	@param start Start point of the line
/// @param end End point of the line
/// @param pos Point to calculate
/// @return double factor of line length,
///         factor == 0.0 : \p pos is same as start point
///         factor == 1.0 : \p pos is same as end point
///         factor < 0.0 : \p pos is in opposite direction
///         factor > 1.0 : \p pos is behind end point
///         factor > 0.0 and < 1.0 : \p pos is somewhere between start and end
double DmVector::posInLine(const DmVector& start, const DmVector& end, const DmVector& pos)
{
	DmVector dirEnd = end - start;
	DmVector dirPos = pos - start;
	double lenSquared = dirEnd.squared();

	if (DM_TOLERANCE2 > lenSquared)
	{
		// line too short
		return start.distanceTo(pos);
	}

	return dotP(dirPos, dirEnd) / lenSquared;
}

// switch x,y for all vectors
DmVector DmVector::flipXY(void) const
{
	return DmVector(y, x);
}

// += operator. Assert: both vectors must be valid.
DmVector DmVector::operator += (const DmVector& v)
{
	x += v.x;
	y += v.y;
	z += v.z;
	return *this;
}

//  -= operator
DmVector DmVector::operator -= (const DmVector& v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	return *this;
}

DmVector DmVector::operator *= (const DmVector& v)
{
	x *= v.x;
	y *= v.y;
	z *= v.z;
	return *this;
}

DmVector DmVector::operator /= (const DmVector& v)
{
	if (fabs(v.x) > DM_TOLERANCE && fabs(v.y) > DM_TOLERANCE)
	{
		x /= v.x;
		y /= v.y;
		if (std::isnormal(v.z))
		{
			z /= v.z;
		}
	}
	return *this;
}

// *= operator
DmVector DmVector::operator *= (double s)
{
	x *= s;
	y *= s;
	z *= s;
	return *this;
}

// /= operator
DmVector DmVector::operator /= (double s)
{
	if (fabs(s) > DM_TOLERANCE)
	{
		x /= s;
		y /= s;
		z /= s;
	}
	return *this;
}

// == operator
bool DmVector::operator == (const DmVector& v) const
{
	return (x == v.x && y == v.y && z == v.z && valid && v.valid);
}

bool DmVector::operator!=(const DmVector& v) const
{
	return !operator==(v);
}

bool DmVector::operator == (bool valid) const
{
	return this->valid == valid;
}

bool DmVector::operator != (bool valid) const
{
	return this->valid != valid;
}

/// @return A m_vectors with the minimum components from the vectors v1 and v2.
///	These might be mixed components from both vectors.
DmVector DmVector::minimum(const DmVector& v1, const DmVector& v2)
{
	if (!v2)
	{
		return v1;
	}
	if (!v1)
	{
		return v2;
	}
	return DmVector(std::min(v1.x, v2.x),std::min(v1.y, v2.y),std::min(v1.z, v2.z));
}

/// @return A m_vectors with the maximum values from the vectors v1 and v2
DmVector DmVector::maximum(const DmVector& v1, const DmVector& v2)
{
	if (!v2)
	{
		return v1;
	}
	if (!v1)
	{
		return v2;
	}
	return DmVector(std::max(v1.x, v2.x),std::max(v1.y, v2.y),std::max(v1.z, v2.z));
}

/// @return Cross product of two vectors.
///	we don't need cross product for 2D vectors
DmVector DmVector::crossP(const DmVector& v1, const DmVector& v2)
{
	return DmVector(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
}


// Constructor for no solution.
DmVectorSolutions::DmVectorSolutions() 
	: m_vectors(0)
	, m_bTangent(false)
{
}

DmVectorSolutions::DmVectorSolutions(const std::vector<DmVector>& l) 
	: m_vectors(l.begin(), l.end())
	, m_bTangent(false)
{
}

// Constructor for num solutions.
DmVectorSolutions::DmVectorSolutions(int num) 
	: m_vectors(num, DmVector(false))
	, m_bTangent(false)
{
}

DmVectorSolutions::DmVectorSolutions(std::initializer_list<DmVector> const& l) 
	: m_vectors(l)
	, m_bTangent(false)
{
}

// Allocates 'num' vectors.
void DmVectorSolutions::alloc(size_t num)
{
	if (num <= m_vectors.size())
	{
		m_vectors.resize(num);
	}
	else
	{
		const std::vector<DmVector> v(num - m_vectors.size());
		m_vectors.insert(m_vectors.end(), v.begin(), v.end());
	}
}

DmVector DmVectorSolutions::get(size_t i) const
{
	if (i < m_vectors.size())
	{
		return m_vectors.at(i);
	}
	return {};
}

const DmVector& DmVectorSolutions::operator [] (const size_t i) const
{
	return m_vectors[i];
}

DmVector& DmVectorSolutions::operator [] (const size_t i)
{
	return m_vectors[i];
}

size_t DmVectorSolutions::size() const
{
	return m_vectors.size();
}

// Deletes m_vectors array and resets everything.
void DmVectorSolutions::clear()
{
	m_vectors.clear();
	m_bTangent = false;
}

/// @return m_vectors solution number i or an invalid m_vectors if there are less solutions.
const DmVector& DmVectorSolutions::at(size_t i) const
{
	return m_vectors.at(i);
}

/// @return Number of solutions available.
size_t DmVectorSolutions::getNumber() const
{
	return m_vectors.size();
}

/// @retval true There's at least one valid solution.
///	@retval false There's no valid solution.
bool DmVectorSolutions::hasValid() const
{
	for (const DmVector& v : m_vectors)
	{
		if (v.valid)
		{
			return true;
		}
	}

	return false;
}

void DmVectorSolutions::resize(size_t n)
{
	m_vectors.resize(n);
}

const std::vector<DmVector>& DmVectorSolutions::getVector() const
{
	return m_vectors;
}

std::vector<DmVector>::const_iterator DmVectorSolutions::begin() const
{
	return m_vectors.begin();
}

std::vector<DmVector>::const_iterator DmVectorSolutions::end() const
{
	return m_vectors.end();
}

std::vector<DmVector>::iterator DmVectorSolutions::begin()
{
	return m_vectors.begin();
}

std::vector<DmVector>::iterator DmVectorSolutions::end()
{
	return m_vectors.end();
}

void DmVectorSolutions::push_back(const DmVector& v)
{
	m_vectors.push_back(v);
}

void DmVectorSolutions::removeAt(const size_t i)
{
	if (m_vectors.size() > i)
	{
		m_vectors.erase(m_vectors.begin() + i);
	}
}

DmVectorSolutions& DmVectorSolutions::push_back(const DmVectorSolutions& v)
{
	m_vectors.insert(m_vectors.end(), v.begin(), v.end());
	return *this;
}

void DmVectorSolutions::set(size_t i, const DmVector& v)
{
	if (i < m_vectors.size())
	{
		m_vectors[i] = v;
	}
	else
	{
		for (size_t j = m_vectors.size(); j <= i; ++j)
		{
			m_vectors.push_back(v);
		}
	}
}

// Sets the m_bTangent flag.
void DmVectorSolutions::setTangent(bool t)
{
	m_bTangent = t;
}

/// @return true if at least one of the solutions is a double solution (m_bTangent).
bool DmVectorSolutions::isTangent() const
{
	return m_bTangent;
}

// Rotates all vectors around (0,0) by the given angle.
void DmVectorSolutions::rotate(double ang)
{
	DmVector angleVector(ang);
	for (auto& vp : m_vectors)
	{
		if (vp.valid)
		{
			vp.rotate(angleVector);
		}
	}
}

// Rotates all vectors around (0,0) by the given angleVector.
void DmVectorSolutions::rotate(const DmVector& angleVector)
{
	for (auto& vp : m_vectors)
	{
		if (vp.valid)
		{
			vp.rotate(angleVector);
		}
	}
}

// Rotates all vectors around the given center by the given angle.
void DmVectorSolutions::rotate(const DmVector& center, double ang)
{
	const DmVector angleVector(ang);
	for (auto& vp : m_vectors)
	{
		if (vp.valid)
		{
			vp.rotate(center, angleVector);
		}
	}
}

void DmVectorSolutions::rotate(const DmVector& center, const DmVector& angleVector)
{
	for (auto& vp : m_vectors)
	{
		if (vp.valid)
		{
			vp.rotate(center, angleVector);
		}
	}
}

// Move all vectors around the given center by the given m_vectors.
void DmVectorSolutions::move(const DmVector& vp)
{
	for (DmVector& v : m_vectors)
	{
		if (v.valid)
		{
			v.move(vp);
		}
	}
}

// Scales all vectors by the given factors with the given center.
void DmVectorSolutions::scale(const DmVector& center, const DmVector& factor)
{
	for (auto& vp : m_vectors)
	{
		if (vp.valid)
		{
			vp.scale(center, factor);
		}
	}
}

void DmVectorSolutions::scale(const DmVector& factor)
{
	for (auto& vp : m_vectors)
	{
		if (vp.valid)
		{
			vp.scale(factor);
		}
	}
}

void DmVectorSolutions::distinct(const double tol)
{
	if (size() < 2)
		return;

	// 方法1
	KDTree<DmVector> tree;
	std::vector<DmVector> pts;
	tree.buildTree(pts);
	for (auto pt : m_vectors)
	{
		auto closeNodes = tree.getPointsWithinCube(pt, tol);
		if (closeNodes.size() == 0)
		{
			tree.insertPoint(pt);
		}
	}
	auto ptsDistinct = tree.getPoints();
	m_vectors = ptsDistinct;

	//// 方法2
	//for (auto it = m_vectors.begin(); it < m_vectors.end() - 1; ++it)
	//{
	//	DmVector p1 = *it;
	//	for(auto it2=it+1; it2!=m_vectors.end();)
	//	{
	//		if (p1.distanceTo(*it2) < tol)
	//		{
	//			it2 = m_vectors.erase(it2);
	//		}
	//		else
	//		{
	//			++it2;
	//		}
	//	}
	//}
}

/// @return m_vectors solution which is the closest to the given coordinate.
///	dist will contain the distance if it doesn't point to NULL (default).
DmVector DmVectorSolutions::getClosest(const DmVector& coord, double* dist, size_t* index) const
{
	double curDist = 0.;
	double minDist = DM_MAXDOUBLE;
	DmVector closestPoint(false);
	int pos = 0;

	for (size_t i = 0; i < m_vectors.size(); i++)
	{
		if (m_vectors[i].valid)
		{
			curDist = (coord - m_vectors[i]).squared();

			if (curDist < minDist || i == 0)
			{
				closestPoint = m_vectors[i];
				minDist = curDist;
				pos = i;
			}
		}
	}
	if (dist)
	{
		*dist = sqrt(minDist);
	}
	if (index)
	{
		*index = pos;
	}
	return closestPoint;
}

///	@param coord, distance to this point
/// @param counts, only consider this many points within solution
/// @return the closest distance from the first counts rs_vectors
double DmVectorSolutions::getClosestDistance(const DmVector& coord, int counts)
{
	double ret = DM_MAXDOUBLE * DM_MAXDOUBLE;
	int i = m_vectors.size();
	if (counts < i && counts >= 0)
	{
		i = counts;
	}
	std::for_each(m_vectors.begin(), m_vectors.begin() + i, [&ret, &coord](DmVector const& vp)
	{
		if (vp.valid)
		{
			double d = (coord - vp).squared();
			if (d < ret)
			{
				ret = d;
			}
		}
	});
	return sqrt(ret);
}

// switch x,y for all vectors
DmVectorSolutions DmVectorSolutions::flipXY(void) const
{
	DmVectorSolutions ret;
	for (const auto& vp : m_vectors)
	{
		ret.push_back(vp.flipXY());
	}
	return ret;
}

std::ostream& operator << (std::ostream& os, const DmVectorSolutions& s)
{
	for (const DmVector& vp : s)
	{
		os << "(" << vp << ")\n";
	}
	os << " m_bTangent: " << (int)s.isTangent() << "\n";
	return os;
}
