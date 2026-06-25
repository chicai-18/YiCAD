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


/// @file DmDimLinear.cpp
/// @brief DmDimLinear 线性标注类的实现

#include<iostream>
#include<cmath>
#include "DmDimLinear.h"
#include "DmLine.h"
#include "DmConstructionLine.h"
#include "DmMText.h"
#include "DmSolid.h"
#include "DmDocument.h"
#include "DmBlockReference.h" 
#include "Math2d.h"
#include "GeometryMethods.h"
#include "ApplicationWindow.h"
#include "Debug.h"

TYPESYSTEM_SOURCE(DmDimLinear, DmEntity, 0);

DmDimLinearData::DmDimLinearData()
	: extensionPoint1(false)
	, extensionPoint2(false)
{
}

DmDimLinearData::DmDimLinearData(const DmVector& _extensionPoint1, const DmVector& _extensionPoint2)
	: extensionPoint1(_extensionPoint1)
	, extensionPoint2(_extensionPoint2)
{
}

/// @para parent Parent Entity Container.
/// @para d Common dimension geometrical data.
/// @para ed Extended geometrical data for linear dimension.
DmDimLinear::DmDimLinear(DmEntityContainer* parent, const DmDimensionData& d, const DmDimLinearData& ed)
	: DmDimension(parent, d)
	, edata(ed)
{
	calculateBorders();
}

DmEntity* DmDimLinear::clone() const
{
	DmDimLinear* d = new DmDimLinear(*this);
	d->container = new DmEntityContainer(d);
	d->update();
	return d;
}

DM::EntityType DmDimLinear::getEntityType() const
{
	return DM::EntityDimLinear;
}

DmDimLinearData DmDimLinear::getEData() const
{
	return edata;
}

DmVector DmDimLinear::getExtensionPoint1() const
{
	return edata.extensionPoint1;
}

DmVector DmDimLinear::getExtensionPoint2() const
{
	return edata.extensionPoint2;
}

double DmDimLinear::getAngle() const
{
	return data.angle;
}

void DmDimLinear::setAngle(double a)
{
	data.angle = Math2d::correctAngle(a);
}

double DmDimLinear::getDistance() const
{
	double dist = 0.0;
	DmVector vec = edata.extensionPoint2 - edata.extensionPoint1;
	if (abs(vec.x) + abs(vec.y) < DM_TOLERANCE)
		return dist;
	DmVector linePt2 = edata.extensionPoint1 + DmVector(Math2d::correctAngle(data.angle - M_PI_2));
	DmConstructionLine line(nullptr, DmConstructionLineData(edata.extensionPoint1, linePt2));
	return line.getDistanceToPoint(edata.extensionPoint2);
}

DmVectorSolutions DmDimLinear::getRefPoints() const
{
	return DmVectorSolutions({ edata.extensionPoint1, edata.extensionPoint2, data.textCenter });
}

bool DmDimLinear::hasEndpointsWithinWindow(const DmVector& v1, const DmVector& v2)
{
	return (edata.extensionPoint1.isInWindow(v1, v2) || edata.extensionPoint2.isInWindow(v1, v2));
}

/// @brief text or the position, alignment, .. changes.
/// @param autoText Automatically reposition the text label
void DmDimLinear::updateDim(bool autoText)
{
	clear();

	//计算边界线一些参数
	double dist = getDistance();
	DmVector boundaryLineDir1, boundaryLineDir2, dimLineStartPt, dimLineEndPt;	//dimLineStartPt，dimLineEndPt为中间结果，不是最终结果
	double boundaryLine1StartOffset, boundaryLine1EndOffset, boundaryLine2StartOffset, boundaryLine2EndOffset;
	getBoundaryLineInfos(dimLineStartPt, dimLineEndPt, boundaryLineDir1, boundaryLineDir2, boundaryLine1StartOffset, boundaryLine1EndOffset, boundaryLine2StartOffset, boundaryLine2EndOffset);
	
	//绘制文字，获得文字范围
	DmEntityContainer* pText = DmDimension::getTextEntity(dist);
	const DmVector textSize = pText->getWidthHeight();
	double txtExt = textSize.x;
	bool dimOut = false;	//是否为外部标注。当文字不能在2点间放下时，采用外部标注
	double extensionDist =dist;	//标注边界间距
	bool extendBoundaryLine1 = false;	//是否延伸尺寸界线。当文字在尺寸界线上时，需要延伸尺寸界线
	bool extendBoundaryLine2 = false;
	if (extensionDist < txtExt + data.arrowSize() * 2.5)
	{
		dimOut = true;
	}
	bool clipBoundLine = false;	//标注界线是否被裁剪。当文字垂直位置居中，水平在标注界线上时，需要裁剪
	//绘制标注线及箭头
	if (dimOut)
	{
		//外部标注
		updateOuterDim(dimLineStartPt, dimLineEndPt, boundaryLineDir1, boundaryLineDir2, boundaryLine1StartOffset, boundaryLine1EndOffset, boundaryLine2StartOffset, boundaryLine2EndOffset, extendBoundaryLine1, extendBoundaryLine2, clipBoundLine, pText, textSize);
	}
	else
	{
		//内部标注
		updateInnerDim(dimLineStartPt, dimLineEndPt, boundaryLineDir1, boundaryLineDir2, boundaryLine1StartOffset, boundaryLine1EndOffset, boundaryLine2StartOffset, boundaryLine2EndOffset, extendBoundaryLine1, extendBoundaryLine2, clipBoundLine, pText, textSize);
	}
	//绘制标注边界线
	DmLayer* layer = getLayer();
	if (!data.hideBoundLine1())
	{
		double extendLen = 0.0;
		if (extendBoundaryLine1 && !clipBoundLine)
			extendLen = textSize.x;
		DmLine* boundLine1 = new DmLine(this, edata.extensionPoint1 + boundaryLineDir1 * boundaryLine1StartOffset, edata.extensionPoint1 + boundaryLineDir1 * (boundaryLine1EndOffset + extendLen));
		boundLine1->setPen(DmPen(data.boundLineColor(), data.boundLineWidth(), data.boundLineType()));
		boundLine1->setLayer(layer);
		addEntity(boundLine1);
	}
	if (!data.hideBoundLine2())
	{
		double extendLen = 0.0;
		if (extendBoundaryLine2 && !clipBoundLine)
			extendLen = textSize.x;
		DmLine* boundLine2 = new DmLine(this, edata.extensionPoint2 + boundaryLineDir2 * boundaryLine2StartOffset, edata.extensionPoint2 + boundaryLineDir2 * (boundaryLine2EndOffset + extendLen));
		boundLine2->setPen(DmPen(data.boundLineColor(), data.boundLineWidth(), data.boundLineType()));
		boundLine2->setLayer(layer);
		addEntity(boundLine2);
	}
	calculateBorders();
}

void DmDimLinear::move(const DmVector& offset)
{
	DmDimension::move(offset);

	edata.extensionPoint1.move(offset);
	edata.extensionPoint2.move(offset);
	update();
}

void DmDimLinear::rotate(const DmVector& center, const DmVector& angleVector)
{
	DmDimension::rotate(center, angleVector);
	edata.extensionPoint1.rotate(center, angleVector);
	edata.extensionPoint2.rotate(center, angleVector);
	data.angle = Math2d::correctAngle(data.angle + angleVector.angle());
	update();
}

void DmDimLinear::scale(const DmVector& center, const DmVector& factor)
{
	DmDimension::scale(center, factor);

	edata.extensionPoint1.scale(center, factor);
	edata.extensionPoint2.scale(center, factor);
	update();
}

void DmDimLinear::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
	DmDimension::mirror(axisPoint1, axisPoint2);

	edata.extensionPoint1.mirror(axisPoint1, axisPoint2);
	edata.extensionPoint2.mirror(axisPoint1, axisPoint2);

	DmVector vec;
	vec.setPolar(1.0, data.angle);
	vec.mirror(DmVector(0.0, 0.0), axisPoint2 - axisPoint1);
	data.angle = vec.angle();

	update();
}

void DmDimLinear::stretch(const DmVector& firstCorner, const DmVector& secondCorner, const DmVector& offset)
{
	if (getMin().isInWindow(firstCorner, secondCorner) && getMax().isInWindow(firstCorner, secondCorner))
	{
		move(offset);
	}
	else
	{
		if (edata.extensionPoint1.isInWindow(firstCorner, secondCorner))
		{
			edata.extensionPoint1.move(offset);
		}
		if (edata.extensionPoint2.isInWindow(firstCorner, secondCorner))
		{
			edata.extensionPoint2.move(offset);
		}
	}
	updateDim(true);
}

void DmDimLinear::moveRef(const DmVector& ref, const DmVector& offset)
{
	if (ref.distanceTo(data.definitionPoint) < 1.0e-4)
	{
		data.definitionPoint += offset;
		updateDim(true);
	}
	else if (ref.distanceTo(data.textCenter) < 1.0e-4)
	{
		// 标注线方向与文字方向平行，经过extensionPoint1
		DmVector dimLineDir = DmVector::polar(1.0, data.angle);
		DmVector normal(-dimLineDir.y, dimLineDir.x);
		DmConstructionLine line(nullptr, DmConstructionLineData(edata.extensionPoint1, edata.extensionPoint1 + dimLineDir));

		DmVector projOrigin = line.getNearestPointOnEntity(data.textCenter);
		double d1 = DmVector::dotP(data.textCenter - projOrigin, normal);

		DmVector newTextCenter = data.textCenter + offset;
		DmVector projNew = line.getNearestPointOnEntity(newTextCenter);
		double d2 = DmVector::dotP(newTextCenter - projNew, normal);

		data.definitionPoint += normal * (d2 - d1);
		updateDim(true);
	}
	else if (ref.distanceTo(edata.extensionPoint1) < 1.0e-4)
	{
		edata.extensionPoint1 += offset;
		updateDim(true);
	}
	else if (ref.distanceTo(edata.extensionPoint2) < 1.0e-4)
	{
		edata.extensionPoint2 += offset;
		updateDim(true);
	}
}

void DmDimLinear::saveStream(OutputStream& wrt) const
{
	DmDimension::saveStream(wrt);

	double angle = getAngle();
	DmVector line1Point = getExtensionPoint1();
	DmVector line2Point = getExtensionPoint2();
	DmVector midLinePoint = getDefinitionPoint();

	wrt << (double)angle << (double)line1Point.x << (double)line1Point.y << (double)line2Point.x << (double)line2Point.y << (double)midLinePoint.x << (double)midLinePoint.y;
}

void DmDimLinear::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
	int fileRev = getRevisionId("DmDimLinear", revs);
	if (revId > fileRev)
	{
        DmDimension::restoreStream(reader, revs);
		// 老文件格式
		restoreStreamWithRev(reader, fileRev);
	}
	else
	{
        restoreStream(reader);
	}
}

void DmDimLinear::restoreStreamWithRev(InputStream& rdr, int rev)
{
	if (rev == 0)
	{
	}
	else //big change, e.g. change supper class of DmDimLinear
	{
		//step1.
		// read all legacy data one by one
	}
}

void DmDimLinear::restoreStream(InputStream& rdr)
{
    DmDimension::restoreStream(rdr);

    double angle;
    DmVector line1Point(true), line2Point(true), midLinePoint(true);
    rdr >> (double&)angle >> (double&)line1Point.x >> (double&)line1Point.y >> (double&)line2Point.x >> (double&)line2Point.y >> (double&)midLinePoint.x >> (double&)midLinePoint.y;

    setAngle(angle);
    data.definitionPoint = midLinePoint;
    edata.extensionPoint1 = line1Point;
    edata.extensionPoint2 = line2Point;
    update();
}

void DmDimLinear::getBoundaryLineInfos(DmVector& dimLineStartPt, DmVector& dimLineEndPt, DmVector& boundaryLineDir1, DmVector& boundaryLineDir2,
	double& boundaryLine1StartOffset, double& boundaryLine1EndOffset, double& boundaryLine2StartOffset, double& boundaryLine2EndOffset) const
{
	//计算标注线起点终点，边界线偏移坐标
	DmVector angleDir(Math2d::correctAngle(data.angle - M_PI_2));
	DmVector defPointProject1 = GeometryMethods::getPerpendicularFoot(edata.extensionPoint1, edata.extensionPoint1 + angleDir, data.definitionPoint);
	const double definitionOffset1 = defPointProject1.distanceTo(edata.extensionPoint1);
	boundaryLine1StartOffset = data.startPtOffset() <= definitionOffset1 ? data.startPtOffset() : definitionOffset1;	//标注界线起点偏移距离
	if (data.isFixedBoundLineLength() && data.fixedBoundLineLength() < definitionOffset1 - data.startPtOffset())
	{
		boundaryLine1StartOffset = definitionOffset1 - data.fixedBoundLineLength();
	}
	boundaryLine1EndOffset = definitionOffset1 + data.extendDimLine();//标注界线终点偏移距离
	dimLineStartPt = defPointProject1;
	DmVector defPointProject2 = GeometryMethods::getPerpendicularFoot(edata.extensionPoint2, edata.extensionPoint2 + angleDir, data.definitionPoint);
	const double definitionOffset2 = defPointProject2.distanceTo(edata.extensionPoint2);
	boundaryLine2StartOffset = data.startPtOffset() <= definitionOffset2 ? data.startPtOffset() : definitionOffset2;	//标注界线起点偏移距离
	if (data.isFixedBoundLineLength() && data.fixedBoundLineLength() < definitionOffset2 - data.startPtOffset())
	{
		boundaryLine2StartOffset = definitionOffset2 - data.fixedBoundLineLength();
	}
	boundaryLine2EndOffset = definitionOffset2 + data.extendDimLine();//标注界线终点偏移距离
	dimLineEndPt = defPointProject2;

	//计算边界线方向
	DmVector tempVec1 = defPointProject1 - edata.extensionPoint1;
	double dotP1 = tempVec1.dotP(angleDir);
	if (dotP1 < 0)
	{
		boundaryLineDir1 = -angleDir;
	}
	else
	{
		boundaryLineDir1 = angleDir;
	}
	DmVector tempVec2 = defPointProject2 - edata.extensionPoint2;
	double dotP2 = tempVec2.dotP(angleDir);
	if (dotP2 < 0)
	{
		boundaryLineDir2 = -angleDir;
	}
	else
	{
		boundaryLineDir2 = angleDir;
	}
}

void DmDimLinear::updateInnerDim(const DmVector& dimLineStartPtTemp, const DmVector& dimLineEndPtTemp, const DmVector& boundaryLineDir1, const DmVector& boundaryLineDir2, const double boundaryLine1StartOffset, const double boundaryLine1EndOffset, const double boundaryLine2StartOffset, const double boundaryLine2EndOffset, bool& extendBoundaryLine1, bool& extendBoundaryLine2, bool& clipBoundLine, DmEntityContainer* pText, const DmVector& textSize)
{
	//生成标注线与箭头
	DmVector dimLineStartPt = dimLineStartPtTemp;
	DmVector dimLineEndPt = dimLineEndPtTemp;
	const DmVector arrow1Pos = dimLineStartPt;
	const DmVector arrow2Pos = dimLineEndPt;
	const DmVector dimLineDir = (dimLineEndPt-dimLineStartPt).normalize();
	DmDocument* curDoc = static_cast<DmDocument*>(ApplicationWindow::getAppWindow()->getDocument());
	DmBlockTable* arrowBlocks = curDoc->getDimStyleTable()->getArrowBlocks();
	double arrow1CutDist = DmDimensionStyle::getArrowCutDistance(data.firstArrow()) * data.arrowSize();
	double arrow2CutDist = DmDimensionStyle::getArrowCutDistance(data.secondArrow()) * data.arrowSize();
	bool isDefPtUp1 = true;	//edata.definitionPoint在标注线上方还是下方
	double z1 = DmVector::crossP(dimLineDir, boundaryLineDir1).z;
	if (z1 < 0)
		isDefPtUp1 = false;
	bool isDefPtUp2 = true;	//edata.definitionPoint在标注线上方还是下方
	double z2 = DmVector::crossP(dimLineDir, boundaryLineDir2).z;
	if (z2 < 0)
		isDefPtUp2 = false;

	DmLayer* layer = getLayer();
	DmLine* dimLine = nullptr;
	if (!data.hideDimLine1() || !data.hideDimLine2())
	{
		//标注线
		if (!data.hideDimLine1())
		{
			dimLineStartPt += dimLineDir * arrow1CutDist;
		}
		else
		{
			dimLineStartPt = (dimLineStartPt + dimLineEndPt) / 2.0;
		}
		if (!data.hideDimLine2())
		{
			dimLineEndPt -= dimLineDir * arrow2CutDist;
		}
		else
		{
			dimLineEndPt = (dimLineStartPt + dimLineEndPt) / 2.0;
		}
		dimLine = new DmLine(this, dimLineStartPt, dimLineEndPt);
		dimLine->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
		dimLine->setLayer(layer);
		addEntity(dimLine);
		//箭头


		DmVector arrowScale(data.arrowSize(), data.arrowSize());
		DmVector space(0.0, 0.0);
		if (!data.hideDimLine1())
		{
			DmBlockReferenceData arrow1Data(DmDimensionStyle::getArrowBlockName(data.firstArrow()), arrow1Pos, arrowScale, Math2d::correctAngle(dimLineDir.angle() + M_PI),
				1, 1, space, arrowBlocks);
			DmBlockReference* arrow1 = new DmBlockReference(this, arrow1Data);
			arrow1->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
			//arrow1->setPen(DmPen(styleDataRef.lineData.dimLineColor, DM::Width00, DM::SolidLine));
			arrow1->setLayer(layer);
			arrow1->update();
			arrow1->forcedCalculateBorders();
			addEntity(arrow1);
		}
		if (!data.hideDimLine2())
		{
			DmBlockReferenceData arrow2Data(DmDimensionStyle::getArrowBlockName(data.secondArrow()), arrow2Pos, arrowScale, dimLineDir.angle(),
				1, 1, space, arrowBlocks);
			DmBlockReference* arrow2 = new DmBlockReference(this, arrow2Data);
			arrow2->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
			//arrow2->setPen(DmPen(styleDataRef.lineData.dimLineColor, DM::Width00, DM::SolidLine));
			arrow2->setLayer(layer);
			arrow2->update();
			arrow2->forcedCalculateBorders();
			addEntity(arrow2);
		}

	}

	//移动标注文字到正确位置
	bool clipUnderLine = false;	//是否剪断标注线。如果文字垂直方向在标注线居中，则需要剪断下面的标注线
	DmVector textBasePt(0.0, 0.0);
	DmVector textDir(dimLineDir);
	bool textDirNeedReverse = false;
	if (abs(textDir.x) < DM_TOLERANCE)	//保证文字方向自左向右，竖直时自下向上
	{
		if (textDir.y < 0)
		{
			textDirNeedReverse = true;
		}
	}
	else if (textDir.x < 0)
	{
		textDirNeedReverse = true;
	}
	if (textDirNeedReverse)
	{
		textDir = -textDir;
	}
	DmVector textOffsetDir = DmVector(textDir).rotate(M_PI / 2.0);
	DmVector dimLineCenter = (arrow1Pos + arrow2Pos) / 2.0;
	DmDimensionStyleTextData::TextHorizontalPos hPos = data.textHorizontalPos();
	switch (hPos)
	{
	case DmDimensionStyleTextData::TextHorizontalPos::Mid:
		textBasePt = dimLineCenter - textDir * textSize.x / 2.0;
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::FirstBoundaryLine:
		textBasePt = arrow1Pos + textDir * data.arrowSize();
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::SecondBoundaryLine:
		textBasePt = arrow2Pos - textDir * (textSize.x + data.arrowSize());
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::AboveFirstBoundaryLine:
		textDir = textDir.rotate(M_PI / 2.0);
		textOffsetDir = textOffsetDir.rotate(M_PI / 2.0);
		if (isDefPtUp1)
		{
			textBasePt = edata.extensionPoint1 + boundaryLineDir1 * boundaryLine1EndOffset;
		}
		else
		{
			textBasePt = edata.extensionPoint1 + boundaryLineDir1 * (boundaryLine1EndOffset + textSize.x);
		}
		extendBoundaryLine1 = true;
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::AboveSecondBoundaryLine:
		textDir = textDir.rotate(M_PI / 2.0);
		textOffsetDir = textOffsetDir.rotate(M_PI / 2.0);
		if (isDefPtUp2)
		{
			textBasePt = edata.extensionPoint2 + boundaryLineDir2 * boundaryLine2EndOffset;
		}
		else
		{
			textBasePt = edata.extensionPoint2 + boundaryLineDir2 * (boundaryLine2EndOffset + textSize.x);
		}

		extendBoundaryLine2 = true;
		break;
	default:
		break;
	}
	DmDimensionStyleTextData::TextVerticalPos vPos = data.textVerticalPos();
	switch (vPos)
	{
	case DmDimensionStyleTextData::TextVerticalPos::Mid:
		textBasePt -= textOffsetDir * textSize.y / 2.0;
		clipUnderLine = true;
		break;
	case DmDimensionStyleTextData::TextVerticalPos::Up:
	case DmDimensionStyleTextData::TextVerticalPos::Extern:
	case DmDimensionStyleTextData::TextVerticalPos::JIS:
		textBasePt += textOffsetDir * data.offsetFromDimLine();
		break;
	case DmDimensionStyleTextData::TextVerticalPos::Down:
		textBasePt -= textOffsetDir * (data.offsetFromDimLine() + textSize.y);
		break;
	default:
		break;
	}
	if (clipUnderLine && nullptr != dimLine)	//剪断标注线
	{
		if (hPos == DmDimensionStyleTextData::TextHorizontalPos::Mid || hPos == DmDimensionStyleTextData::TextHorizontalPos::FirstBoundaryLine || hPos == DmDimensionStyleTextData::TextHorizontalPos::SecondBoundaryLine)
		{
			bool isClip = clipLine(dimLine, textBasePt, textDir, textSize.x);
			if (isClip)
				removeEntity(dimLine);
		}
	}
	clipBoundLine = false;
	if (clipUnderLine)
	{
		if (hPos == DmDimensionStyleTextData::TextHorizontalPos::AboveFirstBoundaryLine || hPos == DmDimensionStyleTextData::TextHorizontalPos::AboveSecondBoundaryLine)
		{
			clipBoundLine = true;
		}
	}

	if (data.viewDirection() == DmDimensionStyleTextData::ViewDirection::RightToLeft)
	{
		pText->rotateAngle(textSize / 2.0, M_PI);
	}
	pText->move(textBasePt);
	pText->rotate(textBasePt, textDir);
	data.textCenter = textBasePt + textSize / 2.0;
	data.textCenter.rotate(textBasePt, textDir.angle());
}

void DmDimLinear::updateOuterDim(const DmVector& dimLineStartPtTemp, const DmVector& dimLineEndPtTemp, const DmVector& boundaryLineDir1, const DmVector& boundaryLineDir2, const double boundaryLine1StartOffset, const double boundaryLine1EndOffset, const double boundaryLine2StartOffset, const double boundaryLine2EndOffset, bool& extendBoundaryLine1, bool& extendBoundaryLine2, bool& clipBoundLine, DmEntityContainer* pText, const DmVector& textSize)
{
	//生成标注线与箭头
	DmVector dimLineStartPt = dimLineStartPtTemp;
	DmVector dimLineEndPt = dimLineEndPtTemp;
	const DmVector arrow1Pos = dimLineStartPt;
	const DmVector arrow2Pos = dimLineEndPt;
	const DmVector dimLineDir = (dimLineEndPt - dimLineStartPt).normalize();
	DmDocument* curDoc = static_cast<DmDocument*>(ApplicationWindow::getAppWindow()->getDocument());
	DmBlockTable* arrowBlocks = curDoc->getDimStyleTable()->getArrowBlocks();
	double arrow1CutDist = DmDimensionStyle::getArrowCutDistance(data.firstArrow()) * data.arrowSize();
	double arrow2CutDist = DmDimensionStyle::getArrowCutDistance(data.secondArrow()) * data.arrowSize();
	bool isDefPtUp1 = true;	//edata.definitionPoint在标注线上方还是下方
	double z1 = DmVector::crossP(dimLineDir, boundaryLineDir1).z;
	if (z1 < 0)
		isDefPtUp1 = false;
	bool isDefPtUp2 = true;	//edata.definitionPoint在标注线上方还是下方
	double z2 = DmVector::crossP(dimLineDir, boundaryLineDir2).z;
	if (z2 < 0)
		isDefPtUp2 = false;

	DmLayer* layer = getLayer();
	DmLine* extendLine1 = nullptr;	//箭头延伸出来的线
	DmLine* extendLine2 = nullptr;
	double extendLen = data.textHeight() * 2.5;
	DmVector extendLine1End = dimLineStartPt - dimLineDir * extendLen;
	DmVector extendLine2End = dimLineEndPt + dimLineDir * extendLen;
	if (!data.hideDimLine1() || !data.hideDimLine2())
	{
		//中间标注线
		if (!data.hideDimLine1() && !data.hideDimLine2())
		{
			DmLine* dimLine = new DmLine(this, dimLineStartPt, dimLineEndPt);
			dimLine->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
			dimLine->setLayer(layer);
			addEntity(dimLine);
		}

		//箭头


		DmVector arrowScale(data.arrowSize(), data.arrowSize());
		DmVector space(0.0, 0.0);
		if (!data.hideDimLine1())
		{
			DmBlockReferenceData arrow1Data(DmDimensionStyle::getArrowBlockName(data.firstArrow()), dimLineStartPt, arrowScale, dimLineDir.angle(),
				1, 1, space, arrowBlocks);
			DmBlockReference* arrow1 = new DmBlockReference(this, arrow1Data);
			arrow1->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
			//arrow1->setPen(DmPen(data.lineData.dimLineColor, DM::Width00, DM::SolidLine));
			arrow1->setLayer(layer);
			arrow1->update();
			arrow1->forcedCalculateBorders();
			addEntity(arrow1);
			//加上一条线
			extendLine1 = new DmLine(this, dimLineStartPt, extendLine1End);
			extendLine1->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
			extendLine1->setLayer(layer);
			addEntity(extendLine1);
		}
		if (!data.hideDimLine2())
		{
			DmBlockReferenceData arrow2Data(DmDimensionStyle::getArrowBlockName(data.secondArrow()), dimLineEndPt, arrowScale, Math2d::correctAngle(dimLineDir.angle() + M_PI),
				1, 1, space, arrowBlocks);
			DmBlockReference* arrow2 = new DmBlockReference(this, arrow2Data);
			arrow2->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
			//arrow2->setPen(DmPen(data.lineData.dimLineColor, DM::Width00, DM::SolidLine));
			arrow2->setLayer(layer);
			arrow2->update();
			arrow2->forcedCalculateBorders();
			addEntity(arrow2);
			//加上一条线
			extendLine2 = new DmLine(this, dimLineEndPt, extendLine2End);
			extendLine2->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
			extendLine2->setLayer(layer);
			addEntity(extendLine2);
		}

	}
	//移动标注文字到正确位置
	bool clipUnderLine = false;	//是否剪断标注线。如果文字垂直方向在标注线居中，则需要剪断下面的标注线
	DmVector textBasePt(0.0, 0.0);
	DmVector textDir(dimLineDir);
	DmVector textOffsetDir = DmVector(dimLineDir).rotate(M_PI / 2.0);
	//DmVector dimLineCenter = (edata.extensionPoint1 + edata.extensionPoint2) / 2.0 + offsetVec;
	DmDimensionStyleTextData::TextHorizontalPos hPos = data.textHorizontalPos();
	switch (hPos)
	{
	case DmDimensionStyleTextData::TextHorizontalPos::Mid:
	case DmDimensionStyleTextData::TextHorizontalPos::SecondBoundaryLine:
		textBasePt = extendLine2End + dimLineDir * textSize.y * 0.2;
		if (extendLine2)
		{
			extendLine2->moveEndpoint(extendLine2End + dimLineDir * textSize.x);
		}
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::FirstBoundaryLine:
		textBasePt = extendLine1End - dimLineDir * (textSize.x + textSize.y * 0.2);
		if (extendLine1)
		{
			extendLine1->moveEndpoint(extendLine1End - dimLineDir * textSize.x);
		}
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::AboveFirstBoundaryLine:
		textDir = textDir.rotate(M_PI / 2.0);
		textOffsetDir = textOffsetDir.rotate(M_PI / 2.0);
		if (isDefPtUp1)
		{
			textBasePt = edata.extensionPoint1 + boundaryLineDir1 * boundaryLine1EndOffset;
		}
		else
		{
			textBasePt = edata.extensionPoint1 + boundaryLineDir1 * (boundaryLine1EndOffset + textSize.x);
		}
		extendBoundaryLine1 = true;
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::AboveSecondBoundaryLine:
		textDir = textDir.rotate(M_PI / 2.0);
		textOffsetDir = textOffsetDir.rotate(M_PI / 2.0);
		if (isDefPtUp2)
		{
			textBasePt = edata.extensionPoint2 + boundaryLineDir2 * boundaryLine2EndOffset;
		}
		else
		{
			textBasePt = edata.extensionPoint2 + boundaryLineDir2 * (boundaryLine2EndOffset + textSize.x);
		}
		extendBoundaryLine2 = true;
		break;
	default:
		break;
	}
	DmDimensionStyleTextData::TextVerticalPos vPos = data.textVerticalPos();
	switch (vPos)
	{
	case DmDimensionStyleTextData::TextVerticalPos::Mid:
		textBasePt -= textOffsetDir * textSize.y / 2.0;
		clipUnderLine = true;
		break;
	case DmDimensionStyleTextData::TextVerticalPos::Up:
	case DmDimensionStyleTextData::TextVerticalPos::Extern:
	case DmDimensionStyleTextData::TextVerticalPos::JIS:
		textBasePt += textOffsetDir * data.offsetFromDimLine();
		break;
	case DmDimensionStyleTextData::TextVerticalPos::Down:
		textBasePt -= textOffsetDir * (data.offsetFromDimLine() + textSize.y);
		break;
	default:
		break;
	}
	if (clipUnderLine)	//剪断标注线
	{
		DmLine* extendLine = nullptr;
		DmVector tempTextDir = textDir;
		if (hPos == DmDimensionStyleTextData::TextHorizontalPos::FirstBoundaryLine)
		{
			extendLine = extendLine1;
			//tempTextDir = -textDir;
		}
		else if (hPos == DmDimensionStyleTextData::TextHorizontalPos::Mid || hPos == DmDimensionStyleTextData::TextHorizontalPos::SecondBoundaryLine)
		{
			extendLine = extendLine2;
		}
		if (nullptr != extendLine)
		{
			bool isClip = clipLine(extendLine, textBasePt, tempTextDir, textSize.x);
			if (isClip)
				removeEntity(extendLine);
		}
	}
	clipBoundLine = false;
	if (clipUnderLine)
	{
		if (hPos == DmDimensionStyleTextData::TextHorizontalPos::AboveFirstBoundaryLine || hPos == DmDimensionStyleTextData::TextHorizontalPos::AboveSecondBoundaryLine)
		{
			clipBoundLine = true;
		}
	}
	if (data.viewDirection() == DmDimensionStyleTextData::ViewDirection::RightToLeft)
	{
		pText->rotateAngle(textSize / 2.0, M_PI);
	}
	pText->move(textBasePt);
	pText->rotate(textBasePt, textDir);
	data.textCenter = textBasePt + textSize / 2.0;
	data.textCenter.rotate(textBasePt, textDir.angle());
}
