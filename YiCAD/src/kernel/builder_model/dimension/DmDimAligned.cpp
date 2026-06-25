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


/// @file DmDimAligned.cpp
/// @brief DmDimAligned 对齐标注类的实现

#include <iostream>
#include <cmath>
#include "DmDimAligned.h"
#include "DmLine.h"

#include "DmDocument.h"
#include "DmUnits.h"
#include "DmConstructionLine.h"
#include "DmDimensionStyle.h"
#include "DmBlockReference.h"
#include "Math2d.h"

TYPESYSTEM_SOURCE(DmDimAligned, DmEntity, 0);

DmDimAlignedData::DmDimAlignedData() 
	: extensionPoint1(false)
	, extensionPoint2(false)
{
}

/// @para extensionPoint1 Definition point. Startpoint of the first extension line.
/// @para extensionPoint2 Definition point. Startpoint of the second extension line.
DmDimAlignedData::DmDimAlignedData(const DmVector& _extensionPoint1, const DmVector& _extensionPoint2)
	: extensionPoint1(_extensionPoint1)
	, extensionPoint2(_extensionPoint2)
{
}

//DmDimAligned::DmDimAligned(DmEntityContainer* parent, const DmDimAlignedData& d)
//	:DmEntityContainer(parent)
//	,edata(d)
//{
//
//}

/// @para parent Parent Entity Container.
/// @para d Common dimension geometrical data.
/// @para ed Extended geometrical data for aligned dimension.
DmDimAligned::DmDimAligned(DmEntityContainer* parent, const DmDimensionData& d, const DmDimAlignedData& ed)
	: DmDimension(parent, d)
	, edata(ed)
{
	calculateBorders();
}

DmEntity* DmDimAligned::clone() const
{
	DmDimAligned* d = new DmDimAligned(*this);
	d->container = new DmEntityContainer(d);
	d->update();
	return d;
}

DM::EntityType DmDimAligned::getEntityType() const
{
	return DM::EntityDimAligned;
}

DmVectorSolutions DmDimAligned::getRefPoints() const
{
	return DmVectorSolutions({ edata.extensionPoint1, edata.extensionPoint2, data.textCenter /*, data.definitionPoint, data.middleOfText */});
}

DmDimAlignedData const& DmDimAligned::getEData() const
{
	return edata;
}

DmVector const& DmDimAligned::getExtensionPoint1() const
{
	return edata.extensionPoint1;
}

DmVector const& DmDimAligned::getExtensionPoint2() const
{
	return edata.extensionPoint2;
}

void DmDimAligned::updateDim(bool autoText /*= false*/)
{
	clear();

	//计算一些参数
	DmVector offsetVec(0.0, 0.0);//标注线偏移矢量
	if ((edata.extensionPoint1 - edata.extensionPoint2).magnitude() < DM_TOLERANCE)	//有时候这2个点完全重合
	{
		offsetVec = data.definitionPoint - edata.extensionPoint1;
	}
	else
	{
		DmConstructionLine tmpLine(nullptr, DmConstructionLineData(edata.extensionPoint1, edata.extensionPoint2));
		DmVector tmpP1 = tmpLine.getNearestPointOnEntity(data.definitionPoint);
		offsetVec = data.definitionPoint - tmpP1;	
	}
	
	const DmVector boundaryLineDir = offsetVec.normalize();
	const double definitionOffset = offsetVec.magnitude();
	double boundaryLineStartOffset = data.startPtOffset() <= definitionOffset ? data.startPtOffset() : definitionOffset;	//标注界线起点偏移距离
	if (data.isFixedBoundLineLength() && data.fixedBoundLineLength() < definitionOffset - data.startPtOffset())
	{
		boundaryLineStartOffset = definitionOffset - data.fixedBoundLineLength();
	}
	double boundaryLineEndOffset = definitionOffset + data.extendDimLine();//标注界线终点偏移距离

	//绘制文字，获得文字范围
	double dist = edata.extensionPoint1.distanceTo(edata.extensionPoint2);
	DmEntityContainer* pText = DmDimension::getTextEntity(dist);
	
	const DmVector textSize = pText->getWidthHeight();
	double txtExt = textSize.x;
	bool dimOut = false;	//是否为外部标注。当文字不能在2点间放下时，采用外部标注
	double extensionDist = edata.extensionPoint1.distanceTo(edata.extensionPoint2);	//标注边界间距
	
	bool extendBoundaryLine1 = false;	//是否延伸尺寸界线。当文字在尺寸界线上时，需要延伸尺寸界线
	bool extendBoundaryLine2 = false;
	if ( extensionDist< txtExt + data.arrowSize() * 2.5)
	{
		dimOut = true;
	}
	bool clipBoundLine = false;
	//绘制标注线及箭头
	if (dimOut)
	{
		//外部标注
		updateOuterDim(offsetVec, boundaryLineDir, boundaryLineStartOffset, boundaryLineEndOffset, extendBoundaryLine1, extendBoundaryLine2,clipBoundLine, pText, textSize);
	}
	else
	{
		//内部标注
		updateInnerDim(offsetVec, boundaryLineDir, boundaryLineStartOffset, boundaryLineEndOffset, extendBoundaryLine1, extendBoundaryLine2, clipBoundLine, pText, textSize);
	}

	//绘制标注边界线
	if (!data.hideBoundLine1())
	{
		double extendLen = 0.0;
		if (extendBoundaryLine1 && !clipBoundLine)
			extendLen = textSize.x;
		DmLine* boundLine1 = new DmLine(this, edata.extensionPoint1 + boundaryLineDir * boundaryLineStartOffset, edata.extensionPoint1 + boundaryLineDir * (boundaryLineEndOffset + extendLen));
		boundLine1->setPen(DmPen(data.boundLineColor(), data.boundLineWidth(), data.boundLineType()));
		boundLine1->setLayer(getLayer());
		addEntity(boundLine1);
	}
	if (!data.hideBoundLine2())
	{
		double extendLen = 0.0;
		if (extendBoundaryLine2 && !clipBoundLine)
			extendLen = textSize.x;
		DmLine* boundLine2 = new DmLine(this, edata.extensionPoint2 + boundaryLineDir * boundaryLineStartOffset, edata.extensionPoint2 + boundaryLineDir * (boundaryLineEndOffset + extendLen));
		boundLine2->setPen(DmPen(data.boundLineColor(), data.boundLineWidth(), data.boundLineType()));
		boundLine2->setLayer(getLayer());
		addEntity(boundLine2);
	}

	calculateBorders();
}

void DmDimAligned::saveStream(OutputStream& wrt) const
{
	DmDimension::saveStream(wrt);

	double angle = data.angle;
	DmVector line1Point = getExtensionPoint1();
	DmVector line2Point = getExtensionPoint2();
	DmVector midLinePoint = getDefinitionPoint();
	std::string strText = getLabel().toStdString();
	//double oblique = data.getOblique(); // 文字倾斜 暂不支持

	wrt << (double)angle << (double)line1Point.x << (double)line1Point.y << (double)line2Point.x << (double)line2Point.y << (double)midLinePoint.x << (double)midLinePoint.y << (std::string)strText;
}

void DmDimAligned::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
	int fileRev = getRevisionId("DmDimAligned", revs);
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

void DmDimAligned::restoreStreamWithRev(InputStream& rdr, int rev)
{
	if (rev == 0)
	{
	}
	else //big change, e.g. change supper class of DmDimAligned
	{
		//step1.
		// read all legacy data one by one
	}
}

void DmDimAligned::restoreStream(InputStream& rdr)
{
    DmDimension::restoreStream(rdr);

    double angle;
    DmVector line1Point(true), line2Point(true), midLinePoint(true);
    std::string strText;

    rdr >> (double&)angle >> (double&)line1Point.x >> (double&)line1Point.y >> (double&)line2Point.x >> (double&)line2Point.y >> (double&)midLinePoint.x >> (double&)midLinePoint.y >> (std::string&)strText;

    data.angle = angle;
    data.definitionPoint = midLinePoint;
    edata.extensionPoint1 = line1Point;
    edata.extensionPoint2 = line2Point;
    setLabel(QString::fromStdString(strText));
	update();
    //edata.oblique = oblique;
}

bool DmDimAligned::hasEndpointsWithinWindow(const DmVector& v1, const DmVector& v2)
{
	return (edata.extensionPoint1.isInWindow(v1, v2) || edata.extensionPoint2.isInWindow(v1, v2));
}

void DmDimAligned::updateInnerDim(const DmVector& offsetVec,  const DmVector& boundaryLineDir, const double boundaryLineStartOffset, const double boundaryLineEndOffset,
	bool& extendBoundaryLine1, bool& extendBoundaryLine2, bool& clipBoundLine, DmEntityContainer* pText, const DmVector& textSize)
{
	//生成标注线与箭头
	const DmVector dimLineDir = (edata.extensionPoint2 - edata.extensionPoint1).normalize();
	DmVector dimLineStartPt = edata.extensionPoint1 + offsetVec;
	DmVector dimLineEndPt = edata.extensionPoint2 + offsetVec;
	DmDocument* curDoc = getDocument();
	DmBlockTable* arrowBlocks = curDoc->getDimStyleTable()->getArrowBlocks();
	double arrow1CutDist = DmDimensionStyle::getArrowCutDistance(data.firstArrow()) * data.arrowSize();
	double arrow2CutDist = DmDimensionStyle::getArrowCutDistance(data.secondArrow()) * data.arrowSize();
	bool isDefPtUp = true;	//edata.definitionPoint在标注线上方还是下方
	double z = DmVector::crossP(dimLineDir, boundaryLineDir).z;
	if (z < 0)
		isDefPtUp = false;

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
		dimLine->setLayer(layer);
		dimLine->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
		addEntity(dimLine);
		//箭头


		DmVector arrowScale(data.arrowSize(), data.arrowSize());
		DmVector space(0.0, 0.0);
		if (!data.hideDimLine1())
		{
			DmBlockReferenceData arrow1Data(DmDimensionStyle::getArrowBlockName(data.firstArrow()), edata.extensionPoint1 + offsetVec, arrowScale, Math2d::correctAngle(dimLineDir.angle() + M_PI),
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
			DmBlockReferenceData arrow2Data(DmDimensionStyle::getArrowBlockName(data.secondArrow()), edata.extensionPoint2 + offsetVec, arrowScale, dimLineDir.angle(),
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
	DmVector dimLineCenter = (edata.extensionPoint1 + edata.extensionPoint2) / 2.0 + offsetVec;
	DmDimensionStyleTextData::TextHorizontalPos hPos = data.textHorizontalPos();
	switch (hPos)
	{
	case DmDimensionStyleTextData::TextHorizontalPos::Mid:
		textBasePt = dimLineCenter - textDir * textSize.x / 2.0;
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::FirstBoundaryLine:
		textBasePt = edata.extensionPoint1 + offsetVec + textDir * data.arrowSize();
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::SecondBoundaryLine:
		textBasePt = edata.extensionPoint2 + offsetVec - textDir * (textSize.x + data.arrowSize());
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::AboveFirstBoundaryLine:
		textDir = textDir.rotate(M_PI / 2.0);
		textOffsetDir = textOffsetDir.rotate(M_PI / 2.0);
		if (isDefPtUp)
		{
			textBasePt = edata.extensionPoint1 + boundaryLineDir * boundaryLineEndOffset;
		}
		else
		{
			textBasePt = edata.extensionPoint1 + boundaryLineDir * (boundaryLineEndOffset + textSize.x);
		}
		extendBoundaryLine1 = true;
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::AboveSecondBoundaryLine:
		textDir = textDir.rotate(M_PI / 2.0);
		textOffsetDir = textOffsetDir.rotate(M_PI / 2.0);
		if (isDefPtUp)
		{
			textBasePt = edata.extensionPoint2 + boundaryLineDir * boundaryLineEndOffset;
		}
		else
		{
			textBasePt = edata.extensionPoint2 + boundaryLineDir * (boundaryLineEndOffset + textSize.x);
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
			bool isClip = clipLine(dimLine,  textBasePt, textDir, textSize.x);
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

void DmDimAligned::updateOuterDim(const DmVector& offsetVec, const DmVector& boundaryLineDir, const double boundaryLineStartOffset, const double boundaryLineEndOffset, bool& extendBoundaryLine1, bool& extendBoundaryLine2, bool& clipBoundLine, DmEntityContainer* pText, const DmVector& textSize)
{
	//生成标注线与箭头
	const DmVector dimLineDir = (edata.extensionPoint2 - edata.extensionPoint1).normalize();
	DmVector dimLineStartPt = edata.extensionPoint1 + offsetVec;
	DmVector dimLineEndPt = edata.extensionPoint2 + offsetVec;
	DmDocument* curDoc = getDocument();
	DmBlockTable* arrowBlocks = curDoc->getDimStyleTable()->getArrowBlocks();
	double arrow1CutDist = DmDimensionStyle::getArrowCutDistance(data.firstArrow()) * data.arrowSize();
	double arrow2CutDist = DmDimensionStyle::getArrowCutDistance(data.secondArrow()) * data.arrowSize();
	bool isDefPtUp = true;	//edata.definitionPoint在标注线上方还是下方
	double z = DmVector::crossP(dimLineDir, boundaryLineDir).z;
	if (z < 0)
		isDefPtUp = false;

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
			DmBlockReferenceData arrow1Data(DmDimensionStyle::getArrowBlockName(data.firstArrow()), edata.extensionPoint1 + offsetVec, arrowScale, dimLineDir.angle(),
				1, 1, space, arrowBlocks);
			DmBlockReference* arrow1 = new DmBlockReference(this, arrow1Data);
			arrow1->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
			//arrow1->setPen(DmPen(styleDataRef.lineData.dimLineColor, DM::Width00, DM::SolidLine));
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
			DmBlockReferenceData arrow2Data(DmDimensionStyle::getArrowBlockName(data.secondArrow()), edata.extensionPoint2 + offsetVec, arrowScale, Math2d::correctAngle(dimLineDir.angle() + M_PI),
				1, 1, space, arrowBlocks);
			DmBlockReference* arrow2 = new DmBlockReference(this, arrow2Data);
			arrow2->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
			//arrow2->setPen(DmPen(styleDataRef.lineData.dimLineColor, DM::Width00, DM::SolidLine));
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
		if (isDefPtUp)
		{
			textBasePt = edata.extensionPoint1 + boundaryLineDir * boundaryLineEndOffset;
		}
		else
		{
			textBasePt = edata.extensionPoint1 + boundaryLineDir * (boundaryLineEndOffset + textSize.x);
		}
		extendBoundaryLine1 = true;
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::AboveSecondBoundaryLine:
		textDir = textDir.rotate(M_PI / 2.0);
		textOffsetDir = textOffsetDir.rotate(M_PI / 2.0);
		if (isDefPtUp)
		{
			textBasePt = edata.extensionPoint2 + boundaryLineDir * boundaryLineEndOffset;
		}
		else
		{
			textBasePt = edata.extensionPoint2 + boundaryLineDir * (boundaryLineEndOffset + textSize.x);
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

void DmDimAligned::move(const DmVector& offset)
{
	DmDimension::move(offset);

	edata.extensionPoint1.move(offset);
	edata.extensionPoint2.move(offset);
	update();
}

void DmDimAligned::rotate(const DmVector& center, const DmVector& angleVector)
{
	DmDimension::rotate(center, angleVector);
	edata.extensionPoint1.rotate(center, angleVector);
	edata.extensionPoint2.rotate(center, angleVector);
	update();
}

void DmDimAligned::scale(const DmVector& center, const DmVector& factor)
{
	DmDimension::scale(center, factor);

	edata.extensionPoint1.scale(center, factor);
	edata.extensionPoint2.scale(center, factor);
	update();
}

void DmDimAligned::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
	DmDimension::mirror(axisPoint1, axisPoint2);

	edata.extensionPoint1.mirror(axisPoint1, axisPoint2);
	edata.extensionPoint2.mirror(axisPoint1, axisPoint2);
	update();
}

//void DmDimAligned::stretch(const DmVector& firstCorner, const DmVector& secondCorner, const DmVector& offset)
//{
//	if (getMin().isInWindow(firstCorner, secondCorner) && getMax().isInWindow(firstCorner, secondCorner))
//	{
//		move(offset);
//	}
//	else
//	{
//		double len = edata.extensionPoint2.distanceTo(data.definitionPoint);
//		double ang1 = edata.extensionPoint1.angleTo(edata.extensionPoint2) + M_PI_2;
//
//		if (edata.extensionPoint1.isInWindow(firstCorner, secondCorner))
//		{
//			edata.extensionPoint1.move(offset);
//		}
//		if (edata.extensionPoint2.isInWindow(firstCorner, secondCorner))
//		{
//			edata.extensionPoint2.move(offset);
//		}
//
//		double ang2 = edata.extensionPoint1.angleTo(edata.extensionPoint2) + M_PI_2;
//
//		double diff = Math2d::getAngleDifference(ang1, ang2);
//		if (diff > M_PI)
//		{
//			diff -= 2 * M_PI;
//		}
//
//		if (fabs(diff) > M_PI_2)
//		{
//			ang2 = Math2d::correctAngle(ang2 + M_PI);
//		}
//
//		DmVector v = DmVector::polar(len, ang2);
//		data.definitionPoint = edata.extensionPoint2 + v;
//	}
//	updateDim(true);
//}

void DmDimAligned::moveRef(const DmVector& ref, const DmVector& offset)
{
	constexpr double kRefThreshold = 1.0e-4;

	if (ref.distanceTo(data.definitionPoint) < kRefThreshold)
	{
		data.definitionPoint += offset;
		updateDim(true);
	}
	else if (ref.distanceTo(data.textCenter) < kRefThreshold)
	{
		// 通过textCenter的偏移反算definitionPoint的新位置
		DmConstructionLine line(nullptr, DmConstructionLineData(edata.extensionPoint1, edata.extensionPoint2));
		DmVector lineDir = (edata.extensionPoint2 - edata.extensionPoint1).normalize();
		DmVector normal(lineDir.y, -lineDir.x);

		DmVector projOrigin = line.getNearestPointOnEntity(data.textCenter);
		double d1 = DmVector::dotP(data.textCenter - projOrigin, normal);

		DmVector newTextCenter = data.textCenter + offset;
		DmVector projNew = line.getNearestPointOnEntity(newTextCenter);
		double d2 = DmVector::dotP(newTextCenter - projNew, normal);

		data.definitionPoint += normal * (d2 - d1);
		updateDim(true);
	}
	else if (ref.distanceTo(edata.extensionPoint1) < kRefThreshold)
	{
		edata.extensionPoint1 += offset;
		updateDim(true);
	}
	else if (ref.distanceTo(edata.extensionPoint2) < kRefThreshold)
	{
		edata.extensionPoint2 += offset;
		updateDim(true);
	}
}
