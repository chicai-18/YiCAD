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


/// @file DmDimDiametric.h
/// @brief 直径标注类

#ifndef DMDIMDIAMETER_H
#define DMDIMDIAMETER_H

#include "DmDimension.h"
#include "DmDimensionStyle.h"

struct DmDimDiametricData
{
	DmDimDiametricData();

	/// @param leader 阴险长度
	DmDimDiametricData(const DmVector& definitionPoint, double leader);

	// （箭头端点，DmDimensionData的definitionPoint是另一端箭头端点）
	DmVector endPoint;
	// 引线长度。负值内部标注，正值外部标注. 
	double leader;
};

/// @brief 直径标注
class DmDimDiametric : public DmDimension
{
	TYPESYSTEM_HEADER();
public:
	DmDimDiametric() = default;
	DmDimDiametric(DmEntityContainer* parent, const DmDimensionData& d, const DmDimDiametricData& ed);

	DmEntity* clone() const override;
	DM::EntityType getEntityType() const override;
	DmDimDiametricData getEData() const;
	DmVectorSolutions getRefPoints() const override;
	void updateDim(bool autoText = false) override;
	DmVector getDefinitionPoint();
	double getLeader();

	void move(const DmVector& offset) override;
	void rotate(const DmVector& center, const DmVector& angleVector) override;
	void scale(const DmVector& center, const DmVector& factor) override;
	void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;

	void moveRef(const DmVector& ref, const DmVector& offset) override;

	// persistent helper
	virtual void saveStream(OutputStream& wrt) const override;
	virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
	virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

private:
	/// @brief 绘制内部标注（除标注界线）
	/// @param extendBoundaryLine1 [out] 延伸标注界线1（水平方向在"尺寸界线上方"才可能延伸标注界线）
	/// @param extendBoundaryLine2 [out] 延伸标注界线2
	/// @param clipUnderLine [out] 是否裁剪标注界线（只有垂直方向居中才可能裁剪）
	void updateInnerDim(DmEntityContainer* pText, const DmVector& textSize);
	void updateOuterDim(DmEntityContainer* pText, const DmVector& textSize);

protected:
	DmDimDiametricData edata;
};

#endif
