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


/// @file DimVars.h
/// @brief 标注样式数据结构定义，包含线/箭头/文字/单位等子数据

#ifndef DIMVARS_H
#define DIMVARS_H

#include "DmColor.h"
#include "Datamodel.h"
#include <QString>
#include "DmLineTypeTable.h"

class DmTextStyle;
class DmDimensionStyle;
//线
class DmDimensionStyleLineData
{
public:
	DmColor dimLineColor{ DM::FlagByLayer };						//尺寸线颜色
	DM::LineWidth dimLineWidth{ DM::WidthByLayer };			//尺寸线线宽
	DmLineType* dimLineType{ DmLineTypeTable::ByLayer };				//尺寸线线型
	bool hideDimLine1{ false };								//隐藏尺寸线1
	bool hideDimLine2{ false };								//隐藏尺寸线2

	DmColor boundLineColor{ DM::FlagByLayer };					//尺寸界线颜色
	DM::LineWidth boundLineWidth{ DM::WidthByLayer };	    //尺寸界线线宽
	DmLineType* boundLineType{ DmLineTypeTable::ByLayer };			//尺寸界线线型
	bool hideBoundLine1{ false };							//隐藏尺寸界线1
	bool hideBoundLine2{ false };							//隐藏尺寸界线2

	double extendDimLine{ 1.25 };						//超出尺寸线（长度）
	double startPtOffset{ 0.625 };							//起点偏移量
	bool isFixedBoundLineLength{ false };			//尺寸界线是否固定长度
	double fixedBoundLineLength{ 1.0 };			//尺寸界线固定长度值
};

//箭头
class DmDimensionStyleArrowData
{
public:
	DM::ArrowType firstArrow{ DM::ArrowType::ClosedFilled };				//第一个（箭头）
	DM::ArrowType secondArrow{ DM::ArrowType::ClosedFilled };			//第二个（箭头）
	DM::ArrowType leaderArrow{ DM::ArrowType::ClosedFilled };			//引线（箭头）
	double arrowSize{ 2.5 };								//箭头大小
};

//文字
class DmDimensionStyleTextData
{
public:
	DmTextStyle* pTextStyle{ nullptr };					//文字样式
	DmColor textColor{ DM::FlagByLayer };								//文字颜色
	DmColor textFillColor{ DM::FlagByLayer };							//填充颜色(todo: CAD默认是无)
	double textHeight{ 2.5 };								//文字高度
	double fractionHeightScale{ 1.0 };				//分数高度比例
	bool isDrawTextBoundary{ false };					//是否绘制文字边框

	//文字位置（垂直）
	enum class TextVerticalPos
	{
		Mid = 0,													//居中
		Up,													//上
		Extern,												//外部
		JIS,													//JIS
		Down												//下
	} textVerticalPos{ TextVerticalPos::Up };

	//文字位置（水平）
	enum class TextHorizontalPos
	{
		Mid,													//居中
		FirstBoundaryLine,							//第一条尺寸界线
		SecondBoundaryLine,						//第二条尺寸界线
		AboveFirstBoundaryLine,				//第一条尺寸界线上方
		AboveSecondBoundaryLine,			//第二条尺寸界线上方
	} textHorizontalPos{ TextHorizontalPos::Mid };

	//观察方向
	enum class ViewDirection
	{
		LeftToRight,										//从左到右
		RightToLeft,										//从右到左
	} viewDirection{ ViewDirection::LeftToRight };

	double offsetFromDimLine{ 0.625 };				//从尺寸线偏移
};

//单位
class DmDimensionStyleUnitData
{
public:
	//单位格式
	enum class UnitFormat
	{
		Science = 0,											//科学
		Decimal,											//小数
		Engineer,											//工程
		Architectural,									//建筑
		Fraction,											//分数
		Windows,											//Windows桌面
	} unitFormat{ UnitFormat::Decimal };

	//精度
	enum class Precision
	{
		Bit0,													//0
		Bit1,													//0.0
		Bit2,													//0.00
		Bit3,													//0.000
		Bit4,													//0.0000
		Bit5,													//0.00000
		Bit6,													//0.000000
		Bit7,													//0.0000000
		Bit8,													//0.00000000
	} precision{ Precision::Bit2 };

	//分数格式
	enum class FractionFormat
	{
		Horizontal,										//水平
		Diagonal,											//对角
		NoStack,											//非堆叠
	} fractionFormat{ FractionFormat::Horizontal };

	//小数分隔符
	enum class DecimalSaparator
	{
		Dot,													//句点
		Comma,											//逗点
		Space												//空格
	} decimalSaparator{ DecimalSaparator::Comma };

	double roundOff{ 0.0 };								//舍入
	QString prefix{ "" };										//前缀
	QString postfix{ "" };									//后缀
	double mesureUnitFactor{ 1.0 };					//测量单位比例
	bool resetPrefix{ false };									//消零前导
	bool resetPostfix{ true };									//消零后续

	//角度标注单位格式
	enum class AngleUnitFormat
	{
		DecimalDegree,								//十进制度数
		DMS,												//度分秒（Degrees Minutes Seconds）
		Gradians,											//百分度
		Radians											//弧度
	} angleUnitFormat{ AngleUnitFormat::DecimalDegree };

	Precision anglePrecision{ Precision::Bit0 };						//角度标注精度
	bool resetAnglePrefix{ false };							//角度标注消零前导
	bool resetAnglePostfix{ false };						//角度标注消零后续
};

// TODO: 宏定义代码生成器，如需改为constexpr函数需较大重构
#define DIM_MEMERS_DECLARE() \
DmDimensionStyleLineData lineData; \
DmDimensionStyleArrowData arrowData; \
DmDimensionStyleTextData textData; \
DmDimensionStyleUnitData unitData; 

#define DIM_FUNCS_DECLARE() \
virtual DmDimensionStyleLineData& lineDataRef(); \
virtual const DmDimensionStyleLineData& lineDataConstRef() const; \
virtual void setLineData(const DmDimensionStyleLineData& lineData); \
virtual DmDimensionStyleArrowData& arrowDataRef(); \
virtual const DmDimensionStyleArrowData& arrowDataConstRef() const; \
virtual void setArrowData(const DmDimensionStyleArrowData& arrowData); \
virtual DmDimensionStyleTextData& textDataRef(); \
virtual const DmDimensionStyleTextData& textDataConstRef() const; \
virtual void setTextData(const DmDimensionStyleTextData& textData); \
virtual DmDimensionStyleUnitData& unitDataRef(); \
virtual const DmDimensionStyleUnitData& unitDataConstRef() const; \
virtual void setUnitData(const DmDimensionStyleUnitData& unitData); \
/*线*/ \
/*dimclrd*/ \
virtual DmColor dimLineColor() const; \
virtual void setDimLineColor(const DmColor& c); \
/*dimlwd*/ \
virtual DM::LineWidth dimLineWidth() const; \
virtual void setDimLineWidth(const DM::LineWidth& w); \
/*dimltype*/ \
virtual DmLineType* dimLineType() const; \
virtual void setDimLineType(DmLineType* lineType); \
/*dimsd1*/ \
virtual bool hideDimLine1() const; \
virtual void setHideDimLine1(const bool& hd); \
/*dimsd2*/ \
virtual bool hideDimLine2() const; \
virtual void setHideDimLine2(const bool& hd); \
/*dimclre*/ \
virtual DmColor boundLineColor() const; \
virtual void setBoundLineColor(const DmColor& c); \
/*dimlwe*/ \
virtual DM::LineWidth boundLineWidth() const; \
virtual void setBoundLineWidth(const DM::LineWidth& w); \
/*dimltex1 及 dimltex2*/ \
virtual DmLineType* boundLineType() const; \
virtual void setBoundLineType(DmLineType* lineType); \
/*dimse1*/ \
virtual bool hideBoundLine1() const; \
virtual void setHideBoundLine1(const bool& hd); \
/*dimse2*/ \
virtual bool hideBoundLine2() const; \
virtual void setHideBoundLine2(const bool& hd); \
/*dimexe*/ \
virtual double extendDimLine() const; \
virtual void setExtendDimLine(const double& ext); \
/*dimexo*/ \
virtual double startPtOffset() const; \
virtual void setStartPtOffset(const double& offset); \
/*dimfxlon*/ \
virtual bool isFixedBoundLineLength() const; \
virtual void setIsFixedBoundLineLength(const bool& fixed); \
/*dimfxl*/ \
virtual double fixedBoundLineLength() const; \
virtual void setFixedBoundLineLength(const double& length); \
/*箭头*/ \
/*dimblk1*/ \
virtual DM::ArrowType firstArrow() const; \
virtual void setFirstArrow(const DM::ArrowType& arrow); \
/*dimblk2*/ \
virtual DM::ArrowType secondArrow() const; \
virtual void setSecondArrow(const DM::ArrowType& arrow); \
/*dimldrblk*/ \
virtual DM::ArrowType leaderArrow() const; \
virtual void setLeaderArrow(const DM::ArrowType& arrow); \
/*dimasz*/ \
virtual double arrowSize() const; \
virtual void setArrowSize(const double& size); \
/*文字*/ \
virtual DmTextStyle* textStyle() const; \
virtual void setTextStyle(DmTextStyle* style); \
/*dimclrt*/ \
virtual DmColor textColor() const; \
virtual void setTextColor(const DmColor& c); \
/*dimtfillclr*/ \
virtual DmColor textFillColor() const; \
virtual void setTextFillColor(const DmColor& c); \
/*dimtxt*/ \
virtual double textHeight() const; \
virtual void setTextHeight(const double& height); \
/*dimtfac*/ \
virtual double fractionHeightScale() const; \
virtual void setFractionHeightScale(const double& scale); \
/*dimgap*/ \
virtual bool isDrawTextBoundary() const; \
virtual void setIsDrawTextBoundary(const bool& drawTextBoundary); \
/*dimtad*/ \
virtual DmDimensionStyleTextData::TextVerticalPos textVerticalPos() const; \
virtual void setTextVerticalPos(const DmDimensionStyleTextData::TextVerticalPos& vPos); \
/*dimjust*/ \
virtual DmDimensionStyleTextData::TextHorizontalPos textHorizontalPos() const; \
virtual void setTextHorizontalPos(const DmDimensionStyleTextData::TextHorizontalPos& hPos); \
/*dimtxtdirection*/ \
virtual DmDimensionStyleTextData::ViewDirection viewDirection() const; \
virtual void setViewDirection(const DmDimensionStyleTextData::ViewDirection& dir); \
/*dimgap*/ \
virtual double offsetFromDimLine() const; \
virtual void setOffsetFromDimLine(const double& offset); \
/*单位*/ \
/*dimlunit*/ \
virtual DmDimensionStyleUnitData::UnitFormat unitFormat() const; \
virtual void setUnitFormat(const DmDimensionStyleUnitData::UnitFormat& unitFormat); \
/*dimdec*/ \
virtual DmDimensionStyleUnitData::Precision precision() const; \
virtual void setPrecision(const DmDimensionStyleUnitData::Precision& precision); \
/*dimfrac*/ \
virtual DmDimensionStyleUnitData::FractionFormat fractionFormat() const; \
virtual void setFractionFormat(const DmDimensionStyleUnitData::FractionFormat& format); \
/*dimdsep*/ \
virtual DmDimensionStyleUnitData::DecimalSaparator decimalSaparator() const; \
virtual void setDecimalSaparator(const DmDimensionStyleUnitData::DecimalSaparator& saparator); \
/*dimrnd*/ \
virtual double roundOff() const; \
virtual void setRoundOff(const double& roundOff); \
/*dimpost*/ \
virtual QString prefix() const; \
virtual void setPrefix(const QString& prefix); \
/*dimpost*/ \
virtual QString postfix() const; \
virtual void setPostfix(const QString& postfix); \
/*dimlfac*/ \
virtual double mesureUnitFactor() const; \
virtual void setMesureUnitFactor(const double& factor); \
/*关联dimzin*/ \
virtual bool resetPrefix() const; \
virtual void setResetPrefix(const bool& reset); \
/*关联dimzin*/ \
virtual bool resetPostfix() const; \
virtual void setResetPostfix(const bool& reset); \
/*dimaunit*/ \
virtual DmDimensionStyleUnitData::AngleUnitFormat angleUnitFormat() const; \
virtual void setAngleUnitFormat(const DmDimensionStyleUnitData::AngleUnitFormat& format); \
/*dimadec*/ \
virtual DmDimensionStyleUnitData::Precision anglePrecision() const; \
virtual void setAnglePrecision(const DmDimensionStyleUnitData::Precision& precision); \
/*关联dimazin*/ \
virtual bool resetAnglePrefix() const; \
virtual void setResetAnglePrefix(const bool& reset); \
/*关联dimazin*/ \
virtual bool resetAnglePostfix() const; \
virtual void setResetAnglePostfix(const bool& reset);


#define DIM_FUNCS_IMPLEMENT(host_class) \
DmDimensionStyleLineData& host_class::lineDataRef() \
{ \
	return lineData; \
} \
const DmDimensionStyleLineData& host_class::lineDataConstRef() const \
{ \
	return lineData; \
} \
void host_class::setLineData(const DmDimensionStyleLineData& lineData) \
{ \
	this->lineData = lineData; \
} \
DmDimensionStyleArrowData& host_class::arrowDataRef() \
{ \
	return arrowData; \
} \
const DmDimensionStyleArrowData& host_class::arrowDataConstRef() const \
{ \
	return arrowData; \
} \
void host_class::setArrowData(const DmDimensionStyleArrowData& arrowData) \
{ \
	this->arrowData = arrowData; \
} \
DmDimensionStyleTextData& host_class::textDataRef() \
{ \
	return textData; \
} \
const DmDimensionStyleTextData& host_class::textDataConstRef() const \
{ \
	return textData; \
} \
void host_class::setTextData(const DmDimensionStyleTextData& textData) \
{ \
	this->textData = textData; \
} \
DmDimensionStyleUnitData& host_class::unitDataRef() \
{ \
	return unitData; \
} \
const DmDimensionStyleUnitData& host_class::unitDataConstRef() const \
{ \
	return unitData; \
} \
void host_class::setUnitData(const DmDimensionStyleUnitData& unitData) \
{ \
	this->unitData = unitData; \
} \
DmColor host_class::dimLineColor() const \
{ \
	return lineData.dimLineColor; \
} \
void host_class::setDimLineColor(const DmColor& c) \
{ \
	lineData.dimLineColor = c; \
} \
DM::LineWidth host_class::dimLineWidth() const \
{ \
	return lineData.dimLineWidth; \
} \
void host_class::setDimLineWidth(const DM::LineWidth& w) \
{ \
	lineData.dimLineWidth = w; \
} \
DmLineType* host_class::dimLineType() const \
{ \
	return lineData.dimLineType; \
} \
void host_class::setDimLineType(DmLineType* lineType ) \
{ \
	lineData.dimLineType = lineType; \
} \
bool host_class::hideDimLine1() const \
{ \
	return lineData.hideDimLine1; \
} \
void host_class::setHideDimLine1(const bool& hd) \
{ \
	lineData.hideDimLine1 = hd; \
} \
bool host_class::hideDimLine2() const \
{ \
	return lineData.hideDimLine2; \
} \
void host_class::setHideDimLine2(const bool& hd) \
{ \
	lineData.hideDimLine2 = hd; \
} \
DmColor host_class::boundLineColor() const \
{ \
	return lineData.boundLineColor; \
} \
void host_class::setBoundLineColor(const DmColor& c) \
{ \
	lineData.boundLineColor = c; \
} \
DM::LineWidth host_class::boundLineWidth() const \
{ \
	return lineData.boundLineWidth; \
} \
void host_class::setBoundLineWidth(const DM::LineWidth& w) \
{ \
	lineData.boundLineWidth = w; \
} \
DmLineType* host_class::boundLineType() const \
{ \
	return lineData.boundLineType; \
} \
void host_class::setBoundLineType(DmLineType* lineType) \
{ \
	lineData.boundLineType = lineType; \
} \
bool host_class::hideBoundLine1() const \
{ \
	return lineData.hideBoundLine1; \
} \
void host_class::setHideBoundLine1(const bool& hd) \
{ \
	lineData.hideBoundLine1 = hd; \
} \
bool host_class::hideBoundLine2() const \
{ \
	return lineData.hideBoundLine2; \
} \
void host_class::setHideBoundLine2(const bool& hd) \
{ \
	lineData.hideBoundLine2 = hd; \
} \
double host_class::extendDimLine() const \
{ \
	return lineData.extendDimLine; \
} \
void host_class::setExtendDimLine(const double& ext) \
{ \
	lineData.extendDimLine = ext; \
} \
double host_class::startPtOffset() const \
{ \
	return lineData.startPtOffset; \
} \
void host_class::setStartPtOffset(const double& offset) \
{ \
	lineData.startPtOffset = offset; \
} \
bool host_class::isFixedBoundLineLength() const \
{ \
	return lineData.isFixedBoundLineLength; \
} \
void host_class::setIsFixedBoundLineLength(const bool& fixed) \
{ \
	lineData.isFixedBoundLineLength = fixed; \
} \
double host_class::fixedBoundLineLength() const \
{ \
	return lineData.fixedBoundLineLength; \
} \
void host_class::setFixedBoundLineLength(const double& length) \
{ \
	lineData.fixedBoundLineLength = length; \
} \
DM::ArrowType host_class::firstArrow() const \
{ \
	return arrowData.firstArrow; \
} \
void host_class::setFirstArrow(const DM::ArrowType& arrow) \
{ \
	arrowData.firstArrow = arrow; \
} \
DM::ArrowType host_class::secondArrow() const \
{ \
	return arrowData.secondArrow; \
} \
void host_class::setSecondArrow(const DM::ArrowType& arrow) \
{ \
	arrowData.secondArrow = arrow; \
} \
DM::ArrowType host_class::leaderArrow() const \
{ \
	return arrowData.leaderArrow; \
} \
void host_class::setLeaderArrow(const DM::ArrowType& arrow) \
{ \
	arrowData.leaderArrow = arrow; \
} \
double host_class::arrowSize() const \
{ \
	return arrowData.arrowSize; \
} \
void host_class::setArrowSize(const double& size) \
{ \
	arrowData.arrowSize = size; \
} \
DmTextStyle* host_class::textStyle() const\
{ \
	return textData.pTextStyle; \
} \
void host_class::setTextStyle(DmTextStyle* style) \
{ \
	textData.pTextStyle = style; \
} \
DmColor host_class::textColor() const \
{ \
	return textData.textColor; \
} \
void host_class::setTextColor(const DmColor& c) \
{ \
	textData.textColor = c; \
} \
DmColor host_class::textFillColor() const \
{ \
	return textData.textFillColor; \
} \
void host_class::setTextFillColor(const DmColor& c) \
{ \
	textData.textFillColor = c; \
} \
double host_class::textHeight() const \
{ \
	return textData.textHeight; \
} \
void host_class::setTextHeight(const double& height) \
{ \
	textData.textHeight = height; \
} \
double host_class::fractionHeightScale() const \
{ \
	return textData.fractionHeightScale; \
} \
void host_class::setFractionHeightScale(const double& scale) \
{ \
	textData.fractionHeightScale = scale; \
} \
bool host_class::isDrawTextBoundary() const \
{ \
	return textData.isDrawTextBoundary; \
} \
void host_class::setIsDrawTextBoundary(const bool& drawTextBoundary) \
{ \
	textData.isDrawTextBoundary = drawTextBoundary; \
} \
DmDimensionStyleTextData::TextVerticalPos host_class::textVerticalPos() const \
{ \
	return textData.textVerticalPos; \
} \
void host_class::setTextVerticalPos(const DmDimensionStyleTextData::TextVerticalPos& vPos) \
{ \
	textData.textVerticalPos = vPos; \
} \
DmDimensionStyleTextData::TextHorizontalPos host_class::textHorizontalPos() const \
{ \
	return textData.textHorizontalPos; \
} \
void host_class::setTextHorizontalPos(const DmDimensionStyleTextData::TextHorizontalPos& hPos) \
{ \
	textData.textHorizontalPos = hPos; \
} \
DmDimensionStyleTextData::ViewDirection host_class::viewDirection() const \
{ \
	return textData.viewDirection; \
} \
void host_class::setViewDirection(const DmDimensionStyleTextData::ViewDirection& dir) \
{ \
	textData.viewDirection = dir; \
} \
double host_class::offsetFromDimLine() const \
{ \
	return textData.offsetFromDimLine; \
} \
void host_class::setOffsetFromDimLine(const double& offset) \
{ \
	textData.offsetFromDimLine = offset; \
} \
DmDimensionStyleUnitData::UnitFormat host_class::unitFormat() const \
{ \
	return unitData.unitFormat; \
} \
void host_class::setUnitFormat(const DmDimensionStyleUnitData::UnitFormat& unitFormat) \
{ \
	unitData.unitFormat = unitFormat; \
} \
DmDimensionStyleUnitData::Precision host_class::precision() const \
{ \
	return unitData.precision; \
} \
void host_class::setPrecision(const DmDimensionStyleUnitData::Precision& precision) \
{ \
	unitData.precision = precision; \
} \
DmDimensionStyleUnitData::FractionFormat host_class::fractionFormat() const \
{ \
	return unitData.fractionFormat; \
} \
void host_class::setFractionFormat(const DmDimensionStyleUnitData::FractionFormat& format) \
{ \
	unitData.fractionFormat = format; \
} \
DmDimensionStyleUnitData::DecimalSaparator host_class::decimalSaparator() const \
{ \
	return unitData.decimalSaparator; \
} \
void host_class::setDecimalSaparator(const DmDimensionStyleUnitData::DecimalSaparator& saparator) \
{ \
	unitData.decimalSaparator = saparator; \
} \
double host_class::roundOff() const \
{ \
	return unitData.roundOff; \
} \
void host_class::setRoundOff(const double& roundOff) \
{ \
	unitData.roundOff = roundOff; \
} \
QString host_class::prefix() const \
{ \
	return unitData.prefix; \
} \
void host_class::setPrefix(const QString& prefix) \
{ \
	unitData.prefix = prefix; \
} \
QString host_class::postfix() const \
{ \
	return unitData.postfix; \
} \
void host_class::setPostfix(const QString& postfix) \
{ \
	unitData.postfix = postfix; \
} \
double host_class::mesureUnitFactor() const \
{ \
	return unitData.mesureUnitFactor; \
} \
void host_class::setMesureUnitFactor(const double& factor) \
{ \
	unitData.mesureUnitFactor = factor; \
} \
bool host_class::resetPrefix() const \
{ \
	return unitData.resetPrefix; \
} \
void host_class::setResetPrefix(const bool& reset) \
{ \
	unitData.resetPrefix = reset; \
} \
bool host_class::resetPostfix() const \
{ \
	return unitData.resetPostfix; \
} \
void host_class::setResetPostfix(const bool& reset) \
{ \
	unitData.resetPostfix = reset; \
} \
DmDimensionStyleUnitData::AngleUnitFormat host_class::angleUnitFormat() const \
{ \
	return unitData.angleUnitFormat; \
} \
void host_class::setAngleUnitFormat(const DmDimensionStyleUnitData::AngleUnitFormat& format) \
{ \
	unitData.angleUnitFormat = format; \
} \
DmDimensionStyleUnitData::Precision host_class::anglePrecision() const \
{ \
	return unitData.anglePrecision; \
} \
void host_class::setAnglePrecision(const DmDimensionStyleUnitData::Precision& precision) \
{ \
	unitData.anglePrecision = precision; \
} \
bool host_class::resetAnglePrefix() const \
{ \
	return unitData.resetAnglePrefix; \
} \
void host_class::setResetAnglePrefix(const bool& reset) \
{ \
	unitData.resetAnglePrefix = reset; \
} \
bool host_class::resetAnglePostfix() const \
{ \
	return unitData.resetAnglePostfix; \
} \
void host_class::setResetAnglePostfix(const bool& reset) \
{ \
	unitData.resetAnglePostfix = reset; \
} 

#endif // !DIMVARS_H
