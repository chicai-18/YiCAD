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
/// @brief GuiDialogFactoryInterface 的 Qt 实现

#include "UIDialogFactory.h"

#include <QMessageBox>
#include <QString>

#include "ApplicationWindow.h"
#include "DmDocument.h"
#include "DmVector.h"
#include "IExclusiveCommand.h"
#include "UIBottomWidget.h"
#include "UICommandWidget.h"
#include "UISnapMiddleOptions.h"
#include "UITabDrawWidget.h"

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

QString UIDialogFactory::requestUntitledDocumentName(DmDocument* document)
{
	SingleTabDrawDataRibbon* drawData = ApplicationWindow::getAppWindow()->getTabDrawWidget()->getTabDrawDataOfDocument(document);
	return drawData ? drawData->name : QString();
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

