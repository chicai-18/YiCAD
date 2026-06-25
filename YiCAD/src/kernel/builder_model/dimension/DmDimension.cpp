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


/// @file DmDimension.cpp
/// @brief DmDimension 标注基类的实现

#include<iostream>
#include<cmath>
#include<string>

#include "Information.h"
#include "DmLine.h"
#include "DmSolid.h"
#include "DmArc.h"
#include "DmConstructionLine.h"
#include "DmUnits.h"
#include "DmDimension.h"
#include "DmTextStyle.h"
#include "DmTextStyleTable.h"
#include "DmDocument.h"
#include "DmDimensionStyle.h"

#include "Math2d.h"
#include "Debug.h"

TYPESYSTEM_SOURCE(DmDimension, DmEntity, 0);
DIM_FUNCS_IMPLEMENT(DmDimensionData)

/// @param definitionPoint Definition point.
/// @param middleOfText Middle point of dimension text.
/// @param valign Vertical alignment.
/// @param halign Horizontal alignment.
/// @param lineSpacingStyle Line spacing style.
/// @param lineSpacingFactor Line spacing factor.
/// @param text Text string entered explicitly by user or null or "<>" for the actual measurement or " " (one blank space).for suppressing the text.
/// @param style Dimension style name.
/// @param angle Rotation angle of dimension text away from  default orientation.
DmDimensionData::DmDimensionData(const DmVector& _definitionPoint, const DmVector& _middleOfText, EMTextVertMode _valign, EMTextHorzMode _halign,
	double _lineSpacingFactor, QString _text, double _angle, DmDimensionStyle* pStyle)
	: definitionPoint(_definitionPoint)
	, textCenter(_middleOfText)
	, valign(_valign)
	, halign(_halign)
	, lineSpacingFactor(_lineSpacingFactor)
	, text(_text)
	, angle(_angle)
	, pDimStyle(pStyle)
{
	//从标注样式中复制信息到替代属性
	if (pDimStyle != nullptr) 
	{
		const DmDimensionStyleData& styleData = pDimStyle->getDataConstRef();
		setLineData(const_cast<DmDimensionStyleData&>(styleData).lineDataRef()); 
		setArrowData(const_cast<DmDimensionStyleData&>(styleData).arrowDataRef()); 
		setTextData(const_cast<DmDimensionStyleData&>(styleData).textDataRef()); 
		setUnitData(const_cast<DmDimensionStyleData&>(styleData).unitDataRef()); 
	}	
}

DmDimension::DmDimension()
	:DmEntity(nullptr)
	, container(new DmEntityContainer(this))
{
}

DmDimension::DmDimension(DmEntity* parent, const DmDimensionData& d)
	: DmEntity(parent)
	, data(d)
	, container(new DmEntityContainer(this))
{
}

DmDimension::~DmDimension()
{
	if (container)
	{
		delete container;
		container = nullptr;
	}
}

DmEntity* DmDimension::clone() const
{
	// 在子类实现
	return nullptr;
}

DmVector DmDimension::getNearestRef(const DmVector& coord, double* dist /*= nullptr*/) const
{
	// override the DmEntityContainer method
	return DmEntity::getNearestRef(coord, dist);
}

DmVector DmDimension::getNearestSelectedRef(const DmVector& coord, double* dist /*= nullptr*/) const
{
	// override the DmEntityContainer method
	return DmEntity::getNearestSelectedRef(coord, dist);
}

DmDimensionData DmDimension::getData() const
{
	return data;
}

DmDimensionData& DmDimension::getDataRef()
{
	return data;
}

QString DmDimension::getLabel() const
{
	return data.text;
}

// Sets a new text for the label.
void DmDimension::setLabel(const QString& l)
{
	data.text = l;
}

void DmDimension::update()
{
	updateDim();
}

void DmDimension::updateDim(bool autoText)
{
}

void DmDimension::addEntity(DmEntity* e)
{
	if (container && e)
	{
		container->addEntity(e);
	}
}

void DmDimension::clear()
{
	container->clear();
}

bool DmDimension::removeEntity(DmEntity* entity)
{
	return container->removeEntity(entity);
}

DmVector DmDimension::getDefinitionPoint() const
{
	return data.definitionPoint;
}

DmVector DmDimension::getMiddleOfText() const
{
	return data.textCenter;
}

EMTextVertMode DmDimension::getVAlign() const
{
	return data.valign;
}

EMTextHorzMode DmDimension::getHAlign() const
{
	return data.halign;
}

double DmDimension::getLineSpacingFactor()
{
	return data.lineSpacingFactor;
}

double DmDimension::getLineSpacingFactor() const
{
	return data.lineSpacingFactor;
}

DmDimensionStyle* DmDimension::getStyle()
{
	return data.pDimStyle;
}

DmDimensionStyle* DmDimension::getStyle() const
{
	return data.pDimStyle;
}

double DmDimension::getAngle()
{
	return data.angle;
}

bool DmDimension::isContainer() const
{
	return false;
}

void DmDimension::calculateBorders()
{
	container->calculateBorders();
	maxV = container->getMax();
	minV = container->getMin();
}

double DmDimension::getDistanceToPoint(const DmVector& coord, DmEntity** entity, DM::ResolveLevel level) const
{
	if (coord.x < maxV.x && coord.y < maxV.y && coord.x > minV.x && coord.y > minV.y)
	{
		double minDist = DM_MAXDOUBLE;
		double curDist = 0.0;
		for (auto e : *container)
		{
			curDist = e->getDistanceToPoint(coord);
			if (curDist < minDist)
			{
				minDist = curDist;
			}
		}
		return minDist;
	}
	return DM_MAXDOUBLE;
}

DmVector DmDimension::getNearestEndpoint(const DmVector& coord, double* dist) const
{
	double minDist = DM_MAXDOUBLE;
	double curDist = DM_MAXDOUBLE;
	DmVector res(false);
	for (auto e : *container)
	{
		// 标注的文字，忽略
		if (e->getEntityType() == DM::EntityContainer)
		{
			continue;
		}
		DmVector temp = e->getNearestEndpoint(coord, &curDist);
		if (temp.valid && curDist < minDist)
		{
			minDist = curDist;
			res = temp;
		}
	}
	if (dist)
	{
		*dist = minDist;
	}
	return res;
}

DmVector DmDimension::getNearestPointOnEntity(const DmVector& coord, bool onEntity, double* dist, DmEntity** entity) const
{
	double minDist = DM_MAXDOUBLE;
	double curDist = DM_MAXDOUBLE;
	DmVector res(false);
	for (auto e : *container)
	{
		DmVector temp = e->getNearestPointOnEntity(coord, true, &curDist);
		if (temp.valid && curDist < minDist)
		{
			minDist = curDist;
			res = temp;
		}
	}
	if (dist)
	{
		*dist = minDist;
	}
	return res;
}

DmVector DmDimension::getNearestCenter(const DmVector& coord, double* dist) const
{
	double minDist = DM_MAXDOUBLE;
	double curDist = DM_MAXDOUBLE;
	DmVector res(false);
	for (auto e : *container)
	{
		// 标注的文字，忽略
		if (e->getEntityType() == DM::EntityContainer)
		{
			continue;
		}
		DmVector temp = e->getNearestCenter(coord, &curDist);
		if (temp.valid && curDist < minDist)
		{
			minDist = curDist;
			res = temp;
		}
	}
	if (dist)
	{
		*dist = minDist;
	}
	return res;
}

DmVector DmDimension::getNearestMiddle(const DmVector& coord, double* dist, int middlePoints) const
{
	double minDist = DM_MAXDOUBLE;
	double curDist = DM_MAXDOUBLE;
	DmVector res(false);
	for (auto e : *container)
	{
		// 标注的文字，忽略
		if (e->getEntityType() == DM::EntityContainer)
		{
			continue;
		}
		DmVector temp = e->getNearestMiddle(coord, &curDist);
		if (temp.valid && curDist < minDist)
		{
			minDist = curDist;
			res = temp;
		}
	}
	if (dist)
	{
		*dist = minDist;
	}
	return res;
}

void DmDimension::setVisible(bool v)
{
	DmEntity::setVisible(v);
	container->setVisible(v);
}

bool DmDimension::setSelected(bool select)
{
	if (DmEntity::setSelected(select))
	{
		container->setSelected(select);
		return true;
	}
	else
	{
		return false;
	}
}

void DmDimension::setHighlighted(bool highlight)
{
	DmEntity::setHighlighted(highlight);
	container->setHighlighted(highlight);
}

void DmDimension::move(const DmVector& offset)
{
	data.definitionPoint.move(offset);
	data.textCenter.move(offset);
}

void DmDimension::rotate(const DmVector& center, const DmVector& angleVector)
{
	data.definitionPoint.rotate(center, angleVector);
	data.textCenter.rotate(center, angleVector);
	data.angle = Math2d::correctAngle(data.angle + angleVector.angle());
}

void DmDimension::scale(const DmVector& center, const DmVector& factor)
{
	data.definitionPoint.scale(center, factor);
	data.textCenter.scale(center, factor);
}

void DmDimension::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
	data.definitionPoint.mirror(axisPoint1, axisPoint2);
	data.textCenter.mirror(axisPoint1, axisPoint2);
}

std::list<DmEntity*> DmDimension::getSubEntities() const
{
	std::list<DmEntity*> subEnts = std::list<DmEntity*>();

	for (auto ent : *container)
	{
		if (ent->getEntityType() == DM::EntityContainer)
		{
			auto textEnts = ((DmEntityContainer*)ent)->getEntityList();
			for (auto text : textEnts)
			{
				auto textSub = text->getSubEntities();
				subEnts.splice(subEnts.end(), textSub);
			}
		}
		else
		{
			auto entSub = ent->getSubEntities();
			if (entSub.size() > 0)
			{
				subEnts.splice(subEnts.end(), entSub);
			}
			else
			{
				subEnts.emplace_back(std::move(ent));
			}
		}
	}
	return subEnts;
}

DM::ArrowType DmDimension::stringToArrow(const std::string& arrowName)
{
	//参考AutoCAD系统变量"DIMBLK"
	std::map< std::string, DM::ArrowType> map{
		{"", DM::ArrowType::ClosedFilled},
		{"_CLOSED", DM::ArrowType::Closed},
		{"_CLOSEDBLANK", DM::ArrowType::ClosedBlank},
		{"_DOT", DM::ArrowType::Dot},
		{"_ARCHTICK", DM::ArrowType::ArchitecturalTick},
		{"_OBLIQUE", DM::ArrowType::Oblique},
		{"_OPEN", DM::ArrowType::Open},
		{"_ORIGIN", DM::ArrowType::OriginIndicator},
		{"_ORIGIN2", DM::ArrowType::OriginIndicator2},
		{"_OPEN90", DM::ArrowType::RightAngle},
		{"_OPEN30", DM::ArrowType::Open30},
		{"_DOTSMALL", DM::ArrowType::DotSmall},
		{"_DOTBLANK", DM::ArrowType::DotBlank},
		{"_SMALL", DM::ArrowType::DotSmallBlank},
		{"_BOXBLANK", DM::ArrowType::Box},
		{"_BOXFILLED", DM::ArrowType::BoxFilled},
		{"_DATUMBLANK", DM::ArrowType::DatumTriangle},
		{"_DATUMFILLED", DM::ArrowType::DatumTriangleFilled},
		{"_INTEGRAL", DM::ArrowType::Integral},
		{"_NONE", DM::ArrowType::None},
	};
	std::string arrowNameUpper(arrowName.size(), 0);
	std::transform(arrowName.begin(), arrowName.end(), arrowNameUpper.begin(), std::bind(&std::toupper<char>, std::placeholders::_1, std::locale()));  //此转换不支持中文，但是此处没有中文，所以没事
	auto it = map.find(arrowNameUpper);
	if (it != map.end())
	{
		return it->second;
	}
	else
	{
		return DM::ArrowType::ClosedFilled;
	}
}

void DmDimension::saveStream(OutputStream& wrt) const
{
	DmEntity::saveStream(wrt);

	int attachmentPoint = 1;
	if (getHAlign() == EMTextHorzMode::kTextLeft)
	{
		attachmentPoint = 1;
	}
	else if (getHAlign() == EMTextHorzMode::kTextCenter)
	{
		attachmentPoint = 2;
	}
	else if (getHAlign() == EMTextHorzMode::kTextRight)
	{
		attachmentPoint = 3;
	}

	if (getVAlign() == EMTextVertMode::kTextTop)
	{
		attachmentPoint += 0;
	}
	else if (getVAlign() == EMTextVertMode::kTextVertMid)
	{
		attachmentPoint += 3;
	}
	else if (getVAlign() == EMTextVertMode::kTextBottom)
	{
		attachmentPoint += 6;
	}

	DmVector textPt = getMiddleOfText();
	std::string dimStyle = getStyle()->getName().toStdString();
	int textAlignMode = attachmentPoint;
	int textLineStyle = 1;	//kAtLeast
	std::string textString = getLabel().toStdString();
	double textLineFactor = getLineSpacingFactor();

	//线
	DmColor clrd = data.dimLineColor();
	int clrdIdx, clrdR, clrdG, clrdB;
	DmColor::toCAD(clrd, clrdIdx, clrdR, clrdG, clrdB);
	int Dimclrd = clrdIdx;
	int Dimclrd_r = clrdR;
	int Dimclrd_g = clrdG;
	int Dimclrd_b = clrdB;
	int Dimlwd = (int)data.dimLineWidth();
	bool Dimsd1 = data.hideDimLine1();
	bool Dimsd2 = data.hideDimLine2();

	DmColor clre = data.boundLineColor();
	int clreIdx, clreR, clreG, clreB;
	DmColor::toCAD(clre, clreIdx, clreR, clreG, clreB);
	int Dimclre = clreIdx;
	int Dimclre_r = clreR;
	int Dimclre_g = clreG;
	int Dimclre_b = clreB;
	int Dimlwe = (int)data.boundLineWidth();
	bool Dimse1 = data.hideBoundLine1();
	bool Dimse2 = data.hideBoundLine2();
	double Dimexe = data.extendDimLine();
	double Dimexo = data.startPtOffset();
	bool Dimfxlon = data.isFixedBoundLineLength();
	double Dimfxl = data.fixedBoundLineLength();

	std::map<DM::ArrowType, std::string> map{
		{DM::ArrowType::ClosedFilled, ""},
		{DM::ArrowType::Closed, "_CLOSED"},
		{DM::ArrowType::ClosedBlank, "_CLOSEDBLANK"},
		{DM::ArrowType::Dot, "_DOT"},
		{DM::ArrowType::ArchitecturalTick, "_ARCHTICK"},
		{DM::ArrowType::Oblique, "_OBLIQUE"},
		{DM::ArrowType::Open, "_OPEN"},
		{DM::ArrowType::OriginIndicator, "_ORIGIN"},
		{DM::ArrowType::OriginIndicator2, "_ORIGIN2"},
		{DM::ArrowType::RightAngle, "_OPEN90"},
		{DM::ArrowType::Open30, "_OPEN30"},
		{DM::ArrowType::DotSmall, "_DOTSMALL"},
		{DM::ArrowType::DotBlank, "_DOTBLANK"},
		{DM::ArrowType::DotSmallBlank, "_SMALL"},
		{DM::ArrowType::Box, "_BOXBLANK"},
		{DM::ArrowType::BoxFilled, "_BOXFILLED"},
		{DM::ArrowType::DatumTriangle, "_DATUMBLANK"},
		{DM::ArrowType::DatumTriangleFilled, "_DATUMFILLED"},
		{DM::ArrowType::Integral, "_INTEGRAL"},
		{DM::ArrowType::None, "_NONE"},
	};

	//箭头
	// out
	std::string Dimblk1;
	auto blk1 = map.find(data.firstArrow());
	if (blk1 != map.end())
		Dimblk1 = blk1->second;
	else
		Dimblk1 = "";

	std::string Dimblk2;
	auto blk2 = map.find(data.secondArrow());
	if (blk2 != map.end())
		Dimblk2 = blk2->second;
	else
		Dimblk2 = "";

	std::string Dimldrblk;
	auto blk = map.find(data.leaderArrow());
	if (blk != map.end())
		Dimldrblk = blk->second;
	else
		Dimldrblk = "";

	double Dimasz = data.arrowSize();

	//文字
	std::string Dimtxsty = data.textStyle()->getName().toStdString();

	DmColor clrt = data.textColor();
	int clrtIdx, clrtR, clrtG, clrtB;
	DmColor::toCAD(clrt, clrtIdx, clrtR, clrtG, clrtB);
	int Dimclrt = clrtIdx;
	int Dimclrt_r = clrtR;
	int Dimclrt_g = clrtG;
	int Dimclrt_b = clrtB;
	double Dimtxt = data.textHeight();
	double Dimtfac = data.fractionHeightScale();
	bool DrawBox = data.isDrawTextBoundary();
	int Dimtad = static_cast<int>(data.textVerticalPos());
	int Dimjust = static_cast<int>(data.textHorizontalPos());
	
	bool viewDir = data.viewDirection() == DmDimensionStyleTextData::ViewDirection::LeftToRight ? false : true;
	bool Dimtxtdirection = viewDir;
	double Dimgap = data.offsetFromDimLine();

	//单位
	int Dimlunit = static_cast<int>(data.unitFormat()) + 1;
	int Dimdec = static_cast<int>(data.precision());
	int Dimfrac = static_cast<int>(data.fractionFormat());

	int c = '.';    //句点
	if (data.decimalSaparator() == DmDimensionStyleUnitData::DecimalSaparator::Comma)//逗点
	{
		c = ',';
	}
	else  if (data.decimalSaparator() == DmDimensionStyleUnitData::DecimalSaparator::Space) //空格
	{
		c = ' ';
	}

	int Dimdsep = c;
	double Dimrnd = data.roundOff();
	std::string Dimpost = data.postfix().toStdString();
	std::string Dimprefix = data.prefix().toStdString();
	double Dimlfac = data.mesureUnitFactor();
	bool ResetPrefix = data.resetPrefix();
	bool ResetPostfix = data.resetPostfix();
	int Dimaunit = static_cast<int>(data.angleUnitFormat());
	int Dimadec = static_cast<int>(data.anglePrecision());
	bool ResetAnglePrefix = data.resetAnglePrefix();
	bool ResetAnglePostfix = data.resetAnglePostfix();

	wrt << (double)textPt.x << (double)textPt.y << (std::string)dimStyle << (int)textAlignMode << (int)textLineStyle << (std::string)textString << (double)textLineFactor << (int)Dimclrd << (int)Dimclrd_r << (int)Dimclrd_g << (int)Dimclrd_b << (int)Dimlwd << (bool)Dimsd1 <<
		(bool)Dimsd2 << (int)Dimclre << (int)Dimclre_r << (int)Dimclre_g << (int)Dimclre_b << (int)Dimlwe << (bool)Dimse1 << (bool)Dimse2 << (double)Dimexe << (double)Dimexo << (bool)Dimfxlon << (double)Dimfxl << (std::string)Dimblk1 << (std::string)Dimblk2 << (std::string)Dimldrblk << (double)Dimasz <<
		(std::string)Dimtxsty << (int)Dimclrt << (int)Dimclrt_r << (int)Dimclrt_g << (int)Dimclrt_b << (double)Dimtxt << (double)Dimtfac << (bool)DrawBox << (int)Dimtad << (int)Dimjust << (bool)Dimtxtdirection << (double)Dimgap << (int)Dimlunit << (int)Dimdec << (int)Dimfrac <<
		(int)Dimdsep << (double)Dimrnd << (std::string)Dimpost << (std::string)Dimprefix << (double)Dimlfac << (bool)ResetPrefix << (bool)ResetPostfix << (int)Dimaunit << (int)Dimadec << (bool)ResetAnglePrefix << (bool)ResetAnglePostfix;
}

void DmDimension::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
	int fileRev = getRevisionId("DmDimension", revs);
	if (revId > fileRev)
	{
        DmEntity::restoreStream(reader, revs);
		// 老文件格式
		restoreStreamWithRev(reader, fileRev);
	}
	else
	{
        restoreStream(reader);
	}
}

void DmDimension::restoreStreamWithRev(InputStream& rdr, int rev)
{
	if (rev == 0)
	{
	}
	else //big change, e.g. change supper class of DmDimension
	{
		//step1.
		// read all legacy data one by one
	}
}

void DmDimension::restoreStream(InputStream& rdr)
{
    DmEntity::restoreStream(rdr);

    DmVector textPt(true);
    std::string dimStyle;
    int textAlignMode;
    int textLineStyle;
    std::string textString;
    double textLineFactor;

    int Dimclrd, Dimclrd_r, Dimclrd_g, Dimclrd_b, Dimlwd;
    bool Dimsd1, Dimsd2;
    int Dimclre, Dimclre_r, Dimclre_g, Dimclre_b, Dimlwe;
    bool Dimse1, Dimse2;
    double Dimexe, Dimexo;
    bool Dimfxlon;
    double Dimfxl;
    std::string Dimblk1, Dimblk2, Dimldrblk;
    double Dimasz;
    std::string Dimtxsty;
    int Dimclrt, Dimclrt_r, Dimclrt_g, Dimclrt_b;
    double Dimtxt, Dimtfac;
    bool DrawBox;
    int Dimtad, Dimjust;
    bool Dimtxtdirection;
    double Dimgap;
    int Dimlunit, Dimdec, Dimfrac, Dimdsep;
    double Dimrnd;
    std::string Dimpost, Dimprefix;
    double Dimlfac;
    bool ResetPrefix, ResetPostfix;
    int Dimaunit, Dimadec;
    bool ResetAnglePrefix, ResetAnglePostfix;

    rdr >> (double&)textPt.x >> (double&)textPt.y >> (std::string&)dimStyle >> (int&)textAlignMode >> (int&)textLineStyle >> (std::string&)textString >> (double&)textLineFactor >> (int&)Dimclrd >> (int&)Dimclrd_r >> (int&)Dimclrd_g >> (int&)Dimclrd_b >> (int&)Dimlwd >>
           (bool&)Dimsd1 >> (bool&)Dimsd2 >> (int&)Dimclre >> (int&)Dimclre_r >> (int&)Dimclre_g >> (int&)Dimclre_b >> (int&)Dimlwe >> (bool&)Dimse1 >> (bool&)Dimse2 >> (double&)Dimexe >> (double&)Dimexo >> (bool&)Dimfxlon >> (double&)Dimfxl >> (std::string&)Dimblk1 >> (std::string&)Dimblk2 >> (std::string&)Dimldrblk >>
           (double&)Dimasz >> Dimtxsty >> (int&)Dimclrt >> (int&)Dimclrt_r >> (int&)Dimclrt_g >> (int&)Dimclrt_b >> (double&)Dimtxt >> (double&)Dimtfac >> (bool&)DrawBox >> (int&)Dimtad >> (int&)Dimjust >> (bool&)Dimtxtdirection >> (double&)Dimgap >> (int&)Dimlunit >> (int&)Dimdec >>
           (int&)Dimfrac >> (int&)Dimdsep >> (double&)Dimrnd >> (std::string&)Dimpost >> (std::string&)Dimprefix >> (double&)Dimlfac >> (bool&)ResetPrefix >> (bool&)ResetPostfix >> (int&)Dimaunit >> (int&)Dimadec >> (bool&)ResetAnglePrefix >> (bool&)ResetAnglePostfix;

    DmVector defP(0, 0); // 父类型获取不到此坐标,在实际调用时赋值
    DmVector midP = textPt;
    EMTextVertMode valign;
    EMTextHorzMode halign;
    QString sty = QString::fromStdString(dimStyle);

    if (fabs(midP.x) < 1.0e-6 && fabs(midP.y) < 1.0e-6)
    {
        midP = DmVector(false);
    }

    int alignMode = textAlignMode;
    if (alignMode <= 3)
    {
        valign = EMTextVertMode::kTextTop;
    }
    else if (alignMode <= 6)
    {
        valign = EMTextVertMode::kTextVertMid;
    }
    else
    {
        valign = EMTextVertMode::kTextBottom;
    }

    if (alignMode % 3 == 1)
    {
        halign = EMTextHorzMode::kTextLeft;
    }
    else if (alignMode % 3 == 2)
    {
        halign = EMTextHorzMode::kTextCenter;
    }
    else
    {
        halign = EMTextHorzMode::kTextRight;
    }
    QString strText = QString::fromStdString(textString);

    DmDimensionStyle* pStyle = getDocument()->getDimStyleTable()->find(sty);
    data.pDimStyle = pStyle;
    data.definitionPoint = defP;
    data.textCenter = midP;
    data.valign = valign;
    data.halign = halign;
    data.lineSpacingFactor = textLineFactor;
    data.text = strText;
    data.angle = 0;

    //线
    data.setDimLineColor(DmColor::fromCAD(Dimclrd, Dimclrd_r, Dimclrd_g, Dimclrd_b));
    data.setDimLineWidth(static_cast<DM::LineWidth>(Dimlwd));
    data.setHideDimLine1(Dimsd1);
    data.setHideDimLine2(Dimsd2);
    data.setBoundLineColor(DmColor::fromCAD(Dimclre, Dimclre_r, Dimclre_g, Dimclre_b));
    data.setBoundLineWidth(static_cast<DM::LineWidth>(Dimlwe));
    data.setHideBoundLine1(Dimse1);
    data.setHideBoundLine2(Dimse2);
    data.setStartPtOffset(Dimexo);
    data.setExtendDimLine(Dimexe);
    data.setIsFixedBoundLineLength(Dimfxlon);
    data.setFixedBoundLineLength(Dimfxl);

    //箭头
    auto strDimblk1 = QString::fromStdString(Dimblk1);
    data.setFirstArrow(stringToArrow(strDimblk1.toStdString()));
    auto strDimblk2 = QString::fromStdString(Dimblk2);
    data.setSecondArrow(stringToArrow(strDimblk2.toStdString()));
    auto strDimldrblk = QString::fromStdString(Dimldrblk);
    data.setLeaderArrow(stringToArrow(strDimldrblk.toStdString()));
    data.setArrowSize(Dimasz);

    //文字
    DmTextStyle* txtStyle = getDocument()->getTextStyleTable()->find(QString::fromStdString(Dimtxsty));
    data.setTextStyle(txtStyle);
    data.setTextColor(DmColor::fromCAD(Dimclrt, Dimclrt_r, Dimclrt_g, Dimclrt_b));
    data.setTextHeight(Dimtxt);
    data.setFractionHeightScale(Dimtfac);
    data.setIsDrawTextBoundary(DrawBox);
    data.setTextVerticalPos(static_cast<DmDimensionStyleTextData::TextVerticalPos>(Dimtad));
    data.setTextHorizontalPos(static_cast<DmDimensionStyleTextData::TextHorizontalPos>(Dimjust));
    data.setViewDirection(Dimtxtdirection == false ? DmDimensionStyleTextData::ViewDirection::LeftToRight : DmDimensionStyleTextData::ViewDirection::RightToLeft);
    data.setOffsetFromDimLine(Dimgap);

    //单位
    data.setUnitFormat(static_cast<DmDimensionStyleUnitData::UnitFormat>(Dimlunit - 1));
    data.setPrecision(static_cast<DmDimensionStyleUnitData::Precision>(Dimdec));
    data.setFractionFormat(static_cast<DmDimensionStyleUnitData::FractionFormat>(Dimfrac));
    int c = Dimdsep;
    if (c == ',')//逗点
    {
        data.setDecimalSaparator(DmDimensionStyleUnitData::DecimalSaparator::Comma);
    }
    else  if (c == ' ') //空格
    {
        data.setDecimalSaparator(DmDimensionStyleUnitData::DecimalSaparator::Space);
    }
    else
    {
        //其他全当句点
        data.setDecimalSaparator(DmDimensionStyleUnitData::DecimalSaparator::Dot);
    }
    data.setRoundOff(Dimrnd);
    data.setPrefix(QString::fromStdString(Dimprefix));
    data.setPostfix(QString::fromStdString(Dimpost));
    data.setMesureUnitFactor(Dimlfac);
    data.setResetPrefix(ResetPrefix);
    data.setResetPostfix(ResetPostfix);
    data.setAngleUnitFormat(static_cast<DmDimensionStyleUnitData::AngleUnitFormat>(Dimaunit));
    data.setAnglePrecision(static_cast<DmDimensionStyleUnitData::Precision>(Dimadec));
    data.setResetAnglePrefix(ResetAnglePrefix);
    data.setResetAnglePostfix(ResetAnglePostfix);
}

bool DmDimension::clipLine(const DmLine* line, const DmVector& clipTextBasePt, const DmVector& textDir, const double clipTextWidth)
{
	DmVector lineStartPt = line->getStartpoint();
	DmVector lineEndPt = line->getEndpoint();
	DmVector dir = (lineEndPt - lineStartPt).normalize();
	DmVector tempTextDir = textDir;
	//if (DmVector::dotP(dir,textDir) < 0)
	//{
	//	tempTextDir = -textDir;
	//}
	bool needReverse = DmVector::dotP(dir, textDir) < 0;
	DmDimension* lineContainer = dynamic_cast<DmDimension*>(line->getParent());
	DmPen pen = line->getPen();
	DmLayer* layer = line->getLayer();
	DmConstructionLine tempLine(nullptr, DmConstructionLineData(lineStartPt, lineEndPt));
	DmVector projTextbasePt = tempLine.getNearestPointOnEntity(clipTextBasePt);
	DmVector projTextEndPt = tempLine.getNearestPointOnEntity(clipTextBasePt + textDir * clipTextWidth);
	if (needReverse)
	{
		DmVector temp = projTextbasePt;
		projTextbasePt = projTextEndPt;
		projTextEndPt = temp;
	}
	double startFactor = DmVector::posInLine(lineStartPt, lineEndPt, projTextbasePt);
	double endFactor = DmVector::posInLine(lineStartPt, lineEndPt, projTextEndPt);

	//假设文字与直线同向
	if (endFactor < 0.0 || startFactor>1.0)	//不相交
		return false;
	if (startFactor < 0.0 && endFactor>1.0)	//完全切除
		return true;
	if (startFactor < 0.0)
	{
		if (endFactor < 1.0)
		{
			DmLine* l = new DmLine(lineContainer, projTextEndPt, lineEndPt);
			l->setPen(pen);
			l->setLayer(layer);
			if (lineContainer)
				lineContainer->addEntity(l);
			return true;
		}
	}
	else if (startFactor < 1.0)
	{
		if (endFactor < 1.0)
		{
			DmLine* l1 = new DmLine(lineContainer, lineStartPt, projTextbasePt);
			DmLine* l2 = new DmLine(lineContainer, projTextEndPt, lineEndPt);
			l1->setPen(pen);
			l1->setLayer(layer);
			l2->setPen(pen);
			l2->setLayer(layer);
			if (lineContainer)
			{
				lineContainer->addEntity(l1);
				lineContainer->addEntity(l2);
			}
			return true;
		}
		else
		{
			DmLine* l = new DmLine(lineContainer, lineStartPt, projTextbasePt);
			l->setPen(pen);
			l->setLayer(layer);
			if (lineContainer)
				lineContainer->addEntity(l);
			return true;
		}
	}
	return false;
}

bool DmDimension::clipArc(const DmArc* arc, const DmVector& clipTextBasePt, const DmVector& textDir, const double clipTextWidth)
{
	DmPen pen = arc->getPen();
	DmLayer* layer = arc->getLayer();
	DmDimension* arcContainer = dynamic_cast<DmDimension*>(arc->getParent());
	double arcStartAngle = arc->getStartAngle();
	double arcEndAngle = arc->getEndAngle();
	double radius = arc->getRadius();
	DmVector arcCenter = arc->getCenter();
	DmVector textStartPt = clipTextBasePt;
	DmVector textEndPt = textStartPt + textDir * clipTextWidth;
	double textStartAngle = (textStartPt - arcCenter).angle();
	double textEndAngle = (textEndPt - arcCenter).angle();

	////保证终点角度大于起点角度
	//const double _2PI = M_PI * 2.0;
	//if (arcEndAngle < arcStartAngle || textEndAngle < textStartAngle)
	//{
	//	arcEndAngle += _2PI;
	//	if (arcEndAngle - arcStartAngle > _2PI)
	//	{
	//		arcStartAngle += _2PI;
	//	}
	//	textEndAngle += _2PI;
	//	if (textEndAngle - textStartAngle > _2PI)
	//	{
	//		textStartAngle += _2PI;
	//	}
	//}

	bool isTextStartInRange = isAngleBetween(textStartAngle, arcStartAngle, arcEndAngle);
	bool isTextEndInRange = isAngleBetween(textEndAngle, arcStartAngle, arcEndAngle);
	//假设文字方向与圆弧同向
	if (!isTextStartInRange && !isTextEndInRange)
	{
		double arcAngle = angleBetween(arcStartAngle, arcEndAngle);
		double textAngle = angleBetween(textStartAngle, textEndAngle);
		if (textAngle < arcAngle)
		{
			//不相交
			return false;
		}
		else
		{
			//完全切除
			return true;
		}
	}

	if (!isTextStartInRange)
	{
		if (isTextEndInRange)
		{
			DmArc* newArc = new DmArc(arcContainer, ArcData(arcCenter, DmVector(0.0,0.0,1.0), radius, textEndAngle, arcEndAngle));
			newArc->setPen(pen);
			newArc->setLayer(layer);
			if (arcContainer)
				arcContainer->addEntity(newArc);
			return true;
		}
	}
	else
	{
		if (isTextEndInRange)
		{
			DmArc* newArc1 = new DmArc(arcContainer, ArcData(arcCenter, DmVector(0.0, 0.0, 1.0), radius, arcStartAngle, textStartAngle));
			DmArc* newArc2 = new DmArc(arcContainer, ArcData(arcCenter, DmVector(0.0, 0.0, 1.0), radius, textEndAngle, arcEndAngle));
			newArc1->setPen(pen);
			newArc1->setLayer(layer);
			newArc2->setPen(pen);
			newArc2->setLayer(layer);
			if (arcContainer)
			{
				arcContainer->addEntity(newArc1);
				arcContainer->addEntity(newArc2);
			}
			return true;
		}
		else
		{
			DmArc* newArc = new DmArc(arcContainer, ArcData(arcCenter, DmVector(0.0, 0.0, 1.0), radius, arcStartAngle, textStartAngle));
			newArc->setPen(pen);
			newArc->setLayer(layer);
			if (arcContainer)
			{
				arcContainer->addEntity(newArc);
			}
			return true;
		}
	}
	return false;
}

bool DmDimension::isAngleBetween(const double angle, const double startAngle, const double endAngle) const
{
	if (abs(angle - startAngle) < DM_TOLERANCE_ANGLE || abs(angle - startAngle) < DM_TOLERANCE_ANGLE)
		return true;

	//角度穿过正x轴
	if (endAngle < startAngle)
	{
		if (angle < startAngle)
			return true;
		if (angle > endAngle)
			return true;
	}
	else
	{
		if (angle > startAngle && angle < endAngle)
			return true;
	}
	return false;
}

double DmDimension::angleBetween(const double startAngle, const double endAngle) const
{
	if (endAngle < startAngle)
	{
		return endAngle + M_PI * 2.0 - startAngle;
	}
	else
	{
		return endAngle - startAngle;
	}
}

DmEntityContainer* DmDimension::getTextEntity(const double& textValue)
{
	DmEntityContainer* pText = nullptr;
	if (data.pDimStyle == nullptr)
	{
		//如果标注样式为空，用替代属性去绘制
		DmDimensionStyleData tempData(&data);
		if (data.text.isEmpty())
		{
			pText = DmDimensionStyle::getText(tempData, textValue, this, getEntityType());
		}
		else
		{
			pText = DmDimensionStyle::getText(tempData, data.text, this);
		}
	}
	else
	{
		//存在标注样式
		if (data.text.isEmpty())
		{
			pText = data.pDimStyle->getText(textValue, this, getEntityType());
		}
		else
		{
			//替代文字
			pText = data.pDimStyle->getText(data.text, this);
		}
	}
	return pText;
}
