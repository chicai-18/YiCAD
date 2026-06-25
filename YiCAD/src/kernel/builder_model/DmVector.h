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


/// @file DmVector.h
/// @brief 二维/三维向量类及向量解集类

#ifndef DMVECTOR_H
#define DMVECTOR_H

#include <iosfwd>
#include <vector>

#include "Datamodel.h"

/// @brief Represents a 3d vector (x/y/z)
class DmVector
{
public:
	DmVector() = default;
	DmVector(double vx, double vy, double vz = 0.0);
	explicit DmVector(double angle);
	explicit DmVector(bool valid);
	~DmVector() = default;

	/// @brief operator bool explicit and implicit conversion to bool
	explicit operator bool() const;

	void set(double angle);  // set to unit vector by the direction of angle
	void set(double vx, double vy, double vz = 0.0);
	void setPolar(double radius, double angle);
	static DmVector polar(double rho, double theta);

	double distanceTo(const DmVector& v) const;

	/// @brief 获得原点到当前点的方位角(0-2pi)
	double angle() const;
	/// @brief 获得当前点到指定点的向量的方位角(0-2pi)
	/// @param [in] 指定点
	double angleTo(const DmVector& v) const;
	/// @brief 获得当前向量逆时针转到指定向量的角度(0-2pi)
	/// @param dir[in] 指定向量，不需要为单位向量
	double angleToDir(const DmVector& dir) const;
    /// @brief 向量的长度
	double magnitude() const;
	double squared() const;                      // return square of length
	double squaredTo(const DmVector& v1) const;  // return square of length
	DmVector lerp(const DmVector& v, double t) const;
	DmVector normalize() const;

	bool isInWindow(const DmVector& firstCorner, const DmVector& secondCorner) const;
	bool isInWindowOrdered(const DmVector& vLow, const DmVector& vHigh) const;

	DmVector toInteger();

	DmVector move(const DmVector& offset);
	/// @brief 逆时针旋转一个弧度角
	DmVector rotate(double ang);
	DmVector rotate(const DmVector& angleVector);
	DmVector rotate(const DmVector& center, double ang);
	DmVector rotate(const DmVector& center, const DmVector& angleVector);
	DmVector scale(double factor);
	DmVector scale(const DmVector& factor);
	DmVector scale(const DmVector& factor) const;
	DmVector scale(const DmVector& center, const DmVector& factor);
	DmVector mirror(const DmVector& axisPoint1, const DmVector& axisPoint2);
	double dotP(const DmVector& v1) const;
    /// @brief 获得指定维度的坐标。0获得x，1获得y，2获得z
    double getDimension(int dim) const;

	DmVector operator+(const DmVector& v) const;
	DmVector operator+(double d) const;
	DmVector operator-(const DmVector& v) const;
	DmVector operator-(double d) const;
	DmVector operator*(const DmVector& v) const;
	DmVector operator/(const DmVector& v) const;
	DmVector operator*(double s) const;
	DmVector operator/(double s) const;
	DmVector operator-() const;

	DmVector operator+=(const DmVector& v);
	DmVector operator-=(const DmVector& v);
	DmVector operator*=(const DmVector& v);
	DmVector operator/=(const DmVector& v);
	DmVector operator*=(double s);
	DmVector operator/=(double s);

	bool operator==(const DmVector& v) const;
	bool operator!=(const DmVector& v) const;

	/// @brief operator == comparison of validity with bool
	///	@param valid boolean parameter
	/// @return true is the parameter valid is the same as validity
	bool operator==(bool valid) const;
	bool operator!=(bool valid) const;

	static DmVector minimum(const DmVector& v1, const DmVector& v2);
	static DmVector maximum(const DmVector& v1, const DmVector& v2);
	// crossP only defined for 3D
	static DmVector crossP(const DmVector& v1, const DmVector& v2);
	static double dotP(const DmVector& v1, const DmVector& v2);
	static double posInLine(const DmVector& start, const DmVector& end, const DmVector& pos);

	// switch x,y for all vectors
	DmVector flipXY(void) const;

	friend std::ostream& operator<<(std::ostream&, const DmVector& v);

public:
	double	x = 0.;
	double	y = 0.;
	double	z = 0.;
	bool	valid = false;

	static DmVector NormalX;
	static DmVector NormalNegativeX;
	static DmVector NormalY;
	static DmVector NormalNegativeY;
	static DmVector NormalZ;
	static DmVector NormalNegativeZ;

    constexpr static int DIMENSION = 3; ///< 向量维度
};

/// @brief Represents one to 4 vectors. Typically used to return multiple solutions from a function.
class DmVectorSolutions
{
public:
	typedef DmVector value_type;
	DmVectorSolutions();
	DmVectorSolutions(const std::vector<DmVector>& s);
	DmVectorSolutions(std::initializer_list<DmVector> const& l);
	DmVectorSolutions(int num);

	~DmVectorSolutions() = default;

	void alloc(size_t num);
	void clear();

	/// @brief get range safe method of member access
	///	@param i member index
	/// @return indexed member, or invalid vector, if out of range
	DmVector get(size_t i) const;
	const DmVector& at(size_t i) const;
	const DmVector& operator[](const size_t i) const;
	DmVector& operator[](const size_t i);
	size_t getNumber() const;
	size_t size() const;
	void resize(size_t n);
	bool hasValid() const;
	void set(size_t i, const DmVector& v);
	void push_back(const DmVector& v);
	void removeAt(const size_t i);
	DmVectorSolutions& push_back(const DmVectorSolutions& v);
	void setTangent(bool t);
	bool isTangent() const;
	DmVector getClosest(const DmVector& coord, double* dist = nullptr, size_t* index = nullptr) const;
	double getClosestDistance(const DmVector& coord, int counts = -1);  // default to search all
	const std::vector<DmVector>& getVector() const;
	std::vector<DmVector>::const_iterator begin() const;
	std::vector<DmVector>::const_iterator end() const;
	std::vector<DmVector>::iterator begin();
	std::vector<DmVector>::iterator end();
	void rotate(double ang);
	void rotate(const DmVector& angleVector);
	void rotate(const DmVector& center, double ang);
	void rotate(const DmVector& center, const DmVector& angleVector);
	void move(const DmVector& vp);
	void scale(const DmVector& center, const DmVector& factor);
	void scale(const DmVector& factor);

	/// @brief 去除重复点
	void distinct(const double tol = 1e-4);

	// switch x,y for all vectors
	DmVectorSolutions flipXY(void) const;

	friend std::ostream& operator<<(std::ostream& os, const DmVectorSolutions& s);

private:
	std::vector<DmVector>	m_vectors;
	bool					m_bTangent;
};

#endif
