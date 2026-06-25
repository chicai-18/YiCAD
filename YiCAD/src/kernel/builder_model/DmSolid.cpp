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


/// @file DmSolid.cpp
/// @brief DmSolid 填充实体类的实现

#include<iostream>
#include<cmath>
#include "DmSolid.h"

#include "DmLine.h"
#include "DmArc.h"
#include "GuiDocumentView.h"
#include "Information.h"
#include "GeometryMethods.h"
#include "Debug.h"

#include "Writer.h"
#include "Reader.h"
#include "Stream.h"

TYPESYSTEM_SOURCE(DmSolid, DmAtomicEntity, 0)

// Default constructor.
DmSolid::DmSolid(DmEntity* parent, const SolidData& d)
	: DmAtomicEntity(parent)
	, data(d)
	, isModify(true)
{
	calculateBorders();
}

DmEntity* DmSolid::clone() const
{
	DmSolid* s = new DmSolid(*this);
    s->m_ulID = DmId();
    s->setSelected(false);
    s->setHighlighted(false);
	return s;
}

DM::EntityType DmSolid::getEntityType() const
{
	return DM::EntitySolid;
}

SolidData const& DmSolid::getData() const
{
	return data;
}

DmVector DmSolid::getCorner(int num) const
{
	return data.getCornerAt(num);
}

void DmSolid::calculateBorders()
{
	resetBorders();
	int size = data.getCornerSize();
	DmVector p1;
	for (int i = 0; i < size; i++)
	{
		p1 = data.getCornerAt(i);
		minV = DmVector::minimum(minV, p1);
		maxV = DmVector::maximum(maxV, p1);
	}
}

DmVector DmSolid::getNearestEndpoint(const DmVector& coord, double* dist /*= nullptr*/)const
{
	double minDist = DM_MAXDOUBLE;
	double curDist = 0.0;
	DmVector ret;
	int size = data.getCornerSize();
	for (int i = 0; i < size; i++)
	{
		if (data.getCornerAt(i).valid)
		{
			curDist = data.getCornerAt(i).distanceTo(coord);
			if (curDist < minDist)
			{
				ret = data.getCornerAt(i);
				minDist = curDist;
			}
		}
	}

	setDistPtr(dist, minDist);

	return ret;
}

bool DmSolid::isInCrossWindow(const DmVector& v1, const DmVector& v2) const
{
	DmVector vBL(true);	//要设置为true，巨坑
	DmVector vTR(true);
	DmVectorSolutions sol;

	// sort input vectors to BottomLeft & TopRight
	if (v1.x < v2.x)
	{
		vBL.x = v1.x;
		vTR.x = v2.x;
	}
	else
	{
		vBL.x = v2.x;
		vTR.x = v1.x;
	}
	if (v1.y < v2.y)
	{
		vBL.y = v1.y;
		vTR.y = v2.y;
	}
	else
	{
		vBL.y = v2.y;
		vTR.y = v1.y;
	}

	// Check if entity is out of window
	if (getMin().x > vTR.x || getMax().x < vBL.x || getMin().y > vTR.y || getMax().y < vBL.y)
	{
		return false;
	}

	//获得solid边界实体
	std::vector<std::unique_ptr<DmEntity>> border;
	int size = data.getCornerSize();
	DmVector p1, p2, p3, p4;
	p1 = data.getCornerAt(0);
	p2 = data.getCornerAt(1);
	p3 = data.getCornerAt(2);
	if (size == 4)
	{
		p4 = data.getCornerAt(3);
		border.emplace_back(new DmLine(nullptr, LineData(p1, p2)));
		border.emplace_back(new DmLine(nullptr, LineData(p2, p4)));
		border.emplace_back(new DmLine(nullptr, LineData(p4, p3)));
		border.emplace_back(new DmLine(nullptr, LineData(p3, p1)));
	}
	else
	{
		border.emplace_back(new DmLine(nullptr, LineData(p1, p2)));
		border.emplace_back(new DmLine(nullptr, LineData(p2, p3)));
		border.emplace_back(new DmLine(nullptr, LineData(p3, p1)));
	}

	// 判断边界是否与窗体相交
	if (getMax().x > vBL.x && getMin().x < vBL.x)
	{    // left
		DmLine edge{ vBL, {vBL.x, vTR.y} };
		for (auto const& line : border)
		{
			sol = Information::getIntersection(&edge, line.get(), true);
			if (sol.hasValid())
			{
				return true;
			}
		}
	}
	if (getMax().x > vTR.x && getMin().x < vTR.x)
	{    // right
		DmLine edge{ {vTR.x, vBL.y}, vTR };
		for (auto const& line : border)
		{
			sol = Information::getIntersection(&edge, line.get(), true);
			if (sol.hasValid())
			{
				return true;
			}
		}
	}
	if (getMax().y > vBL.y && getMin().y < vBL.y)
	{    // bottom
		DmLine edge{ vBL, {vTR.x, vBL.y} };
		for (auto const& line : border)
		{
			sol = Information::getIntersection(&edge, line.get(), true);
			if (sol.hasValid())
			{
				return true;
			}
		}
	}
	if (getMax().y > vTR.y
		&& getMin().y < vTR.y)
	{ // top
		DmLine edge{ {vBL.x, vTR.y}, vTR };
		for (auto const& line : border)
		{
			sol = Information::getIntersection(&edge, line.get(), true);
			if (sol.hasValid())
			{
				return true;
			}
		}
	}

	return false;
}

/// @return true if positive o zero, false if negative.
bool DmSolid::sign(const DmVector& v1, const DmVector& v2, const DmVector& v3) const
{
	double res = (v1.x - v3.x) * (v2.y - v3.y) - (v2.x - v3.x) * (v1.y - v3.y);	//向量v3v1 X v3v2（v3指向v1，v3指向v2，两者叉乘）

	return (res >= 0.0);
}

// todo: Implement this.
DmVector DmSolid::getNearestPointOnEntity(const DmVector& coord, bool onEntity /*= true*/, double* dist /*= nullptr*/, DmEntity** entity /*= nullptr*/) const
{
	// first check if point is inside solid
	//bool s1 = sign(data.getCornerAt(0), data.getCornerAt(1), coord);
	//bool s2 = sign(data.getCornerAt(1), data.getCornerAt(2), coord);
	//bool s3 = sign(data.getCornerAt(2), data.getCornerAt(0), coord);

	//if ((s1 == s2) && (s2 == s3))
	//{
	//	setDistPtr(dist, 0.0);
	//	return coord;
	//}

	//if (!isTriangle())
	//{
	//	s1 = sign(data.getCornerAt(0), data.getCornerAt(2), coord);
	//	s2 = sign(data.getCornerAt(2), data.getCornerAt(3), coord);
	//	s3 = sign(data.getCornerAt(3), data.getCornerAt(0), coord);
	//	if ((s1 == s2) && (s2 == s3))
	//	{
	//		setDistPtr(dist, 0.0);
	//		return coord;
	//	}
	//}

	bool isInside = GeometryMethods::isPtInside(data.getCorners(), coord);
	if (isInside)
	{
		setDistPtr(dist, 0.0);
		return coord;
	}

	// not inside of solid
	// Find nearest distance from each edge
	if (nullptr != entity)
	{
		*entity = const_cast<DmSolid*>(this);
	}

	DmVector ret(false);
	double currDist = DM_MAXDOUBLE;
	double tmpDist = 0.0;
	for (int i = 0, next = i + 1; i < data.getCornerSize(); ++i, ++next)
	{
		// closing edge
		if (next == data.getCornerSize())
		{
			next = 0;
		}

		DmVector direction = data.getCornerAt(next) - data.getCornerAt(i);
		DmVector vpc = coord - data.getCornerAt(i);
		double a = direction.squared();
		if (a < DM_TOLERANCE2)
		{
			// line too short
			vpc = data.getCornerAt(i);
		}
		else
		{
			// find projection on line
			vpc = data.getCornerAt(i) + direction * DmVector::dotP(vpc, direction) / a;
		}
		tmpDist = vpc.distanceTo(coord);
		if (tmpDist < currDist)
		{
			currDist = tmpDist;
			ret = vpc;
		}
	}

	// verify this part
	if (onEntity && !ret.isInWindowOrdered(minV, maxV))
	{
		// projection point not within range, find the nearest endpoint
		ret = getNearestEndpoint(coord, dist);
		currDist = ret.distanceTo(coord);
	}

	setDistPtr(dist, currDist);

	return ret;
}

DmVector DmSolid::getNearestCenter(const DmVector& coord, double* dist /*= nullptr*/) const
{
	Q_UNUSED(coord)
		setDistPtr(dist, DM_MAXDOUBLE);

	return DmVector(false);
}

DmVector DmSolid::getNearestMiddle(const DmVector& coord, double* dist /*= nullptr*/, const int middlePoints /*= 1*/) const
{
	Q_UNUSED(coord)
		Q_UNUSED(middlePoints)
		setDistPtr(dist, DM_MAXDOUBLE);

	return DmVector(false);
}

/// @return Distance from one of the boundary lines of this solid to given point.
double DmSolid::getDistanceToPoint(const DmVector& coord, DmEntity** entity, DM::ResolveLevel level) const
{
	Q_UNUSED(level)
	if (nullptr != entity)
	{
		*entity = const_cast<DmSolid*>(this);
	}

	double ret = 0.;
	getNearestPointOnEntity(coord, true, &ret, entity);

	return ret;
}

void DmSolid::move(const DmVector& offset)
{
	for (int i = 0; i < data.getCornerSize(); ++i)
	{
		if (data.getCornerAt(i).valid)
		{
			data.setCornerAt(i, data.getCornerAt(i).move(offset));
		}
	}
	isModify = true;
	moveBorders(offset);
}

void DmSolid::rotate(const DmVector& center, const DmVector& angleVector)
{
	for (int i = 0; i < data.getCornerSize(); ++i)
	{
		if (data.getCornerAt(i).valid)
		{
			data.setCornerAt(i, data.getCornerAt(i).rotate(center, angleVector));
		}
	}
	isModify = true;
	calculateBorders();
}

void DmSolid::scale(const DmVector& center, const DmVector& factor)
{
	for (int i = 0; i < data.getCornerSize(); ++i)
	{
		if (data.getCornerAt(i).valid)
		{
			data.setCornerAt(i, data.getCornerAt(i).scale(center, factor));
		}
	}
	isModify = true;
	calculateBorders();
}

void DmSolid::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
	for (int i = 0; i < data.getCornerSize(); ++i)
	{
		if (data.getCornerAt(i).valid)
		{
			data.setCornerAt(i, data.getCornerAt(i).mirror(axisPoint1, axisPoint2));
		}
	}
	isModify = true;
	calculateBorders();
}

void DmSolid::setDistPtr(double* dist, const double value) const
{
	if (nullptr != dist)
	{
		*dist = value;
	}
}

void DmSolid::getArcInfo(const DmVector& pt1, const DmVector& pt2, const double bulge, DmVector& center, double& radius, double& startAngle, double& endAngle, bool& reversed) const
{
	reversed = (bulge < 0.0);
	double alpha = atan(bulge) * 4.0;

	DmVector middle;
	double dist;
	double angle;
	middle = (pt2 + pt1) / 2.0;
	dist = pt2.distanceTo(pt1) / 2.0;
	angle = pt2.angleTo(pt1);
	

	// alpha can't be 0.0 at this point
	radius = fabs(dist / sin(alpha / 2.0));

	double const wu = fabs(radius * radius - dist * dist);
	double h = sqrt(wu);

	if (bulge > 0.0)
	{
		angle += M_PI_2;
	}
	else
	{
		angle -= M_PI_2;
	}

	if (fabs(alpha) > M_PI)
	{
		h *= -1.0;
	}

	center = DmVector::polar(h, angle);
	center += middle;

	startAngle = center.angleTo(pt2);
	endAngle = center.angleTo(pt1);
}

std::list<DmEntity*> DmSolid::getSubEntities() const
{
	return std::list<DmEntity*>();
}

void DmSolid::saveStream(OutputStream& wrt) const
{
	DmAtomicEntity::saveStream(wrt);

	auto corners = data.getCorners();
	wrt << (uint32_t)corners.size();
	for (auto& corner : corners)
	{
		wrt << (double)corner.x << (double)corner.y;
	}
}

void DmSolid::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
	int fileRev = getRevisionId("DmSolid", revs);
	if (revId > fileRev)
	{
        DmAtomicEntity::restoreStream(reader, revs);
		// 老文件格式
		restoreStreamWithRev(reader, fileRev);
	}
	else
	{
        restoreStream(reader);
	}
}

void DmSolid::restoreStreamWithRev(InputStream& rdr, int rev)
{
	if (rev == 0)
	{
	}
	else //big change, e.g. change supper class of DmSolid
	{
		//step1.
		// read all legacy data one by one

	}
}

void DmSolid::restoreStream(InputStream& reader)
{
    DmAtomicEntity::restoreStream(reader);

    auto corners = std::vector<DmVector>();
    uint32_t numCorner;
    reader >> (uint32_t&)numCorner;
    for (uint32_t i = 0; i < numCorner; i++)
    {
        double pt_x, pt_y;
        reader >> (double&)pt_x >> (double&)pt_y;
        corners.emplace_back(DmVector(pt_x, pt_y));
    }

    data.setCorners(corners);

    isModify = true;
}
