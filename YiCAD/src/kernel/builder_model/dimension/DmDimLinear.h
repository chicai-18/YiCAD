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


/// @file DmDimLinear.h
/// @brief 线性标注类

#ifndef DMDIMLINEAR_H
#define DMDIMLINEAR_H

#include "DmDimension.h"
#include "DmDimensionStyle.h"

struct DmDimLinearData
{
	DmDimLinearData();
	DmDimLinearData(const DmVector& extensionPoint1, const DmVector& extensionPoint2);

	/// @brief 尺寸界线1的起点
	DmVector extensionPoint1;
	/// @brief 尺寸界线2的起点
	DmVector extensionPoint2;
};

/// @brief 线性标注
class DmDimLinear : public DmDimension
{
	TYPESYSTEM_HEADER();
public:
	DmDimLinear() = default;
	DmDimLinear(DmEntityContainer* parent, const DmDimensionData& d, const DmDimLinearData& ed);
	virtual ~DmDimLinear() = default;

	virtual DmEntity* clone() const;

	/// @return DM::EntityDimLinear
	virtual DM::EntityType getEntityType() const;

	DmDimLinearData getEData() const;

	virtual DmVectorSolutions getRefPoints() const;

	virtual void updateDim(bool autoText = false);

	DmVector getExtensionPoint1() const;

	DmVector getExtensionPoint2() const;

	double getAngle() const;

	void setAngle(double a);

	double getDistance() const;

	virtual void move(const DmVector& offset);
	virtual void rotate(const DmVector& center, const DmVector& angleVector);
	virtual void scale(const DmVector& center, const DmVector& factor);
	virtual void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2);
	virtual bool hasEndpointsWithinWindow(const DmVector& v1, const DmVector& v2);
	virtual void stretch(const DmVector& firstCorner, const DmVector& secondCorner, const DmVector& offset);
	virtual void moveRef(const DmVector& ref, const DmVector& offset);

	// persistent helper
	virtual void saveStream(OutputStream& wrt) const override;
	virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
	virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

private:
	//内部使用
	void getBoundaryLineInfos(DmVector& dimLineStartPt, DmVector& dimLineEndPt, DmVector& boundaryLineDir1, DmVector& boundaryLineDir2,
		double& boundaryLine1StartOffset, double& boundaryLine1EndOffset, double& boundaryLine2StartOffset, double& boundaryLine2EndOffset) const;
	/// @brief 绘制内部标注（除标注界线）
	/// @param extendBoundaryLine1 [out] 延伸标注界线1（水平方向在"尺寸界线上方"才可能延伸标注界线）
	/// @param extendBoundaryLine2 [out] 延伸标注界线2
	/// @param clipUnderLine [out] 是否裁剪标注界线（只有垂直方向居中才可能裁剪）
	void updateInnerDim(const DmVector& dimLineStartPtTemp, const DmVector& dimLineEndPtTemp, const DmVector& boundaryLineDir1, const DmVector& boundaryLineDir2, const double boundaryLine1StartOffset, const double boundaryLine1EndOffset,
		const double boundaryLine2StartOffset, const double boundaryLine2EndOffset,
		bool& extendBoundaryLine1, bool& extendBoundaryLine2, bool& clipBoundLine, DmEntityContainer* pText, const DmVector& textSize);
	void updateOuterDim(const DmVector& dimLineStartPtTemp, const DmVector& dimLineEndPtTemp, const DmVector& boundaryLineDir1, const DmVector& boundaryLineDir2, const double boundaryLine1StartOffset, const double boundaryLine1EndOffset,
		const double boundaryLine2StartOffset, const double boundaryLine2EndOffset,
		bool& extendBoundaryLine1, bool& extendBoundaryLine2, bool& clipBoundLine, DmEntityContainer* pText, const DmVector& textSize);
protected:
	// Extended data.
	DmDimLinearData edata;
};

#endif
