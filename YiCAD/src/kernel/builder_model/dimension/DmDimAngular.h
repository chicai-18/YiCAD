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


/// @file DmDimAngular.h
/// @brief 角度标注类

#ifndef DMDIMANGULAR_H
#define DMDIMANGULAR_H

#include "DmDimension.h"
#include "DmDimensionStyle.h"

/// @brief 角度标注的数据
struct DmDimAngularData
{
	DmDimAngularData() = default;
	DmDimAngularData(const DmVector& line1StartPt, const DmVector& line1EndPt, 
		const DmVector& line2StartPt, const DmVector& line2EndPt, const DmVector& ptOnArc);

	DmVector line1StartPt;	//所标注的直线1起点
	DmVector line1EndPt;	//所标注的直线1终点
	DmVector line2StartPt;	//所标注的直线2起点
	DmVector line2EndPt;	//所标注的直线2终点
	DmVector ptOnArc;	//标注圆弧上的一个点（用来定位标注的圆弧所在象限）
};

/// @brief 角度标注
class DmDimAngular : public DmDimension
{
	TYPESYSTEM_HEADER();
public:
	DmDimAngular() = default;
	DmDimAngular(DmEntityContainer* parent, const DmDimensionData& d, const DmDimAngularData& ed);

	DmEntity* clone() const override;
	DM::EntityType getEntityType() const override;
	DmDimAngularData getEData() const;
	DmVector getCenter() const override;
	DmVectorSolutions getRefPoints() const override;

	void updateDim(bool autoText = false) override;

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
	/// @brief 计算一些后续需要的参数
	void calculateData();
	/// @brief 计算标注界线一些参数
	void getBoundaryLineInfos(bool& needBoundLine1, DmVector& boundaryLineDir1, bool& needBoundLine2, DmVector& boundaryLineDir2,
		double& boundaryLine1StartOffset, double& boundaryLine1EndOffset, double& boundaryLine2StartOffset, double& boundaryLine2EndOffset) const;
	/// @brief 绘制内部标注（除标注界线）
	/// @param extendBoundaryLine1 [out] 延伸标注界线1（水平方向在"尺寸界线上方"才可能延伸标注界线）
	/// @param extendBoundaryLine2 [out] 延伸标注界线2
	/// @param clipUnderLine [out] 是否裁剪标注界线（只有垂直方向居中才可能裁剪）
	void updateInnerDim(const DmVector& boundaryLineDir1, const DmVector& boundaryLineDir2, const double boundaryLine1StartOffset, const double boundaryLine1EndOffset,
		const double boundaryLine2StartOffset, const double boundaryLine2EndOffset,
		bool& extendBoundaryLine1, bool& extendBoundaryLine2, bool& clipBoundLine, DmEntityContainer* pText, const DmVector& textSize);
	void updateOuterDim(const DmVector& boundaryLineDir1, const DmVector& boundaryLineDir2, const double boundaryLine1StartOffset, const double boundaryLine1EndOffset,
		const double boundaryLine2StartOffset, const double boundaryLine2EndOffset,
		bool& extendBoundaryLine1, bool& extendBoundaryLine2, bool& clipBoundLine, DmEntityContainer* pText, const DmVector& textSize);

protected:
	DmDimAngularData edata;

private:
	DmVector m_center;		//圆弧中心
	double m_radius;		//圆弧半径
	DmVector m_arcEndPt1;	//圆弧与标注线1交点
	DmVector m_arcEndPt2;	//圆弧与标注线2交点
	DmVector m_arcEndPt1Dir;//圆心到m_arcEndPt1方向向量
	DmVector m_arcEndPt2Dir;
	double m_arcStartAngle;	//圆弧起始角度
	double m_arcEndAngle;	//圆弧终止角度
};

#endif
