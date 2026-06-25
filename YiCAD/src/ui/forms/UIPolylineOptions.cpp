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

/// @file UIPolylineOptions.cpp
/// @brief 多段线选项控件实现

#include "UIPolylineOptions.h"

#include "ActionDrawPolyline.h"
#include "DmSettings.h"
#include "Math2d.h"
#include "ui_UIPolylineOptions.h"
#include "Debug.h"

using wLists = std::initializer_list<QWidget*>;

UIPolylineOptions::UIPolylineOptions(QWidget* parent, Qt::WindowFlags fl)
	: QWidget(parent, fl)
	, ui(new Ui::Ui_PolylineOptions{})
{
	ui->setupUi(this);

	// 起终点线宽只允许输入数值
	ui->startWeight->setValidator(new QRegExpValidator(QRegExp("[0-9]+$")));
	ui->endWeight->setValidator(new QRegExpValidator(QRegExp("[0-9]+$")));

	connect(ui->startWeight, SIGNAL(editingFinished()), this, SLOT(updateStartLineWeight()));
	connect(ui->endWeight, SIGNAL(editingFinished()), this, SLOT(updateEndLineWeight()));
}

UIPolylineOptions::~UIPolylineOptions()
{
	destroy();
}

/// @brief  Sets the strings of the subwidgets using the current language.
void UIPolylineOptions::languageChange()
{
	ui->retranslateUi(this);
}

void UIPolylineOptions::destroy()
{
	DMSETTINGS->beginGroup("/Draw");
	DMSETTINGS->writeEntry("/PolylineMode", ui->cbMode->currentIndex());
	DMSETTINGS->writeEntry("/PolylineRadius", ui->leRadius->text());
	DMSETTINGS->writeEntry("/PolylineAngle", ui->leAngle->text());
	DMSETTINGS->writeEntry("/PolylineStartWeight", ui->startWeight->text());
	DMSETTINGS->writeEntry("/PolylineEndWeight", ui->endWeight->text());
	DMSETTINGS->writeEntry("/PolylineCW", (int)ui->rbNeg->isChecked());
	DMSETTINGS->endGroup();
}

void UIPolylineOptions::setAction(ActionInterface* a, bool update)
{
	if (a && a->getEntityType() == DM::ActionDrawPolyline)
	{
		action = static_cast<ActionDrawPolyline*>(a);

		double sw = 0.0;
		double ew = 0.0;
		QString sd1;
		QString sd2;
		int mode = 0;
		bool cw(false);	//是否为顺时针

		if (update)
		{
			sw = ui->startWeight->text().toDouble();
			ew = ui->endWeight->text().toDouble();
			sd1 = QString("%1").arg(action->getRadius());
			sd2 = QString("%1").arg(action->getAngle());
			mode = action->getMode();
		}
		else
		{
			DMSETTINGS->beginGroup("/Draw");
			sd1 = DMSETTINGS->readEntry("/PolylineRadius", "1.0");
			sd2 = DMSETTINGS->readEntry("/PolylineAngle", "180.0");
			sw = DMSETTINGS->readEntry("/PolylineStartWeight", "0.0").toDouble();
			ew = DMSETTINGS->readEntry("/PolylineEndWeight", "0.0").toDouble();
			mode = DMSETTINGS->readNumEntry("/PolylineMode", 0);
			cw = DMSETTINGS->readNumEntry("/PolylineCW", 0);
			DMSETTINGS->endGroup();
			action->setStartWeight(sw);
			action->setEndWeight(ew);
			action->setRadius(sd1.toDouble());
			action->setAngle(sd2.toDouble());
			action->setMode((ActionDrawPolyline::SegmentMode)mode);
			action->setCCW(!cw);
		}
		ui->startWeight->setText(QString::number(sw));
		ui->endWeight->setText(QString::number(ew));
		ui->leRadius->setText(sd1);
		ui->leAngle->setText(sd2);
		ui->cbMode->setCurrentIndex(mode);
		ui->rbNeg->setChecked(cw);
		updateMode(mode);
	}
	else
	{
		action = nullptr;
	}
}

void UIPolylineOptions::close()
{
	if (action)
	{
		action->close();
	}
}

void UIPolylineOptions::undo()
{
	if (action)
	{
		action->undo();
	}
}

void UIPolylineOptions::updateRadius(const QString& s)
{
	if (action)
	{
		action->setRadius(Math2d::eval(s));
	}
}

void UIPolylineOptions::updateAngle(const QString& s)
{
	if (action)
	{
		double a = Math2d::eval(s);
		//	QString sr;
		if (a > 359.999)
		{
			a = 359.999;
			ui->leAngle->setText(QString("%1").arg(a));
		}
		else if (a < 0.0)
		{
			a = 0.0;
			ui->leAngle->setText(QString("%1").arg(a));
		}
		action->setAngle(a);
	}
}

void UIPolylineOptions::updateDirection(bool /*pos*/)
{
	if (action)
	{
		action->setCCW(ui->rbPos->isChecked());
	}
}

void UIPolylineOptions::updateMode(int m)
{
	if (action)
	{
		action->setMode((ActionDrawPolyline::SegmentMode)m);
	}
	switch ((ActionDrawPolyline::SegmentMode)m)
	{
	case ActionDrawPolyline::Line:
	case ActionDrawPolyline::Tangential:
	default:
		for (QWidget* p : wLists{ ui->leRadius, ui->leAngle, ui->lRadius, ui->lAngle, ui->rbPos, ui->rbNeg })
		{
			p->hide();
		}
		break;
	case ActionDrawPolyline::TanRad:
		for (QWidget* p : wLists{ ui->leAngle, ui->lAngle, ui->rbPos, ui->rbNeg })
		{
			p->hide();
		}
		for (QWidget* p : wLists{ ui->leRadius, ui->lRadius })
		{
			p->show();
		}
		break;
	case ActionDrawPolyline::Ang:
		for (QWidget* p : wLists{ ui->leRadius, ui->lRadius })
		{
			p->hide();
		}
		for (QWidget* p : wLists{ ui->leAngle, ui->lAngle, ui->rbPos, ui->rbNeg })
		{
			p->show();
		}
		break;
	}
	QWidget* backWidget = parentWidget();	//选项的背板
	adjustSize();
	backWidget->resize(size());
}

void UIPolylineOptions::updateStartLineWeight()
{
	double sw = ui->startWeight->text().toDouble();
	action->setStartWeight(sw);
}

void UIPolylineOptions::updateEndLineWeight()
{
	double ew = ui->endWeight->text().toDouble();
	action->setEndWeight(ew);
}
