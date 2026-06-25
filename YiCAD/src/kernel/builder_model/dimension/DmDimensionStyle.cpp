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


/// @file DmDimensionStyle.cpp
/// @brief DmDimensionStyle 标注样式类的实现

#include "DmDimensionStyle.h"
#include "DmEntityContainer.h"
#include "Math2d.h"
#include "DmText.h"
#include "DmLine.h"
#include "DmTextStyle.h"
#include "DmPolyline.h"
#include "DmDimAligned.h"
#include "DmDimAngular.h"
#include "DmDimRadial.h"
#include "DmArc.h"
#include "DmCircle.h"
#include "DmDimensionStyleTable.h"
#include "DmDocument.h"

DIM_FUNCS_IMPLEMENT(DmDimensionStyleData)

TYPESYSTEM_SOURCE(DmDimensionStyle, DmObject, 0)

DmDimensionStyleData::DmDimensionStyleData(const DmDimensionData* dimData)
{
	setLineData(dimData->lineDataConstRef());
	setArrowData(dimData->arrowDataConstRef());
	setTextData(dimData->textDataConstRef());
	setUnitData(dimData->unitDataConstRef());
}

DmDimensionStyle::DmDimensionStyle(const QString& name, DmTextStyle* pTextStyle)
{
	m_styleData.name = name;
	m_styleData.setTextStyle(pTextStyle);
	m_pDocument = nullptr;
}

DmDimensionStyle::DmDimensionStyle(const DmDimensionStyleData& data)
{
	m_styleData = data;
	m_pDocument = nullptr;
}

DmDimensionStyle::DmDimensionStyle(const DmDimensionStyle& style)
{
	m_styleData = style.m_styleData;
	m_pDocument = nullptr;
}

DmDimensionStyle::DmDimensionStyle(const DmDimensionStyle& temp, const QString& name)
{
	m_styleData = temp.m_styleData;
	m_styleData.name = name;
	m_pDocument = nullptr;
}

QString DmDimensionStyle::getName() const
{
	return m_styleData.name;
}

void DmDimensionStyle::updateData(const DmDimensionStyleData& data)
{
	m_styleData = data;
}

bool DmDimensionStyle::canArrow1Mirror() const
{
	return canArrowMirror(m_styleData.firstArrow());
}
bool DmDimensionStyle::canArrow2Mirror() const
{
	return canArrowMirror(m_styleData.secondArrow());
}

double DmDimensionStyle::getArrow1CutDistance() const
{
	return getArrowCutDistance(m_styleData.firstArrow());
}

double DmDimensionStyle::getArrow2CutDistance() const
{
	return getArrowCutDistance(m_styleData.secondArrow());
}

double DmDimensionStyle::getLeaderArrowCutDistance() const
{
	return getArrowCutDistance(m_styleData.leaderArrow());
}

QString DmDimensionStyle::getArrowBlockName(DM::ArrowType arrowType)
{
	QString blkName;
	switch (arrowType)
	{
	case DM::ArrowType::ClosedFilled:
		blkName = "_ClosedFilled";
		break;
	case DM::ArrowType::ClosedBlank:
		blkName = "_ClosedBlank";
		break;
	case DM::ArrowType::Closed:
		blkName = "_Closed";
		break;
	case DM::ArrowType::Dot:
		blkName = "_Dot";
		break;
	case DM::ArrowType::ArchitecturalTick:
		blkName = "_ArchitecturalTick";
		break;
	case DM::ArrowType::Oblique:
		blkName = "_Oblique";
		break;
	case DM::ArrowType::Open:
		blkName = "_Open";
		break;
	case DM::ArrowType::OriginIndicator:
		blkName = "_OriginIndicator";
		break;
	case DM::ArrowType::OriginIndicator2:
		blkName = "_OriginIndicator2";
		break;
	case DM::ArrowType::RightAngle:
		blkName = "_RightAngle";
		break;
	case DM::ArrowType::Open30:
		blkName = "_Open30";
		break;
	case DM::ArrowType::DotSmall:
		blkName = "_DotSmall";
		break;
	case DM::ArrowType::DotBlank:
		blkName = "_DotBlank";
		break;
	case DM::ArrowType::DotSmallBlank:
		blkName = "_DotSmallBlank";
		break;
	case DM::ArrowType::Box:
		blkName = "_Box";
		break;
	case DM::ArrowType::BoxFilled:
		blkName = "_BoxFilled";
		break;
	case DM::ArrowType::DatumTriangle:
		blkName = "_DatumTriangle";
		break;
	case DM::ArrowType::DatumTriangleFilled:
		blkName = "_DatumTriangleFilled";
		break;
	case DM::ArrowType::Integral:
		blkName = "_Integral";
		break;
	case DM::ArrowType::None:
		blkName = "_None";
		break;
	case DM::ArrowType::UserArrow:
		//todo
		break;
	default:
		break;
	}
	return blkName;
}

double DmDimensionStyle::getValidTextHeight() const
{
	if (m_styleData.textHeight() == 0.0)
	{
		return DEFAULT_TEXT_HEIGHT;
	}
	return m_styleData.textHeight();
}

DmEntityContainer* DmDimensionStyle::getText(const double& value, DmDimension* parent, DM::EntityType entityType) const
{
	return getText(m_styleData, value, parent, entityType);
}

DmEntityContainer* DmDimensionStyle::getText(const DmDimensionStyleData& styleData, const double& value, DmDimension* parent, DM::EntityType entityType)
{
	switch (entityType)
	{
	case DM::EntityDimAligned:
	case DM::EntityDimLinear:
	case DM::EntityDimRadial:
	case DM::EntityDimDiametric:
		//长度标注
		return getTextForLinear(styleData, value, parent, entityType);
		break;
	case DM::EntityDimAngular:
		//角度标注文字
		return getTextForAngular(styleData, value, parent, entityType);
		break;
	case DM::EntityDimLeader:
		break;
	default:
		break;
	}
	return nullptr;
}

DmEntityContainer* DmDimensionStyle::getText(const QString& label, DmDimension* parent) const
{
	return getText(m_styleData, label, parent);
}

DmEntityContainer* DmDimensionStyle::getText(const DmDimensionStyleData& styleData, const QString& label, DmDimension* parent)
{
	DmEntityContainer* pContainer = new DmEntityContainer(parent);
	pContainer->setLayer(parent->getLayer());
	pContainer->setPen(parent->getPen());
	parent->addEntity(pContainer);

	DmVector intersectionPt(0.0, 0.0);
	double letterSpace = styleData.textHeight() * 0.1;
	DmPen textLinePen(styleData.textColor(), DM::Width00, DmLineTypeTable::Continuous);
	TextData textData(intersectionPt, styleData.textHeight(), ETextVertMode::kTextBase, ETextHorzMode::kTextLeft, label, styleData.textStyle(), 0.0);
	DmText* text = new DmText(pContainer, textData);
	text->setLayer(pContainer->getLayer());
	text->setPen(textLinePen);
	//text->setColor(m_styleData.textData.textColor);
	text->update();
	pContainer->addEntity(text);

	return pContainer;
}

QString DmDimensionStyle::getFormatString(const double value, DM::EntityType entityType) const
{
	return getFormatString(m_styleData, value, entityType);
}

QString DmDimensionStyle::getFormatString(const DmDimensionStyleData& styleData, const double value, DM::EntityType entityType)
{
	std::vector<QString> strs;
	switch (entityType)
	{
	case DM::EntityDimAligned:
	case DM::EntityDimLinear:
	case DM::EntityDimRadial:
	case DM::EntityDimDiametric:
		//长度标注
		strs = getFormatStrsForLinear(styleData,value, entityType);
		break;
	case DM::EntityDimAngular:
		//角度标注文字
		strs = getFormatStrsForAngular(styleData,value, entityType);
		break;
	case DM::EntityDimLeader:
		break;
	default:
		break;
	}
	QStringList list(strs.begin(), strs.end());
	return list.join("");
}

void DmDimensionStyle::getPreview(DmEntityContainer* previewContainer) const
{
		              /*
					  * 微软雅黑字体查看下面形状
					  * P1--------------------P2
					  * |								  |
					  * |				  ____     	  |P3---------------------P4
					  * |				/		 \											   \
					  * |			  | 		P0   |		(r1)    							      \
					  * |P7    		\ ____ /													\	
					  * \                                                                            \
					  *    \      (r2)                                                                \
					  *       \----_____P6......................................................................\ P5    
					  */
	DmVector basePt(0.0, 0.0);
	double s = m_styleData.textHeight() * 1.025;
	double r1 = 3.0 * s;
	double r2 = 4.0 * s;
	DmVector p0 = basePt + DmVector(r2, r2);
	DmVector p6 = basePt + DmVector(r2, 0.0);
	DmVector p7 = basePt + DmVector(0.0, r2);
	DmVector p1 = p7 + DmVector(0.0, 6.0 * s);
	DmVector p2 = p1 + DmVector(5.0 * s, 0.0);
	DmVector p3 = p2 - DmVector(0.0, s);
	DmVector p4 = p3 + DmVector(5.0 * s, 0.0);
	DmVector p5 = p6 + DmVector(15.0 * s, 0.0);

	previewContainer->clear();
	DmLine* line1 = new DmLine(previewContainer, p1, p2);
	previewContainer->addEntity(line1);
	DmLine* line2 = new DmLine(previewContainer, p2, p3);
	previewContainer->addEntity(line2);
	DmLine* line3 = new DmLine(previewContainer, p3, p4);
	previewContainer->addEntity(line3);
	DmLine* line4 = new DmLine(previewContainer, p4, p5);
	previewContainer->addEntity(line4);
	DmLine* line5 = new DmLine(previewContainer, p5, p6);
	previewContainer->addEntity(line5);
	DmArc* arc = new DmArc(previewContainer, ArcData(p0, DmVector(0.0,0.0,1.0), r2, M_PI, 3.0 * M_PI_2));
    previewContainer->addEntity(arc);
	DmLine* line6 = new DmLine(previewContainer, p7, p1);
	previewContainer->addEntity(line6);
	DmCircle* circle = new DmCircle(previewContainer, CircleData(p0, r1));
	previewContainer->addEntity(circle);

	DmLayer* layer = getDocument()->getLayerTable()->getActive();
	DmPen pen = getDocument()->getActivePen();
	double offset = 2.0 * s;
	DmDimensionData dd1(p1 + DmVector(0.0, 1.0) * offset, DmVector(0.0, 0.0), EMTextVertMode::kTextVertMid, EMTextHorzMode::kTextCenter, 0.0, "", 0.0, const_cast<DmDimensionStyle*>(this));
	DmDimAlignedData ed1(p1, p2);
	DmDimAligned* pDimAlign1 = new DmDimAligned(previewContainer, dd1, ed1);
    pDimAlign1->setDocument(m_pDocument);
	pDimAlign1->setLayer(layer);
	pDimAlign1->setPen(pen);
	previewContainer->addEntity(pDimAlign1);
	pDimAlign1->update();

	DmDimensionData dd2(p4 + (p5-p4).normalize().rotate(M_PI_2) * offset, DmVector(0.0, 0.0), EMTextVertMode::kTextVertMid, EMTextHorzMode::kTextCenter, 0.0, "", 0.0, const_cast<DmDimensionStyle*>(this));
	DmDimAlignedData ed2(p4, p5);
	DmDimAligned* pDimAlign2 = new DmDimAligned(previewContainer, dd2, ed2);
    pDimAlign2->setDocument(m_pDocument);
	pDimAlign2->setLayer(layer);
	pDimAlign2->setPen(pen);
	previewContainer->addEntity(pDimAlign2);
	pDimAlign2->update();

	DmDimensionData dd3(p6, DmVector(0.0, 0.0), EMTextVertMode::kTextVertMid, EMTextHorzMode::kTextCenter, 0.0, "", 0.0, const_cast<DmDimensionStyle*>(this));
	DmDimAngularData ed3(p5, p4, p5, p6, p5 + DmVector(-8.0 * s, 1.0 * s));
	DmDimAngular* pDimAngular = new DmDimAngular(previewContainer, dd3, ed3);
    pDimAngular->setDocument(m_pDocument);
	pDimAngular->setLayer(layer);
	pDimAngular->setPen(pen);
	previewContainer->addEntity(pDimAngular);
	pDimAngular->update();

	DmDimensionData dd4(p0, DmVector(0.0, 0.0), EMTextVertMode::kTextVertMid, EMTextHorzMode::kTextCenter, 0.0, "", 0.0, const_cast<DmDimensionStyle*>(this));
	DmDimRadialData ed4(p0 - DmVector(1.0, 1.0).normalize() * r2, 2.0 * s);
	DmDimRadial* pDimRadial = new DmDimRadial(previewContainer, dd4, ed4);
    pDimRadial->setDocument(m_pDocument);
	pDimRadial->setLayer(layer);
	pDimRadial->setPen(pen);
	previewContainer->addEntity(pDimRadial);
	pDimRadial->update();

	DmDimensionData dd5(p7 + (p1 - p7).normalize().rotate(M_PI_2) * offset, DmVector(0.0, 0.0), EMTextVertMode::kTextVertMid, EMTextHorzMode::kTextCenter, 0.0, "", 0.0, const_cast<DmDimensionStyle*>(this));
	DmDimAlignedData ed5(p7, p1);
	DmDimAligned* pDimAlign7 = new DmDimAligned(previewContainer, dd5, ed5);
    pDimAlign7->setDocument(m_pDocument);
	pDimAlign7->setLayer(layer);
	pDimAlign7->setPen(pen);
	previewContainer->addEntity(pDimAlign7);
	pDimAlign7->update();
}

DM::ArrowType DmDimensionStyle::stringToArrow(const std::string& arrowName)
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

void DmDimensionStyle::saveStream(OutputStream& wrt) const
{
	DmObject::saveStream(wrt);

	std::string name = m_styleData.name.toStdString();

	//线
	DmColor clrd = m_styleData.dimLineColor();
	int clrdIdx, clrdR, clrdG, clrdB;
	DmColor::toCAD(clrd, clrdIdx, clrdR, clrdG, clrdB);
	int Dimclrd = clrdIdx;
	int Dimclrd_r = clrdR;
	int Dimclrd_g = clrdG;
	int Dimclrd_b = clrdB;
	int Dimlwd = (int)m_styleData.dimLineWidth();
	bool Dimsd1 = m_styleData.hideDimLine1();
	bool Dimsd2 = m_styleData.hideDimLine2();

	DmColor clre = m_styleData.boundLineColor();
	int clreIdx, clreR, clreG, clreB;
	DmColor::toCAD(clre, clreIdx, clreR, clreG, clreB);
	int Dimclre = clreIdx;
	int Dimclre_r = clreR;
	int Dimclre_g = clreG;
	int Dimclre_b = clreB;
	int Dimlwe = (int)m_styleData.boundLineWidth();
	bool Dimse1 = m_styleData.hideBoundLine1();
	bool Dimse2 = m_styleData.hideBoundLine2();
	double Dimexe = m_styleData.extendDimLine();
	double Dimexo = m_styleData.startPtOffset();
	bool Dimfxlon = m_styleData.isFixedBoundLineLength();
	double Dimfxl = m_styleData.fixedBoundLineLength();

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
	auto blk1 = map.find(m_styleData.firstArrow());
	if (blk1 != map.end())
		Dimblk1 = blk1->second;
	else
		Dimblk1 = "";

	std::string Dimblk2;
	auto blk2 = map.find(m_styleData.secondArrow());
	if (blk2 != map.end())
		Dimblk2 = blk2->second;
	else
		Dimblk2 = "";

	std::string Dimldrblk;
	auto blk = map.find(m_styleData.leaderArrow());
	if (blk != map.end())
		Dimldrblk = blk->second;
	else
		Dimldrblk = "";

	double Dimasz = m_styleData.arrowSize();

	//文字
	std::string Dimtxsty = m_styleData.textStyle()->getName().toStdString();

	DmColor clrt = m_styleData.textColor();
	int clrtIdx, clrtR, clrtG, clrtB;
	DmColor::toCAD(clrt, clrtIdx, clrtR, clrtG, clrtB);
	int Dimclrt = clrtIdx;
	int Dimclrt_r = clrtR;
	int Dimclrt_g = clrtG;
	int Dimclrt_b = clrtB;
	double Dimtxt = m_styleData.textHeight();
	double Dimtfac = m_styleData.fractionHeightScale();
	bool DrawBox = m_styleData.isDrawTextBoundary();
	int Dimtad = static_cast<int>(m_styleData.textVerticalPos());
	int Dimjust = static_cast<int>(m_styleData.textHorizontalPos());

	bool viewDir = m_styleData.viewDirection() == DmDimensionStyleTextData::ViewDirection::LeftToRight ? false : true;
	bool Dimtxtdirection = viewDir;
	double Dimgap = m_styleData.offsetFromDimLine();

	//单位
	int Dimlunit = static_cast<int>(m_styleData.unitFormat()) + 1;
	int Dimdec = static_cast<int>(m_styleData.precision());
	int Dimfrac = static_cast<int>(m_styleData.fractionFormat());

	int c = '.';    //句点
	if (m_styleData.decimalSaparator() == DmDimensionStyleUnitData::DecimalSaparator::Comma)//逗点
	{
		c = ',';
	}
	else  if (m_styleData.decimalSaparator() == DmDimensionStyleUnitData::DecimalSaparator::Space) //空格
	{
		c = ' ';
	}

	int Dimdsep = c;
	double Dimrnd = m_styleData.roundOff();
	std::string Dimpost = m_styleData.postfix().toStdString();
	std::string Dimprefix = m_styleData.prefix().toStdString();
	double Dimlfac = m_styleData.mesureUnitFactor();
	bool ResetPrefix = m_styleData.resetPrefix();
	bool ResetPostfix = m_styleData.resetPostfix();
	int Dimaunit = static_cast<int>(m_styleData.angleUnitFormat());
	int Dimadec = static_cast<int>(m_styleData.anglePrecision());
	bool ResetAnglePrefix = m_styleData.resetAnglePrefix();
	bool ResetAnglePostfix = m_styleData.resetAnglePostfix();
	
	wrt << (std::string)name << (int)Dimclrd << (int)Dimclrd_r << (int)Dimclrd_g << (int)Dimclrd_b << (int)Dimlwd << (bool)Dimsd1 << (bool)Dimsd2 << (int)Dimclre << (int)Dimclre_r << (int)Dimclre_g << (int)Dimclre_b << (int)Dimlwe << (bool)Dimse1 << (bool)Dimse2 << (double)Dimexe <<
		(double)Dimexo << (bool)Dimfxlon << (double)Dimfxl << (std::string)Dimblk1 << (std::string)Dimblk2 << (std::string)Dimldrblk << (double)Dimasz << (std::string)Dimtxsty << (int)Dimclrt << (int)Dimclrt_r << (int)Dimclrt_g << (int)Dimclrt_b << (double)Dimtxt << (double)Dimtfac << (bool)DrawBox <<
		(int)Dimtad << (int)Dimjust << (bool)Dimtxtdirection << (double)Dimgap << (int)Dimlunit << (int)Dimdec << (int)Dimfrac << (int)Dimdsep << (double)Dimrnd << (std::string)Dimpost << (std::string)Dimprefix << (double)Dimlfac << (bool)ResetPrefix << (bool)ResetPostfix <<
		(int)Dimaunit << (int)Dimadec << (bool)ResetAnglePrefix << (bool)ResetAnglePostfix;
}

void DmDimensionStyle::restoreStream(InputStream& rdr, const std::vector<PAIR>& revs)
{
	int fileRev = getRevisionId("DmDimensionStyle", revs);
	if (revId > fileRev)
	{
        DmObject::restoreStream(rdr, revs);
		// 老文件格式
		restoreStreamWithRev(rdr, fileRev);
	}
	else
	{
        restoreStream(rdr);
	}
}

void DmDimensionStyle::restoreStreamWithRev(InputStream& rdr, int rev)
{
	if (rev == 0)
	{
	}
	else //big change, e.g. change supper class of DmDimensionStyle
	{
		//step1.
		// read all legacy data one by one
	}
}

void DmDimensionStyle::restoreStream(InputStream& rdr)
{
    DmObject::restoreStream(rdr);

    std::string name;
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

    rdr >> (std::string&)name >> (int&)Dimclrd >> (int&)Dimclrd_r >> (int&)Dimclrd_g >> (int&)Dimclrd_b >> (int&)Dimlwd >> (bool&)Dimsd1 >> (bool&)Dimsd2 >> (int&)Dimclre >> (int&)Dimclre_r >> (int&)Dimclre_g >> (int&)Dimclre_b >> (int&)Dimlwe >> (bool&)Dimse1 >> (bool&)Dimse2 >> (double&)Dimexe >>
        (double&)Dimexo >> (bool&)Dimfxlon >> (double&)Dimfxl >> (std::string&)Dimblk1 >> (std::string&)Dimblk2 >> (std::string&)Dimldrblk >> (double&)Dimasz >> (std::string&)Dimtxsty >> (int&)Dimclrt >> (int&)Dimclrt_r >> (int&)Dimclrt_g >> (int&)Dimclrt_b >> (double&)Dimtxt >> (double&)Dimtfac >> (bool&)DrawBox >>
        (int&)Dimtad >> (int&)Dimjust >> Dimtxtdirection >> (double&)Dimgap >> (int&)Dimlunit >> (int&)Dimdec >> (int&)Dimfrac >> (int&)Dimdsep >> (double&)Dimrnd >> (std::string&)Dimpost >> (std::string&)Dimprefix >> (double&)Dimlfac >> (bool&)ResetPrefix >> (bool&)ResetPostfix >>
        (int&)Dimaunit >> (int&)Dimadec >> (bool&)ResetAnglePrefix >> (bool&)ResetAnglePostfix;

    DmDimensionStyleData data = DmDimensionStyleData();
    data.name = QString::fromStdString(name);
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
    if (m_pDocument)
    {
        DmTextStyle* txtStyle = m_pDocument->getTextStyleTable()->find(QString::fromStdString(Dimtxsty));
        data.setTextStyle(txtStyle);
    }
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
    this->updateData(data);
}

DmEntityContainer* DmDimensionStyle::getTextForLinear(const DmDimensionStyleData& styleData, const double value, DmDimension* parent, DM::EntityType entityType)
{
	DmEntityContainer* pContainer = new DmEntityContainer(parent);
	pContainer->setLayer(parent->getLayer());
	pContainer->setPen(parent->getPen());
	parent->addEntity(pContainer);

	std::vector<QString> strs = getFormatStrsForLinear(styleData,value, entityType);
	createTextForStrs(styleData,pContainer, strs);
	return pContainer;
}

DmEntityContainer* DmDimensionStyle::getTextForAngular(const DmDimensionStyleData& styleData, const double value, DmDimension* parent, DM::EntityType entityType)
{
	DmEntityContainer* pContainer = new DmEntityContainer(parent);
	parent->addEntity(pContainer);

	std::vector<QString> strs = getFormatStrsForAngular(styleData,value, entityType);
	createTextForStrs(styleData,pContainer, strs);
	return pContainer;
}

std::vector<QString> DmDimensionStyle::getFormatStrsForAngular(const DmDimensionStyleData& styleData, const double rad, DM::EntityType entityType)
{
	std::vector<QString> strs;
	QString valueStr;
	double dGradian;
	DmDimensionStyleUnitData::AngleUnitFormat angleUnitFormat = styleData.angleUnitFormat();
	switch (angleUnitFormat)
	{
	default:
	case DmDimensionStyleUnitData::AngleUnitFormat::DecimalDegree:
		valueStr = QString::number(Math2d::rad2deg(rad), 'f', static_cast<int>(styleData.anglePrecision()));	//保留后续的0
		break;
	case DmDimensionStyleUnitData::AngleUnitFormat::DMS:
		valueStr = getStrForDMS(styleData,rad);
		break;
	case DmDimensionStyleUnitData::AngleUnitFormat::Gradians:
		dGradian = rad / (2 * M_PI) * 400.0;	//2PI==400（百分度）
		valueStr = QString::number(dGradian, 'f', static_cast<int>(styleData.anglePrecision()));	
		break;
	case DmDimensionStyleUnitData::AngleUnitFormat::Radians:
		valueStr = QString::number(rad, 'f', static_cast<int>(styleData.anglePrecision()));	
		break;
	}

	//消零处理
	if (angleUnitFormat != DmDimensionStyleUnitData::AngleUnitFormat::DMS)
	{
		if (styleData.resetAnglePostfix())	//后续消零
		{
			valueStr = getTrimEndZero(valueStr);
		}
		if (styleData.resetAnglePrefix())	//前导消零
		{
			if (valueStr.startsWith("0") && valueStr.contains("."))
			{
				valueStr = valueStr.right(valueStr.length() - valueStr.indexOf("."));
			}
		}
	}

	//小数点替换
	if (angleUnitFormat != DmDimensionStyleUnitData::AngleUnitFormat::DMS)
	{
		valueStr.replace(".", getDecimalSaparatorStr(styleData));	//小数分隔符
	}
	strs.push_back(valueStr);

	//添加前后缀
	addPrePostfix(styleData,strs, entityType);
	return strs;
}

std::vector<QString> DmDimensionStyle::getFormatStrsForLinear(const DmDimensionStyleData& styleData, const double value, DM::EntityType entityType)
{
	//获得文字（可能包含分数）
	const double measureVal = styleData.mesureUnitFactor() * value;
	double roundOffVal = Math2d::round(measureVal, styleData.roundOff());
	std::vector<QString> strs;
	QString valueStr;
	switch (styleData.unitFormat())
	{
	case DmDimensionStyleUnitData::UnitFormat::Architectural:
		//todo
		valueStr = QString::number(roundOffVal, 'E', static_cast<int>(styleData.precision()));
		strs.push_back(valueStr);
		break;
	case DmDimensionStyleUnitData::UnitFormat::Engineer:
		//todo
		valueStr = QString::number(roundOffVal, 'E', static_cast<int>(styleData.precision()));
		strs.push_back(valueStr);
		break;
	case DmDimensionStyleUnitData::UnitFormat::Fraction:
		strs = getTextStringOfFraction(styleData,roundOffVal);
		break;
	case DmDimensionStyleUnitData::UnitFormat::Science:
		valueStr = QString::number(roundOffVal, 'E', static_cast<int>(styleData.precision()));
		strs.push_back(valueStr);
		break;
	case DmDimensionStyleUnitData::UnitFormat::Decimal:
	case DmDimensionStyleUnitData::UnitFormat::Windows:
		valueStr = QString::number(roundOffVal, 'f', static_cast<int>(styleData.precision()));	//保留后续的0
		if (styleData.resetPostfix())	//后续消零
		{
			valueStr = getTrimEndZero(valueStr);
		}
		if (styleData.resetPrefix())	//前导消零
		{
			if (valueStr.startsWith("0") && valueStr.contains("."))
			{
				valueStr = valueStr.right(valueStr.length() - valueStr.indexOf("."));
			}
		}
		valueStr.replace(".", getDecimalSaparatorStr(styleData));	//小数分隔符
		strs.push_back(valueStr);
	default:
		break;
	}

	addPrePostfix(styleData,strs, entityType);

	return strs;
}

void DmDimensionStyle::addPrePostfix(const DmDimensionStyleData& styleData, std::vector<QString>& strs, DM::EntityType entityType)
{
	//添加前缀
	if (!styleData.prefix().isEmpty())
	{
		strs.insert(strs.begin(), styleData.prefix());
	}
	else
	{
		//对于半径标注，前缀为空，加上R
		if (entityType == DM::EntityDimRadial)
		{
			strs.insert(strs.begin(), "R");
		}
		//对于直径标注，前缀为空，加上%%c
		else if (entityType == DM::EntityDimDiametric)
		{
			strs.insert(strs.begin(), "%%c");
		}
	}

	//添加后缀
	if (!styleData.postfix().isEmpty())
	{
		strs.emplace_back(styleData.postfix());
	}
	else
	{
		//对于角度标注，后缀为空，根据单位类型加上后缀
		if (entityType == DM::EntityDimAngular)
		{
			//十进制度数
			if (styleData.angleUnitFormat() == DmDimensionStyleUnitData::AngleUnitFormat::DecimalDegree)
			{
				strs.emplace_back("%%d");
			}
			//百分度
			else if (styleData.angleUnitFormat() == DmDimensionStyleUnitData::AngleUnitFormat::Gradians)
			{
				strs.emplace_back("g");
			}
			//弧度
			else if (styleData.angleUnitFormat() == DmDimensionStyleUnitData::AngleUnitFormat::Radians)
			{
				strs.emplace_back("r");
			}
		}
	}
}

QString DmDimensionStyle::getStrForDMS(const DmDimensionStyleData& styleData, const double rad)
{
	//弧度转度
	double dDegree = Math2d::rad2deg(rad);
	int nDegree = (int)dDegree;	//强转舍去小数
	double decimal = dDegree - nDegree;
	double dMinute = decimal * 60.0;
	int nMinute = (int)dMinute;
	double dSecond = (dMinute - nMinute) * 60.0;
	const  QString cDegree = "%%d";
	const QString cMinute = "'";
	const QString cSecond = "\"";
	QString valStr;
	DmDimensionStyleUnitData::Precision pre = styleData.anglePrecision();
	switch (pre)
	{
	case DmDimensionStyleUnitData::Precision::Bit0:	//0d
		valStr = QString::number(round(dDegree), 'f', 0) + cDegree;
		break;
	case DmDimensionStyleUnitData::Precision::Bit1:	//0d00'
	case DmDimensionStyleUnitData::Precision::Bit2:
			//todo
		valStr = QString("%1%2%3%4").arg(nDegree).arg(cDegree).arg(QString::number(round(dMinute), 'f', 0)).arg(cMinute);
		break;
	case DmDimensionStyleUnitData::Precision::Bit3:	//0d00'00"
	case DmDimensionStyleUnitData::Precision::Bit4:
		//todo
		valStr = QString::asprintf("%d%s%d%s%s%s", nDegree, cDegree.toStdString().c_str(), nMinute, cMinute.toStdString().c_str(), QString::number(round(dSecond), 'f', 0).toStdString().c_str(), cSecond.toStdString().c_str());
		break;
	case DmDimensionStyleUnitData::Precision::Bit5:	//0d00'00.0"
		//todo
		valStr = QString("%1%2%3%4%5%6").arg(nDegree).arg(cDegree).arg(QString::number(round(dMinute), 'f', 0)).arg(cMinute).arg(QString::number(round(dSecond), 'f', 1)).arg(cSecond);
		//valStr = QString::asprintf("%d%s%d%s%s%s", nDegree, cDegree, nMinute, cMinute, QString::number(round(dSecond), 'f', 1), cSecond);
		break;
	case DmDimensionStyleUnitData::Precision::Bit6:	//0d00'00.00"
		//todo
		valStr = QString("%1%2%3%4%5%6").arg(nDegree).arg(cDegree).arg(QString::number(round(dMinute), 'f', 0)).arg(cMinute).arg(QString::number(round(dSecond), 'f', 2)).arg(cSecond);
		//valStr = QString::asprintf("%d%s%d%s%s%s", nDegree, cDegree, nMinute, cMinute, QString::number(round(dSecond), 'f', 2), cSecond);
		break;
	case DmDimensionStyleUnitData::Precision::Bit7:	//0d00'00.000"
		//todo
		valStr = QString("%1%2%3%4%5%6").arg(nDegree).arg(cDegree).arg(QString::number(round(dMinute), 'f', 0)).arg(cMinute).arg(QString::number(round(dSecond), 'f', 3)).arg(cSecond);
		//valStr = QString::asprintf("%d%s%d%s%s%s", nDegree, cDegree, nMinute, cMinute, QString::number(round(dSecond), 'f', 3), cSecond);
		break;
	case DmDimensionStyleUnitData::Precision::Bit8:	//0d00'00.0000"
		//todo
		valStr = QString("%1%2%3%4%5%6").arg(nDegree).arg(cDegree).arg(QString::number(round(dMinute), 'f', 0)).arg(cMinute).arg(QString::number(round(dSecond), 'f', 4)).arg(cSecond);
		//valStr = QString::asprintf("%d%s%d%s%s%s", nDegree, cDegree, nMinute, cMinute, QString::number(round(dSecond), 'f', 4), cSecond);
		break;
	default:
		break;
	}
	return valStr;
}

std::vector<QString> DmDimensionStyle::getTextStringOfFraction(const DmDimensionStyleData& styleData, const double value)
{
	std::vector<QString> strs;

	//添加整数部分
	double floorVal = floor(value);	//向下取整
	QString integralStr = QString::number(floorVal, 'f', 0);
	strs.emplace_back(integralStr);
	double decimal = value - floorVal;
	int deno = getPrecisionDenominator(styleData);
	if (deno == 0)
	{
		//没有分数
		return strs;
	}

	//添加分数
	double precision = 1.0 / (double)getPrecisionDenominator(styleData);
	double nd = decimal / precision;
	double n = round(nd);
	int ni = static_cast<int>(n);
	QString frac = QString("%1/%2").arg(ni).arg(deno);
	strs.emplace_back(frac);
	return strs;
}

QString DmDimensionStyle::getTrimEndZero(const QString& valueStr)
{
	//不是小数则返回
	const int idx = valueStr.lastIndexOf(".");
	if (idx == -1)
		return valueStr;

	//获得最后非0字符索引
	int endIdx = valueStr.size() - 1;
	for (int i = valueStr.size() - 1; i >= idx; i--)
	{
		if (valueStr.at(i) != "0")
		{
			endIdx = i;
			break;
		}
	}

	//获得去掉结尾0的字符串
	QString res = valueStr.left(endIdx + 1);
	if (res.endsWith("."))	//点在最后面，去掉之
	{
		res = valueStr.left(idx);
	}
	return res;
}

int DmDimensionStyle::getPrecisionDenominator(const DmDimensionStyleData& styleData)
{ 
	int denominator = 0;
	switch (styleData.precision())
	{
	case DmDimensionStyleUnitData::Precision::Bit0:
		denominator = 0;
		break;
	case DmDimensionStyleUnitData::Precision::Bit1:
		denominator = 2;
		break;
	case DmDimensionStyleUnitData::Precision::Bit2:
		denominator = 4;
		break;
	case DmDimensionStyleUnitData::Precision::Bit3:
		denominator = 8;
		break;
	case DmDimensionStyleUnitData::Precision::Bit4:
		denominator = 16;
		break;
	case DmDimensionStyleUnitData::Precision::Bit5:
		denominator = 32;
		break;
	case DmDimensionStyleUnitData::Precision::Bit6:
		denominator = 64;
		break;
	case DmDimensionStyleUnitData::Precision::Bit7:
		denominator = 128;
		break;
	case DmDimensionStyleUnitData::Precision::Bit8:
		denominator = 256;
		break;
	default:
		break;
	}
	return denominator;
}

QString DmDimensionStyle::getDecimalSaparatorStr(const DmDimensionStyleData& styleData)
{
	QString separator(".");
	switch (styleData.decimalSaparator())
	{
	case DmDimensionStyleUnitData::DecimalSaparator::Dot:
		separator = ".";
		break;
	case DmDimensionStyleUnitData::DecimalSaparator::Comma:
		separator = ",";
		break;
	case DmDimensionStyleUnitData::DecimalSaparator::Space:
		separator = " ";
		break;
	default:
		break;
	}
	return separator;
}

void DmDimensionStyle::createTextForStrs(const DmDimensionStyleData& styleData, DmEntityContainer* contaner, const std::vector<QString>& strs)
{
	DmVector intersectionPt(0.0, 0.0);
	const DmVector originIntersectionPt = intersectionPt;
	double letterSpace = styleData.textHeight() * 0.1;
	DmPen textLinePen(styleData.textColor(), DM::Width00, DmLineTypeTable::Continuous);
	for (auto& str : strs)
	{
		bool normalDraw = true;	//“非分数”及“非堆叠分数”按一般文字处理
		if (str.contains("/"))
		{
			//分数
			int idx = str.indexOf("/");
			QString num1 = str.left(idx);
			QString num2 = str.right(str.length() - idx - 1);
			if (styleData.fractionFormat() == DmDimensionStyleUnitData::FractionFormat::Diagonal)
			{
				//对角
				//分子
				double textHeight = styleData.fractionHeightScale() * styleData.textHeight();
				TextData textData1(DmVector(intersectionPt.x, textHeight), textHeight, ETextVertMode::kTextBase, ETextHorzMode::kTextLeft, num1, styleData.textStyle(), 0.0);
				DmText* text1 = new DmText(contaner, textData1);
				//text1->setColor(styleData.textData.textColor);
				text1->setLayer(contaner->getLayer());
				text1->setPen(textLinePen);
				text1->update();
				contaner->addEntity(text1);
				intersectionPt.x += text1->getUsedTextWidth();
				//斜线
				DmVector p1(intersectionPt.x - 0.25 * textHeight, intersectionPt.y);
				DmVector p2(intersectionPt.x + 0.25 * textHeight, textHeight * 2.0);
				DmLine* line = new DmLine(contaner, p1, p2);
				line->setLayer(contaner->getLayer());
				line->setPen(textLinePen);
				contaner->addEntity(line);
				intersectionPt.x += 0.25 * textHeight;
				//分母
				TextData textData2(DmVector(intersectionPt.x, intersectionPt.y), textHeight, ETextVertMode::kTextBase, ETextHorzMode::kTextLeft, num2, styleData.textStyle(), 0.0);
				DmText* text2 = new DmText(contaner, textData2);
				text2->setLayer(contaner->getLayer());
				text2->setPen(textLinePen);
				//text2->setColor(styleData.textData.textColor);
				text2->update();
				contaner->addEntity(text2);
				intersectionPt.x += text2->getUsedTextWidth() + letterSpace;
				normalDraw = false;
			}
			else if (styleData.fractionFormat() == DmDimensionStyleUnitData::FractionFormat::Horizontal)
			{
				//水平
				//分母
				double textHeight = styleData.fractionHeightScale() * styleData.textHeight();
				TextData textData2(DmVector(intersectionPt.x, intersectionPt.y), textHeight, ETextVertMode::kTextBase, ETextHorzMode::kTextLeft, num2, styleData.textStyle(), 0.0);
				DmText* text2 = new DmText(contaner, textData2);
				text2->setLayer(contaner->getLayer());
				text2->setPen(textLinePen);
				//text2->setColor(styleData.textData.textColor);
				text2->update();
				contaner->addEntity(text2);
				//斜线
				DmVector p1(intersectionPt.x, intersectionPt.y + textHeight * 1.1);
				DmVector p2(intersectionPt.x + text2->getUsedTextWidth(), intersectionPt.y + textHeight * 1.1);
				DmLine* line = new DmLine(contaner, p1, p2);
				line->setLayer(contaner->getLayer());
				line->setPen(textLinePen);
				contaner->addEntity(line);
				//分子
				TextData textData1(DmVector(intersectionPt.x + text2->getUsedTextWidth()/2.0, textHeight * 1.2), textHeight, ETextVertMode::kTextBase, ETextHorzMode::kTextCenter, num1, styleData.textStyle(), 0.0);
				DmText* text1 = new DmText(contaner, textData1);
				text1->setLayer(contaner->getLayer());
				text1->setPen(textLinePen);
				//text1->setColor(styleData.textData.textColor);
				text1->update();
				contaner->addEntity(text1);
				intersectionPt.x += text2->getUsedTextWidth() + letterSpace;
				normalDraw = false;
			}
			else
			{
				//非堆叠按正常处理（补些空白）
				intersectionPt.x += letterSpace * 5.0;
			}
		}
		if(normalDraw)
		{
			TextData textData(intersectionPt, styleData.textHeight(), ETextVertMode::kTextBase, ETextHorzMode::kTextLeft, str, styleData.textStyle(), 0.0);
			DmText* text = new DmText(contaner, textData);
			text->setLayer(contaner->getLayer());
			text->setPen(textLinePen);
			//text->setColor(styleData.textData.textColor);
			text->update();
			contaner->addEntity(text);
			intersectionPt.x += text->getUsedTextWidth() + letterSpace;
		}
	}

	//按需要添加包围框
	if (styleData.isDrawTextBoundary())
	{
		double offset = 0.1 * styleData.textHeight();
		contaner->move(DmVector(0.0, offset));
		DmVector lb(originIntersectionPt + DmVector(-offset,-offset));
		DmVector rb(originIntersectionPt + DmVector(contaner->getMax().x + offset , -offset));
		DmVector rt(originIntersectionPt + contaner->getMax()+ DmVector(offset,offset));
		DmVector lt(originIntersectionPt + DmVector(-offset, contaner->getMax().y +  offset));
		DmPolyline* poly = new DmPolyline(contaner, PolylineData());
		poly->setPen(textLinePen);
		auto pts = { lb,rb,rt,lt };
		for (auto& pt : pts)
		{
			poly->appendVertex(pt);
		}
		poly->setClosed(true);
		poly->update();
		contaner->addEntity(poly);
	}
	contaner->calculateBorders();

	//将文字整体移动，保证最低位置为0
	double offsetY = -contaner->getMin().y;
	contaner->move(DmVector(0.0, offsetY));
}

bool DmDimensionStyle::canArrowMirror(DM::ArrowType arrowType)
{
	if (arrowType == DM::ArrowType::Integral || arrowType == DM::ArrowType::ArchitecturalTick || arrowType == DM::ArrowType::Oblique)
	{
		return false;
	}
	else
	{
		return true;
	}
}

double DmDimensionStyle::getArrowCutDistance(DM::ArrowType arrowType)
{
	double cutDist = 0.0;
	switch (arrowType)
	{
	case DM::ArrowType::ClosedFilled:
		cutDist = 0.0;
		break;
	case DM::ArrowType::ClosedBlank:
		cutDist = 1.0;
		break;
	case DM::ArrowType::Closed:
		cutDist = 0.0;
		break;
	case DM::ArrowType::Dot:
		cutDist = 0.0;
		break;
	case DM::ArrowType::ArchitecturalTick:
		cutDist = 0.0;
		break;
	case DM::ArrowType::Oblique:
		cutDist = 0.0;
		break;
	case DM::ArrowType::Open:
		cutDist = 0.0;
		break;
	case DM::ArrowType::OriginIndicator:
		cutDist = 0.0;
		break;
	case DM::ArrowType::OriginIndicator2:
		cutDist = 0.5;
		break;
	case DM::ArrowType::RightAngle:
		cutDist = 0.0;
		break;
	case DM::ArrowType::Open30:
		cutDist = 0.0;
		break;
	case DM::ArrowType::DotSmall:
		cutDist = 0.0;
		break;
	case DM::ArrowType::DotBlank:
		cutDist = 0.5;
		break;
	case DM::ArrowType::DotSmallBlank:
		cutDist = 0.0;
		break;
	case DM::ArrowType::Box:
		cutDist = 0.5;
		break;
	case DM::ArrowType::BoxFilled:
		cutDist = 0.0;
		break;
	case DM::ArrowType::DatumTriangle:
		cutDist = 1.0;
		break;
	case DM::ArrowType::DatumTriangleFilled:
		cutDist = 0.0;
		break;
	case DM::ArrowType::Integral:
		cutDist = 0.0;
		break;
	case DM::ArrowType::None:
		cutDist = 0.0;
		break;
	case DM::ArrowType::UserArrow:
		cutDist = 0.0;
		break;
	default:
		break;
	}
	return cutDist;
}
