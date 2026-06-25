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

/// @file UIBottomWidget.cpp
/// @brief 底部状态栏控件，包含坐标显示、捕捉模式、正交、网格、线宽显示和单位设置

#include "UIBottomWidget.h"

#include <cmath>

#include "UISnapWidget.h"
#include "DmVector.h"
#include "DmDocument.h"
#include "MDIWindow.h"
#include "DmSettings.h"
#include "GuiDocumentView.h"
#include "QByteArray"
#include "GuiDialogFactory.h"
#include <cmath>
#include <QTableView>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QModelIndex>

UIBottomWindow::UIBottomWindow(QWidget* parent)
	: m_pActionHandler(nullptr)
	, m_pWidget(parent)
	, m_pCurrentMdiWindow(nullptr)
	, m_pBottomWidget(nullptr)
	, m_pBottomToolWidget(nullptr)
	, m_pComboBoxWidget(nullptr)
	, m_pNumberLabel(nullptr)
	, m_pOrthogonalBtn(nullptr)
	, m_pViewportWidget(nullptr)
	//, m_pThemeComBoxWidget(nullptr)
	, m_pViewports(nullptr)
	, m_pViewsModel(nullptr)
{
}

UIBottomWindow::~UIBottomWindow()
{
	for (auto item : m_mapUnitBtn)
	{
		delete item.second;
		item.second = nullptr;
	}
	m_mapUnitBtn.clear();

	if (m_pHLayout)
	{
		delete m_pHLayout;
		m_pHLayout = nullptr;
	}
}

void UIBottomWindow::createBottomWindow(UIActionHandler* pActionHandler, MDIWindow* pCurrentMdiWindow)
{
	QWidget* pBottomWidget = new QWidget(m_pWidget);
	pBottomWidget->setAttribute(Qt::WA_DeleteOnClose);
	pBottomWidget->show();
	pBottomWidget->setAutoFillBackground(true);

	QWidget* pViewportWidget = new QWidget(m_pWidget);
	pViewportWidget->setAttribute(Qt::WA_DeleteOnClose);
	pViewportWidget->show();
	pViewportWidget->lower();
	pViewportWidget->hide();
	pViewportWidget->resize(150, 300);

	QWidget* pBottomToolWidget = new QWidget(m_pWidget);
	pBottomToolWidget->setAttribute(Qt::WA_DeleteOnClose);
	pBottomToolWidget->show();
	pBottomToolWidget->lower();
	pBottomToolWidget->hide();
	pBottomToolWidget->resize(100,150);

	QWidget* pComboBoxWidget = new QWidget(m_pWidget);
	pComboBoxWidget->setAttribute(Qt::WA_DeleteOnClose);
	pComboBoxWidget->setAttribute(Qt::WA_StyledBackground, true);
	pComboBoxWidget->setObjectName("unitComboBoxWidget");
	pComboBoxWidget->show();
	pComboBoxWidget->lower();
	pComboBoxWidget->hide();
	pComboBoxWidget->resize(150, 660);

	//QWidget* pThemeComBoxWidget = new QWidget(m_pWidget);
	//pThemeComBoxWidget->setAttribute(Qt::WA_DeleteOnClose);
	//pThemeComBoxWidget->show();
	//pThemeComBoxWidget->lower();
	//pThemeComBoxWidget->hide();
	//pThemeComBoxWidget->resize(150, 50);

	m_pHLayout = new QHBoxLayout(pBottomWidget);

	m_pActionHandler = pActionHandler;
	m_pCurrentMdiWindow = pCurrentMdiWindow;
	m_pBottomWidget = pBottomWidget;
	m_pViewportWidget = pViewportWidget;
	m_pBottomToolWidget = pBottomToolWidget;
	m_pComboBoxWidget = pComboBoxWidget;
	//m_pThemeComBoxWidget = pThemeComBoxWidget;

	createCoordLable();
	createSelectNumberLable();
	createButton();
	createTool();
	createComboBox(); //unit
	//createThemeComboBox(); //theme

	m_pHLayout->addStretch(1);
	m_pHLayout->addWidget(m_pCoordLabel, 0, Qt::AlignVCenter);
	m_pHLayout->addSpacing(5);
	m_pHLayout->addWidget(m_pNumberLabel, 0, Qt::AlignVCenter);
	m_pHLayout->addSpacing(5); 
	//m_pHLayout->addWidget(m_pViewBtn, 0, Qt::AlignVCenter);
	//m_pHLayout->addSpacing(5);
	m_pHLayout->addWidget(m_pOrthogonalBtn, 0, Qt::AlignVCenter);
	m_pHLayout->addSpacing(5); 
	m_pHLayout->addWidget(m_pGridBtn, 0, Qt::AlignVCenter);
	m_pHLayout->addSpacing(5);
	m_pHLayout->addWidget(m_pSnapModeBtn, 0, Qt::AlignVCenter);
	m_pHLayout->addSpacing(5);
	m_pHLayout->addWidget(m_pShowWidthBtn, 0, Qt::AlignVCenter);
	m_pHLayout->addSpacing(5);
	m_pHLayout->addWidget(m_pUnitBtn, 0, Qt::AlignVCenter);
	//m_pHLayout->addSpacing(5);		//屏蔽主题设置
	//m_pHLayout->addWidget(m_pTheme, 0, Qt::AlignVCenter);
	m_pHLayout->setContentsMargins(0, 5, 5, 2);
}

void UIBottomWindow::createCoordLable()
{
	m_pCoordLabel = new QLabel(tr("0.0000 , 0.0000"), m_pBottomWidget);
	m_pCoordLabel->setAttribute(Qt::WA_DeleteOnClose);
	m_pCoordLabel->show();
}

void UIBottomWindow::createSelectNumberLable()
{
	QString text = tr("number:") + " 0";
	m_pNumberLabel = new QLabel(text, m_pBottomWidget);
	m_pNumberLabel->setAttribute(Qt::WA_DeleteOnClose);
	m_pNumberLabel->show();
}

void UIBottomWindow::createButton()
{
	//// 视口列表
	//m_pViewBtn = new QToolButton();
	//m_pViewBtn->setAttribute(Qt::WA_DeleteOnClose);
	//m_pViewBtn->setIcon(QIcon(":/ribbon/viewport/save_viewport.svg"));
	//m_pViewBtn->setFixedSize(20, 20);
	//m_pViewBtn->setIconSize(QSize(16, 16));
	//m_pViewBtn->setToolTip(QObject::tr("Viewports"));
	//connect(m_pViewBtn, &QPushButton::clicked, [this] { this->showViewportTableWidget(); });

	// 正交模式
	m_pOrthogonalBtn = new QToolButton(m_pWidget);
	m_pOrthogonalBtn->setAttribute(Qt::WA_DeleteOnClose);
	m_pOrthogonalBtn->setIcon(QIcon(":/ribbon/bottom/orthogonal.svg"));
	m_pOrthogonalBtn->setFixedSize(20, 20);
	m_pOrthogonalBtn->setIconSize(QSize(16, 16));
	//m_pOrthogonalBtn->setFlat(true);
	m_pOrthogonalBtn->setCheckable(true);
	m_pOrthogonalBtn->setChecked(false);
	m_pOrthogonalBtn->setToolTip(QObject::tr("Orthogonal"));
	m_pOrthogonalBtn->show();
	connect(m_pOrthogonalBtn, &QPushButton::clicked, m_pActionHandler, [this] {
		if (m_pOrthogonalBtn->isChecked())
		{
			m_pOrthogonalBtn->setChecked(true);
			GUIDIALOGFACTORY->commandMessage(tr("orthogonal: open"));
			m_pActionHandler->slotRestrictOrthogonal();
		}
		else
		{
			m_pOrthogonalBtn->setChecked(false);
			GUIDIALOGFACTORY->commandMessage(tr("orthogonal: close"));
			m_pActionHandler->slotRestrictNothing();
		}
	});

	// 背景网格
	m_pGridBtn = new QToolButton(m_pWidget);
	m_pGridBtn->setAttribute(Qt::WA_DeleteOnClose);
	m_pGridBtn->setIcon(QIcon(":/ribbon/bottom/grid.svg"));
	m_pGridBtn->setFixedSize(20, 20);
	m_pGridBtn->setIconSize(QSize(16, 16));
	//m_pGridBtn->setFlat(true);
	m_pGridBtn->setCheckable(true);
	m_pGridBtn->setChecked(true);
	m_pGridBtn->setToolTip(QObject::tr("Grid"));
	m_pGridBtn->show();

	connect(m_pGridBtn, SIGNAL(clicked()), m_pActionHandler, SLOT(slotViewGrid()));

	// 捕捉模式
	m_pSnapModeBtn = new QToolButton();
	m_pSnapModeBtn->setAttribute(Qt::WA_DeleteOnClose);
	m_pSnapModeBtn->setIcon(QIcon(":/ribbon/bottom/snap.svg"));
	m_pSnapModeBtn->setFixedSize(20, 20);
	m_pSnapModeBtn->setIconSize(QSize(16, 16));
	m_pSnapModeBtn->setToolTip(QObject::tr("Snap"));
	connect(m_pSnapModeBtn, &QPushButton::clicked, m_pWidget, [this] { this->showToolWidget();});

	// 线宽
	m_pShowWidthBtn = new QToolButton();
	m_pShowWidthBtn->setAttribute(Qt::WA_DeleteOnClose);
	m_pShowWidthBtn->setIcon(QIcon(":/ribbon/bottom/show_width.svg"));
	m_pShowWidthBtn->setFixedSize(20, 20);
	m_pShowWidthBtn->setIconSize(QSize(16, 16));
	m_pShowWidthBtn->setCheckable(true);
	m_pShowWidthBtn->setChecked(false);
	m_pShowWidthBtn->setToolTip(QObject::tr("show width"));
	connect(m_pShowWidthBtn, SIGNAL(clicked()), this, SLOT(slotShowWidth()));

	// 单位设置
	m_pUnitBtn = new QToolButton();
	m_pUnitBtn->setAttribute(Qt::WA_DeleteOnClose);
	m_pUnitBtn->setIcon(QIcon(":/ribbon/bottom/unit.svg"));
	m_pUnitBtn->setFixedSize(20, 20);
	m_pUnitBtn->setIconSize(QSize(16, 16));
	m_pUnitBtn->setToolTip(QObject::tr("Unit"));
	connect(m_pUnitBtn, &QPushButton::clicked, [this] { this->showComboBoxWidget(); });

	//// 主题设置
	//m_pTheme = new QToolButton();
	//m_pTheme->setAttribute(Qt::WA_DeleteOnClose);
	//m_pTheme->setIcon(QIcon(":/ribbon/bottom/theme.svg"));
	//m_pTheme->setFixedSize(20, 20);
	//m_pTheme->setIconSize(QSize(16, 16));
	//m_pTheme->setToolTip(QObject::tr("Theme"));
	//connect(m_pTheme, &QPushButton::clicked, [this] { this->showThemeComBoxWidget(); });
}

QWidget* UIBottomWindow::getWidget()
{
	return m_pBottomWidget;
}

QWidget* UIBottomWindow::getToolWidget()
{
	return m_pBottomToolWidget;
}

QWidget* UIBottomWindow::getViewportsWidget()
{
	return m_pViewportWidget;
}

QWidget* UIBottomWindow::getComboBoxWidget()
{
	return m_pComboBoxWidget;
}

//QWidget* UIBottomWindow::getThemeComBoxWidget()
//{
//	return m_pThemeComBoxWidget;
//}

UISnapWidget* UIBottomWindow::getSnapToolBar()
{
	return m_pSnapWidget;
}

void UIBottomWindow::createComboBox()
{
	QVBoxLayout* lay = new QVBoxLayout(m_pComboBoxWidget);
	QString def_unit = QObject::tr("Millimeter");
	DMSETTINGS->beginGroup("/Defaults");
	QString  currentUnit = QObject::tr(DMSETTINGS->readEntry("/Unit", def_unit).toUtf8().data());
	DMSETTINGS->endGroup();

	for (int i = DM::None; i < DM::LastUnit; i++)
	{
		QString mapKey = DmUnits::unitToString((DM::Unit)i);
		QToolButton* map_val = new QToolButton();
		map_val->setText(mapKey);
		map_val->setProperty("yiCadUnitOptionButton", true);
		m_mapUnitBtn[mapKey] = map_val;
		m_mapUnitBtn[mapKey]->setAttribute(Qt::WA_DeleteOnClose);
		m_mapUnitBtn[mapKey]->setFixedSize(150, 30);
		m_mapUnitBtn[mapKey]->setCheckable(true);
		if (currentUnit == mapKey)
		{
			m_mapUnitBtn[mapKey]->setChecked(true);
		}

		//m_mapUnitBtn[mapKey]->setStyleSheet("QPushButton:checked{background-color: #FCD364;}");
		m_mapUnitBtn[mapKey]->connect(m_mapUnitBtn[mapKey], &QPushButton::clicked, [this, mapKey] { this->unitBtnClicked(mapKey); });
		lay->addWidget(m_mapUnitBtn[mapKey]);
	}

 	lay->setContentsMargins(0, 0, 0, 0);
}

//void UIBottomWindow::createThemeComboBox()
//{
//	QVBoxLayout* lay = new QVBoxLayout(m_pThemeComBoxWidget);
//	m_pThemeLight = new QToolButton();
//	m_pThemeLight->setText(QObject::tr("Light"));
//	m_pThemeLight->setAttribute(Qt::WA_DeleteOnClose);
//	m_pThemeLight->setFixedSize(150,25);
//	m_pThemeLight->setCheckable(true);
//	//m_pThemeLight->setStyleSheet("QPushButton:checked{background-color: #FCD364;}");
//	lay->addWidget(m_pThemeLight);
//	m_pThemeDark = new QToolButton();
//	m_pThemeDark->setText(QObject::tr("Dark"));
//	m_pThemeDark->setAttribute(Qt::WA_DeleteOnClose);
//	m_pThemeDark->setFixedSize(150, 25);
//	m_pThemeDark->setCheckable(true);
//	//m_pThemeDark->setStyleSheet("QPushButton:checked{background-color: #FCD364;}");
//	lay->addWidget(m_pThemeDark);
//	lay->setContentsMargins(0, 0, 0, 0);
//
//	DMSETTINGS->beginGroup("/Defaults");
//	int idx = DMSETTINGS->readNumEntry("/Theme", 0);
//	DMSETTINGS->endGroup();
//
//	if (idx) 
//	{
//		m_pThemeDark->setChecked(true);
//	}
//	else
//	{
//		m_pThemeLight->setChecked(true);
//	}
//	QString lightname = m_pThemeLight->text();
//	QString darkname = m_pThemeDark->text();
//
//	m_pThemeLight->connect(m_pThemeLight, &QPushButton::clicked, [this, lightname] { this->slotChangeTheme(lightname); });
//	m_pThemeDark->connect(m_pThemeDark, &QPushButton::clicked, [this, darkname] { this->slotChangeTheme(darkname); });
//}

void UIBottomWindow::createTool()
{
	m_pSnapWidget = new UISnapWidget(m_pBottomToolWidget, m_pActionHandler);
}

void UIBottomWindow::showToolWidget()
{
	if (m_pBottomToolWidget->isHidden())
	{
		m_pBottomToolWidget->show();
		m_pBottomToolWidget->raise();
	}
	else
	{
		m_pBottomToolWidget->lower();
		m_pBottomToolWidget->hide();
	}
}

void UIBottomWindow::showComboBoxWidget()
{
	QString def_unit = QObject::tr("Millimeter");
	QString  currentUnit = QObject::tr(DMSETTINGS->readEntry("/Unit", def_unit).toUtf8().data());
	for (int i = DM::None; i < DM::LastUnit; i++)
	{
		QString mapKey = DmUnits::unitToString((DM::Unit)i);

		if (currentUnit == mapKey)
		{
			m_mapUnitBtn[currentUnit]->setChecked(true);
		}
		else
		{
			m_mapUnitBtn[mapKey]->setChecked(false);
		}
	}

	if (m_pComboBoxWidget->isHidden())
	{
		m_pComboBoxWidget->show();
		m_pComboBoxWidget->raise();
	}
	else
	{
		m_pComboBoxWidget->lower();
		m_pComboBoxWidget->hide();
	}
}

void UIBottomWindow::setCoordinates(const DmVector& pos)
{
	if (m_pCurrentMdiWindow != nullptr)
	{
		auto pDocument = m_pCurrentMdiWindow->getDocument();

		int  iPrec = 4; // 线性精度	
		DM::LinearFormat eFormat = DM::Decimal;

		if (pDocument)
		{
			eFormat = pDocument->getLinearFormat();
			iPrec = pDocument->getLinearPrecision();
		}

		QString coordX = DmUnits::formatLinear(pos.x, pDocument->getUnit(), eFormat, iPrec);
		QString coordY = DmUnits::formatLinear(pos.y, pDocument->getUnit(), eFormat, iPrec);

		int xPosNum = coordX.size()-5;
		int yPosNum = coordY.size()-5;

		xPosNum = pos.x < 0.00 ? xPosNum - 1 : xPosNum;
		yPosNum = pos.y < 0.00 ? yPosNum - 1 : yPosNum;
		if (pos.x >= 1000000.00 || pos.x <= -1000000.00)
		{
			double x = pos.x / std::pow(10, xPosNum - 1);
			coordX = DmUnits::formatLinear(x, pDocument->getUnit(), eFormat, iPrec);
			coordX = coordX + "E+0" + QString::number(xPosNum);
		}

		if (pos.y >= 1000000.00 || pos.y <= -1000000.00)
		{
			double y = pos.y / std::pow(10, yPosNum - 1);
			coordY = DmUnits::formatLinear(y, pDocument->getUnit(), eFormat, iPrec);
			coordY = coordY + "E+0" + QString::number(yPosNum);
		}

		m_pCoordLabel->setText(coordX + " , " + coordY);
	}
	else
	{
		m_pCoordLabel->setText("0.0000 , 0.0000");
	}
}

void UIBottomWindow::setSelectNumber(const int num)
{
	QString text = tr("number:") + " " + QString::number(num);
	m_pNumberLabel->setText(text);
}

void UIBottomWindow::clearBottomWidget()
{
	if (m_pBottomWidget != nullptr && m_pBottomToolWidget != nullptr && m_pComboBoxWidget != nullptr /*&& m_pThemeComBoxWidget != nullptr*/ && m_pViewportWidget != nullptr)
	{
		m_pBottomWidget->close();
		m_pBottomToolWidget->close();
		m_pComboBoxWidget->close();
		//m_pThemeComBoxWidget->close();
		m_pViewportWidget->close();

		m_pBottomWidget = nullptr;
		m_pBottomToolWidget = nullptr;
		m_pComboBoxWidget = nullptr;
		//m_pThemeComBoxWidget = nullptr;
		m_pViewportWidget = nullptr;
	}
}

void UIBottomWindow::redrawBottomWidget(UIActionHandler* pActionHandler, MDIWindow* pCurrentMdiWindow)
{
	m_pActionHandler = pActionHandler;
	m_pCurrentMdiWindow = pCurrentMdiWindow;
}

void UIBottomWindow::slotShowWidth()
{
	if (m_pCurrentMdiWindow != nullptr)
	{
		bool draftMode = m_pCurrentMdiWindow->getDocumentView()->isDraftMode();
		DMSETTINGS->beginGroup("/Appearance");
		DMSETTINGS->writeEntry("/DraftMode", (int)(!draftMode));
		DMSETTINGS->endGroup();

		m_pCurrentMdiWindow->getDocumentView()->setDraftMode(!draftMode);
		GuiDocumentView* gv = m_pCurrentMdiWindow->getDocumentView();
		if (gv)
		{
			gv->redraw();
		}
	}
}

void UIBottomWindow::unitBtnClicked(QString BtnName)
{
	m_mapUnitBtn[QObject::tr(DMSETTINGS->readEntry("/Unit", BtnName).toUtf8().data())]->setChecked(false);

	DMSETTINGS->beginGroup("/Defaults");
	DMSETTINGS->writeEntry("/Unit", DmUnits::unitToString(DmUnits::stringToUnit(BtnName), false));
	DMSETTINGS->endGroup();

	m_pComboBoxWidget->lower();
	m_pComboBoxWidget->hide();
}

void UIBottomWindow::slotChangeTheme(QString BtnName)
{
	DMSETTINGS->beginGroup("/Defaults");
	if (BtnName == QObject::tr("Dark"))
	{
		DMSETTINGS->writeEntry("/Theme", 1);
	}
	else if (BtnName == QObject::tr("Light"))
	{
		DMSETTINGS->writeEntry("/Theme", 0);
	}
	DMSETTINGS->endGroup();

	//m_pThemeComBoxWidget->lower();
	//m_pThemeComBoxWidget->hide();
}

void UIBottomWindow::showThemeComBoxWidget()
{
	int idx = DMSETTINGS->readNumEntry("/Theme", 0);
	
	if (idx) 
	{
		m_pThemeDark->setChecked(true);
		m_pThemeLight->setChecked(false);
	}
	else
	{
		m_pThemeLight->setChecked(true);
		m_pThemeDark->setChecked(false);
	}

	//if (m_pThemeComBoxWidget->isHidden())
	//{
	//	m_pThemeComBoxWidget->show();
	//	m_pThemeComBoxWidget->raise();
	//}
	//else
	//{
	//	m_pThemeComBoxWidget->lower();
	//	m_pThemeComBoxWidget->hide();
	//}
}
