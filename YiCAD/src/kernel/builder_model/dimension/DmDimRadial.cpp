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


/// @file DmDimRadial.cpp
/// @brief DmDimRadial 半径标注类的实现

#include <iostream>
#include <cmath>
#include "DmDimRadial.h"
#include "DmLine.h"
#include "DmMText.h"
#include "DmSolid.h"
#include "DmDocument.h"
#include "DmBlockReference.h"
#include "Math2d.h"
#include "ApplicationWindow.h"
#include "Debug.h"

TYPESYSTEM_SOURCE(DmDimRadial, DmEntity, 0);

DmDimRadialData::DmDimRadialData()
	: endPoint(false)
	, leader(0.0)
{
}

/// @param definitionPoint Definition point of the radial dimension.
/// @param leader Leader length.
DmDimRadialData::DmDimRadialData(const DmVector& _endPoint,double _leader)
	: endPoint(_endPoint)
	, leader(_leader)
{
}

/// @para parent Parent Entity Container.
/// @para d Common dimension geometrical data.
/// @para ed Extended geometrical data for radial dimension.
DmDimRadial::DmDimRadial(DmEntityContainer* parent,const DmDimensionData& d,const DmDimRadialData& ed)
	: DmDimension(parent, d)
	, edata(ed)
{
}

DmEntity* DmDimRadial::clone() const
{
	DmDimRadial* d = new DmDimRadial(*this);
	d->container = new DmEntityContainer(d);
	d->update();
	return d;
}

DM::EntityType DmDimRadial::getEntityType() const
{
	return DM::EntityDimRadial;
}

DmDimRadialData DmDimRadial::getEData() const
{
	return edata;
}

DmVector DmDimRadial::getDefinitionPoint()
{
	return edata.endPoint;
}

double DmDimRadial::getLeader()
{
	return edata.leader;
}

DmVectorSolutions DmDimRadial::getRefPoints() const
{
	return DmVectorSolutions({ edata.endPoint, data.definitionPoint });
}

/// @brief Updates the sub entities of this dimension. Called when the dimension or the position, alignment, .. changes.
/// @param autoText Automatically reposition the text label
void DmDimRadial::updateDim(bool autoText)
{
	clear();

	//绘制文字，获得文字范围
	double radius = edata.endPoint.distanceTo(data.definitionPoint);
	DmEntityContainer* pText = DmDimension::getTextEntity(radius);

	const DmVector textSize = pText->getWidthHeight();
	double txtExt = textSize.x;
	bool dimOut = false;	//是否为外部标注。当文字不能在2点间放下时，采用外部标注
	if (edata.leader > 0 || (edata.leader < 0 && data.arrowSize() * 1.5 + txtExt > radius))
	{
		dimOut = true;
	}
	//绘制标注线及箭头
	if (dimOut)
	{
		//外部标注
		updateOuterDim(pText, textSize);
	}
	else
	{
		//内部标注
		updateInnerDim(pText, textSize);
	}

	calculateBorders();
}

void DmDimRadial::move(const DmVector& offset)
{
	DmDimension::move(offset);

	edata.endPoint.move(offset);
	update();
}

void DmDimRadial::rotate(const DmVector& center, const DmVector& angleVector)
{
	DmDimension::rotate(center, angleVector);

	edata.endPoint.rotate(center, angleVector);
	update();
}

void DmDimRadial::scale(const DmVector& center, const DmVector& factor)
{
	DmDimension::scale(center, factor);

	edata.endPoint.scale(center, factor);
	edata.leader *= factor.x;
	update();
}

void DmDimRadial::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
	DmDimension::mirror(axisPoint1, axisPoint2);

	edata.endPoint.mirror(axisPoint1, axisPoint2);
	update();
}

void DmDimRadial::moveRef(const DmVector& ref, const DmVector& offset)
{
	if (ref.distanceTo(edata.endPoint) < 1.0e-4)
	{
		double d = data.definitionPoint.distanceTo(edata.endPoint);
		double a = data.definitionPoint.angleTo(edata.endPoint + offset);

		DmVector v = DmVector::polar(d, a);
		edata.endPoint = data.definitionPoint + v;
		updateDim(true);
	}
	else if (ref.distanceTo(data.textCenter) < 1.0e-4)
	{
		data.textCenter.move(offset);
		updateDim(false);
	}
}

void DmDimRadial::saveStream(OutputStream& wrt) const
{
	DmDimension::saveStream(wrt);

	DmVector centerPt =  data.definitionPoint;
	DmVector diameterPt = edata.endPoint;
	double leaderLength = edata.leader;

	wrt << (double)centerPt.x << (double)centerPt.y << (double)diameterPt.x << (double)diameterPt.y << (double)leaderLength;
}

void DmDimRadial::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
	int fileRev = getRevisionId("DmDimRadial", revs);
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

void DmDimRadial::restoreStreamWithRev(InputStream& rdr, int rev)
{
	if (rev == 0)
	{
	}
	else //big change, e.g. change supper class of DmDimRadial
	{
		//step1.
		// read all legacy data one by one
	}
}

void DmDimRadial::restoreStream(InputStream& rdr)
{
    DmDimension::restoreStream(rdr);

    DmVector centerPt(true), diameterPt(true);
    double leaderLength = edata.leader;
    rdr >> (double&)centerPt.x >> (double&)centerPt.y >> (double&)diameterPt.x >> (double&)diameterPt.y >> (double&)leaderLength;

    data.definitionPoint = centerPt;
    edata.endPoint = diameterPt;
    edata.leader = leaderLength;
    update();
}

void DmDimRadial::updateInnerDim(DmEntityContainer* pText, const DmVector& textSize)
{
	//生成标注线与箭头
	const double leaderLen = abs(edata.leader);
	const DmVector dimLineDir = (edata.endPoint - data.definitionPoint).normalize();
	DmVector dimLineStartPt = edata.endPoint - dimLineDir * leaderLen;
	DmVector dimLineEndPt = edata.endPoint;
	DmDocument* curDoc = static_cast<DmDocument*>(ApplicationWindow::getAppWindow()->getDocument());
	DmBlockTable* arrowBlocks = curDoc->getDimStyleTable()->getArrowBlocks();
	double arrowCutDist = DmDimensionStyle::getArrowCutDistance(data.secondArrow()) * data.arrowSize();

	DmLayer* layer = getLayer();
	DmLine* dimLine = nullptr;
	if (!data.hideDimLine1() || !data.hideDimLine2())
	{
		//标注线
		dimLineEndPt -= dimLineDir * arrowCutDist;
		dimLine = new DmLine(this, dimLineStartPt, dimLineEndPt);
		dimLine->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
		dimLine->setLayer(layer);
		addEntity(dimLine);
		//箭头


		DmVector arrowScale(data.arrowSize(), data.arrowSize());
		DmVector space(0.0, 0.0);
		DmBlockReferenceData arrow2Data(DmDimensionStyle::getArrowBlockName(data.secondArrow()), edata.endPoint, arrowScale, dimLineDir.angle(),
			1, 1, space, arrowBlocks);
		DmBlockReference* arrow2 = new DmBlockReference(this, arrow2Data);
		arrow2->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
		//arrow2->setPen(DmPen(data.lineData.dimLineColor, DM::Width00, DM::SolidLine));
		arrow2->setLayer(layer);
		arrow2->update();
		arrow2->forcedCalculateBorders();
		addEntity(arrow2);

	}

	//移动标注文字到正确位置
	bool clipUnderLine = false;	//是否剪断标注线。如果文字垂直方向在标注线居中，则需要剪断下面的标注线
	DmVector textBasePt(0.0, 0.0);
	DmVector textDir(dimLineDir);
	DmVector textOffsetDir = DmVector(dimLineDir).rotate(M_PI / 2.0);
	textBasePt = dimLineStartPt;
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
			bool isClip = clipLine(dimLine, textBasePt, textDir, textSize.x);
			if (isClip)
				removeEntity(dimLine);
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

void DmDimRadial::updateOuterDim(DmEntityContainer* pText, const DmVector& textSize)
{
	//生成标注线与箭头
	const double leaderLen = abs(edata.leader);
	const DmVector dimLineDir = (edata.endPoint - data.definitionPoint).normalize();
	DmVector dimLineStartPt = data.definitionPoint;
	DmVector dimLineEndPt = edata.endPoint;
	DmDocument* curDoc = static_cast<DmDocument*>(ApplicationWindow::getAppWindow()->getDocument());
	DmBlockTable* arrowBlocks = curDoc->getDimStyleTable()->getArrowBlocks();
	double arrowCutDist = DmDimensionStyle::getArrowCutDistance(data.secondArrow()) * data.arrowSize();

	DmLayer* layer = getLayer();
	DmLine* extendLine = nullptr;	//箭头延伸出来的线
	double extendLen = textSize.x ;
	DmVector extendLineStart = dimLineEndPt + dimLineDir *data.arrowSize();
	DmVector extendLineEnd = extendLineStart + dimLineDir * leaderLen;
	if (!data.hideDimLine1() || !data.hideDimLine2())
	{
		//中间标注线
		DmLine* dimLine = new DmLine(this, dimLineStartPt, dimLineEndPt);
		dimLine->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
		dimLine->setLayer(layer);
		addEntity(dimLine);

		//箭头


		DmVector arrowScale(data.arrowSize(), data.arrowSize());
		DmVector space(0.0, 0.0);
		DmBlockReferenceData arrow2Data(DmDimensionStyle::getArrowBlockName(data.secondArrow()), edata.endPoint, arrowScale, Math2d::correctAngle(dimLineDir.angle() + M_PI),
			1, 1, space, arrowBlocks);
		DmBlockReference* arrow2 = new DmBlockReference(this, arrow2Data);
		arrow2->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
		//arrow2->setPen(DmPen(data.lineData.dimLineColor, DM::Width00, DM::SolidLine));
		arrow2->setLayer(layer);
		arrow2->update();
		arrow2->forcedCalculateBorders();
		addEntity(arrow2);
		//加上一条线
		extendLine = new DmLine(this, extendLineStart, extendLineEnd);
		extendLine->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
		extendLine->setLayer(layer);
		addEntity(extendLine);

	}

	//移动标注文字到正确位置
	bool clipUnderLine = false;	//是否剪断标注线。如果文字垂直方向在标注线居中，则需要剪断下面的标注线
	DmVector textBasePt(0.0, 0.0);
	DmVector textDir(dimLineDir);
	DmVector textOffsetDir = DmVector(dimLineDir).rotate(M_PI / 2.0);
	textBasePt = extendLineEnd - textDir* textSize.x;
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
	if (clipUnderLine && nullptr != extendLine)	//剪断标注线
	{
		bool isClip = clipLine(extendLine, textBasePt, textDir, textSize.x);
		if (isClip)
			removeEntity(extendLine);
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
