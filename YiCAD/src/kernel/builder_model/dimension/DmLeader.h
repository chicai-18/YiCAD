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


/// @file DmLeader.h
/// @brief 引线标注类

#ifndef DMLEADER_H
#define DMLEADER_H

#include "DmEntity.h"
#include "DmDimensionStyle.h"
#include "DimVars.h"

class DmLeaderData
{
public:
	DmLeaderData() = default;
	DmLeaderData(DmDimensionStyle* style, const std::vector<DmVector>& _vertexes);
	DmDimensionStyle* pStyle;
	std::vector<DmVector> vertextes;

	///////以下为替代属性
public:
	DIM_FUNCS_DECLARE()
private:
	DIM_MEMERS_DECLARE()
};

/// @brief 引线标注的引线
class DmLeader : public DmEntity
{
	TYPESYSTEM_HEADER();
public:
	DmLeader(DmEntity* parent = NULL);
	DmLeader(DmEntity* parent, const DmLeaderData& d);
	~DmLeader();
	DmEntity* clone() const override;

	DM::EntityType getEntityType() const override;
	void update() override;
	DmLeaderData getData() const;
	DmLeaderData& getDataRef();
	DmVectorSolutions getRefPoints() const override;

	void addEntity(DmEntity* e);
	void clear();

	DmEntity* addVertex(const DmVector& v);

	bool isContainer() const override;
	void calculateBorders() override;
	double getDistanceToPoint(const DmVector& coord, DmEntity** entity = nullptr, DM::ResolveLevel level = DM::ResolveNone) const override;
	DmVector getNearestEndpoint(const DmVector& coord, double* dist = nullptr) const override;
	DmVector getNearestPointOnEntity(const DmVector& coord, bool onEntity = true, double* dist = nullptr, DmEntity** entity = nullptr) const override;
	DmVector getNearestCenter(const DmVector& coord, double* dist = nullptr) const override;
	DmVector getNearestMiddle(const DmVector& coord, double* dist = nullptr, int middlePoints = 1) const override;
	void setVisible(bool v) override;
	bool setSelected(bool select = true) override;
	void setHighlighted(bool highlight = true) override;

	void move(const DmVector& offset) override;
	void rotate(const DmVector& center, const DmVector& angleVector) override;
	void scale(const DmVector& center, const DmVector& factor) override;
	void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;
	void moveRef(const DmVector& ref, const DmVector& offset) override;

	std::list<DmEntity*> getSubEntities() const override;

	static std::string arrowToString(DM::ArrowType arrowType);
	static DM::ArrowType stringToArrow(const std::string& arrowName);

	// persistent helper
	virtual void saveStream(OutputStream& wrt) const override;
	virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
	virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

protected:
	DmLeaderData	data;
	bool			empty;
	DmEntityContainer* container;
};

#endif
