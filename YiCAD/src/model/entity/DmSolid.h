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


/// @file DmSolid.h
/// @brief 填充实体类（如尺寸箭头），支持三角形和四边形

#ifndef DMSOLID_H
#define DMSOLID_H

#include <array>

#include "DmAtomicEntity.h"
#include "DmVector.h"
#include "SolidData.h"

/// @brief 用于填充实体（如尺寸箭头）的类
class DmSolid : public DmAtomicEntity
{
	TYPESYSTEM_HEADER();

public:
	DmSolid() = default;
	DmSolid(DmEntity* parent, const SolidData& d);

	DmEntity* clone() const override;
	DM::EntityType getEntityType() const override;
	SolidData const& getData() const;
	DmVector getCorner(int num) const;

	DmVector getNearestEndpoint(const DmVector& coord, double* dist = nullptr) const override;
	DmVector getNearestPointOnEntity(const DmVector& coord, bool onEntity = true, double* dist = nullptr, DmEntity** entity = nullptr) const override;
	DmVector getNearestCenter(const DmVector& coord, double* dist = nullptr) const override;
	DmVector getNearestMiddle(const DmVector& coord, double* dist = nullptr, int middlePoints = 1) const override;

	double getDistanceToPoint(const DmVector& coord, DmEntity** entity = nullptr, DM::ResolveLevel level = DM::ResolveNone) const override;

	void move(const DmVector& offset) override;
	void rotate(const DmVector& center, const DmVector& angleVector) override;
	void scale(const DmVector& center, const DmVector& factor) override;
	void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;

	void calculateBorders() override;

	/// @brief Check if is intersected by v1, v2 window.
	/// @return true if is crossed false otherwise.
	bool isInCrossWindow(const DmVector& v1, const DmVector& v2) const;

	/// @brief 对于2点及凸度构成的圆弧，计算其圆弧信息（参考DmPolyline::createVertex）
	void getArcInfo(const DmVector& pt1, const DmVector& pt2, const double bulge, DmVector& center, double& radius, double& startAngle, double& endAngle, bool& reversed) const;

	std::list<DmEntity*> getSubEntities() const override;

	// persistent helper
	virtual void saveStream(OutputStream& wrt) const override;
	virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
	virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

protected:
	SolidData data;

private:
	// helper method for getNearestPointOnEntity
	bool sign(const DmVector& v1, const DmVector& v2, const DmVector& v3) const;
	void setDistPtr(double* dist, const double value) const;

private:
	bool isModify;
};

#endif
