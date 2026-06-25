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


/// @file DmLeader.cpp
/// @brief DmLeader 引线标注类的实现

#include "DmLeader.h"

#include<iostream>

#include "Debug.h"
#include "DmLine.h"
#include "DmSolid.h"
#include "DmBlockReference.h"
#include "DmDocument.h"
#include "ApplicationWindow.h"
#include "Math2d.h"

TYPESYSTEM_SOURCE(DmLeader, DmEntity, 0);
DIM_FUNCS_IMPLEMENT(DmLeaderData)

DmLeader::DmLeader(DmEntity* parent)
	: DmEntity(parent)
	, empty(true)
	, container(new DmEntityContainer(this))
{
}

DmLeader::DmLeader(DmEntity* parent, const DmLeaderData& d)
	: DmEntity(parent)
	, data(d)
	, container(new DmEntityContainer(this))
{
	empty = true;
}

DmLeader::~DmLeader()
{
	if (container)
	{
		delete container;
		container = nullptr;
	}
}

DmEntity* DmLeader::clone() const
{
	DmLeader* d = new DmLeader(*this);
	d->container = new DmEntityContainer(d);
	d->update();
	return d;
}

DM::EntityType DmLeader::getEntityType() const
{
	return DM::EntityDimLeader;
}

// Implementation of update. Updates the arrow.
void DmLeader::update()
{
	clear();

	if(data.vertextes.size() == 0)
	{ 
		return;
	}
	DmPen pen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType());
	DmVector firstPt = data.vertextes.front();
	DmVector secondPt = data.vertextes.at(1);
	DmVector firstDir = (secondPt - firstPt).normalize();
	double firstDist = firstPt.distanceTo(secondPt);
	//箭头
	if (firstDist > data.arrowSize())
	{
		DmDocument* curDoc = static_cast<DmDocument*>(ApplicationWindow::getAppWindow()->getDocument());
		DmBlockTable* arrowBlocks = curDoc->getDimStyleTable()->getArrowBlocks();
		double arrowCutDist = DmDimensionStyle::getArrowCutDistance(data.leaderArrow()) * data.arrowSize();
		DmVector arrowScale(data.arrowSize(), data.arrowSize());
		DmVector space(0.0, 0.0);
		double arrowAngle = Math2d::correctAngle(firstPt.angleTo(secondPt) + M_PI);


		DmBlockReferenceData arrowData(DmDimensionStyle::getArrowBlockName(data.leaderArrow()), firstPt, arrowScale, arrowAngle,
			1, 1, space, arrowBlocks);
		DmBlockReference* arrow = new DmBlockReference(this, arrowData);
		arrow->setPen(pen);
		arrow->setLayer(getLayer());
		arrow->update();
		arrow->forcedCalculateBorders();
		addEntity(arrow);

		//箭头延伸的线
		if (firstDist > arrowCutDist)
		{
			DmVector arrowEndPt = firstPt + firstDir * arrowCutDist;
			DmLine* arrowLine = new DmLine(this, arrowEndPt, secondPt);
			addEntity(arrowLine);
			arrowLine->setPen(pen);
			arrowLine->setLayer(getLayer());
			arrowLine->update();
		}
	}
	else
	{
		//无法绘制箭头
		DmLine* line = new DmLine(this, firstPt, secondPt);
		addEntity(line);
		line->setPen(pen);
		line->setLayer(getLayer());
		line->update();
	}

	//后面的线
	if (data.vertextes.size() > 2)
	{
		DmVector pt1, pt2;
		for (auto it = data.vertextes.begin() + 1; it != data.vertextes.end() - 1; ++it)
		{
			pt1 = *it;
			pt2 = *(it + 1);
			DmLine* line = new DmLine(this, pt1, pt2);
			addEntity(line);
			line->setPen(pen);
			line->setLayer(getLayer());
			line->update();
		}
	}
	calculateBorders();
}

DmLeaderData DmLeader::getData() const
{
	return data;
}

DmLeaderData& DmLeader::getDataRef()
{
	return data;
}

DmVectorSolutions DmLeader::getRefPoints() const
{
	return DmVectorSolutions({data.vertextes[0]});
}

void DmLeader::addEntity(DmEntity* e)
{
	if (container && e)
	{
		container->addEntity(e);
	}
}

void DmLeader::clear()
{
	container->clear();
}

//bool DmLeader::hasArrowHead()
//{
//	return data.arrowHead;
//}

/// @brief Adds a vertex from the endpoint of the last element or sets the startpoint to the point 'v'.
///	The very first vertex added is the starting point.
/// @param v vertex coordinate
/// @return Pointer to the entity that was added or nullptr if this was the first vertex added.
DmEntity* DmLeader::addVertex(const DmVector& v)
{
	DmEntity* entity = nullptr;
	static DmVector last = DmVector(false);

	if (empty)
	{
		last = v;
		empty = false;
	}
	else
	{
		// add line to the leader:
		entity = new DmLine(this, {last, v});
		entity->setPen(DmPen(DM::FlagInvalid));
		entity->setLayer(nullptr);
		addEntity(entity);

		if (container->size() == 1 /*&& hasArrowHead()*/)
		{
			update();
		}

		last = v;
	}

	return entity;
}

bool DmLeader::isContainer() const
{
	return false;
}

void DmLeader::calculateBorders()
{
	container->calculateBorders();
	maxV = container->getMax();
	minV = container->getMin();
}

double DmLeader::getDistanceToPoint(const DmVector& coord, DmEntity** entity, DM::ResolveLevel level) const
{
	double minDist = DM_MAXDOUBLE;
	double curDist = 0.0;
	for (auto e : *container)
	{
		curDist = e->getDistanceToPoint(coord);
		if (curDist < minDist)
		{
			minDist = curDist;
		}
	}
	return minDist;
}

DmVector DmLeader::getNearestEndpoint(const DmVector& coord, double* dist) const
{
	double minDist = DM_MAXDOUBLE;
	double curDist = DM_MAXDOUBLE;
	DmVector res(false);
	for (auto e : *container)
	{
		DmVector temp = e->getNearestEndpoint(coord, &curDist);
		if (temp.valid && curDist < minDist)
		{
			minDist = curDist;
			res = temp;
		}
	}
	if (dist)
	{
		*dist = minDist;
	}
	return res;
}

DmVector DmLeader::getNearestPointOnEntity(const DmVector& coord, bool onEntity, double* dist, DmEntity** entity) const
{
	double minDist = DM_MAXDOUBLE;
	double curDist = DM_MAXDOUBLE;
	DmVector res(false);
	for (auto e : *container)
	{
		DmVector temp = e->getNearestPointOnEntity(coord, true, &curDist);
		if (temp.valid && curDist < minDist)
		{
			minDist = curDist;
			res = temp;
		}
	}
	if (dist)
	{
		*dist = minDist;
	}
	return res;
}

DmVector DmLeader::getNearestCenter(const DmVector& coord, double* dist) const
{
	double minDist = DM_MAXDOUBLE;
	double curDist = DM_MAXDOUBLE;
	DmVector res(false);
	for (auto e : *container)
	{
		DmVector temp = e->getNearestCenter(coord, &curDist);
		if (temp.valid && curDist < minDist)
		{
			minDist = curDist;
			res = temp;
		}
	}
	if (dist)
	{
		*dist = minDist;
	}
	return res;
}

DmVector DmLeader::getNearestMiddle(const DmVector& coord, double* dist, int middlePoints) const
{
	double minDist = DM_MAXDOUBLE;
	double curDist = DM_MAXDOUBLE;
	DmVector res(false);
	for (auto e : *container)
	{
		DmVector temp = e->getNearestMiddle(coord, &curDist);
		if (temp.valid && curDist < minDist)
		{
			minDist = curDist;
			res = temp;
		}
	}
	if (dist)
	{
		*dist = minDist;
	}
	return res;
}

void DmLeader::setVisible(bool v)
{
	DmEntity::setVisible(v);
	container->setVisible(v);
}

bool DmLeader::setSelected(bool select)
{
	if (DmEntity::setSelected(select))
	{
		container->setSelected(select);
		return true;
	}
	else
	{
		return false;
	}
}

void DmLeader::setHighlighted(bool highlight)
{
	DmEntity::setHighlighted(highlight);
	container->setHighlighted(highlight);
}

void DmLeader::move(const DmVector& offset)
{
	for (auto& v : data.vertextes)
	{
		v.move(offset);
	}
	update();
}

void DmLeader::rotate(const DmVector& center, const DmVector& angleVector)
{
	for (auto& v : data.vertextes)
	{
		v.rotate(center, angleVector);
	}
	update();
}

void DmLeader::scale(const DmVector& center, const DmVector& factor)
{
	for (auto& v : data.vertextes)
	{
		v.scale(center, factor);
	}
	update();
}

void DmLeader::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
	for (auto& v : data.vertextes)
	{
		v.mirror(axisPoint1, axisPoint2);
	}
	update();
}

void DmLeader::moveRef(const DmVector& ref, const DmVector& offset)
{
	if (ref.distanceTo(data.vertextes[0]) < 1.0e-4)
	{
		data.vertextes[0] = data.vertextes[0] + offset;
		update();
	}
}

std::list<DmEntity*> DmLeader::getSubEntities() const
{
	std::list<DmEntity*> subEnts = std::list<DmEntity*>();

	for (auto ent : *container)
	{
		if (ent->getEntityType() == DM::EntityContainer)
		{
			auto textEnts = ((DmEntityContainer*)ent)->getEntityList();
			for (auto text : textEnts)
			{
				auto textSub = text->getSubEntities();
				subEnts.splice(subEnts.end(), textSub);
			}
		}
		else
		{
			auto entSub = ent->getSubEntities();
			if (entSub.size() > 0)
			{
				subEnts.splice(subEnts.end(), entSub);
			}
			else
			{
				subEnts.emplace_back(std::move(ent));
			}
		}
	}
	return subEnts;
}

std::string DmLeader::arrowToString(DM::ArrowType arrowType)
{
	//参考AutoCAD系统变量"DIMBLK"
	std::map<DM::ArrowType, std::string> map{
		{DM::ArrowType::ClosedFilled, ""},
		{DM::ArrowType::Closed, "_CLOSED"},
		{DM::ArrowType::ClosedBlank, "_CLOSEDBLANK"},
		{DM::ArrowType::Dot, "_DOT"},
		{DM::ArrowType::ArchitecturalTick, "_ARCHTICK"},
		{DM::ArrowType::Oblique, "_OBLIQUE"},
		{DM::ArrowType::Open, "_OPEN"},
		{DM::ArrowType::OriginIndicator, "_ORIGIN"},
		{DM::ArrowType::OriginIndicator2, "_ORIGIN2"},
		{DM::ArrowType::RightAngle, "_OPEN90"},
		{DM::ArrowType::Open30, "_OPEN30"},
		{DM::ArrowType::DotSmall, "_DOTSMALL"},
		{DM::ArrowType::DotBlank, "_DOTBLANK"},
		{DM::ArrowType::DotSmallBlank, "_SMALL"},
		{DM::ArrowType::Box, "_BOXBLANK"},
		{DM::ArrowType::BoxFilled, "_BOXFILLED"},
		{DM::ArrowType::DatumTriangle, "_DATUMBLANK"},
		{DM::ArrowType::DatumTriangleFilled, "_DATUMFILLED"},
		{DM::ArrowType::Integral, "_INTEGRAL"},
		{DM::ArrowType::None, "_NONE"},
	};
	auto it = map.find(arrowType);
	if (it != map.end())
	{
		return it->second;
	}
	else
	{
		return "";
	}
}

DM::ArrowType DmLeader::stringToArrow(const std::string& arrowName)
{
	//参考AutoCAD系统变量"DIMBLK"
	static std::map< std::string, DM::ArrowType> map{
		{"", DM::ArrowType::ClosedFilled},
		{"_CLOSED", DM::ArrowType::Closed},
		{"_CLOSEDBLANK", DM::ArrowType::ClosedBlank},
		{"_DOT", DM::ArrowType::Dot},
		{"_ARCHTICK", DM::ArrowType::ArchitecturalTick},
		{"_OBLIQUE", DM::ArrowType::Oblique},
		{"_OPEN", DM::ArrowType::Open},
		{"_ORIGIN", DM::ArrowType::OriginIndicator},
		{"_ORIGIN2", DM::ArrowType::OriginIndicator2},
		{"_OPEN90", DM::ArrowType::RightAngle},
		{"_OPEN30", DM::ArrowType::Open30},
		{"_DOTSMALL", DM::ArrowType::DotSmall},
		{"_DOTBLANK", DM::ArrowType::DotBlank},
		{"_SMALL", DM::ArrowType::DotSmallBlank},
		{"_BOXBLANK", DM::ArrowType::Box},
		{"_BOXFILLED", DM::ArrowType::BoxFilled},
		{"_DATUMBLANK", DM::ArrowType::DatumTriangle},
		{"_DATUMFILLED", DM::ArrowType::DatumTriangleFilled},
		{"_INTEGRAL", DM::ArrowType::Integral},
		{"_NONE", DM::ArrowType::None},
	};
	std::string arrowNameUpper(arrowName.size(), 0);
	std::transform(arrowName.begin(), arrowName.end(), arrowNameUpper.begin(), std::bind(&std::toupper<char>, std::placeholders::_1, std::locale()));  //此转换不支持中文，但是此处没有中文，所以没事
	auto it = map.find(arrowNameUpper);
	if (it != map.end())
	{
		return it->second;
	}
	else
	{
		return DM::ArrowType::ClosedFilled;
	}
}

void DmLeader::saveStream(OutputStream& wrt) const
{
	DmEntity::saveStream(wrt);

	std::string dimStyleName = data.pStyle->getName().toStdString();

	// 替代属性
	std::string Dimldrblk = arrowToString(data.leaderArrow());
	double Dimasz = data.arrowSize();
	DmColor clrd = data.dimLineColor();
	int Dimclrd, Dimclrd_r, Dimclrd_g, Dimclrd_b;
	DmColor::toCAD(clrd, Dimclrd, Dimclrd_r, Dimclrd_g, Dimclrd_b);
	int Dimlwd = static_cast<int>(data.dimLineWidth());
	double Dimgap = data.offsetFromDimLine();
	int Dimtad = static_cast<int>(data.textVerticalPos());

	wrt << (std::string)dimStyleName << (std::string)Dimldrblk << (double)Dimasz << (int)Dimclrd << (int)Dimclrd_r << (int)Dimclrd_g << (int)Dimclrd_b << (int)Dimlwd << (double)Dimgap << (int)Dimtad;

	uint32_t num = data.vertextes.size();
	wrt << (uint32_t)num;
	for (auto& pt : data.vertextes)
	{
		wrt << (double)pt.x << (double)pt.y;
	}
}

void DmLeader::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
	int fileRev = getRevisionId("DmLeader", revs);
	if (revId > fileRev)
	{
        DmEntity::restoreStream(reader, revs);
		// 老文件格式
		restoreStreamWithRev(reader, fileRev);
	}
	else
	{
        restoreStream(reader);
	}
}

void DmLeader::restoreStreamWithRev(InputStream& rdr, int rev)
{
	if (rev == 0)
	{
	}
	else //big change, e.g. change supper class of DmLeader
	{
		//step1.
		// read all legacy data one by one
	}
}

void DmLeader::restoreStream(InputStream& rdr)
{
    DmEntity::restoreStream(rdr);

    std::string dimStyleName;
    // 替代属性
    std::string Dimldrblk;
    double Dimasz;
    int Dimclrd, Dimclrd_r, Dimclrd_g, Dimclrd_b;
    int Dimlwd;
    double Dimgap;
    int Dimtad;

    rdr >> (std::string&)dimStyleName >> (std::string&)Dimldrblk >> (double&)Dimasz >> (int&)Dimclrd >> (int&)Dimclrd_r >> (int&)Dimclrd_g >> (int&)Dimclrd_b >> (int&)Dimlwd >> (double&)Dimgap >> (int&)Dimtad;

    data.pStyle = getDocument()->getDimStyleTable()->find(QString::fromStdString(dimStyleName));
    data.setLeaderArrow(stringToArrow(Dimldrblk));
    data.setArrowSize(Dimasz);
    data.setDimLineColor(DmColor::fromCAD(Dimclrd, Dimclrd_r, Dimclrd_g, Dimclrd_b));
    data.setDimLineWidth(static_cast<DM::LineWidth>(Dimlwd));
    data.setOffsetFromDimLine(Dimgap);
    data.setTextVerticalPos(static_cast<DmDimensionStyleTextData::TextVerticalPos>(Dimtad));

    uint32_t count;
    rdr >> (uint32_t&)count;
    std::vector<DmVector> pts = std::vector<DmVector>();
    for (int i = 0; i < count; ++i)
    {
        DmVector pt(true);
        rdr >> (double&)pt.x >> (double&)pt.y;
        pts.emplace_back(pt);
    }

    data.vertextes = pts;
    update();
}

//DmLeaderData::DmLeaderData(bool arrowHeadFlag)
//{
//	arrowHead = arrowHeadFlag;
//}

DmLeaderData::DmLeaderData(DmDimensionStyle* style, const std::vector<DmVector>& _vertexes)
{
	pStyle = style;
	vertextes = _vertexes;
}
