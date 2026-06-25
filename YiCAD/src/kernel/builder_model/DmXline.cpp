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


/// @file DmXline.cpp
/// @brief DmXline 无限长构造线实体类的实现

#include "DmXline.h"
#include "DmRect.h"
#include "GuiDocumentView.h"

#include "GeometryMethods.h"
#include "Math2d.h"

TYPESYSTEM_SOURCE(DmXline, DmAtomicEntity, 0)

DmXline::DmXline(DmEntity* parent, const XLineData& d)
	: DmAtomicEntity(parent)
	, data(d)
	, isModify(true)
{
	calculateBorders();
}

DmXline::~DmXline()
{
}

void DmXline::calculateBorders()
{
	DmVector basePt = data.getBasePoint();
	DmVector dir = data.getDirection();
	if (dir.x == 0.0)
	{
		minV.x = maxV.x = basePt.x;
		minV.y = DM_MINDOUBLE;
		maxV.y = DM_MAXDOUBLE;
	}
	else if (dir.y == 0.0)
	{
		minV.y = maxV.y = basePt.y;
		minV.x = DM_MINDOUBLE;
		maxV.x = DM_MAXDOUBLE;
	}
	else
	{
		minV.x = DM_MINDOUBLE;
		minV.y = DM_MINDOUBLE;
		maxV.x = DM_MAXDOUBLE;
		maxV.y = DM_MAXDOUBLE;
	}
}

DmEntity* DmXline::clone() const
{
	DmXline* c = new DmXline(*this);
    c->m_ulID = DmId();
    c->setSelected(false);
    c->setHighlighted(false);
	return c;
}

DM::EntityType DmXline::getEntityType() const
{
	return DM::EntityXline;
}

DmVector DmXline::getBasePoint()
{
	return data.getBasePoint();
}

void DmXline::setBasePoint(const DmVector& pt)
{
	data.getBasePoint() = pt;
}

DmVector DmXline::getDirecion()
{
	return data.getDirection();
}

void DmXline::setDirection(const DmVector& vec)
{
	data.setDirection(vec);
}

XLineData DmXline::getData() const
{
	return data;
}

double DmXline::getAngle() const
{
	double ang = std::atan2(data.getDirection().y, data.getDirection().x);
	return Math2d::correctAngle(ang);
}

DmVectorSolutions DmXline::getRefPoints() const
{
	return DmVectorSolutions({ data.getBasePoint(), data.getBasePoint() + data.getDirection() });
}

DmVector DmXline::getNearestEndpoint(const DmVector& coord, double* dist) const
{
	return data.getBasePoint();
}

DmVector DmXline::getNearestPointOnEntity(const DmVector& coord, bool onEntity, double* dist, DmEntity** entity) const
{
	if (entity)
	{
		*entity = const_cast<DmXline*>(this);
	}

	DmVector vDir = data.getDirection().rotate(M_PI_2);
	DmVector crossPt = GeometryMethods::getIntersectionOfTwoLine(data.getBasePoint(), data.getBasePoint() + data.getDirection(), coord, coord + vDir);
	if (dist)
	{
		*dist = crossPt.distanceTo(coord);
	}
	return crossPt;
}

DmVector DmXline::getNearestMiddle(const DmVector& coord, double* dist, int middlePoints) const
{
	return {};
}

void DmXline::move(const DmVector& offset)
{
	DmVector basePt = data.getBasePoint();
	basePt.move(offset);
	data.setBasePoint(basePt);
	moveBorders(offset);
}

void DmXline::rotate(const double& angle)
{
	DmVector pt(data.getBasePoint() + data.getDirection());
	DmVector rvp(angle);
	DmVector vec = data.getBasePoint().rotate(rvp);
	data.setBasePoint(vec);
	pt.rotate(rvp);
	data.setDirection(pt - data.getBasePoint());
	isModify = true;
	calculateBorders();
}

void DmXline::rotate(const DmVector& center, const DmVector& angleVector)
{
	DmVector pt(data.getBasePoint() + data.getDirection());
	DmVector vec = data.getBasePoint().rotate(center, angleVector);
	data.setBasePoint(vec);
	pt.rotate(center, angleVector);
	data.setDirection(pt - data.getBasePoint());
	isModify = true;
	calculateBorders();
}

void DmXline::scale(const DmVector& factor)
{
	DmVector pt = data.getBasePoint().scale(factor);
	DmVector dir = data.getDirection().scale(factor);
	data.setBasePoint(pt);
	data.setDirection(dir);
	isModify = true;
	calculateBorders();
}

void DmXline::scale(const DmVector& center, const DmVector& factor)
{
	DmVector pt = data.getBasePoint().scale(center, factor);
	DmVector dir = data.getDirection().scale(center, factor);
	data.setBasePoint(pt);
	data.setDirection(dir);
	isModify = true;
	calculateBorders();
}

void DmXline::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
	DmVector pt(data.getBasePoint() + data.getDirection());
	DmVector basePt = data.getBasePoint().mirror(axisPoint1, axisPoint2);
	data.setBasePoint(basePt);
	pt.mirror(axisPoint1, axisPoint2);
	DmVector dir = pt - data.getBasePoint();
	data.setDirection(dir);
	isModify = true;
	calculateBorders();
}

void DmXline::moveRef(const DmVector& ref, const DmVector& offset)
{

}

Quadratic DmXline::getQuadratic() const
{
	std::vector<double> ce(3, 0.);
	auto dvp = data.getDirection();
	DmVector normal(-dvp.y, dvp.x);
	ce[0] = normal.x;
	ce[1] = normal.y;
	ce[2] = -normal.dotP(data.getBasePoint() + data.getDirection());
	return Quadratic(ce);
}

std::list<DmEntity*> DmXline::getSubEntities() const
{
	return std::list<DmEntity*>();
}

void DmXline::saveStream(OutputStream& wrt) const
{
	DmAtomicEntity::saveStream(wrt);

	auto base = data.getBasePoint();
	auto dir = data.getDirection();

	wrt << (double)base.x << (double)base.y << (double)dir.x << (double)dir.y;
}

void DmXline::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
	DmAtomicEntity::restoreStream(reader, revs);

	int fileRev = getRevisionId("DmXline", revs);
	if (revId > fileRev)
	{
		// 老文件格式
		restoreStreamWithRev(reader, fileRev);
	}
	else
	{
		DmVector base(true), dir(true);
		reader >> (double&)base.x >> (double&)base.y >> (double&)dir.x >> (double&)dir.y;

		setBasePoint(base);
		setDirection(dir);
		isModify = true;
	}
}

void DmXline::restoreStreamWithRev(InputStream& rdr, int rev)
{
	if (rev == 0)
	{
	}
	else //big change, e.g. change supper class of DmXline
	{
		//step1.
		// read all legacy data one by one

	}
}
