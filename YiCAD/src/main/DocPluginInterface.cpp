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

/// @file DocPluginInterface.cpp
/// @brief 插件文档接口类实现，提供实体序列化/反序列化及文档级操作能力

#include "DocPluginInterface.h"
#include <QEventLoop>
#include <QList>
#include <QInputDialog>
#include <QFileInfo>
#include "GuiDocumentView.h"
#include "ActionInterface.h"
#include "GuiEventHandler.h"
#include "ActionSelect.h"
#include "ActionFileNew.h"
#include "DmArc.h"
#include "DmCircle.h"
#include "DmLine.h"
#include "DmPoint.h"
#include "DmLayer.h"
#include "DmImage.h"
#include "DmBlock.h"
#include "DmBlockReference.h"
#include "DmPolyline.h"
#include "DmEllipse.h"
#include "DmPolyline.h"
#include "DmSpline.h"
#include "DmRay.h"
#include "DmXline.h"
#include "DmSolid.h"
#include "DmMText.h"
#include "DmText.h"
#include "DmAttributeDefinition.h"
#include "DmDimLinear.h"
#include "DmDimAligned.h"
#include "DmDimAngular.h"
#include "DmDimRadial.h"
#include "DmDimDiametric.h"
#include "DmLeader.h"
#include "Math2d.h"
#include "Debug.h"
#include "EllipseData.h"
#include "DmHatch.h"

#include "UIFileDialog.h"
#include "ApplicationWindow.h"

Doc_plugin_interface::Doc_plugin_interface(DmDocument* d, GuiDocumentView* gv, QWidget* parent)
	: m_pDocument(d)
	, m_pView(gv)
	, m_jsonConverter(new FilterJsonIO())
{
}

QString Doc_plugin_interface::save(QString& formatType)
{
	UIFileDialog dlg;
	QString fileName = dlg.getSaveFile(formatType);
	QFileInfo file(fileName);
	if (!fileName.isEmpty())
	{
		ApplicationWindow* appWindow = ApplicationWindow::getAppWindow();
		// 设置当前画布选项卡名和提示
		appWindow->setDrawingTabName(fileName);
	}
	return fileName;
}

QString Doc_plugin_interface::open()
{
	UIFileDialog dlg;
	QString fileName = dlg.getOpenFile();
	QFileInfo file(fileName);
	if (!fileName.isEmpty())
	{
		ApplicationWindow* appWindow = ApplicationWindow::getAppWindow();
		GuiDocumentView* docView = appWindow->getDocumentView();
		ActionFileNew* action = new ActionFileNew(appWindow->getDocument(), docView);
		action->setFileName(fileName);
		if (docView)
		{
			docView->setCurrentAction(action);
		}
		//在没有打开文档的情况，Action无法被管理，但是需要触发一下
		else
		{
			action->trigger();
			delete action;
		}
		m_pDocument = appWindow->getDocument();
	}
	return fileName;
}


void Doc_plugin_interface::addLineType(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmLineType* linetype = m_jsonConverter->jsonToLineType(m_pDocument, json);
		if (linetype)
		{
			m_pDocument->getLineTypeTable()->add_direct(linetype);
		}
	}
}

void Doc_plugin_interface::addLayer(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmLayer* layer = m_jsonConverter->jsonToLayer(m_pDocument, json);
		if (layer)
		{
			m_pDocument->getLayerTable()->add_direct(layer);
		}
	}
}

void Doc_plugin_interface::addBlockTableRecord(const nlohmann::json& json)
{
    if (m_pDocument)
    {
        DmBlock* data = m_jsonConverter->jsonToBlockTableRecord(m_pDocument, json);
        // TODO: block not added to block table, need to confirm whether add_direct should be called
    }
}

void Doc_plugin_interface::addTextStyle(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmTextStyle* textStyle = m_jsonConverter->jsonToTextStyle(m_pDocument, json);
		if (textStyle)
		{
			m_pDocument->getTextStyleTable()->add_direct(textStyle);
		}
	}
}

void Doc_plugin_interface::addDimStyle(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmDimensionStyle* dimStyle = m_jsonConverter->jsonToDimensionStyle(m_pDocument, json);
		if (dimStyle)
		{
			m_pDocument->getDimStyleTable()->add_direct(dimStyle);
		}
	}
}

void Doc_plugin_interface::addPoint(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmPoint* entity = m_jsonConverter->jsonToPoint(m_pDocument, json);
		addEntity(entity);
	}
}

void Doc_plugin_interface::addLine(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmLine* entity = m_jsonConverter->jsonToline(m_pDocument, json);
		addEntity(entity);
	}
}

void Doc_plugin_interface::addCircle(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmCircle* entity = m_jsonConverter->jsonToCircle(m_pDocument, json);
		addEntity(entity);
	}
}

void Doc_plugin_interface::addHatch(const nlohmann::json& json)
{
    if (m_pDocument)
    {
        DmHatch* entity = m_jsonConverter->jsonToHatch(m_pDocument, json);
        addEntity(entity);
    }
}

void Doc_plugin_interface::addEllipse(const nlohmann::json& json)
{
    if (m_pDocument)
    {
        DmEllipse* entity = m_jsonConverter->jsonToEllipse(m_pDocument, json);
        addEntity(entity);
    }
}

void Doc_plugin_interface::addArc(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmArc* entity = m_jsonConverter->jsonToArc(m_pDocument, json);
		addEntity(entity);
	}
}

void Doc_plugin_interface::addRay(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmRay* entity = m_jsonConverter->jsonToRay(m_pDocument, json);
		addEntity(entity);
	}
}

void Doc_plugin_interface::addXline(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmXline* entity = m_jsonConverter->jsonToXline(m_pDocument, json);
		addEntity(entity);
	}
}

void Doc_plugin_interface::addPolyline(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmPolyline* entity = m_jsonConverter->jsonToPolyline(m_pDocument, json);
		addEntity(entity);
	}
}

void Doc_plugin_interface::addSpline(const nlohmann::json& json)
{
	if (m_pDocument)
	{
        // Spline
        DmSpline* entity = m_jsonConverter->jsonToSpline(m_pDocument, json);
        addEntity(entity);
	}
}

void Doc_plugin_interface::addSolid(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmSolid* entity = m_jsonConverter->jsonToSolid(m_pDocument, json);
		addEntity(entity);
	}
}

void Doc_plugin_interface::addText(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmText* entity = m_jsonConverter->jsonToText(m_pDocument, json);
		addEntity(entity);
	}
}

void Doc_plugin_interface::addMText(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmMText* entity = m_jsonConverter->jsonToMText(m_pDocument, json);
		addEntity(entity);
	}
}

void Doc_plugin_interface::addAttributeDefinition(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmAttributeDefinition* entity = m_jsonConverter->jsonToAttributeDefinition(m_pDocument, json);
		addEntity(entity);
	}
}

void Doc_plugin_interface::addDimLinear(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmDimLinear* entity = m_jsonConverter->jsonToDimLinear(m_pDocument, json);
		addEntity(entity);
	}
}

void Doc_plugin_interface::addDimAligned(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmDimAligned* entity = m_jsonConverter->jsonToDimAligned(m_pDocument, json);
		addEntity(entity);
	}
}

void Doc_plugin_interface::addDimAngular(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmDimAngular* entity = m_jsonConverter->jsonToDimAngular(m_pDocument, json);
		addEntity(entity);
	}
}

void Doc_plugin_interface::addDimRadial(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmDimRadial* entity = m_jsonConverter->jsonToDimRadial(m_pDocument, json);
		addEntity(entity);
	}
}

void Doc_plugin_interface::addDimDiametric(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmDimDiametric* entity = m_jsonConverter->jsonToDimDiametric(m_pDocument, json);
		addEntity(entity);
	}
}

void Doc_plugin_interface::addLeader(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmLeader* entity = m_jsonConverter->jsonToDimLeader(m_pDocument, json);
		addEntity(entity);
	}
}

void Doc_plugin_interface::addBlockReference(const nlohmann::json& json)
{
	if (m_pDocument)
	{
		DmBlockReference* entity = m_jsonConverter->jsonToBlockReference(m_pDocument, json);
		addEntity(entity);
	}
}

QList<nlohmann::json> Doc_plugin_interface::getLineTypes()
{
	auto jsons = QList<nlohmann::json>();
	const auto& lintypes = m_pDocument->getLineTypeTable();
	for (const auto& linetype : *lintypes)
	{
		auto json = m_jsonConverter->lineTypeToJson(linetype);
		jsons.append(json);
	}
	return jsons;
}

QList<nlohmann::json> Doc_plugin_interface::getLayers()
{
	auto jsons = QList<nlohmann::json>();
	const auto& layers = m_pDocument->getLayerTable();
	for (const auto& layer : *layers)
	{
		auto json = m_jsonConverter->layerToJson(layer);
		jsons.append(json);
	}

	return jsons;
}

QList<nlohmann::json> Doc_plugin_interface::getBlockTableRecords()
{
	auto jsons = QList<nlohmann::json>();
	const auto& blockTableRecords = m_pDocument->getBlockTable();
	for (const auto& block : *blockTableRecords)
	{
		auto json = m_jsonConverter->blockTableRecordToJson(block);
		jsons.append(json);
	}

	return jsons;
}

QList<nlohmann::json> Doc_plugin_interface::getTextstyles()
{
	auto jsons = QList<nlohmann::json>();
	const auto& styles = m_pDocument->getTextStyleTable();
	for (const auto& style : *styles)
	{
		auto json = m_jsonConverter->textStyleToJson(style);
		jsons.append(json);
	}

	return jsons;
}

QList<nlohmann::json> Doc_plugin_interface::getDimstyles()
{
	auto jsons = QList<nlohmann::json>();
	const auto& styles = m_pDocument->getDimStyleTable();
	for (const auto& style : *styles)
	{
		auto json = m_jsonConverter->dimStyleToJson(style);
		jsons.append(json);
	}

	return jsons;
}

QList<nlohmann::json> Doc_plugin_interface::getAllEntities()
{
	auto jsons = QList<nlohmann::json>();
	auto table = m_pDocument->getEntityTable();
	for (auto e : *table)
	{
		nlohmann::json json;
		auto entType = e->getEntityType();
		switch (entType)
		{
		case DM::EntityAttribute:
			break;
		case DM::EntityAttributeDefinition:
			json = m_jsonConverter->attributeDefinitionToJson((DmAttributeDefinition*)e);
			break;
		case DM::EntityContainer:
			break;
		case DM::EntityBlockReference:
			json = m_jsonConverter->blockReferenceToJson((DmBlockReference*)e);
			break;
		case DM::EntityPoint:
			json = m_jsonConverter->pointToJson((DmPoint*)e);
			break;
		case DM::EntityLine:
			json = m_jsonConverter->lineToJson((DmLine*)e);
			break;
		case DM::EntityPolyline:
			json = m_jsonConverter->polylineToJson((DmPolyline*)e);
			break;
		case DM::EntityArc:
			json = m_jsonConverter->arcToJson((DmArc*)e);
			break;
		case DM::EntityCircle:
			json = m_jsonConverter->circleToJson((DmCircle*)e);
			break;
		case DM::EntityEllipse:
			json = m_jsonConverter->ellipseToJson((DmEllipse*)e);
			break;
		case DM::EntitySolid:
			json = m_jsonConverter->solidToJson((DmSolid*)e);
			break;
		case DM::EntityConstructionLine:
			break;
		case DM::EntityMText:
			json = m_jsonConverter->mtextToJson((DmMText*)e);
			break;
		case DM::EntityText:
			json = m_jsonConverter->textToJson((DmText*)e);
			break;
		case DM::EntityDimAligned:
			json = m_jsonConverter->dimAlignedToJson((DmDimAligned*)e);
			break;
		case DM::EntityDimLinear:
			json = m_jsonConverter->dimLinearToJson((DmDimLinear*)e);
			break;
		case DM::EntityDimRadial:
			json = m_jsonConverter->dimRadialToJson((DmDimRadial*)e);
			break;
		case DM::EntityDimDiametric:
			json = m_jsonConverter->dimDiametricToJson((DmDimDiametric*)e);
			break;
		case DM::EntityDimAngular:
			json = m_jsonConverter->dimAngularToJson((DmDimAngular*)e);
			break;
		case DM::EntityDimLeader:
			json = m_jsonConverter->dimLeaderToJson((DmLeader*)e);
			break;
		case DM::EntityHatch:
			json = m_jsonConverter->hatchToJson((DmHatch*)e);
			break;
		case DM::EntityImage:
			break;
		case DM::EntitySpline:
			json = m_jsonConverter->splineToJson((DmSpline*)e);
			break;
		case DM::EntityRay:
			json = m_jsonConverter->rayToJson((DmRay*)e);
			break;
		case DM::EntityXline:
			json = m_jsonConverter->xLineToJson((DmXline*)e);
			break;
		default:
			continue;
			break;
        }
		jsons.append(json);
	}
	return jsons;
}

void Doc_plugin_interface::setActiveLayer(const std::wstring& layerName)
{
	if (m_pDocument)
	{
		QString layer = QString::fromStdWString(layerName);
        m_pDocument->getLayerTable()->activate_direct(layer);
	}
}

void Doc_plugin_interface::setActiveTextStyle(const std::wstring& styleName)
{
	if (m_pDocument)
	{
		QString style = QString::fromStdWString(styleName);
        auto s = m_pDocument->getTextStyleTable()->find(style);
		m_pDocument->getTextStyleTable()->activate_direct(s);
	}
}

void Doc_plugin_interface::setActiveDimStyle(const std::wstring& styleName)
{
	if (m_pDocument)
	{
		QString styleStr = QString::fromStdWString(styleName);
        auto style = m_pDocument->getDimStyleTable()->find(styleStr);
        m_pDocument->getDimStyleTable()->activate_direct(style);
	}
}

std::wstring Doc_plugin_interface::getActiveLayer()
{
	if (m_pDocument)
	{
		return m_pDocument->getLayerTable()->getActive()->getName().toStdWString();
	}
	return L"";
}

std::wstring Doc_plugin_interface::getActiveTextStyle()
{
	if (m_pDocument)
	{
		return m_pDocument->getTextStyleTable()->getActive()->getName().toStdWString();
	}
	return L"";
}

std::wstring Doc_plugin_interface::getActiveDimStyle()
{
	if (m_pDocument)
	{
		return m_pDocument->getDimStyleTable()->getActive()->getName().toStdWString();
	}
	return L"";
}

int Doc_plugin_interface::getMeasurement()
{
	return m_pDocument->getVariableInt("MEASUREMENT", 0);
}

void Doc_plugin_interface::setMeasurement(int m)
{
	m_pDocument->addVariable("MEASUREMENT", m, m);
}

void Doc_plugin_interface::zoomAutoView()
{
	ApplicationWindow* appWindow = ApplicationWindow::getAppWindow();
	appWindow->getDocumentView()->getDocument()->regenerate();
	appWindow->getDocumentView()->zoomAuto();
}

void Doc_plugin_interface::updateAppWindow()
{
	ApplicationWindow* appWindow = ApplicationWindow::getAppWindow();
	appWindow->getTabDrawWidget()->tabChangeEvent();
}

void Doc_plugin_interface::setFormatType(const std::string& formatType)
{
	if (m_pDocument)
	{
		m_pDocument->setFormatType(QString::fromStdString(formatType));
	}
}

std::string Doc_plugin_interface::getFormatType()
{
	if (m_pDocument)
	{
		return m_pDocument->getFormatType().toStdString();
	}

	return "";
}

void Doc_plugin_interface::setFileName(const std::wstring& filename)
{
	if (m_pDocument)
	{
		m_pDocument->setFilename(QString::fromStdWString(filename));
	}
}

std::wstring Doc_plugin_interface::getFileName()
{
	if (m_pDocument)
	{
		return m_pDocument->getFilename().toStdWString();
	}

	return L"";
}

void Doc_plugin_interface::addEntity(DmEntity* entity)
{
	if (entity)
	{
		m_pDocument->getEntityTable()->add_direct(entity);
	}
}
