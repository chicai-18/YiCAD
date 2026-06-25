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

/// @file UISnapWidget.cpp
/// @brief 捕捉模式工具栏控件，支持端点/中点/交点等多种CAD捕捉模式

#include "UISnapWidget.h"
#include "UIActionHandler.h"
#include <QHBoxLayout>
#include <QPixmap>
#include "QApplication"
#include "DmSettings.h"

UISnapWidget::UISnapWidget(QWidget* parent, UIActionHandler* ah)
    : QWidget(parent)
    , m_pActionHandler(ah)
{
	this->resize(100, 150);

	layout1 = new QHBoxLayout();
	layout2 = new QHBoxLayout();
	layout3 = new QHBoxLayout();
	layout4 = new QHBoxLayout();
	layout5 = new QHBoxLayout();
	layout6 = new QHBoxLayout();
	//layout7 = new QHBoxLayout();
	//layout8 = new QHBoxLayout();
	//layout9 = new QHBoxLayout();


	//-------------------------------------------------------------------------------------------------------------------------
	m_pTextLable = new QLabel();
	m_pTextLable->setFixedSize(60, 20);
	m_pTextLable->setText(QObject::tr("SnapGrid"));
	m_pTextLable->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	m_pCheckGridLable = new QLabel;
	m_pCheckGridLable->setFixedSize(20, 20);

	m_pIconLable = new QLabel;
	m_pIconLable->setFixedSize(20, 20);

	m_pSnapGrid = new QPushButton();

	m_pSnapGrid->setCheckable(true);
	m_pSnapGrid->setFixedSize(100, 20);
	m_pSnapGrid->setObjectName(tr("SnapGrid"));
	//m_pSnapGrid->setStyleSheet("QPushButton{background: #FF0000}""QPushButton::hover{background: #00ff00}");
	connect(m_pSnapGrid, &QPushButton::clicked, parent, [this] { this->actionTriggered(); });

	layout1->setContentsMargins(0, 0, 0, 0);
	layout1->setSpacing(0);
	layout1->addWidget(m_pCheckGridLable);
	layout1->addWidget(m_pIconLable);
	layout1->addWidget(m_pTextLable);
	layout1->addStretch();
	m_pSnapGrid->setLayout(layout1);

	//--------------------------------------------------------------------------------------------------------------------------
	m_pTextLable = new QLabel;
	m_pTextLable->setFixedSize(60, 20);
	m_pTextLable->setText(QObject::tr("SnapEnd"));
	m_pTextLable->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	m_pCheckEndLable = new QLabel;
	m_pCheckEndLable->setFixedSize(20, 20);

	m_pIconLable = new QLabel;
	m_pIconLable->setFixedSize(20, 20);
	QPixmap pixmapSnapEnd(":/ribbon/snap_point/snap_endpoint.svg");
	pixmapSnapEnd = pixmapSnapEnd.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	m_pIconLable->setPixmap(pixmapSnapEnd);

	m_pSnapEnd = new QPushButton();
	m_pSnapEnd->setObjectName(QObject::tr("SnapEnd"));
	m_pSnapEnd->setCheckable(true);
	m_pSnapEnd->setFixedSize(100, 20);
	connect(m_pSnapEnd, &QPushButton::clicked, parent, [this] { this->actionTriggered(); });

	layout2->setContentsMargins(0, 0, 0, 0);
	layout2->setSpacing(0);
	layout2->addWidget(m_pCheckEndLable);
	layout2->addWidget(m_pIconLable);
	layout2->addStretch(1);
	layout2->addWidget(m_pTextLable);
	m_pSnapEnd->setLayout(layout2);

	//-------------------------------------------------------------------------------------------------------------------------------------

	m_pTextLable = new QLabel;
	m_pTextLable->setFixedSize(60, 20);
	m_pTextLable->setText(QObject::tr("SnapEntity"));
	m_pTextLable->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	m_pCheckEntityLable = new QLabel;
	m_pCheckEntityLable->setFixedSize(20, 20);

	m_pIconLable = new QLabel;
	m_pIconLable->setFixedSize(20, 20);
	QPixmap pixmapSnapOnEntity(":/ribbon/snap_point/snap_on_entity.svg");
	pixmapSnapOnEntity = pixmapSnapOnEntity.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	m_pIconLable->setPixmap(pixmapSnapOnEntity);

	m_pSnapOnEntity = new QPushButton();
	m_pSnapOnEntity->setObjectName(tr("SnapEntity"));
	m_pSnapOnEntity->setCheckable(true);
	m_pSnapOnEntity->setFixedSize(100, 20);
	connect(m_pSnapOnEntity, &QPushButton::clicked, parent, [this] { this->actionTriggered(); });

	layout3->setContentsMargins(0, 0, 0, 0);
	layout3->setSpacing(0);
	layout3->addWidget(m_pCheckEntityLable);
	layout3->addWidget(m_pIconLable);
	layout3->addStretch(1);
	layout3->addWidget(m_pTextLable);
	m_pSnapOnEntity->setLayout(layout3);

	//-----------------------------------------------------------------------------------------------------------------------------

	m_pTextLable = new QLabel;
	m_pTextLable->setFixedSize(60, 20);
	m_pTextLable->setText(QObject::tr("SnapCenter"));
	m_pTextLable->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	m_pCheckCenterLable = new QLabel;
	m_pCheckCenterLable->setFixedSize(20, 20);

	m_pIconLable = new QLabel;
	m_pIconLable->setFixedSize(20, 20);
	QPixmap pixmapSnapCenter(":/ribbon/snap_point/snap_center.svg");
	pixmapSnapCenter = pixmapSnapCenter.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	m_pIconLable->setPixmap(pixmapSnapCenter);

	m_pSnapCenter = new QPushButton();
	m_pSnapCenter->setObjectName(tr("SnapCenter"));
	m_pSnapCenter->setCheckable(true);
	m_pSnapCenter->setFixedSize(100, 20);
	connect(m_pSnapCenter, &QPushButton::clicked, parent, [this] { this->actionTriggered(); });

	layout4->setContentsMargins(0, 0, 0, 0);
	layout4->setSpacing(0);
	layout4->addWidget(m_pCheckCenterLable);
	layout4->addWidget(m_pIconLable);
	layout4->addStretch(1);
	layout4->addWidget(m_pTextLable);
	m_pSnapCenter->setLayout(layout4);

	//----------------------------------------------------------------------------------------------------------------------------
	m_pTextLable = new QLabel;
	m_pTextLable->setFixedSize(60, 20);
	m_pTextLable->setText(QObject::tr("SnapMiddle"));
	m_pTextLable->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	m_pCheckMiddleLable = new QLabel;
	m_pCheckMiddleLable->setFixedSize(20, 20);

	m_pIconLable = new QLabel;
	m_pIconLable->setFixedSize(20, 20);
	QPixmap pixmapSnapMiddle(":/ribbon/snap_point/snap_middle.svg");
	pixmapSnapMiddle = pixmapSnapMiddle.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	m_pIconLable->setPixmap(pixmapSnapMiddle);

	m_pSnapMiddle = new QPushButton();
	m_pSnapMiddle->setObjectName(tr("SnapMiddle"));
	m_pSnapMiddle->setCheckable(true);
	m_pSnapMiddle->setFixedSize(100, 20);
	connect(m_pSnapMiddle, &QPushButton::clicked, parent, [this] { this->actionTriggered(); });

	layout5->setContentsMargins(0, 0, 0, 0);
	layout5->setSpacing(0);
	layout5->addWidget(m_pCheckMiddleLable);
	layout5->addWidget(m_pIconLable);
	layout5->addStretch(1);
	layout5->addWidget(m_pTextLable);
	m_pSnapMiddle->setLayout(layout5);
	
	//-------------------------------------------------------------------------------------------------------------------------------------
	m_pTextLable = new QLabel;
	m_pTextLable->setFixedSize(60, 20);
	m_pTextLable->setText(QObject::tr("SnapIntersection"));
	m_pTextLable->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	m_pCheckIntersectionLable = new QLabel;
	m_pCheckIntersectionLable->setFixedSize(20, 20);

	m_pIconLable = new QLabel;
	m_pIconLable->setFixedSize(20, 20);
	QPixmap pixmapSnapIntersection(":/ribbon/snap_point/snap_intersection.svg");
	pixmapSnapIntersection = pixmapSnapIntersection.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	m_pIconLable->setPixmap(pixmapSnapIntersection);

	m_pSnapIntersection = new QPushButton();
	m_pSnapIntersection->setObjectName(tr("SnapIntersection"));
	m_pSnapIntersection->setCheckable(true);
	m_pSnapIntersection->setFixedSize(100, 20);
	connect(m_pSnapIntersection, &QPushButton::clicked, parent, [this] { this->actionTriggered(); });

	layout6->setContentsMargins(0, 0, 0, 0);
	layout6->setSpacing(0);
	layout6->addWidget(m_pCheckIntersectionLable);
	layout6->addWidget(m_pIconLable);
	layout6->addStretch(1);
	layout6->addWidget(m_pTextLable);
	m_pSnapIntersection->setLayout(layout6);
	/*
		//-------------------------------------------------------------------------------------------------------------------------
	m_pTextLable = new QLabel;
	m_pTextLable->setFixedSize(60, 20);
	m_pTextLable->setText(QObject::tr("SnapDistance"));
	m_pTextLable->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	m_pCheckDistanceLable = new QLabel;
	m_pCheckDistanceLable->setFixedSize(20, 20);

	m_pIconLable = new QLabel;
	m_pIconLable->setFixedSize(20, 20);
	QPixmap pixmapSnapDistance("");
	pixmapSnapDistance = pixmapSnapDistance.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	m_pIconLable->setPixmap(pixmapSnapDistance);

	m_pSnapDistance = new QPushButton();
	m_pSnapDistance->setObjectName(tr("SnapDistance"));
	m_pSnapDistance->setCheckable(true);
	m_pSnapDistance->setFixedSize(100, 20);
	connect(m_pSnapDistance, &QPushButton::clicked, parent, [this] { this->actionTriggered(); });

	layout6->addWidget(m_pCheckDistanceLable);
	layout6->addWidget(m_pIconLable);
	layout6->addStretch(1);
	layout6->addWidget(m_pTextLable);
	m_pSnapDistance->setLayout(layout6);

	//-------------------------------------------------------------------------------------------------------------------------------------
	m_pTextLable = new QLabel;
	m_pTextLable->setFixedSize(140, 20);
	m_pTextLable->setText(QObject::tr("SnapFree"));
	m_pTextLable->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	m_pCheckFreeLable = new QLabel;
	m_pCheckFreeLable->setFixedSize(20, 20);
	m_pCheckFreeLable->setPixmap(QPixmap(":/ribbon/bottom/check_mark.svg"));

	m_pIconLable = new QLabel;
	m_pIconLable->setFixedSize(20, 20);

	m_pSnapFree = new QPushButton();
	m_pSnapFree->setObjectName(tr("m_pSnapFree"));
	m_pSnapFree->setCheckable(true);
	m_pSnapFree->setChecked(true);
	m_pSnapFree->setFixedSize(200, 20);
	connect(m_pSnapFree, &QPushButton::clicked, parent, [this] { this->actionTriggered(); });

	layout8->addWidget(m_pCheckFreeLable);
	layout8->addWidget(m_pIconLable);
	layout8->addStretch(1);
	layout8->addWidget(m_pTextLable);
	m_pSnapFree->setLayout(layout8);

	//-------------------------------------------------------------------------------------------------------------------------------------
	m_pTextLable = new QLabel;
	m_pTextLable->setFixedSize(140, 20);
	m_pTextLable->setText(QObject::tr("SnapSubsection"));
	m_pTextLable->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	m_pCheckSubsectionLable = new QLabel;
	m_pCheckSubsectionLable->setFixedSize(20, 20);

	m_pIconLable = new QLabel;
	m_pIconLable->setFixedSize(30, 20);

	m_pSnapSubsection = new QPushButton();
	m_pSnapSubsection->setObjectName(tr("m_pSnapSubsection"));
	m_pSnapSubsection->setCheckable(true);
	m_pSnapSubsection->setFixedSize(200, 20);
	connect(m_pSnapSubsection, &QPushButton::clicked, parent, [this] { this->actionTriggered(); });

	layout9->addWidget(m_pCheckSubsectionLable);
	layout9->addWidget(m_pIconLable);
	layout9->addStretch(1);
	layout9->addWidget(m_pTextLable);
	m_pSnapSubsection->setLayout(layout9);*/

	//-------------------------------------------------------------------------------------------------------------------------------------
	layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 5, 0, 5);
	layout->setSpacing(10);
	//layout->addWidget(m_pSnapFree); // 屏蔽自由捕捉,它应始终保持true
	layout->addWidget(m_pSnapGrid);
	layout->addWidget(m_pSnapEnd);
	layout->addWidget(m_pSnapOnEntity);
	layout->addWidget(m_pSnapCenter);
	layout->addWidget(m_pSnapMiddle);
	layout->addWidget(m_pSnapIntersection);
	//layout->addWidget(m_pSnapDistance);	// todo: 屏蔽距离捕捉和分段捕捉(与AutoCAD保持一致)
	//layout->addWidget(m_pSnapSubsection);
	

	DMSETTINGS->beginGroup("/Snap");
	m_pSnapGrid->setChecked((bool)DMSETTINGS->readNumEntry("/SnapGrid", 0));
	m_pSnapEnd->setChecked((bool)DMSETTINGS->readNumEntry("/SnapEnd", 0));
	m_pSnapOnEntity->setChecked((bool)DMSETTINGS->readNumEntry("/SnapOnEntity", 0));
	m_pSnapCenter->setChecked((bool)DMSETTINGS->readNumEntry("/SnapCenter", 0));
	m_pSnapMiddle->setChecked((bool)DMSETTINGS->readNumEntry("/SnapMiddle", 0));
	m_pSnapIntersection->setChecked((bool)DMSETTINGS->readNumEntry("/SnapIntersection", 0));
	DMSETTINGS->endGroup();
	actionTriggered();
}

UISnapWidget::~UISnapWidget()
{
}

void UISnapWidget::setSnaps(SnapMode const& s)
{
    if (getSnaps() == s)
    {
        return;
    }
    m_pSnapGrid->setChecked(s.snapGrid);
	m_pSnapEnd->setChecked(s.snapEndpoint);
	m_pSnapOnEntity->setChecked(s.snapOnEntity);
	m_pSnapCenter->setChecked(s.snapCenter);
	m_pSnapMiddle->setChecked(s.snapMiddle);
	//m_pSnapDistance->setChecked(s.snapDistance);
	m_pSnapIntersection->setChecked(s.snapIntersection);
	//m_pSnapFree->setChecked(s.snapFree);
	//m_pSnapSubsection->setChecked(s.snapSubsection);
}

void UISnapWidget::actionTriggered()
{
	DMSETTINGS->beginGroup("/Snap");
	DMSETTINGS->writeEntry("/SnapGrid", m_pSnapGrid->isChecked());
	DMSETTINGS->writeEntry("/SnapEnd", m_pSnapEnd->isChecked());
	DMSETTINGS->writeEntry("/SnapOnEntity", m_pSnapOnEntity->isChecked());
	DMSETTINGS->writeEntry("/SnapCenter", m_pSnapCenter->isChecked());
	DMSETTINGS->writeEntry("/SnapMiddle", m_pSnapMiddle->isChecked());
	DMSETTINGS->writeEntry("/SnapIntersection", m_pSnapIntersection->isChecked());
	DMSETTINGS->endGroup();

	if (m_pSnapGrid->isChecked())
	{
		m_pCheckGridLable->setPixmap(QPixmap(":/ribbon/bottom/check_mark.svg"));
	}
	else
	{
		m_pCheckGridLable->setPixmap(QPixmap(""));
	}

	if (m_pSnapEnd->isChecked())
	{
		m_pCheckEndLable->setPixmap(QPixmap(":/ribbon/bottom/check_mark.svg"));
	}
	else
	{
		m_pCheckEndLable->setPixmap(QPixmap(""));
	}

	if (m_pSnapOnEntity->isChecked())
	{
		m_pCheckEntityLable->setPixmap(QPixmap(":/ribbon/bottom/check_mark.svg"));
	}
	else
	{
		m_pCheckEntityLable->setPixmap(QPixmap(""));
	}

	if (m_pSnapCenter->isChecked())
	{
		m_pCheckCenterLable->setPixmap(QPixmap(":/ribbon/bottom/check_mark.svg"));
	}
	else
	{
		m_pCheckCenterLable->setPixmap(QPixmap(""));
	}

	if (m_pSnapMiddle->isChecked())
	{
		m_pCheckMiddleLable->setPixmap(QPixmap(":/ribbon/bottom/check_mark.svg"));
	}
	else
	{
		m_pCheckMiddleLable->setPixmap(QPixmap(""));
	}

	/*
	if (m_pSnapDistance->isChecked())
	{
		m_pCheckDistanceLable->setPixmap(QPixmap(":/ribbon/bottom/check_mark.svg"));
	}
	else
	{
		m_pCheckDistanceLable->setPixmap(QPixmap(""));
	}*/

	if (m_pSnapIntersection->isChecked())
	{
		m_pCheckIntersectionLable->setPixmap(QPixmap(":/ribbon/bottom/check_mark.svg"));
	}
	else
	{
		m_pCheckIntersectionLable->setPixmap(QPixmap(""));
	}
	/*
	if (m_pSnapSubsection->isChecked())
	{
		m_pCheckSubsectionLable->setPixmap(QPixmap(":/ribbon/bottom/check_mark.svg"));
	}
	else
	{
		m_pCheckSubsectionLable->setPixmap(QPixmap(""));
	}

	if (m_pSnapFree->isChecked())
	{
		m_pCheckFreeLable->setPixmap(QPixmap(":/ribbon/bottom/check_mark.svg"));
	}
	else
	{
		m_pCheckFreeLable->setPixmap(QPixmap(""));
	}*/
	m_pActionHandler->slotSetSnaps(getSnaps());
}

SnapMode UISnapWidget::getSnaps(void) const
{
	SnapMode s;
	s.snapGrid = m_pSnapGrid->isChecked();
	s.snapEndpoint = m_pSnapEnd->isChecked();
	s.snapOnEntity = m_pSnapOnEntity->isChecked();
	s.snapCenter = m_pSnapCenter->isChecked();
	s.snapMiddle = m_pSnapMiddle->isChecked();
	//s.snapDistance = m_pSnapDistance->isChecked();
	s.snapIntersection = m_pSnapIntersection->isChecked();
	//s.snapFree = m_pSnapFree->isChecked();
	//s.snapSubsection = m_pSnapSubsection->isChecked();
	return s;
}
