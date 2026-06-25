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


/// @file DmDimAngular.cpp
/// @brief DmDimAngular 角度标注类的实现

#include<iostream>
#include<cmath>
#include "DmDimAngular.h"
#include "Math2d.h"

#include "DmArc.h"
#include "DmLine.h"
#include "DmDocument.h"
#include "Information.h"
#include "DmSolid.h"
#include "DmBlockReference.h"
#include "DmMText.h"
#include "GeometryMethods.h"
#include "ApplicationWindow.h"
#include "Debug.h"

TYPESYSTEM_SOURCE(DmDimAngular, DmEntity, 0);

DmDimAngularData::DmDimAngularData(const DmVector& line1StartPt, const DmVector& line1EndPt,
	const DmVector& line2StartPt, const DmVector& line2EndPt, const DmVector& ptOnArc)
{
	this->line1StartPt = line1StartPt;
	this->line1EndPt = line1EndPt;
	this->line2StartPt = line2StartPt;
	this->line2EndPt = line2EndPt;
	this->ptOnArc = ptOnArc;
}

DmDimAngular::DmDimAngular(DmEntityContainer* parent, const DmDimensionData& d, const DmDimAngularData& ed)
	: DmDimension(parent, d)
	, edata(ed)
{
}

DmEntity* DmDimAngular::clone() const
{
	DmDimAngular* d = new DmDimAngular(*this);
	d->container = new DmEntityContainer(d);
	d->update();

	return d;
}

DM::EntityType DmDimAngular::getEntityType() const
{
	return DM::EntityDimAngular;
}

DmDimAngularData DmDimAngular::getEData() const
{
	return edata;
}

/// @return Center of the measured dimension.
DmVector DmDimAngular::getCenter() const
{
	return m_center;
}

DmVectorSolutions DmDimAngular::getRefPoints() const
{
	return DmVectorSolutions({edata.ptOnArc});
}

void DmDimAngular::calculateData()
{
	double angleOfPtOnArc = (edata.ptOnArc - m_center).angle();
	DmVector line1Dir = (edata.line1EndPt - edata.line1StartPt).normalize();
	DmVector line2Dir = (edata.line2EndPt - edata.line2StartPt).normalize();
	DmVector line1Pt1Candidate = m_center + line1Dir * m_radius;
	DmVector line1Pt2Candidate = m_center - line1Dir * m_radius;
	DmVector line2Pt1Candidate = m_center + line2Dir * m_radius;
	DmVector line2Pt2Candidate = m_center - line2Dir * m_radius;
	std::vector<std::pair<DmVector, double>> ptsAngle;
	ptsAngle.reserve(4);
	ptsAngle.emplace_back(std::make_pair(line1Pt1Candidate, 0.0));
	ptsAngle.emplace_back(std::make_pair(line1Pt2Candidate, 0.0));
	ptsAngle.emplace_back(std::make_pair(line2Pt1Candidate, 0.0));
	ptsAngle.emplace_back(std::make_pair(line2Pt2Candidate, 0.0));
	for (auto& it : ptsAngle)
	{
		DmVector& pt = it.first;
		double angle = (pt - m_center).angle();
		it = std::make_pair(pt, angle);
	}
	std::sort(ptsAngle.begin(), ptsAngle.end(),
		[](const std::pair<DmVector, double>& item1, const std::pair<DmVector, double>& item2)
		{
			return item1.second < item2.second;
		});
	for (int i = 0; i < 4; i++)
	{
		auto curItem = ptsAngle.at(i);
		int nextIdx = i + 1;
		if (nextIdx == 4)
			nextIdx = 0;
		auto nextItem = ptsAngle.at(nextIdx);
		double curAngle = curItem.second;
		double nextAngle = nextItem.second;
		bool isBetween = false;
		if (curAngle > nextAngle)
		{
			if (angleOfPtOnArc >= curAngle || angleOfPtOnArc < nextAngle)
			{
				isBetween = true;
			}
		}
		else
		{
			if (angleOfPtOnArc >= curAngle && angleOfPtOnArc < nextAngle)
			{
				isBetween = true;
			}
		}
		if (isBetween)
		{
			m_arcStartAngle = curAngle;
			m_arcEndAngle = nextAngle;
			if (curItem.first == line1Pt1Candidate || curItem.first == line1Pt2Candidate)
			{
				m_arcEndPt1 = curItem.first;
				m_arcEndPt2 = nextItem.first;
			}
			else
			{
				m_arcEndPt2 = curItem.first;
				m_arcEndPt1 = nextItem.first;
			}
			m_arcEndPt1Dir = (m_arcEndPt1 - m_center).normalize();
			m_arcEndPt2Dir = (m_arcEndPt2 - m_center).normalize();
			break;
		}
	}
}

void DmDimAngular::getBoundaryLineInfos(bool& needBoundLine1, DmVector& boundaryLineDir1, bool& needBoundLine2, DmVector& boundaryLineDir2,
	double& boundaryLine1StartOffset, double& boundaryLine1EndOffset, double& boundaryLine2StartOffset, double& boundaryLine2EndOffset) const
{
	//判断是否需要标注边界
	double pos1 = DmVector::posInLine(edata.line1StartPt, edata.line1EndPt, m_arcEndPt1);
	double pos2 = DmVector::posInLine(edata.line2StartPt, edata.line2EndPt, m_arcEndPt2);
	if (pos1 < 1.0 && pos1>0.0)
	{
		needBoundLine1 = false;
	}
	else
	{
		needBoundLine1 = true;
	}
	if (pos2 < 1.0 && pos2>0.0)
	{
		needBoundLine2 = false;
	}
	else
	{
		needBoundLine2 = true;
	}

	//计算标注边界偏移
	if (needBoundLine1)
	{
		if (pos1 > 1.0)	//偏移为正
		{
			const double definitionOffset1 = m_arcEndPt1.distanceTo(edata.line1EndPt);
			boundaryLine1StartOffset = data.startPtOffset() <= definitionOffset1 ? data.startPtOffset() : definitionOffset1;	//标注界线起点偏移距离
			if (data.isFixedBoundLineLength() && data.fixedBoundLineLength() < definitionOffset1 - data.startPtOffset())
			{
				boundaryLine1StartOffset = definitionOffset1 - data.fixedBoundLineLength();
			}
			boundaryLine1EndOffset = definitionOffset1 + data.extendDimLine();//标注界线终点偏移距离
			boundaryLineDir1 = m_arcEndPt1Dir;
		}
		//偏移为负
		else
		{
			const double definitionOffset1 = m_arcEndPt1.distanceTo(edata.line1StartPt);
			boundaryLine1StartOffset = data.startPtOffset() <= definitionOffset1 ? data.startPtOffset() : definitionOffset1;	//标注界线起点偏移距离
			if (data.isFixedBoundLineLength() && data.fixedBoundLineLength() < definitionOffset1 - data.startPtOffset())
			{
				boundaryLine1StartOffset = definitionOffset1 - data.fixedBoundLineLength();
			}
			boundaryLine1EndOffset = definitionOffset1 + data.extendDimLine();//标注界线终点偏移距离
			//取负
			boundaryLine1StartOffset = -boundaryLine1StartOffset;
			boundaryLine1EndOffset = -boundaryLine1EndOffset;
			boundaryLineDir1 = m_arcEndPt1Dir;
		}
	}
	else
	{
		boundaryLineDir1 = m_arcEndPt1Dir;
		boundaryLine1StartOffset = 0.0;
		boundaryLine1EndOffset = 0.0;
	}

	if (needBoundLine2)
	{
		if (pos2 > 1.0)	//偏移为正
		{
			const double definitionOffset2 = m_arcEndPt2.distanceTo(edata.line2EndPt);
			boundaryLine2StartOffset = data.startPtOffset() <= definitionOffset2 ? data.startPtOffset() : definitionOffset2;	//标注界线起点偏移距离
			if (data.isFixedBoundLineLength() && data.fixedBoundLineLength() < definitionOffset2 - data.startPtOffset())
			{
				boundaryLine2StartOffset = definitionOffset2 - data.fixedBoundLineLength();
			}
			boundaryLine2EndOffset = definitionOffset2 + data.extendDimLine();//标注界线终点偏移距离
			boundaryLineDir2 = m_arcEndPt2Dir;
		}
		//偏移为负
		else
		{
			const double definitionOffset2 = m_arcEndPt2.distanceTo(edata.line2StartPt);
			boundaryLine2StartOffset = data.startPtOffset() <= definitionOffset2 ? data.startPtOffset() : definitionOffset2;	//标注界线起点偏移距离
			if (data.isFixedBoundLineLength() && data.fixedBoundLineLength() < definitionOffset2 - data.startPtOffset())
			{
				boundaryLine2StartOffset = definitionOffset2 - data.fixedBoundLineLength();
			}
			boundaryLine2EndOffset = definitionOffset2 + data.extendDimLine();//标注界线终点偏移距离
			//取负
			boundaryLine2StartOffset = -boundaryLine2StartOffset;
			boundaryLine2EndOffset = -boundaryLine2EndOffset;
			boundaryLineDir2 = m_arcEndPt2Dir;
		}
	}
	else
	{
		boundaryLineDir2 = m_arcEndPt2Dir;
		boundaryLine2StartOffset = 0.0;
		boundaryLine2EndOffset = 0.0;
	}
}

void DmDimAngular::updateInnerDim(const DmVector& boundaryLineDir1, const DmVector& boundaryLineDir2, 
	const double boundaryLine1StartOffset, const double boundaryLine1EndOffset, const double boundaryLine2StartOffset, const double boundaryLine2EndOffset, 
	bool& extendBoundaryLine1, bool& extendBoundaryLine2, bool& clipBoundLine, DmEntityContainer* pText, const DmVector& textSize)
{
	//生成标注线与箭头
	DmVector dimLineStartPt = m_center + DmVector(m_arcStartAngle) * m_radius;
	DmVector dimLineEndPt = m_center + DmVector(m_arcEndAngle) * m_radius;
	DmVector vDir1 = DmVector(m_arcStartAngle).rotate(M_PI_2);
	DmVector vDir2 = DmVector(m_arcEndAngle).rotate(-M_PI_2);

	DmLayer* layer = getLayer();
	const DmVector arrow1Pos = dimLineStartPt;
	const DmVector arrow2Pos = dimLineEndPt;
	DmDocument* curDoc = static_cast<DmDocument*>(ApplicationWindow::getAppWindow()->getDocument());
	DmBlockTable* arrowBlocks = curDoc->getDimStyleTable()->getArrowBlocks();
	double arrow1CutDist = DmDimensionStyle::getArrowCutDistance(data.firstArrow()) * data.arrowSize();
	double arrow2CutDist = DmDimensionStyle::getArrowCutDistance(data.secondArrow()) * data.arrowSize();
	double angleCutByArrow1 = atan(arrow1CutDist / m_radius);
	double angleCutByArrow2 = atan(arrow2CutDist / m_radius);

	DmArc* dimArc = nullptr;
	if (!data.hideDimLine1() || !data.hideDimLine2())
	{
		//标注圆弧
		dimArc = new DmArc(this, ArcData(m_center, DmVector(0.0, 0.0, 1.0), m_radius, m_arcStartAngle, m_arcEndAngle));
		dimArc->setLayer(layer);
		dimArc->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
		addEntity(dimArc);
		//箭头


		DmVector arrowScale(data.arrowSize(), data.arrowSize());
		DmVector space(0.0, 0.0);
		DmBlockReferenceData arrow1Data(DmDimensionStyle::getArrowBlockName(data.firstArrow()), arrow1Pos, arrowScale, Math2d::correctAngle(m_arcStartAngle - M_PI_2),
			1, 1, space, arrowBlocks);
		DmBlockReference* arrow1 = new DmBlockReference(this, arrow1Data);
		arrow1->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
		//arrow1->setPen(DmPen(styleDataRef.lineData.dimLineColor, DM::Width00, DM::SolidLine));
		arrow1->setLayer(layer);
		arrow1->update();
		arrow1->forcedCalculateBorders();
		addEntity(arrow1);
		DmBlockReferenceData arrow2Data(DmDimensionStyle::getArrowBlockName(data.secondArrow()), arrow2Pos, arrowScale, Math2d::correctAngle(m_arcEndAngle + M_PI_2),
			1, 1, space, arrowBlocks);
		DmBlockReference* arrow2 = new DmBlockReference(this, arrow2Data);
		arrow2->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
		//arrow2->setPen(DmPen(styleDataRef.lineData.dimLineColor, DM::Width00, DM::SolidLine));
		arrow2->setLayer(layer);
		arrow2->update();
		arrow2->forcedCalculateBorders();
		addEntity(arrow2);

	}

	//移动标注文字到正确位置
	bool clipUnderLine = false;	//是否剪断标注线。如果文字垂直方向在标注线居中，则需要剪断下面的标注线
	DmVector textCenterPt(0.0, 0.0);
	DmVector textDir((m_arcEndPt2 - m_arcEndPt1).normalize());
	DmVector textOffsetDir = DmVector(textDir).rotate(-M_PI / 2.0);	//从原心指向文字中心
	DmVector centerOnArc = m_center + (m_arcEndPt1Dir + m_arcEndPt2Dir).normalize() * m_radius;	//圆弧与对角线的交点
	DmVector tempDir(false);
	const double textArrowGap = data.arrowSize() * 0.5;
	DmDimensionStyleTextData::TextHorizontalPos hPos = data.textHorizontalPos();
	//设置文字水平位置
	switch (hPos)
	{
	case DmDimensionStyleTextData::TextHorizontalPos::Mid:
		textCenterPt = centerOnArc;
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::FirstBoundaryLine:
		tempDir = (m_arcEndPt1 - m_center) + vDir1 * (textSize.x / 2.0 + data.arrowSize() + textArrowGap);
		tempDir = tempDir.normalize();
		textOffsetDir = tempDir;
		textDir = DmVector(textOffsetDir).rotate(M_PI_2);
		textCenterPt = m_center + textOffsetDir * m_radius;
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::SecondBoundaryLine:
		tempDir = (m_arcEndPt2 - m_center) + vDir2 * (textSize.x / 2.0 + data.arrowSize() + textArrowGap);
		tempDir = tempDir.normalize();
		textOffsetDir = tempDir;
		textDir = DmVector(textOffsetDir).rotate(M_PI_2);
		textCenterPt = m_center + textOffsetDir * m_radius;
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::AboveFirstBoundaryLine:
		textDir = textDir.rotate(M_PI / 2.0);
		textOffsetDir = textOffsetDir.rotate(M_PI / 2.0);
		if (boundaryLine1StartOffset > 0)
		{
			textCenterPt = edata.line1EndPt + boundaryLineDir1 * boundaryLine1EndOffset;
		}
		else
		{
			textCenterPt = edata.line1StartPt + boundaryLineDir1 * (-boundaryLine1EndOffset + textSize.x);
		}
		extendBoundaryLine1 = true;
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::AboveSecondBoundaryLine:
		textDir = textDir.rotate(M_PI / 2.0);
		textOffsetDir = textOffsetDir.rotate(M_PI / 2.0);
		if (boundaryLine2StartOffset)
		{
			textCenterPt = edata.line2EndPt + boundaryLineDir2 * boundaryLine2EndOffset;
		}
		else
		{
			textCenterPt = edata.line2StartPt + boundaryLineDir2 * ( -boundaryLine2EndOffset + textSize.x);
		}

		extendBoundaryLine2 = true;
		break;
	default:
		break;
	}
	//设置文字垂直位置
	DmDimensionStyleTextData::TextVerticalPos vPos = data.textVerticalPos();
	switch (vPos)
	{
	case DmDimensionStyleTextData::TextVerticalPos::Mid:
		//textCenterPt -= textOffsetDir * textSize.y / 2.0;
		clipUnderLine = true;
		break;
	case DmDimensionStyleTextData::TextVerticalPos::Up:
	case DmDimensionStyleTextData::TextVerticalPos::Extern:
	case DmDimensionStyleTextData::TextVerticalPos::JIS:
		textCenterPt += textOffsetDir * (data.offsetFromDimLine() + textSize.y / 2.0);
		break;
	case DmDimensionStyleTextData::TextVerticalPos::Down:
		textCenterPt -= textOffsetDir * (data.offsetFromDimLine() + textSize.y / 2.0);
		break;
	default:
		break;
	}
	//按需要剪断标注圆弧
	if (clipUnderLine && nullptr != dimArc)	
	{
		if (hPos == DmDimensionStyleTextData::TextHorizontalPos::Mid || hPos == DmDimensionStyleTextData::TextHorizontalPos::FirstBoundaryLine || hPos == DmDimensionStyleTextData::TextHorizontalPos::SecondBoundaryLine)
		{
			bool isClip = clipArc(dimArc, textCenterPt - textDir * textSize.x / 2.0 - textOffsetDir*textSize.y /2.0, textDir, textSize.x);
			if (isClip)
				removeEntity(dimArc);
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
	pText->move(textCenterPt - textSize / 2.0);
	pText->rotate(textCenterPt, textDir);
	data.textCenter = textCenterPt;
	//data.textCenter.rotate(textCenterPt, textDir.angle());
}

void DmDimAngular::updateOuterDim(const DmVector& boundaryLineDir1, const DmVector& boundaryLineDir2, 
	const double boundaryLine1StartOffset, const double boundaryLine1EndOffset, const double boundaryLine2StartOffset, const double boundaryLine2EndOffset, bool& extendBoundaryLine1, 
	bool& extendBoundaryLine2, bool& clipBoundLine, DmEntityContainer* pText, const DmVector& textSize)
{
	//生成标注线与箭头
	DmVector dimLineStartPt = m_center + DmVector(m_arcStartAngle) * m_radius;
	DmVector dimLineEndPt = m_center + DmVector(m_arcEndAngle) * m_radius;
	DmVector vDir1 = DmVector(m_arcStartAngle).rotate(-M_PI_2);
	DmVector vDir2 = DmVector(m_arcEndAngle).rotate(M_PI_2);
	DmDocument* curDoc = static_cast<DmDocument*>(ApplicationWindow::getAppWindow()->getDocument());
	DmBlockTable* arrowBlocks = curDoc->getDimStyleTable()->getArrowBlocks();
	double arrow1CutDist = DmDimensionStyle::getArrowCutDistance(data.firstArrow()) * data.arrowSize();
	double arrow2CutDist = DmDimensionStyle::getArrowCutDistance(data.secondArrow()) * data.arrowSize();

	DmLayer* layer = getLayer();
	const DmVector arrow1Pos = dimLineStartPt;
	const DmVector arrow2Pos = dimLineEndPt;

	DmLine* extendLine1 = nullptr;	//箭头延伸出来的线
	DmLine* extendLine2 = nullptr;
	double extendLen = data.textHeight() * 2.5;
	DmVector extendLine1Start = arrow1Pos + vDir1 * arrow1CutDist;	//箭头延长线
	DmVector extendLine1End = extendLine1Start + vDir1 * extendLen;
	DmVector extendLine2Start = arrow2Pos + vDir2 * arrow2CutDist;
	DmVector extendLine2End = extendLine2Start + vDir2 * extendLen;
	DmArc* dimArc = nullptr;
	if (!data.hideDimLine1() || !data.hideDimLine2())
	{
		//标注圆弧
		dimArc = new DmArc(this, ArcData(m_center, DmVector(0.0, 0.0, 1.0), m_radius, m_arcStartAngle, m_arcEndAngle));
		dimArc->setLayer(layer);
		dimArc->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
		addEntity(dimArc);
		//箭头


		DmVector arrowScale(data.arrowSize(), data.arrowSize());
		DmVector space(0.0, 0.0);
		DmBlockReferenceData arrow1Data(DmDimensionStyle::getArrowBlockName(data.firstArrow()), arrow1Pos, arrowScale, Math2d::correctAngle(m_arcStartAngle + M_PI_2),
			1, 1, space, arrowBlocks);
		DmBlockReference* arrow1 = new DmBlockReference(this, arrow1Data);
		arrow1->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
		//arrow1->setPen(DmPen(styleDataRef.lineData.dimLineColor, DM::Width00, DM::SolidLine));
		arrow1->setLayer(layer);
		arrow1->update();
		arrow1->forcedCalculateBorders();
		addEntity(arrow1);
		//加上一条线
		extendLine1 = new DmLine(this, extendLine1Start, extendLine1End);
		extendLine1->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
		extendLine1->setLayer(layer);
		addEntity(extendLine1);

		DmBlockReferenceData arrow2Data(DmDimensionStyle::getArrowBlockName(data.secondArrow()), arrow2Pos, arrowScale, Math2d::correctAngle(m_arcEndAngle - M_PI_2),
			1, 1, space, arrowBlocks);
		DmBlockReference* arrow2 = new DmBlockReference(this, arrow2Data);
		arrow2->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
		//arrow2->setPen(DmPen(styleDataRef.lineData.dimLineColor, DM::Width00, DM::SolidLine));
		arrow2->setLayer(layer);
		arrow2->update();
		arrow2->forcedCalculateBorders();
		addEntity(arrow2);
		//加上一条线
		extendLine2 = new DmLine(this, extendLine2Start, extendLine2End);
		extendLine2->setPen(DmPen(data.dimLineColor(), data.dimLineWidth(), data.dimLineType()));
		extendLine2->setLayer(layer);
		addEntity(extendLine2);

	}

	//移动标注文字到正确位置
	bool clipUnderLine = false;	//是否剪断标注线。如果文字垂直方向在标注线居中，则需要剪断下面的标注线
	DmVector textCenterPt(0.0, 0.0);
	DmVector textDir((m_arcEndPt2 - m_arcEndPt1).normalize());
	DmVector textOffsetDir = DmVector(textDir).rotate(-M_PI / 2.0);	//从原心指向文字中心
	DmVector centerOnArc = m_center + (m_arcEndPt1Dir + m_arcEndPt2Dir).normalize() * m_radius;	//圆弧与对角线的交点
	DmVector tempDir(false);
	const double textArrowGap = data.arrowSize() * 0.5;
	DmDimensionStyleTextData::TextHorizontalPos hPos = data.textHorizontalPos();
	//设置文字水平位置
	switch (hPos)
	{
	case DmDimensionStyleTextData::TextHorizontalPos::FirstBoundaryLine:
		textDir = vDir1;
		textOffsetDir = DmVector(textDir).rotate(-M_PI_2);
		textCenterPt = extendLine1End + textDir * textSize.x / 2.0;
		if (extendLine1)
		{
			extendLine1->moveEndpoint(extendLine1End + textDir * textSize.x);
		}
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::Mid:
	case DmDimensionStyleTextData::TextHorizontalPos::SecondBoundaryLine:
		textDir = vDir2;
		textOffsetDir = DmVector(textDir).rotate(M_PI_2);
		textCenterPt = extendLine2End + textDir * textSize.x / 2.0;
		if (extendLine2)
		{
			extendLine2->moveEndpoint(extendLine2End + textDir * textSize.x);
		}
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::AboveFirstBoundaryLine:
		textDir = textDir.rotate(M_PI / 2.0);
		textOffsetDir = textOffsetDir.rotate(M_PI / 2.0);
		if (boundaryLine1StartOffset > 0)
		{
			textCenterPt = edata.line1EndPt + boundaryLineDir1 * boundaryLine1EndOffset;
		}
		else
		{
			textCenterPt = edata.line1StartPt + boundaryLineDir1 * (-boundaryLine1EndOffset + textSize.x);
		}
		extendBoundaryLine1 = true;
		break;
	case DmDimensionStyleTextData::TextHorizontalPos::AboveSecondBoundaryLine:
		textDir = textDir.rotate(M_PI / 2.0);
		textOffsetDir = textOffsetDir.rotate(M_PI / 2.0);
		if (boundaryLine2StartOffset)
		{
			textCenterPt = edata.line2EndPt + boundaryLineDir2 * boundaryLine2EndOffset;
		}
		else
		{
			textCenterPt = edata.line1StartPt + boundaryLineDir2 * (-boundaryLine2EndOffset + textSize.x);
		}

		extendBoundaryLine2 = true;
		break;
	default:
		break;
	}
	//设置文字垂直位置
	DmDimensionStyleTextData::TextVerticalPos vPos = data.textVerticalPos();
	switch (vPos)
	{
	case DmDimensionStyleTextData::TextVerticalPos::Mid:
		//textCenterPt -= textOffsetDir * textSize.y / 2.0;
		clipUnderLine = true;
		break;
	case DmDimensionStyleTextData::TextVerticalPos::Up:
	case DmDimensionStyleTextData::TextVerticalPos::Extern:
	case DmDimensionStyleTextData::TextVerticalPos::JIS:
		textCenterPt += textOffsetDir * (data.offsetFromDimLine() + textSize.y / 2.0);
		break;
	case DmDimensionStyleTextData::TextVerticalPos::Down:
		textCenterPt -= textOffsetDir * (data.offsetFromDimLine() + textSize.y / 2.0);
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
			bool isClip = clipLine(extendLine, textCenterPt - textDir * textSize.x / 2.0 - textOffsetDir * textSize.y / 2.0, tempTextDir, textSize.x);
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
	pText->move(textCenterPt - textSize / 2.0);
	pText->rotate(textCenterPt, textDir);
	data.textCenter = textCenterPt;
}

/// Updates the sub entities of this dimension. Called when the dimension or the position, alignment, .. changes.
/// @param autoText Automatically reposition the text label
void DmDimAngular::updateDim(bool autoText /*= false*/)
{
	Q_UNUSED(autoText)

	clear();

	//计算圆心
	m_center = GeometryMethods::getIntersectionOfTwoLine(edata.line1StartPt, edata.line1EndPt, edata.line2StartPt, edata.line2EndPt);
	if (!m_center.valid)	//平行的时候没有圆弧圆心，不标注
	{
		return;
	}
	m_radius = m_center.distanceTo(edata.ptOnArc);
	
	//计算一些参数
	calculateData();
	
	DmVector boundaryLineDir1, boundaryLineDir2;
	bool needBoundLine1 = true;
	bool needBoundLine2 = true;
	double boundaryLine1StartOffset, boundaryLine1EndOffset, boundaryLine2StartOffset, boundaryLine2EndOffset;
	getBoundaryLineInfos(needBoundLine1,boundaryLineDir1, needBoundLine2, boundaryLineDir2, boundaryLine1StartOffset, boundaryLine1EndOffset, boundaryLine2StartOffset, boundaryLine2EndOffset);

	//绘制文字，获得文字范围
	//double angle = (edata.line1EndPt - edata.line1StartPt).angleTo2(edata.line2EndPt - edata.line2StartPt);
	double angle = Math2d::correctAngle(m_arcEndAngle - m_arcStartAngle);
	DmEntityContainer* pText = DmDimension::getTextEntity(angle);

	const DmVector textSize = pText->getWidthHeight();
	double txtExt = textSize.x;
	bool dimOut = false;	//是否为外部标注。当文字不能在2点间放下时，采用外部标注
	double extensionDist = m_arcEndPt1.distanceTo(m_arcEndPt2);	//标注边界间距
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
		updateOuterDim(boundaryLineDir1, boundaryLineDir2, boundaryLine1StartOffset, boundaryLine1EndOffset, boundaryLine2StartOffset, boundaryLine2EndOffset, extendBoundaryLine1, extendBoundaryLine2, clipBoundLine, pText, textSize);
	}
	else
	{
		//内部标注
		updateInnerDim(boundaryLineDir1, boundaryLineDir2, boundaryLine1StartOffset, boundaryLine1EndOffset, boundaryLine2StartOffset, boundaryLine2EndOffset, extendBoundaryLine1, extendBoundaryLine2, clipBoundLine, pText, textSize);
	}

	//绘制标注边界线
	if (!data.hideBoundLine1() && needBoundLine1)
	{
		double extendLen = 0.0;
		if (extendBoundaryLine1 && !clipBoundLine)
			extendLen = textSize.x;
		DmLine* boundLine1 = nullptr;
		if (boundaryLine1StartOffset > 0.0)	//从直线1的终点延伸
		{
			boundLine1 = new DmLine(this, edata.line1EndPt + boundaryLineDir1 * boundaryLine1StartOffset, edata.line1EndPt + boundaryLineDir1 * (boundaryLine1EndOffset + extendLen));
		}
		else
		{
			//从直线1的起点延伸
			boundLine1 = new DmLine(this, edata.line1StartPt + boundaryLineDir1 * (-boundaryLine1StartOffset), edata.line1StartPt + boundaryLineDir1 * (-boundaryLine1EndOffset + extendLen));
		}
		boundLine1->setPen(DmPen(data.boundLineColor(), data.boundLineWidth(), data.boundLineType()));
		addEntity(boundLine1);
	}
	if (!data.hideBoundLine2() && needBoundLine2)
	{
		double extendLen = 0.0;
		if (extendBoundaryLine2 && !clipBoundLine)
			extendLen = textSize.x;
		DmLine* boundLine2 = nullptr;
		if (boundaryLine2StartOffset > 0.0)	//从直线2的终点延伸
		{
			boundLine2 = new DmLine(this, edata.line2EndPt + boundaryLineDir2 * boundaryLine2StartOffset, edata.line2EndPt + boundaryLineDir2 * (boundaryLine2EndOffset + extendLen));
		}
		else
		{
			//从直线2的起点延伸
			boundLine2 = new DmLine(this, edata.line2StartPt + boundaryLineDir2 * (-boundaryLine2StartOffset), edata.line2StartPt + boundaryLineDir2 * (-boundaryLine2EndOffset + extendLen));
		}
		boundLine2->setPen(DmPen(data.boundLineColor(), data.boundLineWidth(), data.boundLineType()));
		addEntity(boundLine2);
	}

	calculateBorders();
}

void DmDimAngular::move(const DmVector& offset)
{
	DmDimension::move(offset);

	edata.line1StartPt.move(offset);
	edata.line1EndPt.move(offset);
	edata.line2StartPt.move(offset);
	edata.line2EndPt.move(offset);
	edata.ptOnArc.move(offset);
	update();
}

void DmDimAngular::rotate(const DmVector& center, const DmVector& angleVector)
{
	DmDimension::rotate(center, angleVector);

	edata.line1StartPt.rotate(center, angleVector);
	edata.line1EndPt.rotate(center, angleVector);
	edata.line2StartPt.rotate(center, angleVector);
	edata.line2EndPt.rotate(center, angleVector);
	edata.ptOnArc.rotate(center, angleVector);
	update();
}

void DmDimAngular::scale(const DmVector& center, const DmVector& factor)
{
	DmDimension::scale(center, factor);

	edata.line1StartPt.scale(center, factor);
	edata.line1EndPt.scale(center, factor);
	edata.line2StartPt.scale(center, factor);
	edata.line2EndPt.scale(center, factor);
	edata.ptOnArc.scale(center, factor);
	update();
}

void DmDimAngular::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
	DmDimension::mirror(axisPoint1, axisPoint2);

	edata.line1StartPt.mirror(axisPoint1, axisPoint2);
	edata.line1EndPt.mirror(axisPoint1, axisPoint2);
	edata.line2StartPt.mirror(axisPoint1, axisPoint2);
	edata.line2EndPt.mirror(axisPoint1, axisPoint2);
	edata.ptOnArc.mirror(axisPoint1, axisPoint2);
	update();
}

void DmDimAngular::moveRef(const DmVector& ref, const DmVector& offset)
{
	if (ref.distanceTo(edata.ptOnArc) < 1.0e-4)
	{
		edata.ptOnArc.move(offset);
		updateDim(true);
	}
}

void DmDimAngular::saveStream(OutputStream& wrt) const
{
	DmDimension::saveStream(wrt);

	DmVector xLine1Start = edata.line1StartPt;
	DmVector xLine1End = edata.line1EndPt;
	DmVector xLine2Start = edata.line2StartPt;
	DmVector xLine2End = edata.line2EndPt;
	DmVector xPtOnArc = edata.ptOnArc;

	wrt << (double)xLine1Start.x << (double)xLine1Start.y << (double)xLine1End.x << (double)xLine1End.y << (double)xLine2Start.x << (double)xLine2Start.y << (double)xLine2End.x << (double)xLine2End.y << (double)xPtOnArc.x << (double)xPtOnArc.y;
}

void DmDimAngular::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
	int fileRev = getRevisionId("DmDimAngular", revs);
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

void DmDimAngular::restoreStreamWithRev(InputStream& rdr, int rev)
{
	if (rev == 0)
	{
	}
	else //big change, e.g. change supper class of DmDimAngular
	{
		//step1.
		// read all legacy data one by one
	}
}

void DmDimAngular::restoreStream(InputStream& rdr)
{
    DmDimension::restoreStream(rdr);

    DmVector xLine1Start(true), xLine1End(true), xLine2Start(true), xLine2End(true), xPtOnArc(true);
    rdr >> (double&)xLine1Start.x >> (double&)xLine1Start.y >> (double&)xLine1End.x >> (double&)xLine1End.y >> (double&)xLine2Start.x >> (double&)xLine2Start.y >> (double&)xLine2End.x >> (double&)xLine2End.y >> (double&)xPtOnArc.x >> (double&)xPtOnArc.y;

    edata.line1StartPt = xLine1Start;
    edata.line1EndPt = xLine1End;
    edata.line2StartPt = xLine2Start;
    edata.line2EndPt = xLine2End;
    edata.ptOnArc = xPtOnArc;
    update();
}
