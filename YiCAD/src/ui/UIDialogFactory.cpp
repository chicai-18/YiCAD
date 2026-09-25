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
#include <QString>
#include <QInputDialog>

#include "DmPatternList.h"
#include "DmSettings.h"
#include "DmSystem.h"
#include "IExclusiveCommand.h"
#include "DmDocument.h"
#include "DmHatch.h"
#include "DmDimLinear.h"
#include "ApplicationWindow.h"
#include "UITabDrawWidget.h"
#include "Fileio.h"


#include "UICommandWidget.h"
#include "UIDlgArc.h"
#include "UIDlgCircle.h"
#include "UIDlgDefineAttribute.h"
#include "UIDlgEllipse.h"
#include "UIDlgHatch.h"
#include "UIDlgImage.h"
#include "UIDlgInsert.h"
#include "UIDlgLine.h"

#include "UIDlgPoint.h"
#include "UIDlgPolyline.h"
#include "UIDlgSpline.h"
#include "UIDlgText.h"
#include "UISnapMiddleOptions.h"
#include "DmBlockTable.h"
#include "DmVector.h"
#include "Debug.h"
#include "UIBottomWidget.h"
#include "Transaction.h"

/// @brief Constructor
/// @param parent Pointer to parent widget which can host dialogs.
/// @param ow Pointer to widget that can host option widgets.
UIDialogFactory::UIDialogFactory(QWidget* parent, QWidget* ow)
	: GuiDialogFactoryInterface()
	, parent(parent)
{
	setOptionWidget(ow);
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

void UIDialogFactory::setOptionWidget(QWidget* ow)
{
	optionWidget = ow;
}

// Shows a message dialog.
void UIDialogFactory::requestWarningDialog(const QString& warning)
{
	QMessageBox::information(parent, QMessageBox::tr("Warning"), warning, QMessageBox::Ok);
}

bool UIDialogFactory::requestConfirmDialog(const QString& title, const QString& message)
{
	return QMessageBox::critical(parent, title, message, QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok;
}

DialogAnswer UIDialogFactory::requestYesNoCancelDialog(const QString& title, const QString& message)
{
	// 父窗口为空，与原块编辑 Action 里的 QMessageBox::question 一致
	switch (QMessageBox::question(nullptr, title, message, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel))
	{
	case QMessageBox::Yes:
		return DialogAnswer::Yes;
	case QMessageBox::No:
		return DialogAnswer::No;
	default:
		return DialogAnswer::Cancel;
	}
}

DmDocument* UIDialogFactory::requestActiveDocument()
{
	return ApplicationWindow::getAppWindow()->getDocument();
}

QString UIDialogFactory::requestUntitledDocumentName(DmDocument* document)
{
	SingleTabDrawDataRibbon* drawData = ApplicationWindow::getAppWindow()->getTabDrawWidget()->getTabDrawDataOfDocument(document);
	return drawData ? drawData->name : QString();
}

bool UIDialogFactory::requestFileExport(DmDocument& document, const QString& file, const QString& formatType)
{
	return FileIO::instance()->fileExport(document, file, formatType);
}

bool UIDialogFactory::requestFileImport(DmDocument& document, const QString& file)
{
	return FileIO::instance()->fileImport(document, file);
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

void UIDialogFactory::requestCommandOptions(IExclusiveCommand* command, bool on, bool update)
{
	if (!command)
	{
		return;
	}
	// 选项条都随命令注册在 CommandRegistry（CommandInfo::commandOptionsFactory）；原先内置命令
	// 按命令 ID 分派到这里的 request*Options，随内置命令拆进扩展删除（迁移计划 9.4 节）
	const CommandRegistry& registry = CommandRegistry::instance();
	if (ExclusiveCommandOptionsFactory factory = registry.commandOptionsFactory(command->commandId()))
	{
		showOptions(m_pRegisteredOptions, [&](QWidget* parent) { return factory(parent, command, update); }, on,
		            registry.commandOptionsHeight(command->commandId()));
	}
}

void UIDialogFactory::requestEditModeOptions(const std::function<QWidget*(QWidget*)>& build, bool on)
{
	showOptions(m_pEditModeOptions, build, on);
}

void UIDialogFactory::showOptions(QPointer<QWidget>& slot, const std::function<QWidget*(QWidget*)>& build, bool on,
                                  int height)
{
	if (!optionWidget)
	{
		return;
	}
	if (slot)
	{
		delete slot;
		optionWidget->hide();
	}
	if (on && build)
	{
		slot = build(optionWidget);
		if (slot)
		{
			slot->show();
			optionWidget->resize(slot->width(), height);
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

