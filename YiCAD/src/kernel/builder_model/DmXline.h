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


/// @file DmXline.h
/// @brief 无限长构造线实体类

#ifndef DMXLINE_H
#define DMXLINE_H

#include "DmAtomicEntity.h"
#include "XLineData.h"
#include "Quadratic.h"

/// @brief 无限长构造线
class DmXline : public DmAtomicEntity
{
	TYPESYSTEM_HEADER();

public:
	DmXline() = default;
	DmXline(DmEntity* parent, const XLineData& d);
	~DmXline();

public:
	DmEntity* clone() const override;

	/// @brief 获取实体类型
	DM::EntityType getEntityType() const override;

	/// @brief 获取线段几何数据
	XLineData getData() const;
	double getAngle() const;

	DmVector getBasePoint();
	void setBasePoint(const DmVector& pt);

	DmVector getDirecion();
	void setDirection(const DmVector& vec);

	DmVectorSolutions getRefPoints() const override;
	DmVector getNearestEndpoint(const DmVector& coord, double* dist = nullptr) const override;
	DmVector getNearestPointOnEntity(const DmVector& coord, bool onEntity = true, double* dist = nullptr, DmEntity** entity = nullptr) const override;
	DmVector getNearestMiddle(const DmVector& coord, double* dist = nullptr, int middlePoints = 1) const override;

	void move(const DmVector& offset) override;
	void rotate(const double& angle);
	void rotate(const DmVector& center, const DmVector& angleVector) override;
	void scale(const DmVector& factor) override;
	void scale(const DmVector& center, const DmVector& factor) override;
	void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;
	void moveRef(const DmVector& ref, const DmVector& offset) override;

	void calculateBorders() override;

	Quadratic getQuadratic() const;

	std::list<DmEntity*> getSubEntities() const override;

	// persistent helper
	virtual void saveStream(OutputStream& wrt) const override;
	virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
	virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;

protected:
	XLineData data;

private:
	bool isModify;
};

#endif // !DMXLINE_H

