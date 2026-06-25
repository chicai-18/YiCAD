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

/// @file FilterJsonIO.cpp
/// @brief JSON文件转换器类实现

#include "FilterJsonIO.h"

#include "DmArc.h"
#include "DmCircle.h"
#include "DmDimensionStyle.h"
#include "DmDimAligned.h"
#include "DmDimAngular.h"
#include "DmDimDiametric.h"
#include "DmDimLinear.h"
#include "DmDimRadial.h"
#include "DmEllipse.h"
#include "DmHatch.h"
#include "DmImage.h"
#include "DmBlockReference.h"
#include "DmLayer.h"
#include "DmLeader.h"
#include "DmLine.h"
#include "DmMText.h"
#include "DmMText.h"
#include "DmPoint.h"
#include "DmPolyline.h"
#include "DmSolid.h"
#include "DmSpline.h"
#include "DmSystem.h"
#include "DmText.h"
#include "DmAttributeDefinition.h"
#include "DmAttribute.h"
#include "GuiDocumentView.h"
#include "GuiDialogFactory.h"
#include "Math2d.h"
#include "DmDimRadial.h"
#include "DmXline.h"
#include "DmRay.h"
#include "DmFontList.h"
#include "DmFont.h"
#include "DmSettings.h"
#include "DmLineType.h"
#include "DmHatch.h"
#include "DmEntityHelper.h"


FilterJsonIO::FilterJsonIO()
{
}

FilterJsonIO::~FilterJsonIO()
{
}

nlohmann::json FilterJsonIO::lineTypeToJson(DmLineType* data)
{
	nlohmann::json json;
	json["linetype"]["comment"] = data->getLineTypeDesp().toStdWString();
	json["linetype"]["numberDashes"] = data->getLineTypeData().size();
	json["linetype"]["patternLength"] = 0;
	json["linetype"]["bScaleToFit"] = 0;
	json["linetype"]["name"] = data->getLineTypeName().toStdWString();
	json["linetype"]["dash"] = data->getLineTypeData();

	return json;
}

nlohmann::json FilterJsonIO::layerToJson(DmLayer* data)
{
	nlohmann::json json;
	json["layer"]["strName"] = data->getName().toStdWString();
	json["layer"]["strLineType"] = data->getPen().getLineType()->getLineTypeName().toStdWString();
	json["layer"]["Color_r"] = data->getPen().getColor().red();
	json["layer"]["Color_g"] = data->getPen().getColor().green();
	json["layer"]["Color_b"] = data->getPen().getColor().blue();
	if (data->getPen().getColor().isByLayer())
	{
		json["layer"]["ColorIndex"] = 256;
	}
	else if (data->getPen().getColor().isByBlock())
	{
		json["layer"]["ColorIndex"] = 0;
	}
	else
	{
		json["layer"]["ColorIndex"] = -1;
	}
	json["layer"]["isPlot"] = data->isPrint() ? 1 : 0;
	json["layer"]["strPlotstyle"] = "";
	json["layer"]["LineWeight"] = (int)data->getPen().getWidth();
	json["layer"]["isFreeze"] = data->isFrozen() ? 1 : 0;
	json["layer"]["isLock"] = data->isLocked() ? 1 : 0;

	return json;
}

nlohmann::json FilterJsonIO::blockTableRecordToJson(DmBlock* data)
{
	nlohmann::json json;
	json["strBlockName"] = data->getName().toStdWString();
	json["ptOrigin"]["x"] = data->getBasePoint().x;
	json["ptOrigin"]["y"] = data->getBasePoint().y;
	json["ptOrigin"]["z"] = data->getBasePoint().z;
	json["type"] = getStrEntityType(data->getEntityType()).toStdString();

	nlohmann::json jsonEnities;
	for (auto& ent : data->getEntityTable())
	{
		auto type = ent->getEntityType();
		switch (type)
		{
		case DM::EntityContainer:
			break;
		case DM::EntityBlock:
			break;
		case DM::EntityBlockReference:
			jsonEnities.emplace_back(blockReferenceToJson((DmBlockReference*)ent));
			break;
		case DM::EntityPoint:
			jsonEnities.emplace_back(pointToJson((DmPoint*)ent));
			break;
		case DM::EntityLine:
			jsonEnities.emplace_back(lineToJson((DmLine*)ent));
			break;
		case DM::EntityPolyline:
			jsonEnities.emplace_back(polylineToJson((DmPolyline*)ent));
			break;
		case DM::EntityArc:
			jsonEnities.emplace_back(arcToJson((DmArc*)ent));
			break;
		case DM::EntityCircle:
			jsonEnities.emplace_back(circleToJson((DmCircle*)ent));
			break;
		case DM::EntityEllipse:
			jsonEnities.emplace_back(ellipseToJson((DmEllipse*)ent));
			break;
		case DM::EntitySolid:
			jsonEnities.emplace_back(solidToJson((DmSolid*)ent));
			break;
		case DM::EntityMText:
			jsonEnities.emplace_back(mtextToJson((DmMText*)ent));
			break;
		case DM::EntityText:
			jsonEnities.emplace_back(textToJson((DmText*)ent));
			break;
		case DM::EntityDimAligned:
			jsonEnities.emplace_back(dimAlignedToJson((DmDimAligned*)ent));
			break;
		case DM::EntityDimLinear:
			jsonEnities.emplace_back(dimLinearToJson((DmDimLinear*)ent));
			break;
		case DM::EntityDimRadial:
			jsonEnities.emplace_back(dimRadialToJson((DmDimRadial*)ent));
			break;
		case DM::EntityDimDiametric:
			jsonEnities.emplace_back(dimDiametricToJson((DmDimDiametric*)ent));
			break;
		case DM::EntityDimAngular:
			jsonEnities.emplace_back(dimAngularToJson((DmDimAngular*)ent));
			break;
		case DM::EntityDimLeader:
			jsonEnities.emplace_back(dimLeaderToJson((DmLeader*)ent));
			break;
		case DM::EntityHatch:
			jsonEnities.emplace_back(hatchToJson((DmHatch*)ent));
			break;
		case DM::EntityImage:
			break;
		case DM::EntitySpline:
			jsonEnities.emplace_back(splineToJson((DmSpline*)ent));
			break;
		case DM::EntityRay:
			jsonEnities.emplace_back(rayToJson((DmRay*)ent));
			break;
		case DM::EntityXline:
			jsonEnities.emplace_back(xLineToJson((DmXline*)ent));
			break;
		default:
			break;
		}
	}
	json["entities"] = jsonEnities;

	return json;
}

nlohmann::json FilterJsonIO::textStyleToJson(DmTextStyle* style)
{
	nlohmann::json json;
	const DmTextStyleData& data = style->getData();
	json["textStyle"]["strName"] = data.name.toStdWString();
	json["textStyle"]["strBigFontName"] = L"";
	json["textStyle"]["strSysFontFileName"] = L"";
	json["textStyle"]["strSysFontName"] = L"";
	json["textStyle"]["SysFontBold"] = 0;
	json["textStyle"]["SysFontItalic"] = 0;
    if (data.pBigFont != nullptr)
    {
		json["textStyle"]["strBigFontName"] = data.pBigFont->getFileName().toStdWString();
    }

    if (data.pAsciiFont != nullptr)
    {
		// todo : 默认的txt.shx无法写出
		json["textStyle"]["strSysFontFileName"] = data.pAsciiFont->getFileName().toStdWString();
    }
	if (data.pSysFont != nullptr)
	{
		json["textStyle"]["strSysFontFileName"] = data.pSysFont->getFileName().toStdWString();
		json["textStyle"]["strSysFontName"] = data.sysFontFamily.toStdWString();
		json["textStyle"]["SysFontBold"] = data.isSysFontBold ? 1 : 0;
		json["textStyle"]["SysFontItalic"] = data.isSysFontItalic ? 1 : 0;
	}

    //如果有不存在的字体，写入
    if (!data.invalidAsciiFont.isEmpty())
    {
		json["textStyle"]["strSysFontFileName"] = data.invalidAsciiFont.toStdWString();
    }
    if (!data.invalidBigFont.isEmpty())
    {
		json["textStyle"]["strBigFontName"] = data.invalidBigFont.toStdWString();
    }
    if (!data.invalidSysFontFamily.isEmpty())
    {
		json["textStyle"]["strSysFontFileName"] = data.invalidSysFontFamily.toStdWString();
		json["textStyle"]["strSysFontName"] = data.sysFontFamily.toStdWString();
    }
	json["textStyle"]["IsBackward"] = data.isReverseDirection ? 1 : 0;
	json["textStyle"]["IsUpsidedown"] = data.isUpsideDown ? 1 : 0;
	json["textStyle"]["IsVertical"] = data.isVertical ? 1 : 0;
	json["textStyle"]["ObliquingAngle"]  = data.slashAngle;
	json["textStyle"]["TextSize"] = data.defaultHeight;
	json["textStyle"]["WidthFactor"] = data.widhFactor;
	return json;
}

nlohmann::json FilterJsonIO::dimStyleToJson(DmDimensionStyle* data)
{
	const DmDimensionStyleData* dmDimData = &data->getDataConstRef();
	nlohmann::json json;
	json["dimStyle"]["Name"] = dmDimData->name.toStdWString();
	setJsonDimData<DmDimensionStyleData>(json, dmDimData, "dimStyle");
	return json;
}

DmDimensionData FilterJsonIO::jsonToDimensionData(DmDocument* doc, const nlohmann::json& json)
{
	DmVector defP(0, 0); // 父类型获取不到此坐标,在实际调用时赋值
    DmVector midP(json["textPt"]["x"], json["textPt"]["y"]);
    EMTextVertMode valign;
    EMTextHorzMode halign;
    QString sty = QString::fromStdWString(json["dimStyle"]);

    if (fabs(midP.x) < 1.0e-6 && fabs(midP.y) < 1.0e-6)
    {
        midP = DmVector(false);
    }

	int alignMode = json["textAlignMode"];
    if (alignMode <= 3)
    {
        valign = EMTextVertMode::kTextTop;
    }
    else if (alignMode <= 6)
    {
        valign =  EMTextVertMode::kTextVertMid;
    }
    else
    {
        valign =  EMTextVertMode::kTextBottom;
    }

    if (alignMode % 3 == 1)
    {
        halign =  EMTextHorzMode::kTextLeft;
    }
    else if (alignMode % 3 == 2)
    {
        halign =  EMTextHorzMode::kTextCenter;
    }
    else
    {
        halign =  EMTextHorzMode::kTextRight;
    }
    QString t = QString::fromStdWString(json["textString"]);

    QString dimTxsty = QString::fromStdWString(json["dimension"]["Dimtxsty"]);
    DmTextStyle* txtStyle = doc->getTextStyleTable()->find(dimTxsty);
	DmDimensionStyle* pStyle = doc->getDimStyleTable()->find(sty);
    DmDimensionData dd(defP, midP, valign, halign, json["textLineFactor"], t, 0, pStyle);
    // 读取替代属性
    setDmDimData<DmDimensionData>(dd, json, txtStyle, "dimension");
    return dd;
}

TextData FilterJsonIO::jsonToTextData(DmDocument* doc, const nlohmann::json& json)
{
	DmVector refPoint = DmVector(json["position"]["x"], json["position"]["y"]);
    double angle = json["angle"];

    ETextVertMode valign = (ETextVertMode)json["textVertMode"];
    ETextHorzMode halign = (ETextHorzMode)json["textHorzMode"];
    QString sty = QString::fromStdWString(json["style"]);
    QString textStr = QString::fromStdWString(json["textString"]);
    // use default style for the drawing:
    if (sty.isEmpty())
    {
        sty = DEFAULT_TEXTSTYLE_NAME;
    }

    DmTextStyleTable* textStyleTable = doc->getTextStyleTable();

    DmTextStyle* pStyle = textStyleTable->find(sty);
    TextData d(refPoint, json["height"], valign, halign, textStr, pStyle, angle, EUpdateMode::NoUpdate);
    d.setAlignment(DmVector(json["alignment"]["x"], json["alignment"]["y"]));
    d.setUpsideDown(json["isMirrorInY"] == 1);
    d.setReverseDirection(json["isMirrorInX"] == 1);
    d.setWidthFactor(json["widthFactor"]);
    d.setSlashAngle(json["oblique"]);
    return d;
}

MTextData FilterJsonIO::jsonToMTextData(DmDocument* doc, const nlohmann::json& json)
{
	DmVector pos = DmVector(json["position"]["x"], json["position"]["y"]);
	double angle = json["angle"];

	EMTextVertMode valign = (EMTextVertMode)json["textVertMode"];
	EMTextHorzMode halign = (EMTextHorzMode)json["textHorzMode"];
	QString sty = QString::fromStdWString(json["style"]);
	QString content = QString::fromStdWString(json["content"]);
	if (sty.isEmpty())
	{
		sty = DEFAULT_TEXTSTYLE_NAME;
	}
	DmTextStyleTable* textStyleTable = doc->getTextStyleTable();
	DmTextStyle* pStyle = textStyleTable->find(sty);
	MTextData d(pos, json["textHeight"], valign, halign, 1.0, json["width"], content, pStyle, angle);
	d.setDefineHeight(json["height"]);
	d.setLineSpacingFactor(json["lineSpacingFactor"]);
	return d;
}

nlohmann::json FilterJsonIO::pointToJson(const DmPoint* data)
{
	nlohmann::json json = entityAttributesToJson(data);

	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	auto point = data->getData().getPosition();
	json["start"]["x"] = point.x;
	json["start"]["y"] = point.y;
	// todo: 以下数据yicad不支持 只为补齐数据
	json["thickness"] = 0;
	json["normal"]["x"] = 0;
	json["normal"]["y"] = 0;
	json["normal"]["z"] = 1;
	return json;
}

nlohmann::json FilterJsonIO::lineToJson(const DmLine* data)
{
	nlohmann::json json = entityAttributesToJson(data);

	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	auto start = data->getData().getStartPoint();
	auto end = data->getData().getEndPoint();
	json["start"]["x"] = start.x;
	json["start"]["y"] = start.y;
	json["end"]["x"] = end.x;
	json["end"]["y"] = end.y;
	return json;
}

nlohmann::json FilterJsonIO::arcToJson(const DmArc* data)
{
	nlohmann::json json = entityAttributesToJson(data);

	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	json["center"]["x"] = data->getCenter().x;
	json["center"]["y"] = data->getCenter().y;
	json["center"]["z"] = data->getCenter().z;
	json["StartAngle"] = data->getStartAngle();
	json["EndAngle"] = data->getEndAngle();
	json["radius"] = data->getRadius();
	json["Normal"]["x"] = data->getNormal().x;
	json["Normal"]["y"] = data->getNormal().y;
	json["Normal"]["z"] = data->getNormal().z;

	return json;
}

nlohmann::json FilterJsonIO::circleToJson(const DmCircle* data)
{
	nlohmann::json json = entityAttributesToJson(data);

	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	json["center"]["x"] = data->getCenter().x;
	json["center"]["y"] = data->getCenter().y;
	json["center"]["z"] = data->getCenter().z;
	json["radius"] = data->getRadius();

	return json;
}

nlohmann::json FilterJsonIO::ellipseToJson(const DmEllipse* data)
{
	nlohmann::json json = entityAttributesToJson(data);

	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	json["center"]["x"] = data->getCenter().x;
	json["center"]["y"] = data->getCenter().y;
	json["center"]["z"] = data->getCenter().z;
	json["MajorP"]["x"] = data->getMajorP().x;
	json["MajorP"]["y"] = data->getMajorP().y;
	json["MajorP"]["z"] = data->getMajorP().z;
	json["Ratio"] = data->getRatio();
	json["Closed"] = data->isClosed() ? 1 : 0;
	json["Normal"]["x"] = data->getNormal().x;
	json["Normal"]["y"] = data->getNormal().y;
	json["Normal"]["z"] = data->getNormal().z;
	json["StartParam"] = data->getStartParam();
	json["EndParam"] = data->getEndParam();
	return json;
}

nlohmann::json FilterJsonIO::rayToJson(const DmRay* data)
{
	nlohmann::json json = entityAttributesToJson(data);

	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	json["BasePoint"]["x"] = data->getData().getBasePoint().x;
	json["BasePoint"]["y"] = data->getData().getBasePoint().y;
	json["BasePoint"]["z"] = data->getData().getBasePoint().z;
	json["Direction"]["x"] = data->getData().getDirection().x;
	json["Direction"]["y"] = data->getData().getDirection().y;
	json["Direction"]["z"] = data->getData().getDirection().z;

	return json;
}

nlohmann::json FilterJsonIO::xLineToJson(const DmXline* data)
{
	nlohmann::json json = entityAttributesToJson(data);

	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	json["BasePoint"]["x"] = data->getData().getBasePoint().x;
	json["BasePoint"]["y"] = data->getData().getBasePoint().y;
	json["BasePoint"]["z"] = data->getData().getBasePoint().z;
	json["Direction"]["x"] = data->getData().getDirection().x;
	json["Direction"]["y"] = data->getData().getDirection().y;
	json["Direction"]["z"] = data->getData().getDirection().z;

	return json;
}

nlohmann::json FilterJsonIO::polylineToJson(const DmPolyline* data)
{
	nlohmann::json json = entityAttributesToJson(data);

    auto plData = data->getData();

    auto vertexs = std::vector<double>();

    auto pts = plData.getVertexs();
    for (auto pt : pts)
    {
		vertexs.emplace_back(pt.x);
		vertexs.emplace_back(pt.y);
    }
	
	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	json["isClosed"] = plData.getIsClosed() ? 1 : 0;
	json["Bulges"] = plData.getBulges();
	std::vector<double> vecLineWeights = plData.getLineWeights();
	json["LineWeights"] = vecLineWeights;
	json["Vertexs"] = vertexs;
	return json;
}

nlohmann::json FilterJsonIO::splineToJson(const DmSpline* data)
{
	nlohmann::json json = entityAttributesToJson(data);

	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	json["isType"] = 1;
	if (data->getData().getKnots().size() > 0)
	{
		json["Knots"] = data->getData().getKnots();
	}
	else
	{
		int k = data->getData().getDegree() + 1;
		auto nknots = data->getNumberOfControlPoints() + data->getDegree() + 1;
		std::vector<double> knotslist;
		for (int i = 1; i <= nknots; i++)
		{
			if (i <= k)
			{
				knotslist.emplace_back(0.0);
			}
			else if (i <= nknots - k)
			{
				knotslist.emplace_back(1.0 / ((double)nknots - 2 * k + 1) * ((double)i - k));
			}
			else
			{
				knotslist.emplace_back(1.0);
			}
		}

		json["Knots"] = knotslist;
	}
	json["Closed"] = data->getData().getIsClosed() ? 1 : 0;
	json["Degree"] = data->getData().getDegree();

	auto pts = std::vector<double>();
	for (auto& pt : data->getData().getControlPoints())
	{
		pts.emplace_back(pt.x);
		pts.emplace_back(pt.y);
	}

	json["ControlPoints"] = pts;

	return json;
}

nlohmann::json FilterJsonIO::solidToJson(const DmSolid* data)
{
	nlohmann::json json = entityAttributesToJson(data);

	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	auto vertexs = std::vector<double>();
	for (auto& v : data->getData().getCorners())
	{
		vertexs.emplace_back(v.x);
		vertexs.emplace_back(v.y);
	}
	json["vertexs"] = vertexs;

	return json;
}

nlohmann::json FilterJsonIO::textToJson(const DmText* data)
{
	nlohmann::json json = entityAttributesToJson(data);
	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	fillTextJson(json, data);
	return json;
}

nlohmann::json FilterJsonIO::mtextToJson(const DmMText* data)
{
	nlohmann::json json = entityAttributesToJson(data);
	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	json["position"]["x"] = data->getPosition().x;
	json["position"]["y"] = data->getPosition().y;
	json["position"]["z"] = data->getPosition().z;
	json["angle"] = data->getDataConstPtr()->getAngle();
	auto dataPtr = data->getDataConstPtr();
	json["textVertMode"] = (int)dataPtr->getTextVertMode();
	json["textHorzMode"] = (int)dataPtr->getTextHorzMode();
	json["style"] = dataPtr->getTextStyle()->getName().toStdWString();
	json["content"] = dataPtr->getTextString().toStdWString();
	json["textHeight"] = dataPtr->getCharHeight();
	json["width"] = dataPtr->getDefineWidth();
	json["lineSpacingFactor"] = dataPtr->getLineSpacingFactor();
	json["height"] = dataPtr->getDefineHeight();
	json["drawingDierction"] = 0;
	return json;
}

nlohmann::json FilterJsonIO::attributeDefinitionToJson(const DmAttributeDefinition* data)
{
	nlohmann::json json = entityAttributesToJson(data);
	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	fillTextJson(json, data);
	
	json["tag"] = data->getTag().toStdWString();
	json["prompt"] = data->getPrompt().toStdWString();
	return json;
}

void FilterJsonIO::fillTextJson(nlohmann::json& json, const DmText* text)
{
	DmText* text_noConst = const_cast<DmText*>(text);
	json["position"]["x"] = text_noConst->getPosition().x;
	json["position"]["y"] = text_noConst->getPosition().y;
	json["position"]["z"] = text_noConst->getPosition().z;
	json["height"] = text_noConst->getHeight();
	json["angle"] = text_noConst->getAngle() * 180 / M_PI;
	json["style"] = text_noConst->getStyle()->getName().toStdWString();
	json["textHorzMode"] = (int)text_noConst->getHAlign();
	json["textVertMode"] = (int)text_noConst->getVAlign();
	json["isMirrorInX"] = text_noConst->getUpsideDown() ? 1 : 0;
	json["isMirrorInY"] = text_noConst->getReverseDirection() ? 1 : 0;
	json["widthFactor"] = text_noConst->getWidthFactor();
	json["oblique"] = text_noConst->getSlashAngle();
	json["alignment"]["x"] = text_noConst->getAlignment().x;
	json["alignment"]["y"] = text_noConst->getAlignment().y;
	json["alignment"]["z"] = text_noConst->getAlignment().z;
	json["textString"] = text_noConst->getText().toStdWString();
}

nlohmann::json FilterJsonIO::attributeToJson(const DmAttribute* data)
{
	nlohmann::json json = entityAttributesToJson(data);
	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	fillTextJson(json, data);

	json["tag"] = data->getTag().toStdWString();
	return json;
}

nlohmann::json FilterJsonIO::dimLinearToJson(const DmDimLinear* data)
{
	nlohmann::json json = entityAttributesToJson(data);
	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	dimensionAttributesToJson(json, data);
	json["angle"] = data->getAngle();
	json["xLine1Point"]["x"] = data->getExtensionPoint1().x;
	json["xLine1Point"]["y"] = data->getExtensionPoint1().y;
	json["xLine1Point"]["z"] = data->getExtensionPoint1().z;
	json["xLine2Point"]["x"] = data->getExtensionPoint2().x;
	json["xLine2Point"]["y"] = data->getExtensionPoint2().y;
	json["xLine2Point"]["z"] = data->getExtensionPoint2().z;
	json["midLinePoint"]["x"] = data->getDefinitionPoint().x;
	json["midLinePoint"]["y"] = data->getDefinitionPoint().y;
	json["midLinePoint"]["z"] = data->getDefinitionPoint().z;
	json["oblique"] = data->getAngle();
	return json;
}

nlohmann::json FilterJsonIO::dimAlignedToJson(const DmDimAligned* data)
{
	nlohmann::json json = entityAttributesToJson(data);
	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	dimensionAttributesToJson(json, data);
	json["angle"] = data->getData().angle;
	json["xLine1Point"]["x"] = data->getExtensionPoint1().x;
	json["xLine1Point"]["y"] = data->getExtensionPoint1().y;
	json["xLine1Point"]["z"] = data->getExtensionPoint1().z;
	json["xLine2Point"]["x"] = data->getExtensionPoint2().x;
	json["xLine2Point"]["y"] = data->getExtensionPoint2().y;
	json["xLine2Point"]["z"] = data->getExtensionPoint2().z;
	json["midLinePoint"]["x"] = data->getDefinitionPoint().x;
	json["midLinePoint"]["y"] = data->getDefinitionPoint().y;
	json["midLinePoint"]["z"] = data->getDefinitionPoint().z;
	json["oblique"] = 0.0;	//标注文字的倾斜，暂不支持
	return json;
}

nlohmann::json FilterJsonIO::dimAngularToJson(const DmDimAngular* data)
{
	nlohmann::json json = entityAttributesToJson(data);
	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	dimensionAttributesToJson(json, data);
	auto edata = data->getEData();
	json["xLine1Start"]["x"] = edata.line1StartPt.x;
	json["xLine1Start"]["y"] = edata.line1StartPt.y;
	json["xLine1Start"]["z"] = edata.line1StartPt.z;
	json["xLine1End"]["x"] = edata.line1EndPt.x;
	json["xLine1End"]["y"] = edata.line1EndPt.y;
	json["xLine1End"]["z"] = edata.line1EndPt.z;
	json["xLine2Start"]["x"] = edata.line2StartPt.x;
	json["xLine2Start"]["y"] = edata.line2StartPt.y;
	json["xLine2Start"]["z"] = edata.line2StartPt.z;
	json["xLine2End"]["x"] = edata.line2EndPt.x;
	json["xLine2End"]["y"] = edata.line2EndPt.y;
	json["xLine2End"]["z"] = edata.line2EndPt.z;
	json["xPtOnArc"]["x"] = edata.ptOnArc.x;
	json["xPtOnArc"]["y"] = edata.ptOnArc.y;
	json["xPtOnArc"]["z"] = edata.ptOnArc.z;
	return json;
}

nlohmann::json FilterJsonIO::dimRadialToJson(const DmDimRadial* data)
{
	nlohmann::json json = entityAttributesToJson(data);
	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	dimensionAttributesToJson(json, data);
	auto edata = data->getEData();
	auto d = data->getData();
	json["centerPt"]["x"] = d.definitionPoint.x;
	json["centerPt"]["y"] = d.definitionPoint.y;
	json["centerPt"]["z"] = d.definitionPoint.z;
	json["diameterPt"]["x"] = edata.endPoint.x;
	json["diameterPt"]["y"] = edata.endPoint.y;
	json["diameterPt"]["z"] = edata.endPoint.z;
	json["leaderLength"] = edata.leader;
	return json;
}

nlohmann::json FilterJsonIO::dimDiametricToJson(const DmDimDiametric* data)
{
	nlohmann::json json = entityAttributesToJson(data);
	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	dimensionAttributesToJson(json, data);
	auto edata = data->getEData();
	auto d = data->getData();
	json["diameter1Point"]["x"] = d.definitionPoint.x;
	json["diameter1Point"]["y"] = d.definitionPoint.y;
	json["diameter1Point"]["z"] = d.definitionPoint.z;
	json["diameter2Point"]["x"] = edata.endPoint.x;
	json["diameter2Point"]["y"] = edata.endPoint.y;
	json["diameter2Point"]["z"] = edata.endPoint.z;
	json["leaderLength"] = edata.leader;
	return json;
}

nlohmann::json FilterJsonIO::dimLeaderToJson(const DmLeader* data)
{
	nlohmann::json json = entityAttributesToJson(data);
	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	auto d = data->getData();
	std::vector<double> pts;
	pts.reserve(d.vertextes.size() * 3);
	for (auto pt : d.vertextes)
	{
		pts.emplace_back(pt.x);
		pts.emplace_back(pt.y);
		pts.emplace_back(pt.z);
	}
	if (pts.size() == 0)
	{
		return json;
	}
	json["dimStyle"]["Name"] = d.pStyle->getName().toStdWString();
	json["vertexsNumber"] = (int)d.vertextes.size();
	json["vertexs"] = pts;

	// 替代属性
	const char* JsonGroupName = "dimension";
	json[JsonGroupName]["Dimldrblk"] = arrowToString(d.leaderArrow());
	json[JsonGroupName]["Dimasz"] =d.arrowSize();
    DmColor clrd = d.dimLineColor();
    int clrdIdx, clrdR, clrdG, clrdB;
    DmColor::toCAD(clrd, clrdIdx, clrdR, clrdG, clrdB);
	json[JsonGroupName]["Dimclrd"] = clrdIdx;
	json[JsonGroupName]["Dimclrd_r"] = clrdR;
	json[JsonGroupName]["Dimclrd_g"] = clrdG;
	json[JsonGroupName]["Dimclrd_b"] = clrdB;
	json[JsonGroupName]["Dimlwd"] = static_cast<int>(d.dimLineWidth());
	json[JsonGroupName]["Dimgap"] =d.offsetFromDimLine();
	json[JsonGroupName]["Dimtad"] = static_cast<int>(d.textVerticalPos());
	return json;
}

nlohmann::json FilterJsonIO::blockReferenceToJson(const DmBlockReference* data)
{
	nlohmann::json json = entityAttributesToJson(data);

	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	json["normal"]["x"] = 0.;
	json["normal"]["y"] = 0.;
	json["normal"]["z"] = 1.;
	json["position"]["x"] = data->getInsertionPoint().x;;
	json["position"]["y"] = data->getInsertionPoint().y;
	json["position"]["z"] = data->getInsertionPoint().z;
	json["RotationAngle"] = data->getAngle();
	json["scale"]["x"] = data->getScale().x;
	json["scale"]["y"] = data->getScale().y;
	json["scale"]["z"] = data->getScale().z;
	json["BlockTableRecordHandle"] = data->getId().asString();
	json["strBlockName"] = data->getName().toStdWString();

	// 属性
	std::list<DmAttribute*> attrs = data->getAttributes();
	if (attrs.size() > 0)
	{
		nlohmann::json attrsNode;
		for (auto attr : attrs)
		{
			auto attrNode = attributeToJson(attr);
			attrsNode.emplace_back(attrNode);
		}
		json["attributes"] = attrsNode;
	}
	return json;
}

nlohmann::json FilterJsonIO::hatchToJson(const DmHatch* data)
{
	nlohmann::json json = entityAttributesToJson(data);
	auto hatchData = data->getData();

	json["type"] = getStrEntityType(data->getEntityType()).toStdString();
	json["patternname"] = hatchData.getPatternName();
	json["patternangle"] = hatchData.getPatternAngle();
	json["numloop"] = hatchData.getLoops();

    json["patternscale"] = hatchData.getPatternScale();

	//loops
	nlohmann::json jsonEnities;
	auto hatch = const_cast<DmHatch*>(data);
    // TODO : 保存边界及孔洞
//	DmEntityContainerPtr bound = hatch->getBoundary();
//	for(auto ent: *bound)
//	{
//		auto type = ent->getEntityType();
//		switch (type)
//		{
//		case DM::EntityLine:
//			jsonEnities.emplace_back(lineToJson((DmLine*)ent));
//			break;
//		case DM::EntityPolyline:
//			jsonEnities.emplace_back(polylineToJson((DmPolyline*)ent));
//			break;
//		case DM::EntityEllipse:
//			jsonEnities.emplace_back(ellipseToJson((DmEllipse*)ent));
//			break;
//		case DM::EntityCircle:
//			jsonEnities.emplace_back(circleToJson((DmCircle*)ent));
//			break;
//		case DM::EntityArc:
//			jsonEnities.emplace_back(arcToJson((DmArc*)ent));
//			break;
//		default:
//			break;
//		}
//	}
	json["entities"] = jsonEnities;

	/*
	nlohmann::json jsonlooptypes;
	// 存储hatch中loops的类型
	// 自建图纸无looptype类型
	for (int i = 0; i < hatchData.getHatchLoopType().size(); i++)
	{
		//json
		nlohmann::json json;
		//读入图纸有该参数
		//自建图纸无该参数，一律统一设置
		json["type"] = (EHatchLoopType)hatchData.getHatchLoopType().at(i);
		jsonlooptypes.emplace_back(json);
	}
	json["looptypes"] = jsonlooptypes;
	*/
	nlohmann::json jsonPattern;
	for (int i = 0; i < hatchData.getPattern().getPatternData().size(); i++)
	{
		auto hatchPatternLine = hatchData.getPattern().getPatternData().at(i);
		nlohmann::json json;
		json["angle"] = hatchPatternLine[0];
		json["basepoint"]["x"] = hatchPatternLine[1];
		json["basepoint"]["y"] = hatchPatternLine[2];
		json["offset"]["x"] = hatchPatternLine[3];
		json["offset"]["y"] = hatchPatternLine[4];
		for (int i = 5; i < hatchPatternLine.size(); i++)
		{
			std::string s = QString::number(i).toStdString();
			json[s.c_str()] = hatchPatternLine[i];
		}
		jsonPattern.emplace_back(json);
	}
	json["pattern"] = jsonPattern;

	auto value = json.dump();
	return json;
}

nlohmann::json FilterJsonIO::entityAttributesToJson(const DmEntity* entity)
{
	nlohmann::json json;

	// 设置图层
	DmLayer* dLayer = entity->getLayer();
	if (nullptr != dLayer)
	{
		json["layer"] = dLayer->getName().toStdWString();
	}
	else
	{
		json["layer"] = L"0";
	}

    DmPen pen = entity->getPen(true);

   if (pen.getColor().isByLayer())
   {
	   json["colorIndex"] = 256;
   }
   else if (pen.getColor().isByBlock())
   {
	   json["colorIndex"] = 0;
   }
   else
   {
	   json["colorIndex"] = -1;
   }

    // 设置颜色
   json["color"]["r"] = pen.getColor().red();
   json["color"]["g"] = pen.getColor().green();
   json["color"]["b"] = pen.getColor().blue();

    // 设置线型
	json["lineType"] = pen.getLineType()->getLineTypeName().toStdWString();

    // 设置线宽
	json["lineWeight"] = (int)pen.getWidth();

	json["visible"] = entity->isVisible() ? 1 : 0;

	json["LtypeScale"] = 1.0;

	return json;
}

void FilterJsonIO::dimensionAttributesToJson(nlohmann::json& json, const DmDimension* dmDim)
{
	DmDimension* dmDim_noConst = const_cast<DmDimension*>(dmDim);
	int attachmentPoint = 1;
    if (dmDim->getHAlign() ==  EMTextHorzMode::kTextLeft)
    {
        attachmentPoint = 1;
    }
    else if (dmDim->getHAlign() ==  EMTextHorzMode::kTextCenter)
    {
        attachmentPoint = 2;
    }
    else if (dmDim->getHAlign() ==  EMTextHorzMode::kTextRight)
    {
        attachmentPoint = 3;
    }

    if (dmDim->getVAlign() ==  EMTextVertMode::kTextTop)
    {
        attachmentPoint += 0;
    }
    else if (dmDim->getVAlign() ==  EMTextVertMode::kTextVertMid)
    {
        attachmentPoint += 3;
    }
    else if (dmDim->getVAlign() ==  EMTextVertMode::kTextBottom)
    {
        attachmentPoint += 6;
    }
	json["textPt"]["x"] = dmDim->getMiddleOfText().x;
	json["textPt"]["y"] = dmDim->getMiddleOfText().y;
	json["textPt"]["z"] = dmDim->getMiddleOfText().z;
	json["dimStyle"] = dmDim_noConst->getStyle()->getName().toStdWString();
	json["textAlignMode"] = attachmentPoint;
	json["textLineStyle"] = 1;	//kAtLeast
	json["textString"] = dmDim_noConst->getLabel().toStdWString();
	json["textLineFactor"] = dmDim_noConst->getLineSpacingFactor();

    //写入替代属性
    DmDimensionData& dataRef = dmDim_noConst->getDataRef();
	setJsonDimData< DmDimensionData>(json, &dataRef, "dimension");
}


DmLineType* FilterJsonIO::jsonToLineType(DmDocument* doc, const nlohmann::json& json)
{
	QString name = QString::fromStdWString(json["linetype"]["name"]);
	if ((name == "ByLayer" || name == "ByBlock" || name == "Continuous") && doc->getLineTypeTable()->find(name))
	{
		return nullptr;
	}

	DmLineType* linetype = new DmLineType(name);
	QString desp = QString::fromStdWString(json["linetype"]["comment"]);
	QString outward = desp.replace(QRegExp("[a-zA-Z0-9()]"), "");
	std::vector<double> pattern;
	for (auto& d : json["linetype"]["dash"])
	{
		pattern.emplace_back((double)d);
	}
	linetype->setLineTypeData(pattern);
	linetype->setLineTypeDesp(desp);
	linetype->setLineTypeName(name);
	linetype->setLineTypeOutWard(outward.trimmed());
	return linetype;
}

DmLayer* FilterJsonIO::jsonToLayer(DmDocument* doc, const nlohmann::json& json)
{
    QString name = QString::fromStdWString(json["layer"]["strName"]);
    if (name != "0" && doc->getLayerTable()->find(name))
    {
        return nullptr;
    }
    DmLayer* layer = new DmLayer(name);
    layer->setPen(attributesToPen(json, doc));
   
	if (json["layer"]["isFreeze"] == 1)
	{
		layer->freeze(true); // todo: autocad里freeze是冻结yicad里freeze是图层开关
	}
	else
	{
		layer->freeze(false);
	}
   
	if (json["layer"]["isLock"] == 1)
	{
		layer->lock(true);
	}
	else
	{
		layer->lock(false);
	}
  
	if (json["layer"]["isPlot"] == 1)
	{
		layer->setPrint(true);
	}
	else
	{
		layer->setPrint(false);
	}

	return layer;
}

DmBlock* FilterJsonIO::jsonToBlockTableRecord(DmDocument* doc, const nlohmann::json& json)
{
	// 给子块表记录添加到m_blockHash
	for (auto subBlockHash : json["subBlocks"])
	{
		auto blockReference = doc->getBlockTable()->find(QString::fromStdWString(subBlockHash["strBlockName"]));
		if (!blockReference)
		{
			jsonToBlockTableRecord(doc, subBlockHash);
		}
	}

	auto name = QString::fromStdWString(json["strBlockName"]);
	auto basePoint = DmVector(json["ptOrigin"]["x"], json["ptOrigin"]["y"], json["ptOrigin"]["z"]);
	auto frozen = false;
	auto handle = json["handle"];

	DmBlockData data(name, basePoint, frozen);
	DmBlock* pBlock = new DmBlock(doc, std::move(data));

	for (auto& entJson : json["entities"])
	{
		DmEntity* pEnt = nullptr;

		if (entJson["type"] == "EntityPoint")
		{
			pEnt = jsonToPoint(doc, entJson);
		}
		else if (entJson["type"] == "EntityLine")
		{
			pEnt = jsonToline(doc, entJson);
		}
		else if (entJson["type"] == "EntityArc")
		{
			pEnt = jsonToArc(doc, entJson);
		}
		else if (entJson["type"] == "EntityCircle")
		{
			pEnt = jsonToCircle(doc, entJson);
		}
		else if (entJson["type"] == "EntityEllipse")
		{
			pEnt = jsonToEllipse(doc, entJson);
		}
		else if (entJson["type"] == "EntityRay")
		{
			pEnt = jsonToRay(doc, entJson);
		}
		else if (entJson["type"] == "EntityXline")
		{
			pEnt = jsonToXline(doc, entJson);
		}
		else if (entJson["type"] == "EntityPolyline")
		{
			pEnt = jsonToPolyline(doc, entJson);
		}
		else if (entJson["type"] == "EntitySpline")
		{
			pEnt = jsonToSpline(doc, entJson);
		}
		else if (entJson["type"] == "EntitySolid")
		{
			pEnt = jsonToSolid(doc, entJson);
		}
		else if (entJson["type"] == "EntityText")
		{
			pEnt = jsonToText(doc, entJson);
		}
		else if (entJson["type"] == "EntityMText")
		{
			pEnt = jsonToMText(doc, entJson);
		}
		else if (entJson["type"] == "EntityDimAligned")
		{
			pEnt = jsonToDimAligned(doc, entJson);
		}
		else if (entJson["type"] == "EntityDimLinear")
		{
			pEnt = jsonToDimLinear(doc, entJson);
		}
		else if (entJson["type"] == "EntityDimRadial")
		{
			pEnt = jsonToDimRadial(doc, entJson);
		}
		else if (entJson["type"] == "EntityDimDiametric")
		{
			pEnt = jsonToDimDiametric(doc, entJson);
		}
		else if (entJson["type"] == "EntityDimAngular")
		{
			pEnt = jsonToDimAngular(doc, entJson);
		}
		else if (entJson["type"] == "EntityDimLeader")
		{
			pEnt = jsonToDimLeader(doc, entJson);
		}
		else if (entJson["type"] == "EntityHatch")
		{
			pEnt = jsonToHatch(doc, entJson);
		}
		else if (entJson["type"] == "EntityBlockReference")
		{
			pEnt = jsonToBlockReference(doc, entJson);
		}
		//else if (entJson["type"] == "EntityArrayRect")
		//{
		//	pEnt = jsonToArrayRect(doc, entJson);
		//}
		//else if (entJson["type"] == "EntityArrayPolar")
		//{
		//	pEnt = jsonToArrayPolar(doc, entJson);
		//}
		if (pEnt)
		{
			pEnt->setParent(nullptr);
			pBlock->getEntityTable().add_direct(pEnt);
		}
	}

	doc->getBlockTable()->add_direct(pBlock);

	return pBlock;
}

DmTextStyle* FilterJsonIO::jsonToTextStyle(DmDocument* doc, const nlohmann::json& json)
{
	std::unique_ptr<DmTextStyle> pStyle = std::make_unique<DmTextStyle>(QString::fromStdWString(json["textStyle"]["strName"]));
    DmTextStyleData styleData = pStyle->getData();
    styleData.pAsciiFont = nullptr; //默认有一个字体，置空
    QString bigFontName= QString::fromStdWString(json["textStyle"]["strBigFontName"]);
    if (!bigFontName.isEmpty())
    {
        if (!bigFontName.toLower().endsWith(".shx"))        //大字体GBCBIG没有后缀
        {
            bigFontName.append(".shx");
        }
        DmFont* pBigFont = DMFONTLIST->requestFont(bigFontName, false);
        if (pBigFont)
        {
            styleData.pBigFont = pBigFont;
        }
        else
        {
            styleData.invalidBigFont = bigFontName;
		}
		styleData.isUseBigfont = true;
        styleData.isSystemFont = false;
    }
    else
    {
        styleData.isUseBigfont = false;
    }

    QString fontName = QString::fromStdWString(json["textStyle"]["strSysFontName"]);
    if (!fontName.isEmpty())
    {
        QString fontLower = fontName.toLower();
        //对于宋体，getSysFontName()返回SimSun。对于仿宋getSysFontName()返回"仿宋"。AutoCAD是啥奇葩操作
        static std::map<QString, QString> covertMap{ {"simsun", QString::fromLocal8Bit("宋体")},
            {"simhei", QString::fromLocal8Bit("黑体")},
            {"nsimsun",QString::fromLocal8Bit("新宋体")} };
        auto it = covertMap.find(fontLower);
        if (covertMap.end() != it)
        {
            fontName = it->second;
        }
		bool isSysBold = json["textStyle"]["SysFontBold"] == 1 ? true : false;
		bool isSysItalic = json["textStyle"]["SysFontItalic"] == 1 ? true : false;
		DmFont* pFont = DMFONTLIST->requestSysFont(fontName, isSysBold, isSysItalic);
        if (pFont)
        {
            styleData.sysFontFamily = fontName;
            styleData.pSysFont = pFont;
		}
		else
		{
            styleData.invalidSysFontFamily = fontName;
		}
        styleData.isSystemFont = true;
    }
    else
    {
        QString fontFileName = QString::fromStdWString(json["textStyle"]["strSysFontFileName"]);
        if (!fontFileName.isEmpty())
        {
            if (!fontFileName.endsWith(".shx"))
            {
                fontFileName += ".shx";
            }
            DmFont* asciiFont = DMFONTLIST->requestFont(fontFileName);
            if (asciiFont)
            {
                styleData.pAsciiFont = asciiFont;
            }
            else
            {
                styleData.invalidAsciiFont = fontFileName;
            }
        }
        else
        {
            styleData.invalidAsciiFont = fontFileName;
        }
        styleData.isSystemFont = false;
    }

    styleData.isReverseDirection = json["textStyle"]["IsBackward"] == 1? true : false;
    styleData.isUpsideDown = json["textStyle"]["IsUpsidedown"] == 1 ? true : false;
    styleData.isVertical = json["textStyle"]["IsVertical"] == 1 ? true : false;
    styleData.slashAngle = json["textStyle"]["ObliquingAngle"];
    styleData.defaultHeight = json["textStyle"]["TextSize"];
    styleData.widhFactor = json["textStyle"]["WidthFactor"];
    pStyle->setData(styleData);
	return pStyle.release();
}

DmDimensionStyle* FilterJsonIO::jsonToDimensionStyle(DmDocument* doc, const nlohmann::json& json)
{
	QString dimStyleName = QString::fromStdWString(json["dimStyle"]["Name"]);
	QString dimTxsty = QString::fromStdWString(json["dimStyle"]["Dimtxsty"]);
    DmTextStyle* txtStyle = doc->getTextStyleTable()->find(dimTxsty);
    if (txtStyle == nullptr)
    {
        return nullptr;
    }

	DmDimensionStyle* pStyle = new DmDimensionStyle(dimStyleName, txtStyle);
    DmDimensionStyleData& dataRef = pStyle->getDataRef();
	setDmDimData<DmDimensionStyleData>(dataRef, json, txtStyle, "dimStyle");
	return pStyle;
}

DmPoint* FilterJsonIO::jsonToPoint(DmDocument* doc, const nlohmann::json& json)
{
	double x = json["start"]["x"];
	double y = json["start"]["y"];
	DmPoint* entity = new DmPoint(nullptr, PointData(DmVector(x, y)));

	jsonToEntityAttributes(doc, entity, json);

	return entity;
}

DmLine* FilterJsonIO::jsonToline(DmDocument* doc, const nlohmann::json& json)
{
	double start_x = json["start"]["x"];
	double start_y = json["start"]["y"];
	double end_x = json["end"]["x"];
	double end_y = json["end"]["y"];
	DmLine* entity = new DmLine(nullptr, LineData(DmVector(start_x, start_y), DmVector(end_x, end_y)));

	jsonToEntityAttributes(doc, entity, json);

	return entity;
}

DmCircle* FilterJsonIO::jsonToCircle(DmDocument* doc, const nlohmann::json& json)
{
	double center_x = json["center"]["x"];
	double center_y = json["center"]["y"];
	double center_z = json["center"]["z"];
	double radius = json["radius"];
	DmCircle* entity = new DmCircle(nullptr, CircleData(DmVector(center_x, center_y, center_z), radius));

	jsonToEntityAttributes(doc, entity, json);

	return entity;
}

DmArc* FilterJsonIO::jsonToArc(DmDocument* doc, const nlohmann::json& json)
{
	double ptCenter_x = json["center"]["x"];
	double ptCenter_y = json["center"]["y"];
	double ptCenter_z = json["center"]["z"];
	double ptNormal_x = json["Normal"]["x"];
	double ptNormal_y = json["Normal"]["y"];
	double ptNormal_z = json["Normal"]["z"];
	double dStartAngle = json["StartAngle"];
	double dEndAngle = json["EndAngle"];
	double dRadius = json["radius"];

	ArcData data;
	data.setCenter(DmVector(ptCenter_x, ptCenter_y, ptCenter_z));
	data.setRadius(dRadius);
	data.setStartAngle(dStartAngle);
	data.setEndAngle(dEndAngle);
	data.setNormal(DmVector(ptNormal_x, ptNormal_y, ptNormal_z));

	DmArc* entity = new DmArc(nullptr, std::move(data));

	jsonToEntityAttributes(doc, entity, json);

	return entity;
}

DmEllipse* FilterJsonIO::jsonToEllipse(DmDocument* doc, const nlohmann::json& json)
{
	double center_x = json["center"]["x"];
	double center_y = json["center"]["y"];
	double center_z = json["center"]["z"];
	double majorP_x = json["MajorP"]["x"];
	double majorP_y = json["MajorP"]["y"];
	double majorP_z = json["MajorP"]["z"];
	double ratio = json["Ratio"];
	bool isClosed = json["Closed"] == 1 ? true : false;
	double normal_x = json["Normal"]["x"];
	double normal_y = json["Normal"]["y"];
	double normal_z = json["Normal"]["z"];
	double startPara = json["StartParam"];
	double endPara = json["EndParam"];

	EllipseData data;
	data.setCenter(DmVector(center_x, center_y, center_z));
	data.setMajorP(DmVector(majorP_x, majorP_y, majorP_z));
	data.setRatio(ratio);
	data.setNormal(DmVector(normal_x, normal_y, normal_z));
	data.setStartParam(startPara);
	data.setEndParam(endPara);
	data.setIsClosed(isClosed);
	DmEllipse* entity = new DmEllipse(nullptr, std::move(data));

	jsonToEntityAttributes(doc, entity, json);

	return entity;
}

DmRay* FilterJsonIO::jsonToRay(DmDocument* doc, const nlohmann::json& json)
{
	RayData data;
	double basePoint_x = json["BasePoint"]["x"];
	double basePoint_y = json["BasePoint"]["y"];
	double basePoint_z = json["BasePoint"]["z"];
	double direction_x = json["Direction"]["x"];
	double direction_y = json["Direction"]["y"];
	double direction_z = json["Direction"]["z"];
	data.setBasePoint(DmVector(basePoint_x, basePoint_y, basePoint_z));
	data.setDirection(DmVector(direction_x, direction_y, direction_z));

	DmRay* entity = new DmRay(nullptr, std::move(data));

	jsonToEntityAttributes(doc, entity, json);

	return entity;
}

DmXline* FilterJsonIO::jsonToXline(DmDocument* doc, const nlohmann::json& json)
{
	XLineData data;
	double basePoint_x = json["BasePoint"]["x"];
	double basePoint_y = json["BasePoint"]["y"];
	double basePoint_z = json["BasePoint"]["z"];
	double direction_x = json["Direction"]["x"];
	double direction_y = json["Direction"]["y"];
	double direction_z = json["Direction"]["z"];
	data.setBasePoint(DmVector(basePoint_x, basePoint_y, basePoint_z));
	data.setDirection(DmVector(direction_x, direction_y, direction_z));

	DmXline* entity = new DmXline(nullptr, std::move(data));

	jsonToEntityAttributes(doc, entity, json);

	return entity;
}

DmPolyline* FilterJsonIO::jsonToPolyline(DmDocument* doc, const nlohmann::json& json)
{
	PolylineData data;
	if (json["isClosed"] == 1)
	{
		data.setIsClosed(true);
	}
	else
	{
		data.setIsClosed(false);
	}

	auto bulges = std::vector<double>();
	for (auto& v : json["Bulges"])
	{
		bulges.emplace_back(v);
	}
	data.setBulges(bulges);

	auto lineWeights = std::vector<double>();
	for (auto& v : json["LineWeights"])
	{
		lineWeights.emplace_back((double)v);
	}
	data.setLineWeights(lineWeights);

	auto dVertexs = std::vector<double>();
	for (auto& v : json["Vertexs"])
	{
		dVertexs.emplace_back((double)v);
	}
	auto vertexs = std::vector<DmVector>();
	for (int i = 0; i < dVertexs.size(); i+=2)
	{
		vertexs.emplace_back(DmVector(dVertexs[i], dVertexs[i + 1], 0.0));
	}
	data.setVertexs(vertexs);

	DmPolyline* entity = new DmPolyline(nullptr, std::move(data));
	entity->update();
	jsonToEntityAttributes(doc, entity, json);

	return entity;
}

DmSpline* FilterJsonIO::jsonToSpline(DmDocument* doc, const nlohmann::json& json)
{
	SplineData data;

	auto knots = std::vector<double>();
	for (auto& i : json["Knots"])
	{
		knots.emplace_back((double)i);
	}
	data.setKnots(knots);

	if (json["Closed"] == 1)
	{
		data.setIsClosed(true);
	}
	else
	{
		data.setIsClosed(false);
	}

	data.setDegree((int)json["Degree"]);

	auto pts = std::vector<double>();
	for (auto const& vert : json["ControlPoints"])
	{
		pts.emplace_back(vert);
	}

	DmSpline* entity = new DmSpline(nullptr, std::move(data));

	for (int i = 0; i < pts.size(); i += 2)
	{
		entity->addControlPoint(DmVector(pts[i], pts[i + 1], 0.0));
	}
	entity->update();

	jsonToEntityAttributes(doc, entity, json);

	return entity;
}

DmSolid* FilterJsonIO::jsonToSolid(DmDocument* doc, const nlohmann::json& json)
{
	SolidData data;

	auto pts = std::vector<double>();
	for (auto& v : json["vertexs"])
	{
		pts.emplace_back(v);
	}

	auto vertexs = std::vector<DmVector>();
	for (int i = 0; i < pts.size(); i += 2)
	{
		vertexs.emplace_back(DmVector(pts[i], pts[i + 1], 0.0));
	}
	data.setCorners(vertexs);

	DmSolid* entity = new DmSolid(nullptr, std::move(data));

	jsonToEntityAttributes(doc, entity, json);

	return entity;
}

DmText* FilterJsonIO::jsonToText(DmDocument* doc, const nlohmann::json& json)
{
	TextData d = jsonToTextData(doc, json);
	DmText* entity = new DmText(nullptr, std::move(d));
    entity->update();
	jsonToEntityAttributes(doc, entity, json);
    return entity;
}

DmMText* FilterJsonIO::jsonToMText(DmDocument* doc, const nlohmann::json& json)
{
	MTextData d = jsonToMTextData(doc, json);
	DmMText* entity = new DmMText(nullptr, std::move(d));
	entity->update();
	jsonToEntityAttributes(doc, entity, json);
	return entity;
}

DmAttributeDefinition* FilterJsonIO::jsonToAttributeDefinition(DmDocument* doc, const nlohmann::json& json)
{
	TextData d = jsonToTextData(doc, json);
    AttributeDefinitionData attrData;
    attrData.setTag(QString::fromStdWString(json["tag"]));
    attrData.setPrompt(QString::fromStdWString(json["prompt"]));
    DmAttributeDefinition* entity = new DmAttributeDefinition(nullptr, d, attrData);
    entity->update();
	jsonToEntityAttributes(doc, entity, json);
    return entity;
}

DmAttribute* FilterJsonIO::jsonToAttribute(DmDocument* doc, DmBlockReference* blkReference, const nlohmann::json& json)
{
	TextData d = jsonToTextData(doc, json);
	AttributeData attrData;
	attrData.setTag(QString::fromStdWString(json["tag"]));
	DmAttribute* entity = new DmAttribute(blkReference, d, attrData);
	entity->update();
	jsonToEntityAttributes(doc, entity, json);
	return entity;
}

DmDimLinear* FilterJsonIO::jsonToDimLinear(DmDocument* doc, const nlohmann::json& json)
{
	DmDimensionData dimensionData = jsonToDimensionData(doc, json);
	DmVector dxt1(json["xLine1Point"]["x"], json["xLine1Point"]["y"]);
    DmVector dxt2(json["xLine2Point"]["x"], json["xLine2Point"]["y"]);

    dimensionData.definitionPoint = DmVector(json["midLinePoint"]["x"], json["midLinePoint"]["y"]);
    double angle = json["angle"];
    dimensionData.angle = angle;
    DmDimLinearData d(dxt1, dxt2);

    DmDimLinear* entity = new DmDimLinear(nullptr, dimensionData, d);
    entity->update();
	jsonToEntityAttributes(doc, entity, json);
    return entity;
}

DmDimAligned* FilterJsonIO::jsonToDimAligned(DmDocument* doc, const nlohmann::json& json)
{
	DmDimensionData dimensionData = jsonToDimensionData(doc, json);
	DmVector dxt1(json["xLine1Point"]["x"], json["xLine1Point"]["y"]);
	DmVector dxt2(json["xLine2Point"]["x"], json["xLine2Point"]["y"]);

	dimensionData.definitionPoint = DmVector(json["midLinePoint"]["x"], json["midLinePoint"]["y"]);
	double angle = json["angle"];
	dimensionData.angle = angle;
	double oblique = json["oblique"];
	DmDimAlignedData d(dxt1, dxt2);

	DmDimAligned* entity = new DmDimAligned(nullptr, dimensionData, d);
	entity->update();
	jsonToEntityAttributes(doc, entity, json);
	return entity;
}

DmDimAngular* FilterJsonIO::jsonToDimAngular(DmDocument* doc, const nlohmann::json& json)
{
	DmDimensionData dimensionData = jsonToDimensionData(doc, json);
	DmVector line1Start(json["xLine1Start"]["x"], json["xLine1Start"]["y"]);
	DmVector line1End(json["xLine1End"]["x"], json["xLine1End"]["y"]);
	DmVector line2Start(json["xLine2Start"]["x"], json["xLine2Start"]["y"]);
	DmVector line2End(json["xLine2End"]["x"], json["xLine2End"]["y"]);
	DmVector ptOnArc(json["xPtOnArc"]["x"], json["xPtOnArc"]["y"]);

	DmDimAngularData d(line1Start, line1End, line2Start, line2End, ptOnArc);
	DmDimAngular* entity = new DmDimAngular(nullptr, dimensionData, d);
    entity->update();
	jsonToEntityAttributes(doc, entity, json);
    return entity;
}

DmDimRadial* FilterJsonIO::jsonToDimRadial(DmDocument* doc, const nlohmann::json& json)
{
	DmDimensionData dimensionData = jsonToDimensionData(doc, json);
	DmVector endPt(json["diameterPt"]["x"], json["diameterPt"]["y"]);
	DmVector defPt(json["centerPt"]["x"], json["centerPt"]["y"]);
	double leaderLength = json["leaderLength"];
	dimensionData.definitionPoint = defPt;
	DmDimRadialData d(endPt, leaderLength);
	DmDimRadial* entity = new DmDimRadial(nullptr, dimensionData, d);
	entity->update();
	jsonToEntityAttributes(doc, entity, json);
	return entity;
}

DmDimDiametric* FilterJsonIO::jsonToDimDiametric(DmDocument* doc, const nlohmann::json& json)
{
	DmDimensionData dimensionData = jsonToDimensionData(doc, json);
	DmVector p1(json["diameter1Point"]["x"], json["diameter1Point"]["y"]);
	DmVector p2(json["diameter2Point"]["x"], json["diameter2Point"]["y"]);
	double leaderLength = json["leaderLength"];
	dimensionData.definitionPoint = p1;
	DmDimDiametricData d(p2, leaderLength);
	DmDimDiametric* entity = new DmDimDiametric(nullptr, dimensionData, d);
	entity->update();
	jsonToEntityAttributes(doc, entity, json);
	return entity;
}

DmLeader* FilterJsonIO::jsonToDimLeader(DmDocument* doc, const nlohmann::json& json)
{
	DmDimensionStyle* pStyle = doc->getDimStyleTable()->find(QString::fromStdWString(json["dimStyle"]["Name"]));
    if (nullptr == pStyle)
        return nullptr;

	//取出所有顶点
    std::vector<DmVector> pts;
	DmVector tempPt(0.0, 0.0);
	int num = json["vertexsNumber"];
	if (num > 0)
	{
		auto vertexsNode = json["vertexs"];
		int i = 0;
		for (auto& val : vertexsNode)
		{
			if (i % 3 == 0)
			{
				tempPt.x = val;
			}
			else if (i % 3 == 1)
			{
				tempPt.y = val;
			}
			else
			{
				tempPt.z = val;
				pts.push_back(tempPt);
			}
			i++;
		}
	}
	
    DmLeaderData d(pStyle, pts);
    DmLeader* leader = new DmLeader(nullptr, d);
    
    //替代属性
	const char* JsonGroupName = "dimension";
    DmLeaderData& ddRef = leader->getDataRef();
	auto strDimldrblk = QString::fromStdWString(json[JsonGroupName]["Dimldrblk"]);
    ddRef.setLeaderArrow(stringToArrow(strDimldrblk.toStdWString()));
    ddRef.setArrowSize(json[JsonGroupName]["Dimasz"]);
	int dimclrd = json[JsonGroupName]["Dimclrd"];
	int dimclrd_r = json[JsonGroupName]["Dimclrd_r"];
	int dimclrd_g = json[JsonGroupName]["Dimclrd_g"]; 
	int dimclrd_b = json[JsonGroupName]["Dimclrd_b"];
    ddRef.setDimLineColor(DmColor::fromCAD(dimclrd, dimclrd_r, dimclrd_g, dimclrd_b));
    ddRef.setDimLineWidth(static_cast<DM::LineWidth>(json[JsonGroupName]["Dimlwd"]));
    ddRef.setOffsetFromDimLine(json[JsonGroupName]["Dimgap"]);
    ddRef.setTextVerticalPos(static_cast<DmDimensionStyleTextData::TextVerticalPos>(json[JsonGroupName]["Dimtad"]));
    leader->update();

	jsonToEntityAttributes(doc, leader, json);
    return leader;
}

DmBlockReference* FilterJsonIO::jsonToBlockReference(DmDocument* doc, const nlohmann::json& json)
{
	auto name = QString::fromStdWString(json["strBlockName"]);
	auto insertionPoint = DmVector(json["position"]["x"], json["position"]["y"]);
	auto scaleFactor = DmVector(json["scale"]["x"], json["scale"]["y"]);
    DmVector sp(1.0, 1.0);
	double rotationAngle = json["RotationAngle"];

	DmBlockReferenceData data(name, insertionPoint, scaleFactor, rotationAngle, 1, 1, sp, doc->getBlockTable(), DM::Update, "");

	auto block = doc->getBlockTable()->find(name);
	if (block)
	{
		DmBlockReference* entity = new DmBlockReference(nullptr, std::move(data));
		//属性
		if (json.find("attributes") != json.end())
		{
			std::list<DmAttribute*> attrs;
			for (auto attrNode : json["attributes"])
			{
				DmAttribute* attr = jsonToAttribute(doc, entity, attrNode);
				attrs.emplace_back(attr);
			}
			entity->addAttributes(attrs);
		}

		if (json["normal"]["z"] < 0)
		{
			entity->mirror(DmVector(entity->getInsertionPoint().x, 0), DmVector(entity->getInsertionPoint().x, 1));
		}

		jsonToEntityAttributes(doc, entity, json);

		return entity;
	}
	else
	{
		return nullptr;
	}
}

DmHatch* FilterJsonIO::jsonToHatch(DmDocument* doc, const nlohmann::json& json)
{
	std::wstring patname = json["patternname"];
	bool isSolid = patname == L"SOLID";
	HatchData hdata(isSolid, patname);
	double angle = json["patternangle"];
	hdata.setPatternAngle(angle);
    double scale = json["patternscale"];
    hdata.setPatternScale(scale);

	DmEntityContainerPtr boundry(new DmEntityContainer());
	for (int i = 0; i < json["entities"].size(); i++)
	{
		for (int j = 0; j < json["entities"][i]["subjson"].size(); j++)
		{
			auto subj = json["entities"][i]["subjson"][j];
			if (subj["type"] == "EntityLine")
			{
				DmVector start(subj["start"]["x"], subj["start"]["y"]);
				DmVector end(subj["end"]["x"], subj["end"]["y"]);
				DmLine* line = new DmLine(boundry.get(), start, end);
				boundry->appendEntity(line);
			}
			else if (subj["type"] == "EntityPolyline")
			{
				PolylineData data;

				auto bulges = std::vector<double>();
				for (auto& v : subj["Bulges"])
				{
					bulges.emplace_back((double)v);
				}
				data.setBulges(bulges);

				auto lineWeights = std::vector<double>();
				for (auto& v : subj["LineWeights"])
				{
					lineWeights.emplace_back((double)v);
				}
				data.setLineWeights(lineWeights);

				if (subj["isClosed"] == 1)
				{
					data.setIsClosed(true);
				}
				else
				{
					data.setIsClosed(false);
				}

				auto dVertexs = std::vector<double>();
				for (auto& v : subj["Vertexs"])
				{
					dVertexs.emplace_back((double)v);
				}

				auto vertexs = std::vector<DmVector>();
				for (int i = 0; i < dVertexs.size(); i += 2)
				{
					vertexs.emplace_back(DmVector(dVertexs[i], dVertexs[i + 1], 0.0));
				}
				data.setVertexs(vertexs);

				DmPolyline* entity = new DmPolyline(boundry.get(), std::move(data));
				entity->update();
				boundry->appendEntity(entity);
			}
			else if (subj["type"] == "EntityArc")
			{
				DmVector center(subj["center"]["x"], subj["center"]["y"]);
				double radius(subj["radius"]);
				double startangle(subj["StartAngle"]);
				double endangle(subj["EndAngle"]);

				DmArc* arc = new DmArc(boundry.get(), ArcData(center, DmVector(0.0, 0.0, 1.0), radius, startangle, endangle));
				boundry->appendEntity(arc);
			}
			else if (subj["type"] == "EntityEllipse")
			{
				double center_x = subj["center"]["x"];
				double center_y = subj["center"]["y"];
				double majorP_x = subj["MajorP"]["x"];
				double majorP_y = subj["MajorP"]["y"];
				double ratio = subj["Ratio"];
				double normal_x = subj["Normal"]["x"];
				double normal_y = subj["Normal"]["y"];
				double normal_z = 1;
				double startPara = subj["StartParam"];
				double endPara = subj["EndParam"];
				EllipseData data;
				data.setCenter(DmVector(center_x, center_y));
				data.setMajorP(DmVector(majorP_x, majorP_y));
				data.setRatio(ratio);
				data.setNormal(DmVector(normal_x, normal_y, normal_z));
				data.setStartParam(startPara);
				data.setEndParam(endPara);

				DmEllipse* ellipse = new DmEllipse(boundry.get(), data);

				boundry->appendEntity(ellipse);
			}
		}
	}
	
	DmPattern *pat = new DmPattern(patname);
	std::vector<std::vector<double>> patdata = std::vector<std::vector<double>>{};
	for (int i = 0; i < json["pattern"].size(); i++)
	{
		auto patdataline = json["pattern"][i];

		std::vector<double> ptl = std::vector<double>{};
		ptl.emplace_back(patdataline["angle"]);
		ptl.emplace_back(patdataline["basepoint"]["x"]);
		ptl.emplace_back(patdataline["basepoint"]["y"]);
		ptl.emplace_back(patdataline["offset"]["x"]);
		ptl.emplace_back(patdataline["offset"]["y"]);
		//patdataline["offset"]["x"]和patdataline["offset"]["y"]在一个节点下
		for (int i = 5; i < patdataline.size() + 2; i++)
		{
			std::string s = QString::number(i).toStdString();
			ptl.emplace_back(patdataline[s]);
		}
		patdata.emplace_back(ptl);
	}
	pat->setPatternData(patdata);
	hdata.setPattern(*pat);

	DmHatch* entity = new DmHatch(nullptr, hdata);
    // TODO ：暂不导出json
//	entity->setBoundary(boundry);
//	entity->update();

	jsonToEntityAttributes(doc, entity, json);

	return entity;

}

void FilterJsonIO::jsonToEntityAttributes(DmDocument* doc, DmEntity* pEnt, const nlohmann::json& attrib)
{
	DmPen pen;
	pen.setColor({ 0,0,0 });
	pen.setLineType(DmLineTypeTable::Continuous);
	pEnt->setDocument(doc);

	// 设置图层
	QString layName = QString::fromStdWString(attrib["layer"]);
	pEnt->setLayer(layName);

	// 设置颜色
	auto dmColor = DmColor::fromCAD(attrib["colorIndex"], attrib["color"]["r"], attrib["color"]["g"], attrib["color"]["b"]);
	pen.setColor(dmColor);

	// 设置线型
	pen.setLineType(nameToLineType(QString::fromStdWString(attrib["lineType"]), doc));

	// 设置线宽
	pen.setWidth(numberToWidth(attrib["lineWeight"]));

	pEnt->setPen(pen);

	// 设置是否可见
	pEnt->DmEntity::setVisible(attrib["visible"] == 1);
}

DmLineType* FilterJsonIO::nameToLineType(const  QString& name, DmDocument* pDocument)
{
    QString uName = name.toUpper();

    // Standard linetypes for AutoCAD
    if (uName.isEmpty() || uName == "BYLAYER")
    {
        return DmLineTypeTable::ByLayer;
    }
    else if (uName == "BYBLOCK")
    {
        return DmLineTypeTable::ByBlock;
    }
    else if (uName == "CONTINUOUS" || uName == "ACAD_ISO01W100")
    {
        return DmLineTypeTable::Continuous;
    }
    else
    {
        DmLineTypeTable* lineTypeTable = pDocument->getLineTypeTable();
        for(auto lt:*lineTypeTable)
        {
            if (name == lt->getLineTypeName())
            {
                return lt;
            }
        }
        return DmLineTypeTable::Continuous;
    }
}

DM::LineWidth FilterJsonIO::numberToWidth(const int& iLineWeight)
{
	DM::LineWidth eLineWeight = (DM::LineWidth)iLineWeight;
	return eLineWeight;
}

DM::ArrowType FilterJsonIO::stringToArrow(const std::wstring& arrowName)
{
	//参考AutoCAD系统变量"DIMBLK"
    static std::map< std::wstring, DM::ArrowType> map{
        {L"", DM::ArrowType::ClosedFilled},
        {L"_CLOSED", DM::ArrowType::Closed},
        {L"_CLOSEDBLANK", DM::ArrowType::ClosedBlank},
        {L"_DOT", DM::ArrowType::Dot},
        {L"_ARCHTICK", DM::ArrowType::ArchitecturalTick},
        {L"_OBLIQUE", DM::ArrowType::Oblique},
        {L"_OPEN", DM::ArrowType::Open},
        {L"_ORIGIN", DM::ArrowType::OriginIndicator},
        {L"_ORIGIN2", DM::ArrowType::OriginIndicator2},
        {L"_OPEN90", DM::ArrowType::RightAngle},
        {L"_OPEN30", DM::ArrowType::Open30},
        {L"_DOTSMALL", DM::ArrowType::DotSmall},
        {L"_DOTBLANK", DM::ArrowType::DotBlank},
        {L"_SMALL", DM::ArrowType::DotSmallBlank},
        {L"_BOXBLANK", DM::ArrowType::Box},
        {L"_BOXFILLED", DM::ArrowType::BoxFilled},
        {L"_DATUMBLANK", DM::ArrowType::DatumTriangle},
        {L"_DATUMFILLED", DM::ArrowType::DatumTriangleFilled},
        {L"_INTEGRAL", DM::ArrowType::Integral},
        {L"_NONE", DM::ArrowType::None},
    };
    std::wstring arrowNameUpper(arrowName.size(), 0);
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

std::wstring FilterJsonIO::arrowToString(DM::ArrowType arrowType)
{
	//参考AutoCAD系统变量"DIMBLK"
    static std::map<DM::ArrowType, std::wstring> map{
		{ DM::ArrowType::ClosedFilled, L""},
		{ DM::ArrowType::Closed, L"_CLOSED"},
		{DM::ArrowType::ClosedBlank, L"_CLOSEDBLANK"},
		{DM::ArrowType::Dot, L"_DOT"},
		{DM::ArrowType::ArchitecturalTick, L"_ARCHTICK"},
		{DM::ArrowType::Oblique, L"_OBLIQUE"},
		{DM::ArrowType::Open, L"_OPEN"},
		{DM::ArrowType::OriginIndicator, L"_ORIGIN"},
		{DM::ArrowType::OriginIndicator2, L"_ORIGIN2"},
		{DM::ArrowType::RightAngle, L"_OPEN90"},
		{DM::ArrowType::Open30, L"_OPEN30"},
		{DM::ArrowType::DotSmall, L"_DOTSMALL"},
		{DM::ArrowType::DotBlank, L"_DOTBLANK"},
		{DM::ArrowType::DotSmallBlank, L"_SMALL"},
		{DM::ArrowType::Box, L"_BOXBLANK"},
		{DM::ArrowType::BoxFilled, L"_BOXFILLED"},
		{DM::ArrowType::DatumTriangle, L"_DATUMBLANK"},
		{DM::ArrowType::DatumTriangleFilled, L"_DATUMFILLED"},
		{DM::ArrowType::Integral, L"_INTEGRAL"},
        {DM::ArrowType::None, L"_NONE"},
    };
    auto it = map.find(arrowType);
    if (it != map.end())
    {
        return it->second;
    }
    else
    {
        return L"";
    }
}

DmPen FilterJsonIO::attributesToPen(const nlohmann::json& json, DmDocument* pDocument)
{
	DmColor dmColor;
	dmColor = DmColor(json["layer"]["Color_r"], json["layer"]["Color_g"], json["layer"]["Color_b"]);
	
	// 设置画笔
	DmPen pen(dmColor, numberToWidth(json["layer"]["LineWeight"]), nameToLineType(QString::fromStdString(json["layer"]["strLineType"]), pDocument));
	return pen;
}

QString FilterJsonIO::getStrEntityType(DM::EntityType type)
{
    std::string typeStr1 = DmEntityHelper::getEntityNameByType(type);
    return QString("Entity%1").arg(QString::fromStdString(typeStr1));
}
