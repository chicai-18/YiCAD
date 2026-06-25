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

/// @file UIDialogFactory.cpp
/// @brief 对话框工厂类，集中管理所有实体编辑对话框、选项工具栏和文件选择对话框的创建与生命周期

#include "UIDialogFactory.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QImageReader>
#include <QString>
#include <QFileDialog>
#include <QToolBar>
#include <QRegularExpression>
#include <QInputDialog>

#include "DmPatternList.h"
#include "DmSettings.h"
#include "DmSystem.h"
#include "ActionInterface.h"
#include "DmDocument.h"
#include "DmHatch.h"
#include "DmDimLinear.h"

#include "ActionDimLinear.h"

#include "UIArcOptions.h"
#include "UIArcTangentialOptions.h"
#include "UIBevelOptions.h"
#include "UIBlockDialog.h"
#include "UIBlockEditOptions.h"
#include "UIDlgEditAttributes.h"
#include "UICircleTan2Options.h"
#include "UICloudLineOptions.h"
#include "UICommandWidget.h"
#include "UIDimLinearOptions.h"
#include "UIDlgArc.h"
#include "UIDlgCircle.h"
#include "UIDlgDefineAttribute.h"
#include "UIDlgEllipse.h"
#include "UIDlgHatch.h"
#include "UIDlgImage.h"
#include "UIDlgInsert.h"
#include "UIDlgLine.h"
#include "UIDlgOptionsDrawing.h"
#include "UIDlgOptionsGeneral.h"

#include "UIDlgPoint.h"
#include "UIDlgPolyline.h"
#include "UIDlgSpline.h"
#include "UIDlgText.h"
#include "UIImageOptions.h"
#include "UIInsertOptions.h"
#include "UILayerDialog.h"
#include "UILineBisectorOptions.h"
#include "UILineOptions.h"
#include "UILinePolygonOptions.h"
#include "UIModifyOffsetOptions.h"
#include "UIMTextOptions.h"
#include "UIRoundOptions.h"
#include "UISnapDistOptions.h"
#include "UISnapMiddleOptions.h"
#include "UISplineOptions.h"
#include "UITextOptions.h"
#include "UIPolylineOptions.h"
#include "DmBlockTable.h"
#include "UISnapMiddleOptions.h"
#include "UISnapDistOptions.h"
#include "DmVector.h"
#include "Debug.h"
#include "UIBottomWidget.h"
#include "UIDlgTextStyle.h"
#include "UIDlgDimensionStyle.h"
#include "UIDlgDimensionStyleMgr.h"
#include "UIDlgLineType.h"
#include "Transaction.h"

/// @brief Constructor
/// @param parent Pointer to parent widget which can host dialogs.
/// @param ow Pointer to widget that can host option widgets.
UIDialogFactory::UIDialogFactory(QWidget* parent, QWidget* ow)
	: GuiDialogFactoryInterface()
	, parent(parent)
	, m_pLineAngleOptions(nullptr)
{
	setOptionWidget(ow);

	bottomWidget = nullptr;
	m_pCommandWidget = nullptr;
	polylineEquidistantOptions = nullptr;
	m_pSnapMiddleOptions = nullptr;
	m_pSnapDistOptions = nullptr;
	m_pModifyOffsetOptions = nullptr;
	m_pPrintPreviewOptions = nullptr;
	m_pDimensionStyleMgr = nullptr;
	m_pDimensionStyle = nullptr;
	m_pTextStyle = nullptr;
	m_pActionHandler = nullptr;
	m_pTableStyle = nullptr;
	m_pTableStyleMgr = nullptr;
}

UIDialogFactory::~UIDialogFactory()
{
}

void UIDialogFactory::setBottomWidget(UIBottomWindow* bw)
{
	bottomWidget = bw;
}

void UIDialogFactory::setCommandWidget(UICommandWidget* command)
{
	m_pCommandWidget = command;
}

void UIDialogFactory::setActionHandle(UIActionHandler* handle)
{
	m_pActionHandler = handle;
}

void UIDialogFactory::setOptionWidget(QWidget* ow)
{
	optionWidget = ow;
}

// Shows a message dialog.
void UIDialogFactory::requestWarningDialog(const QString& warning)
{
	QMessageBox::information(parent, QMessageBox::tr("Warning"), warning, QMessageBox::Ok);
}

/// @brief Shows a dialog for adding a layer. Doesn't add the layer.This is up to the caller.
/// @return a pointer to the newly created layer that should be added.
DmLayer* UIDialogFactory::requestNewLayerDialog(DmLayerTable* layerTable)
{
	DmLayer* layer = nullptr;

	QString layer_name;
	QString newLayerName;
	if (nullptr != layerTable)
	{
		layer_name = layerTable->getActive()->getName();
		if (layer_name.isEmpty() || !layer_name.compare("0"))
		{
			layer_name = QObject::tr("Level");
		}
		newLayerName = layer_name;

		// 从当前图层名中匹配【基本图层名+数字】，取出数字往上累加获得新图层名
		QString sBaseLayerName(layer_name);
		QString sNumLayerName;
		int nlen = 1;
		int i = 0;
		QRegularExpression re("^(.*\\D+|)(\\d+)$");
		QRegularExpressionMatch match(re.match(layer_name));
		if (match.hasMatch())
		{
			sBaseLayerName = match.captured(1);
			if (1 < match.lastCapturedIndex())
			{
				sNumLayerName = match.captured(2);
				nlen = sNumLayerName.length();
				i = sNumLayerName.toInt();
			}
		}

		do
		{
			newLayerName = QString("%1%2").arg(sBaseLayerName).arg(++i, nlen, 10, QChar('0'));
		} while (layerTable->find(newLayerName));
	}

	// Layer for parameter livery
	layer = new DmLayer(newLayerName);
    layer->setDocument(layerTable->getDocument());
	UILayerDialog dlg(parent, "Layer Dialog");
	dlg.setLayer(layer);
	dlg.setLayerTable(layerTable);
	dlg.getQLineEdit()->selectAll();
	if (dlg.exec())
	{
		dlg.updateLayer();
	}
	else
	{
		delete layer;
		layer = nullptr;
	}

	return layer;
}

/// @brief Shows a dialog for editing a layer. A new layer is created and returned. Modifying the layer is up to the caller.
/// @return A pointer to a new layer with the changed attributes or nullptr if the dialog was cancelled.
DmLayer* UIDialogFactory::requestEditLayerDialog(DmLayerTable* layerTable)
{
	DmLayer* layer = nullptr;

	if (!layerTable)
	{
		return nullptr;
	}

	// Layer for parameter livery
	if (layerTable->getActive())
	{
		layer = layerTable->getActive()->clone();

		UILayerDialog dlg(parent, QMessageBox::tr("Layer Dialog"));
		dlg.setLayer(layer);
		dlg.setLayerTable(layerTable);
		dlg.setEditLayer(true);
		if (dlg.exec())
		{
			dlg.updateLayer();
		}
		else
		{
			delete layer;
			layer = nullptr;
		}
	}

	return layer;
}

/// @brief Shows a dialog for adding a block. Doesn't add the block. This is up to the caller.
/// @return a pointer to the newly created block that should be added.
DmBlockData UIDialogFactory::requestNewBlockDialog(DmBlockTable* blockTable)
{
	DmBlockData ret;
	ret = DmBlockData("", DmVector(false), false);

	if (!blockTable)
	{
		return ret;
	}

	UIBlockDialog dlg(parent);
	dlg.setBlockList(blockTable);
	if (dlg.exec())
	{
		ret = dlg.getBlockData();
	}

	return ret;
}

bool UIDialogFactory::requestBlockEditAttributeDialog(const QString& blkName, const std::list<DmAttributeDefinition*>& attrDefs, std::list< DmAttribute*>& attrs)
{
	UIDlgEditAttributes dlg(parent);
	dlg.setData(blkName,attrDefs);
	bool ret = dlg.exec();
	attrs = dlg.getAttributes();
	return ret;
}

bool UIDialogFactory::requestDefineAttributesDialog(DmAttributeDefinition* attrDef)
{
	if (!attrDef)
	{
		return false;
	}

	UIDlgDefineAttribute dlg(parent);
	dlg.setAttributeDefinition(*attrDef, true);
	if (dlg.exec())
	{
		dlg.updateAttributeDefinition();
		return true;
	}

	return false;
}

/// @brief Shows a dialog for choosing a file name. Opening the file is up to the caller.
/// @return File name with path and extension to determine the file type or an empty string if the dialog was cancelled.
QString UIDialogFactory::requestImageOpenDialog()
{
	QString strFileName = "";

	// read default settings:
	DMSETTINGS->beginGroup("/Paths");
	QString defDir = DMSETTINGS->readEntry("/OpenImage", DMSYSTEM->getHomeDir());
	QString defFilter = DMSETTINGS->readEntry("/ImageFilter", "");
	DMSETTINGS->endGroup();

	QStringList filters;
	QString all = "";
	bool haveJpeg = false;
	for (const QByteArray& format : QImageReader::supportedImageFormats())
	{
		if (format.toUpper() == "JPG" || format.toUpper() == "JPEG")
		{
			if (!haveJpeg)
			{
				haveJpeg = true;
				filters.append("jpeg (*.jpeg *.jpg)");
				all += " *.jpeg *.jpg";
			}
		}
		else
		{
			filters.append(QString("%1 (*.%1)").arg(QString(format)));
			all += QString(" *.%1").arg(QString(format));
		}
	}
	QString strAllImageFiles = QObject::tr("All Image Files (%1)").arg(all);
	filters.append(strAllImageFiles);
	filters.append(QObject::tr("All Files (*.*)"));

	QFileDialog fileDlg(nullptr, "");
	fileDlg.setModal(true);
	fileDlg.setFileMode(QFileDialog::ExistingFile);
	fileDlg.setWindowTitle(QObject::tr("Open Image"));
	fileDlg.setDirectory(defDir);
	fileDlg.setNameFilters(filters);
	if (defFilter.isEmpty())
	{
		defFilter = strAllImageFiles;
	}
	fileDlg.selectNameFilter(defFilter);

	if (QDialog::Accepted == fileDlg.exec())
	{
		QStringList strSelectedFiles = fileDlg.selectedFiles();
		if (!strSelectedFiles.isEmpty())
		{
			strFileName = strSelectedFiles.first();
		}

		// store new default settings:
		DMSETTINGS->beginGroup("/Paths");
		DMSETTINGS->writeEntry("/OpenImage", QFileInfo(strFileName).absolutePath());
		DMSETTINGS->writeEntry("/ImageFilter", fileDlg.selectedNameFilter());
		DMSETTINGS->endGroup();
	}

	return strFileName;
}

void UIDialogFactory::requestOptions(ActionInterface* action, bool on, bool update)
{
	if (!action)
	{
		return;
	}

	switch (action->getEntityType())
	{
	case DM::ActionDrawLine:
		requestLineOptions(action, on);
		break;

	case DM::ActionDrawPolyline:
		requestPolylineOptions(action, on, update);
		break;

	case DM::ActionCloudLineRectangle:
	case DM::ActionCloudLinePolygon:
	case DM::ActionCloudLineFree:
		requestCloudLineOptions(action, on, update);
		break;

	case DM::ActionDrawLineBisector:
		requestLineBisectorOptions(action, on, update);
		break;

	case DM::ActionDrawLinePolygonCenCor:
		requestLinePolygonOptions(action, on, update);
		break;

	case DM::ActionDrawArc:
		requestArcOptions(action, on, update);
		break;

	case DM::ActionDrawArcTangential:
		requestArcTangentialOptions(action, on, update);
		break;

	case DM::ActionDrawCircleTan2:
		requestCircleTan2Options(action, on, update);
		break;

	case DM::ActionDrawSpline:
	case DM::ActionDrawSplinePoints:
		requestSplineOptions(action, on, update);
		break;

	case DM::ActionDrawText:
		requestTextOptions(action, on, update);
		break;

	case DM::ActionBlocksInsert:
		requestInsertOptions(action, on, update);
		break;

	case DM::ActionBlocksEdit:
	case DM::ActionBlocksEditNoSelect:
		requestBlockEditOptions(action, on);
		break;

	case DM::ActionDrawImage:
		requestImageOptions(action, on, update);
		break;

	case DM::ActionDimLinear:
			requestDimLinearOptions(action, on, update);
		break;

	case DM::ActionModifyBevel:
		requestBevelOptions(action, on, update);
		break;

	case DM::ActionModifyRound:
		requestRoundOptions(action, on, update);
		break;
	default:
		break;
	}
}

// Shows a widget for options for the action: "print preview"
//void UIDialogFactory::requestPrintPreviewOptions(ActionInterface* action, bool on, bool update)
//{
//	if (!on)
//	{
//		if (m_pPrintPreviewOptions)
//		{
//			delete m_pPrintPreviewOptions;
//			m_pPrintPreviewOptions = nullptr;
//			optionWidget->hide();
//		}
//		return;
//	}
//	if (optionWidget)
//	{
//		if (!m_pPrintPreviewOptions)
//		{
//			m_pPrintPreviewOptions = new UIPrintPreviewOptions(optionWidget);
//			m_pPrintPreviewOptions->setAction(action, false);		}
//		if (update)
//		{
//			m_pPrintPreviewOptions->setAction(action, update);
//		}
//		m_pPrintPreviewOptions->show();
//		optionWidget->resize(m_pPrintPreviewOptions->width(), 23);
//		optionWidget->show();
//	}
//
//}

// Shows a widget for options for the action: "draw line"
void UIDialogFactory::requestLineOptions(ActionInterface* action, bool on)
{
	if (optionWidget)
	{
		static UILineOptions* toolWidget = nullptr;
		if (toolWidget)
		{
			delete toolWidget;
			toolWidget = nullptr;
			optionWidget->hide();
		}
		if (on)
		{
			toolWidget = new UILineOptions(optionWidget);
			toolWidget->setAction(action);
			toolWidget->show();
			optionWidget->resize(toolWidget->width(), 23);
			optionWidget->show();
		}
	}
}

// Shows a widget for options for the action: "draw polyline"
void UIDialogFactory::requestPolylineOptions(ActionInterface* action, bool on, bool update)
{
	if (optionWidget)
	{
		static UIPolylineOptions* toolWidget = nullptr;
		if (toolWidget)
		{
			delete toolWidget;
			toolWidget = nullptr;
			optionWidget->hide();
		}
		if (on)
		{
			toolWidget = new UIPolylineOptions(optionWidget);
			toolWidget->setAction(action, update);
			toolWidget->show();
			optionWidget->resize(toolWidget->width(), 23);
			optionWidget->show();
		}
	}
}

// Shows a widget for options for the action: "line angle"
void UIDialogFactory::requestLineBisectorOptions(ActionInterface* action, bool on, bool update)
{
	if (optionWidget)
	{
		static UILineBisectorOptions* toolWidget = nullptr;
		if (toolWidget)
		{
			delete toolWidget;
			toolWidget = nullptr;
			optionWidget->hide();
		}
		if (on)
		{
			toolWidget = new UILineBisectorOptions(optionWidget);
			toolWidget->setAction(action, update);
			toolWidget->show();
			optionWidget->resize(toolWidget->width(), 23);
			optionWidget->show();
		}
	}
}

// Shows a widget for options for the action: "draw polygon"
void UIDialogFactory::requestLinePolygonOptions(ActionInterface* action, bool on, bool update)
{
	if (optionWidget)
	{
		static UILinePolygonOptions* toolWidget = nullptr;
		if (toolWidget)
		{
			delete toolWidget;
			toolWidget = nullptr;
			optionWidget->hide();
		}
		if (on)
		{
			toolWidget = new UILinePolygonOptions(optionWidget);
			toolWidget->setAction(action, update);
			toolWidget->show();
			optionWidget->resize(toolWidget->width(), 23);
			optionWidget->show();
		}
	}
}

void UIDialogFactory::requestCloudLineOptions(ActionInterface* action, bool on, bool update)
{
	if (optionWidget)
	{
		static UICloudLineOptions* toolWidget = nullptr;
		if (toolWidget)
		{
			delete toolWidget;
			toolWidget = nullptr;
			optionWidget->hide();
		}
		if (on)
		{
			toolWidget = new UICloudLineOptions(optionWidget);
			toolWidget->setAction(action, update);
			toolWidget->show();
			optionWidget->resize(toolWidget->width(), 23);
			optionWidget->show();
		}
	}
}

// Shows a widget for arc options.
void UIDialogFactory::requestArcOptions(ActionInterface* action, bool on, bool update)
{
	if (optionWidget)
	{
		static UIArcOptions* toolWidget = nullptr;
		if (toolWidget)
		{
			delete toolWidget;
			toolWidget = nullptr;
			optionWidget->hide();
		}
		if (on)
		{
			toolWidget = new UIArcOptions(optionWidget);
			toolWidget->setAction(action, update);
			toolWidget->show();
			optionWidget->resize(toolWidget->width(), 23);
			optionWidget->show();
		}
	}
}

// Shows a widget for tangential arc options.
void UIDialogFactory::requestArcTangentialOptions(ActionInterface* action, bool on, bool /*update*/)
{
	if (optionWidget)
	{
		static UIArcTangentialOptions* toolWidget = nullptr;
		if (toolWidget && !on)
		{
			delete toolWidget;
			toolWidget = nullptr;
			optionWidget->hide();
		}
		if (on)
		{
			bool useUpdate = toolWidget;
			if (!toolWidget)
			{
				toolWidget = new UIArcTangentialOptions(optionWidget);
			}
			toolWidget->setAction(action, useUpdate);
			toolWidget->show();
			optionWidget->resize(toolWidget->width(), 23);
			optionWidget->show();
		}
		arcTangentialOptions = toolWidget;
	}
}

void UIDialogFactory::updateArcTangentialOptions(const double& radius, const bool& lockRadius, const double& angle, const bool& lockAngle)
{
	if (!arcTangentialOptions) return;

	if (!lockRadius)
	{
		arcTangentialOptions->updateRadius(QString::number(radius, 'g', 5));
	}
	if (!lockAngle)
	{
		arcTangentialOptions->updateAngle(QString::number(angle, 'g', 5));
	}
}

// Shows a widget for arc options.
void UIDialogFactory::requestCircleTan2Options(ActionInterface* action, bool on, bool update)
{
	if (optionWidget)
	{
		static UICircleTan2Options* toolWidget = nullptr;
		if (toolWidget)
		{
			delete toolWidget;
			toolWidget = nullptr;
			optionWidget->hide();
		}
		if (on)
		{
			toolWidget = new UICircleTan2Options(optionWidget);
			toolWidget->setAction(action, update);
			toolWidget->show();
			optionWidget->resize(toolWidget->width(), 23);
			optionWidget->show();
		}
	}
}

// Shows a widget for spline options.
void UIDialogFactory::requestSplineOptions(ActionInterface* action, bool on, bool update)
{
	if (optionWidget)
	{
		static UISplineOptions* toolWidget = nullptr;
		if (toolWidget)
		{
			delete toolWidget;
			toolWidget = nullptr;
			optionWidget->hide();
		}
		if (on)
		{
			toolWidget = new UISplineOptions(optionWidget);
			toolWidget->setAction(action, update);
			toolWidget->show();
			optionWidget->resize(toolWidget->width(), 26);
			optionWidget->show();
		}
	}
}

// Shows a widget for text options.
void UIDialogFactory::requestTextOptions(ActionInterface* action, bool on, bool update)
{
	if (optionWidget)
	{
		static UITextOptions* toolWidget = nullptr;
		if (toolWidget)
		{
			delete toolWidget;
			toolWidget = nullptr;
			optionWidget->hide();
		}
		if (on)
		{
			toolWidget = new UITextOptions(optionWidget);
			toolWidget->setAction(action, update);
			toolWidget->show();
			optionWidget->resize(toolWidget->width(), 23);
			optionWidget->show();
		}
	}
}

// Shows a widget for insert options.
void UIDialogFactory::requestInsertOptions(ActionInterface* action, bool on, bool update)
{
	if (optionWidget)
	{
		static UIInsertOptions* toolWidget = nullptr;
		if (toolWidget)
		{
			delete toolWidget;
			toolWidget = nullptr;
			optionWidget->hide();
		}
		if (on)
		{
			toolWidget = new UIInsertOptions(optionWidget);
			toolWidget->setAction(action, update);
			toolWidget->show();
			optionWidget->resize(toolWidget->width(), 23);
			optionWidget->show();
		}
	}
}

// Shows a widget for block edit options.
void UIDialogFactory::requestBlockEditOptions(ActionInterface* action, bool on)
{
	if (optionWidget)
	{
		static UIBlockEditOptions* toolWidget = nullptr;
		if (toolWidget)
		{
			delete toolWidget;
			toolWidget = nullptr;
			optionWidget->hide();
		}
		if (on)
		{
			toolWidget = new UIBlockEditOptions(optionWidget);
			toolWidget->setAction(action);
			toolWidget->show();
			optionWidget->resize(toolWidget->width(), 23);
			optionWidget->show();
		}
	}
}

// Shows a widget for image options.
void UIDialogFactory::requestImageOptions(ActionInterface* action, bool on, bool update)
{
	if (optionWidget)
	{
		static UIImageOptions* toolWidget = nullptr;
		if (toolWidget)
		{
			delete toolWidget;
			toolWidget = nullptr;
			optionWidget->hide();
		}
		if (on)
		{
			toolWidget = new UIImageOptions(optionWidget);
			toolWidget->setAction(action, update);
			toolWidget->show();
			optionWidget->resize(toolWidget->width(), 23);
			optionWidget->show();
		}
	}
}

// Shows a widget for linear dimension options.
void UIDialogFactory::requestDimLinearOptions(ActionInterface* action, bool on, bool update)
{
	if (optionWidget)
	{
		static UIDimLinearOptions* toolWidget = nullptr;
		if (toolWidget)
		{
			delete toolWidget;
			toolWidget = nullptr;
			optionWidget->hide();
		}
		if (on)
		{
			toolWidget = new UIDimLinearOptions(optionWidget);
			toolWidget->setAction(action, update);
			toolWidget->show();
			optionWidget->resize(toolWidget->width(), 23);
			optionWidget->show();
		}
	}
}

// Shows a widget for 'snap to equidistant middle points ' options.
void UIDialogFactory::requestSnapMiddleOptions(int& middlePoints, bool on)
{
	if (!on)
	{
		if (m_pSnapMiddleOptions)
		{
			delete m_pSnapMiddleOptions;
			m_pSnapMiddleOptions = nullptr;
			optionWidget->hide();
		}
		return;
	}
	if (optionWidget)
	{
		if (!m_pSnapMiddleOptions)
		{
			m_pSnapMiddleOptions = new UISnapMiddleOptions(middlePoints, optionWidget);
			m_pSnapMiddleOptions->setMiddlePoints(middlePoints);
		}
		else
		{
			m_pSnapMiddleOptions->setMiddlePoints(middlePoints, false);
		}
		m_pSnapMiddleOptions->show();
		optionWidget->resize(m_pSnapMiddleOptions->width(), 23);
		optionWidget->show();
		
	}
}

// Shows a widget for 'snap to a point with a given distance' options.
void UIDialogFactory::requestSnapDistOptions(double& dist, bool on)
{
	if (!on)
	{
		if (m_pSnapDistOptions)
		{
			delete m_pSnapDistOptions;
			m_pSnapDistOptions = nullptr;
			optionWidget->hide();
		}
		return;
	}
	if (optionWidget)
	{
		if (!m_pSnapDistOptions)
		{
			m_pSnapDistOptions = new UISnapDistOptions(optionWidget);
			m_pSnapDistOptions->setDist(dist);
		}
		else
		{
			m_pSnapDistOptions->setDist(dist, false);
		}
		m_pSnapDistOptions->show();
		optionWidget->resize(m_pSnapDistOptions->width(), 23);
		optionWidget->show();
	}
}

// Shows a widget for beveling options.
void UIDialogFactory::requestBevelOptions(ActionInterface* action, bool on, bool update)
{
	if (optionWidget)
	{
		static UIBevelOptions* toolWidget = nullptr;
		if (toolWidget)
		{
			delete toolWidget;
			toolWidget = nullptr;
			optionWidget->hide();
		}
		if (on)
		{
			toolWidget = new UIBevelOptions(optionWidget);
			toolWidget->setAction(action, update);
			toolWidget->show();
			optionWidget->resize(toolWidget->width(), 23);
			optionWidget->show();
		}
	}
}

// Shows a widget for rounding options.
void UIDialogFactory::requestRoundOptions(ActionInterface* action, bool on, bool update)
{
	if (optionWidget)
	{
		static UIRoundOptions* toolWidget = nullptr;
		if (toolWidget)
		{
			delete toolWidget;
			toolWidget = nullptr;
			optionWidget->hide();
		}
		if (on)
		{
			toolWidget = new UIRoundOptions(optionWidget);
			toolWidget->setAction(action, update);
			toolWidget->show();
			optionWidget->resize(toolWidget->width(), 23);
			optionWidget->show();
		}
	}
}

void UIDialogFactory::requestModifySingleOffsetOptions(double& dist, bool on, bool update)
{
	if (!on)
	{
		if (m_pModifyOffsetOptions)
		{
			delete m_pModifyOffsetOptions;
			m_pModifyOffsetOptions = nullptr;
			optionWidget->hide();
		}
		return;
	}
	if (optionWidget)
	{
		if (!m_pModifyOffsetOptions)
		{
			m_pModifyOffsetOptions = new UIModifyOffsetOptions(optionWidget);
			m_pModifyOffsetOptions->setDist(dist);
		}
		else
		{
			m_pModifyOffsetOptions->setDist(dist, false);
		}
		m_pModifyOffsetOptions->show();
		optionWidget->resize(m_pModifyOffsetOptions->width(), 23);
		optionWidget->show();
	}
}

// Shows a dialog to edit the given entity.
bool UIDialogFactory::requestModifyEntityDialog(DmEntity* entity)
{
	if (!entity)
	{
		return false;
	}

	bool ret = false;

	switch (entity->getEntityType())
	{
	case DM::EntityPoint:
	{
		UIDlgPoint dlg(parent);
		dlg.setPoint(*((DmPoint*)entity));
		if (dlg.exec())
		{
			dlg.updatePoint();
			ret = true;
		}
	}
	break;
	case DM::EntityLine:
	{
		UIDlgLine dlg(parent);
		dlg.setLine(*((DmLine*)entity));
		if (dlg.exec())
		{
			dlg.updateLine();
			ret = true;
		}
	}
	break;
	case DM::EntityArc:
	{
		UIDlgArc dlg(parent);
		dlg.setArc(*((DmArc*)entity));
		if (dlg.exec())
		{
			dlg.updateArc();
			ret = true;
		}
	}
	break;
	case DM::EntityCircle:
	{
		UIDlgCircle dlg(parent);
		dlg.setCircle(*((DmCircle*)entity));
		if (dlg.exec())
		{
			dlg.updateCircle();
			ret = true;
		}
	}
	break;
	case DM::EntityEllipse:
	{
		UIDlgEllipse dlg(parent);
		dlg.setEllipse(*((DmEllipse*)entity));
		if (dlg.exec())
		{
			dlg.updateEllipse();
			ret = true;
		}
	}
	break;
	case DM::EntitySpline:
	{
		UIDlgSpline dlg(nullptr, false);
		dlg.setSpline(*((DmSpline*)entity));
		if (dlg.exec())
		{
			dlg.updateSpline();
			ret = true;
		}
	}
	break;
	case DM::EntityBlockReference:
	{
		UIDlgInsert dlg(parent);
		dlg.setInsert(*((DmBlockReference*)entity));
		if (dlg.exec())
		{
			dlg.updateInsert();
			ret = true;
			entity->update();
		}
	}
	break;
	case DM::EntityAttributeDefinition:
	{
		UIDlgDefineAttribute dlg(parent);
		dlg.setAttributeDefinition(*static_cast<DmAttributeDefinition *>(entity), false);
		if (dlg.exec())
		{
			dlg.updateAttributeDefinition();
			ret = true;
		}
	}
	break;
	case DM::EntityDimAligned:
	case DM::EntityDimAngular:
	case DM::EntityDimDiametric:
	case DM::EntityDimRadial:
	case DM::EntityDimLinear:
	{
		DmDimension* dim = static_cast<DmDimension*>(entity);
		QString text = dim->getLabel();
		bool ok = false;
		QString newTert = QInputDialog::getText(parent, QObject::tr("Modify dimension text"), QObject::tr("New dimension text:"), QLineEdit::Normal, text, &ok);
		if (ok)
		{
            Transaction t(QObject::tr("Modify dimension").toStdString(), dim->getDocument());
            t.start();
            dim->getDocument()->getEntityTable()->startModify(dim);
			dim->setLabel(newTert);
			dim->update();
            t.commit();
			ret = true;
		}
	}
	break;
	case DM::EntityText:
	{
		UIDlgText dlg(parent);
		dlg.setText(*((DmText*)entity), false);
		if (dlg.exec())
		{
			dlg.updateText();
			ret = true;
		}
	}
	break;
    case DM::EntityHatch:
    {
        UIDlgHatch dlg(parent);
        dlg.setHatch(*((DmHatch*)entity), false);
        if (dlg.exec())
        {
            dlg.updateHatch();
            ret = true;
        }
    }
        break;
	case DM::EntityPolyline:
	{
		UIDlgPolyline dlg(parent);
		dlg.setPolyline(*((DmPolyline*)entity));
		if (dlg.exec())
		{
			dlg.updatePolyline();
			ret = true;
		}
	}
	break;
	case DM::EntityImage:
	{
		UIDlgImage dlg(parent);
		dlg.setImage(*((DmImage*)entity));
		if (dlg.exec())
		{
			dlg.updateImage();
			ret = true;
		}
	}
	break;
	default:
		break;
	}

	return ret;
}

// Shows a dialog to edit the attributes of the given text entity.
bool UIDialogFactory::requestTextDialog(DmText* text)
{
	if (!text)
	{
		return false;
	}

	UIDlgText dlg(parent);
	dlg.setText(*text, true);
	if (dlg.exec())
	{
		dlg.updateText();
		return true;
	}

	return false;
}


bool UIDialogFactory::requestTextStyleDialog(DmTextStyleTable* textStyleTable, DmDocument* document)
{
	m_pTextStyle = new UIDlgTextStyle(parent, true);
	m_pTextStyle->setStyleList(textStyleTable,document);
	if (m_pTextStyle->exec())
	{
		return true;
	}
	else
	{
		return false;
	}
}

void UIDialogFactory::requestDimStyleMgrDialog(DmDimensionStyleTable* dimStyleTable, DmDocument* document)
{
	m_pDimensionStyleMgr = new UIDlgDimensionStyleMgr(parent, true);
	m_pDimensionStyleMgr->init(dimStyleTable, document);
	m_pDimensionStyleMgr->exec();
}

bool UIDialogFactory::requestDimStyleModifyDialog(DmDimensionStyle* dimStyle, DmDocument* document)
{
	m_pDimensionStyle = new UIDlgDimensionStyle(parent, this);
	m_pDimensionStyle->init(dimStyle, document);
	if (m_pDimensionStyle->exec())
	{
		return true;
	}
	else
	{
		return false;
	}

	// 防止白屏或者黑白屏 刷新控件
	m_pDimensionStyleMgr->update();
}

// Shows a dialog to edit pattern / hatch attributes of the given entity.
bool UIDialogFactory::requestHatchDialog(DmHatch* hatch)
{
	if (!hatch)
	{
		return false;
	}

	UIDlgHatch dlg(parent);
	dlg.setHatch(*hatch, true);
	if (dlg.exec())
	{
		dlg.updateHatch();
		return true;
	}
	return false;
}

// Shows dialog for general application options.
void UIDialogFactory::requestOptionsGeneralDialog()
{
	UIDlgOptionsGeneral dlg(parent);
	dlg.exec();
}

// Shows dialog for drawing options.
void UIDialogFactory::requestOptionsDrawingDialog(DmDocument& document)
{
	UIDlgOptionsDrawing dlg(parent);
	dlg.setDocument(&document);
	dlg.exec();
}



QString UIDialogFactory::requestFileSaveAsDialog(const QString& caption, const QString& dir, const QString& filter, QString* selectedFilter)
{
	return QFileDialog::getSaveFileName(parent, caption, dir, filter, selectedFilter);
}


bool UIDialogFactory::requestLineTypeDialog(DmLineTypeTable* lineTypeTable, DmDocument* document)
{
	m_pLineType = new UIDlgLineType(parent,true);
	m_pLineType->setLineTypeTable(lineTypeTable, document);
	if (m_pLineType->exec()) 
	{
		return true;
	}
	else
	{
		return false;
	}
}

// Called whenever the mouse position changed.
void UIDialogFactory::updateCoordinateWidget(const DmVector& abs, const DmVector& rel, bool updateFormat)
{
	if (bottomWidget)
	{
		bottomWidget->setCoordinates(abs);
	}
}

void UIDialogFactory::updateMouseWidget(const QString& left, const QString& right)
{
	if (m_pCommandWidget)
	{
		m_pCommandWidget->appCmdTempText(left);
	}
}

void UIDialogFactory::updateSelectionWidget(int num)
{
	if (bottomWidget)
	{
		bottomWidget->setSelectNumber(num);
	}
}

// Called when an action needs to communicate 'message' to the user.
void UIDialogFactory::commandMessage(const QString& message)
{
	if (m_pCommandWidget)
	{
		m_pCommandWidget->appCmdTempText(message);
	}
}

/// @brief Converts an extension to a format description.  e.g. "PNG" to "Portable Network Document"
/// @param [in ] ext Extension
/// @return Format description
QString UIDialogFactory::extToFormat(const QString& ext)
{
	QString e = ext.toLower();

	if (e == "bmp")
	{
		return QObject::tr("Windows Bitmap");
	}
	else if (e == "jpeg" || e == "jpg")
	{
		return QObject::tr("Joint Photo document Experts Group");
	}
	else if (e == "gif")
	{
		return QObject::tr("Documents Interchange Format");
	}
	else if (e == "mng")
	{
		return QObject::tr("Multiple-image Network Documents");
	}
	else if (e == "pbm")
	{
		return QObject::tr("Portable Bit Map");
	}
	else if (e == "pgm")
	{
		return QObject::tr("Portable Grey Map");
	}
	else if (e == "png")
	{
		return QObject::tr("Portable Network Document");
	}
	else if (e == "ppm")
	{
		return QObject::tr("Portable Pixel Map");
	}
	else if (e == "xbm")
	{
		return QObject::tr("X Bitmap Format");
	}
	else if (e == "xpm")
	{
		return QObject::tr("X Pixel Map");
	}
	else if (e == "svg")
	{
		return QObject::tr("Scalable Vector Documents");
	}
	else if (e == "bw")
	{
		return QObject::tr("SGI Black & White");
	}
	else if (e == "eps")
	{
		return QObject::tr("Encapsulated PostScript");
	}
	else if (e == "epsf")
	{
		return QObject::tr("Encapsulated PostScript Format");
	}
	else if (e == "epsi")
	{
		return QObject::tr("Encapsulated PostScript Interchange");
	}
	else if (e == "ico")
	{
		return QObject::tr("Windows Icon");
	}
	else if (e == "jp2")
	{
		return QObject::tr("JPEG 2000");
	}
	else if (e == "pcx")
	{
		return QObject::tr("ZSoft Paintbrush");
	}
	else if (e == "pic")
	{
		return QObject::tr("PC Paint");
	}
	else if (e == "rgb" || e == "rgba" || e == "sgi")
	{
		return QObject::tr("SGI-Bilddatei");
	}
	else if (e == "tga")
	{
		return QObject::tr("Targa Image File");
	}
	else if (e == "tif" || e == "tiff")
	{
		return QObject::tr("Tagged Image File Format");
	}
	else
	{
		return ext.toUpper();
	}
}

