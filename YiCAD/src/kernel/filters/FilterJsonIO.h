/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file FilterJsonIO.h
/// @brief JSON文件转换器类头文件

#ifndef FILTERJSONCONVERTER_H
#define FILTERJSONCONVERTER_H

#include "FilterInterface.h"

#include <nlohmann/json.hpp>

#include "DmColor.h"
#include "DmText.h"
#include "DmMText.h"
#include "DmDimension.h"

#include "PointData.h"
#include "LineData.h"

class DmPoint;
class DmLine;
class DmCircle;
class DmArc;
class DmEllipse;
class DmSolid;
class DmPolyline;
class DmSpline;
class DmSplinePoints;
class DmBlockReference;
class DmMText;
class DmMText;
class DmText;
class DmAttributeDefinition;
class DmAttribute;
class DmHatch;
class DmImage;
class DmLeader;
class DmDimRadial;
class DmDimAligned;
class DmDimLinear;
class DmDimDiametric;
class DmDimAngular;
class DmXline;
class DmRay;
class DmEntityContainer;
class DmLineType;

class FilterJsonIO
{
public:
	FilterJsonIO();
	~FilterJsonIO();

	// ================================yicad转json======================================
	nlohmann::json lineTypeToJson(DmLineType* data);
	nlohmann::json layerToJson(DmLayer* data);
	nlohmann::json blockTableRecordToJson(DmBlock* data);
	nlohmann::json textStyleToJson(DmTextStyle* data);
	nlohmann::json dimStyleToJson(DmDimensionStyle* data);
    /// @brief 通过DmDimensionStyleData（或DmDimensionData）设置json的数据
    template<class DmDimData>
    void setJsonDimData(nlohmann::json& json, const DmDimData* dmDimData, const char* JsonGroupName);
    /// @brief 通过json设置DmDimensionStyleData（或DmDimensionData）的数据
    template <class DmDimData>
    void setDmDimData(DmDimData& dd, const nlohmann::json& json, DmTextStyle* txtStyle, const char* JsonGroupName);

	nlohmann::json pointToJson(const DmPoint* data);
	nlohmann::json lineToJson(const DmLine* data);
	nlohmann::json arcToJson(const DmArc* data);
	nlohmann::json circleToJson(const DmCircle* data);
	nlohmann::json ellipseToJson(const DmEllipse* data);
	nlohmann::json rayToJson(const DmRay* data);
	nlohmann::json xLineToJson(const DmXline* data);
	nlohmann::json polylineToJson(const DmPolyline* data);
	nlohmann::json splineToJson(const DmSpline* data);
	nlohmann::json solidToJson(const DmSolid* data);
    nlohmann::json textToJson(const DmText* data);
    nlohmann::json mtextToJson(const DmMText* data);
    nlohmann::json attributeDefinitionToJson(const DmAttributeDefinition* data);
    void fillTextJson(nlohmann::json& json, const DmText* data);
    nlohmann::json attributeToJson(const DmAttribute* data);
    nlohmann::json dimLinearToJson(const DmDimLinear* data);
    nlohmann::json dimAlignedToJson(const DmDimAligned* data);
    nlohmann::json dimAngularToJson(const DmDimAngular* data);
    nlohmann::json dimRadialToJson(const DmDimRadial* data);
    nlohmann::json dimDiametricToJson(const DmDimDiametric* data);
    nlohmann::json dimLeaderToJson(const DmLeader* data);
	nlohmann::json blockReferenceToJson(const DmBlockReference* data);
    nlohmann::json hatchToJson(const DmHatch* data);

	/// @brief yicad实体属性赋给json(转换通用数据)
	nlohmann::json entityAttributesToJson(const DmEntity* entity);
    void dimensionAttributesToJson(nlohmann::json& json, const DmDimension* entity);

	// ================================json转yicad======================================
	DmLineType* jsonToLineType(DmDocument* doc, const nlohmann::json& json);
	DmLayer* jsonToLayer(DmDocument* doc, const nlohmann::json& json);
	DmBlock* jsonToBlockTableRecord(DmDocument* doc, const nlohmann::json& json);
	DmTextStyle* jsonToTextStyle(DmDocument* doc, const nlohmann::json& json);
	DmDimensionStyle* jsonToDimensionStyle(DmDocument* doc, const nlohmann::json& json);

	DmPoint* jsonToPoint(DmDocument* doc, const nlohmann::json& json);
	DmLine* jsonToline(DmDocument* doc, const nlohmann::json& json);
	DmCircle* jsonToCircle(DmDocument* doc, const nlohmann::json& json);
	DmArc* jsonToArc(DmDocument* doc, const nlohmann::json& json);
	DmEllipse* jsonToEllipse(DmDocument* doc, const nlohmann::json& json);
	DmRay* jsonToRay(DmDocument* doc, const nlohmann::json& json);
	DmXline* jsonToXline(DmDocument* doc, const nlohmann::json& json);
	DmPolyline* jsonToPolyline(DmDocument* doc, const nlohmann::json& json);
	DmSpline* jsonToSpline(DmDocument* doc, const nlohmann::json& json);
	DmSolid* jsonToSolid(DmDocument* doc, const nlohmann::json& json);
    DmText* jsonToText(DmDocument* doc, const nlohmann::json& json);
    DmMText* jsonToMText(DmDocument* doc, const nlohmann::json& json);
    DmAttributeDefinition* jsonToAttributeDefinition(DmDocument* doc, const nlohmann::json& json);
    DmAttribute* jsonToAttribute(DmDocument* doc, DmBlockReference* blkReference, const nlohmann::json& json);
    DmDimLinear* jsonToDimLinear(DmDocument* doc, const nlohmann::json& json);
    DmDimAligned* jsonToDimAligned(DmDocument* doc, const nlohmann::json& json);
    DmDimAngular* jsonToDimAngular(DmDocument* doc, const nlohmann::json& json);
    DmDimRadial* jsonToDimRadial(DmDocument* doc, const nlohmann::json& json);
    DmDimDiametric* jsonToDimDiametric(DmDocument* doc, const nlohmann::json& json);
    DmLeader* jsonToDimLeader(DmDocument* doc, const nlohmann::json& json);
	DmBlockReference* jsonToBlockReference(DmDocument* doc, const nlohmann::json& json);
    DmHatch* jsonToHatch(DmDocument* doc, const nlohmann::json& json);

	/// @brief json实体数据赋给yicad(转换通用数据)
	void jsonToEntityAttributes(DmDocument* doc, DmEntity* pEnt, const nlohmann::json& attrib);
    /// @brief 由json获得DmDimensionData
    DmDimensionData jsonToDimensionData(DmDocument* doc, const nlohmann::json& json);
    /// @brief 由json获得TextData
    TextData jsonToTextData(DmDocument* doc, const nlohmann::json& json);
    MTextData jsonToMTextData(DmDocument* doc, const nlohmann::json& json);

	DmLineType* nameToLineType(const QString& name, DmDocument* m_pDocument);
	DM::LineWidth numberToWidth(const int& iLineWeight);
    /// @brief 由字符串获得yicad箭头类型
    static DM::ArrowType stringToArrow(const std::wstring& arrowName);
    /// @brief 由yicad箭头类型获得箭头字符串
    static std::wstring arrowToString(DM::ArrowType arrowType);

	/// @brief 图层json转换成yicad画笔
    DmPen attributesToPen(const nlohmann::json& json, DmDocument* pDocument);

private:
	QString getStrEntityType(DM::EntityType type);
};

template<class DmDimData>
inline void FilterJsonIO::setJsonDimData(nlohmann::json& json, const DmDimData* dmDimData, const char* JsonGroupName)
{
    //线
    DmColor clrd = dmDimData->dimLineColor();
    int clrdIdx, clrdR, clrdG, clrdB;
    DmColor::toCAD(clrd, clrdIdx, clrdR, clrdG, clrdB);
    json[JsonGroupName]["Dimclrd"] = clrdIdx;
    json[JsonGroupName]["Dimclrd_r"] = clrdR;
    json[JsonGroupName]["Dimclrd_g"] = clrdG;
    json[JsonGroupName]["Dimclrd_b"] = clrdB;
    json[JsonGroupName]["Dimlwd"] = (int)dmDimData->dimLineWidth();
    json[JsonGroupName]["Dimsd1"] = dmDimData->hideDimLine1() ? 1 : 0; 
    json[JsonGroupName]["Dimsd2"] = dmDimData->hideDimLine2() ? 1 : 0;
    DmColor clre = dmDimData->boundLineColor();
    int clreIdx, clreR, clreG, clreB;
    DmColor::toCAD(clre, clreIdx, clreR, clreG, clreB);
    json[JsonGroupName]["Dimclre"] = clreIdx;
    json[JsonGroupName]["Dimclre_r"] = clreR;
    json[JsonGroupName]["Dimclre_g"] = clreG;
    json[JsonGroupName]["Dimclre_b"] = clreB;
    json[JsonGroupName]["Dimlwe"] = (int)dmDimData->boundLineWidth();
    json[JsonGroupName]["Dimse1"] = dmDimData->hideBoundLine1() ? 1 : 0;
    json[JsonGroupName]["Dimse2"] = dmDimData->hideBoundLine2() ? 1 : 0;
    json[JsonGroupName]["Dimexe"] = dmDimData->extendDimLine();
    json[JsonGroupName]["Dimexo"] = dmDimData->startPtOffset();
    json[JsonGroupName]["Dimfxlon"] = dmDimData->isFixedBoundLineLength() ? 1 : 0;
    json[JsonGroupName]["Dimfxl"] = dmDimData->fixedBoundLineLength();

    //箭头
    json[JsonGroupName]["Dimblk1"] = arrowToString(dmDimData->firstArrow());
    json[JsonGroupName]["Dimblk2"] = arrowToString(dmDimData->secondArrow());
    json[JsonGroupName]["Dimldrblk"] = arrowToString(dmDimData->leaderArrow());
    json[JsonGroupName]["Dimasz"] = dmDimData->arrowSize();

    //文字
    json[JsonGroupName]["Dimtxsty"] = dmDimData->textStyle()->getName().toStdWString();
    DmColor clrt = dmDimData->textColor();
    int clrtIdx, clrtR, clrtG, clrtB;
    DmColor::toCAD(clrt, clrtIdx, clrtR, clrtG, clrtB);
    json[JsonGroupName]["Dimclrt"] = clrtIdx;
    json[JsonGroupName]["Dimclrt_r"] = clrtR;
    json[JsonGroupName]["Dimclrt_g"] = clrtG;
    json[JsonGroupName]["Dimclrt_b"] = clrtB;
    json[JsonGroupName]["Dimtxt"] = dmDimData->textHeight();
    json[JsonGroupName]["Dimtfac"] = dmDimData->fractionHeightScale();
    json[JsonGroupName]["DrawBox"] = dmDimData->isDrawTextBoundary() ? 1 : 0;
    json[JsonGroupName]["Dimtad"] = static_cast<int>(dmDimData->textVerticalPos());
    json[JsonGroupName]["Dimjust"] = static_cast<int>(dmDimData->textHorizontalPos());
    bool viewDir = dmDimData->viewDirection() == DmDimensionStyleTextData::ViewDirection::LeftToRight ? false : true;
    json[JsonGroupName]["Dimtxtdirection"] = viewDir ? 1 : 0;
    json[JsonGroupName]["Dimgap"] = dmDimData->offsetFromDimLine();

    //单位
    json[JsonGroupName]["Dimlunit"] = static_cast<int>(dmDimData->unitFormat()) + 1; 
    json[JsonGroupName]["Dimdec"] = static_cast<int>(dmDimData->precision());
    json[JsonGroupName]["Dimfrac"] = static_cast<int>(dmDimData->fractionFormat());
    int c = '.';    //句点
    if (dmDimData->decimalSaparator() == DmDimensionStyleUnitData::DecimalSaparator::Comma)//逗点
    {
        c = ',';
    }
    else  if (dmDimData->decimalSaparator() == DmDimensionStyleUnitData::DecimalSaparator::Space) //空格
    {
        c = ' ';
    }
    json[JsonGroupName]["Dimdsep"] = c; 
    json[JsonGroupName]["Dimrnd"] = dmDimData->roundOff();
    json[JsonGroupName]["Dimpost"] = dmDimData->postfix().toStdWString();
    json[JsonGroupName]["Dimprefix"] = dmDimData->prefix().toStdWString();
    json[JsonGroupName]["Dimlfac"] = dmDimData->mesureUnitFactor();
    json[JsonGroupName]["ResetPrefix"] = dmDimData->resetPrefix() ? 1 : 0;
    json[JsonGroupName]["ResetPostfix"] = dmDimData->resetPostfix() ? 1 : 0;
    json[JsonGroupName]["Dimaunit"] = static_cast<int>(dmDimData->angleUnitFormat());
    json[JsonGroupName]["Dimadec"] = static_cast<int>(dmDimData->anglePrecision());
    json[JsonGroupName]["ResetAnglePrefix"] = dmDimData->resetAnglePrefix() ? 1 : 0;
    json[JsonGroupName]["ResetAnglePostfix"] = dmDimData->resetAnglePostfix() ? 1 : 0;
}

template<class DmDimData>
inline void FilterJsonIO::setDmDimData(DmDimData& dd, const nlohmann::json& json, DmTextStyle* txtStyle, const char* JsonGroupName)
{
    //线
    int dimclrd = json[JsonGroupName]["Dimclrd"];
    int dimclrd_r = json[JsonGroupName]["Dimclrd_r"]; 
    int dimclrd_g = json[JsonGroupName]["Dimclrd_g"]; 
    int dimclrd_b = json[JsonGroupName]["Dimclrd_b"]; 
    dd.setDimLineColor(DmColor::fromCAD(dimclrd, dimclrd_r, dimclrd_g, dimclrd_b));
    dd.setDimLineWidth(static_cast<DM::LineWidth>(json[JsonGroupName]["Dimlwd"]));
    dd.setHideDimLine1(json[JsonGroupName]["Dimsd1"] == 1);
    dd.setHideDimLine2(json[JsonGroupName]["Dimsd2"] == 1);
    int dimclre = json[JsonGroupName]["Dimclre"];
    int dimclre_r = json[JsonGroupName]["Dimclre_r"];
    int dimclre_g = json[JsonGroupName]["Dimclre_g"];
    int dimclre_b = json[JsonGroupName]["Dimclre_b"];
    dd.setBoundLineColor(DmColor::fromCAD(dimclre, dimclre_r, dimclre_g, dimclre_b));
    dd.setBoundLineWidth(static_cast<DM::LineWidth>(json[JsonGroupName]["Dimlwe"]));
    dd.setHideBoundLine1(json[JsonGroupName]["Dimse1"] == 1);
    dd.setHideBoundLine2(json[JsonGroupName]["Dimse2"] == 1);
    dd.setStartPtOffset(json[JsonGroupName]["Dimexo"]);
    dd.setExtendDimLine(json[JsonGroupName]["Dimexe"]);
    dd.setIsFixedBoundLineLength(json[JsonGroupName]["Dimfxlon"] == 1);
    dd.setFixedBoundLineLength(json[JsonGroupName]["Dimfxl"]);

    //箭头
    auto strDimblk1 = QString::fromStdWString(json[JsonGroupName]["Dimblk1"]);
    dd.setFirstArrow(stringToArrow(strDimblk1.toStdWString()));
    auto strDimblk2 = QString::fromStdWString(json[JsonGroupName]["Dimblk2"]);
    dd.setSecondArrow(stringToArrow(strDimblk2.toStdWString()));
    auto strDimldrblk = QString::fromStdWString(json[JsonGroupName]["Dimldrblk"]);
    dd.setLeaderArrow(stringToArrow(strDimldrblk.toStdWString()));
    dd.setArrowSize(json[JsonGroupName]["Dimasz"]);
    
    //文字
    dd.setTextStyle(txtStyle);
    int dimclrt = json[JsonGroupName]["Dimclrt"]; 
    int dimclrt_r = json[JsonGroupName]["Dimclrt_r"]; 
    int dimclrt_g = json[JsonGroupName]["Dimclrt_g"]; 
    int dimclrt_b = json[JsonGroupName]["Dimclrt_b"]; 
    dd.setTextColor(DmColor::fromCAD(dimclrt, dimclrt_r, dimclrt_g, dimclrt_b));
    dd.setTextHeight(json[JsonGroupName]["Dimtxt"]);
    dd.setFractionHeightScale(json[JsonGroupName]["Dimtfac"]);
    dd.setIsDrawTextBoundary(json[JsonGroupName]["DrawBox"] == 1);
    dd.setTextVerticalPos(static_cast<DmDimensionStyleTextData::TextVerticalPos>(json[JsonGroupName]["Dimtad"]));
    dd.setTextHorizontalPos(static_cast<DmDimensionStyleTextData::TextHorizontalPos>(json[JsonGroupName]["Dimjust"]));
    bool bViewDir = json[JsonGroupName]["Dimtxtdirection"] == 1;
    dd.setViewDirection(bViewDir == false ? DmDimensionStyleTextData::ViewDirection::LeftToRight : DmDimensionStyleTextData::ViewDirection::RightToLeft);
    dd.setOffsetFromDimLine(json[JsonGroupName]["Dimgap"]);

    //单位
    dd.setUnitFormat(static_cast<DmDimensionStyleUnitData::UnitFormat>((int)json[JsonGroupName]["Dimlunit"] - 1));
    dd.setPrecision(static_cast<DmDimensionStyleUnitData::Precision>(json[JsonGroupName]["Dimdec"]));
    dd.setFractionFormat(static_cast<DmDimensionStyleUnitData::FractionFormat>(json[JsonGroupName]["Dimfrac"]));
    int c = json[JsonGroupName]["Dimdsep"];
    if (c == ',')//逗点
    {
        dd.setDecimalSaparator(DmDimensionStyleUnitData::DecimalSaparator::Comma);
    }
    else  if (c == ' ') //空格
    {
        dd.setDecimalSaparator(DmDimensionStyleUnitData::DecimalSaparator::Space);
    }
    else
    {
        //其他全当句点
        dd.setDecimalSaparator(DmDimensionStyleUnitData::DecimalSaparator::Dot);
    }
    dd.setRoundOff(json[JsonGroupName]["Dimrnd"]);
    dd.setPrefix(QString::fromStdWString(json[JsonGroupName]["Dimprefix"]));
    dd.setPostfix(QString::fromStdWString(json[JsonGroupName]["Dimpost"]));
    dd.setMesureUnitFactor(json[JsonGroupName]["Dimlfac"]);
    dd.setResetPrefix(json[JsonGroupName]["ResetPrefix"] == 1);
    dd.setResetPostfix(json[JsonGroupName]["ResetPostfix"] == 1);
    dd.setAngleUnitFormat(static_cast<DmDimensionStyleUnitData::AngleUnitFormat>(json[JsonGroupName]["Dimaunit"]));
    dd.setAnglePrecision(static_cast<DmDimensionStyleUnitData::Precision>(json[JsonGroupName]["Dimadec"]));
    dd.setResetAnglePrefix(json[JsonGroupName]["ResetAnglePrefix"] == 1);
    dd.setResetAnglePostfix(json[JsonGroupName]["ResetAnglePostfix"] == 1);
}

#endif
