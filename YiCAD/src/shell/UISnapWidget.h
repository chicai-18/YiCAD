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

/// @file UISnapWidget.h
/// @brief 捕捉模式选择控件

#ifndef UISNAPWIDGETNEW
#define UISNAPWIDGETNEW

#include <QWidget>
#include <QPushButton>
#include <QBoxLayout>
#include <QLabel>

#include "Snapper.h"
#include "qcoreevent.h"

class UIActionHandler;

class UISnapWidget : public QWidget
{
	Q_OBJECT
public:
	UISnapWidget(QWidget* parent, UIActionHandler* ah);
	~UISnapWidget();
	SnapMode getSnaps(void) const;

public slots:
	void setSnaps(SnapMode const& s);

private slots:
	void actionTriggered(void);

private:
	UIActionHandler*	m_pActionHandler = nullptr;
	QPushButton*		m_pSnapGrid = nullptr;
	QPushButton*		m_pSnapEnd = nullptr;
	QPushButton*		m_pSnapOnEntity = nullptr;
	QPushButton*		m_pSnapCenter = nullptr;
	QPushButton*		m_pSnapMiddle = nullptr;
	QPushButton*		m_pSnapDistance = nullptr;
	QPushButton*		m_pSnapIntersection = nullptr;
	QPushButton*		m_pSnapSubsection = nullptr;
	QPushButton*		m_pSnapFree = nullptr;
	QHBoxLayout*		layout1 = nullptr;
	QHBoxLayout*		layout2 = nullptr;
	QHBoxLayout*		layout3 = nullptr;
	QHBoxLayout*		layout4 = nullptr;
	QHBoxLayout*		layout5 = nullptr;
	QHBoxLayout*		layout6 = nullptr;
	QHBoxLayout*		layout7 = nullptr;
	QHBoxLayout*		layout8 = nullptr;
	QHBoxLayout*		layout9 = nullptr;
	QVBoxLayout*		layout = nullptr;
	QLabel*				m_pTextLable = nullptr;
	QLabel*				m_pIconLable = nullptr;
	QLabel*				m_pCheckGridLable = nullptr;
	QLabel*				m_pCheckFreeLable = nullptr;
	QLabel*				m_pCheckEndLable = nullptr;
	QLabel*				m_pCheckEntityLable = nullptr;
	QLabel*				m_pCheckCenterLable = nullptr;
	QLabel*				m_pCheckMiddleLable = nullptr;
	QLabel*				m_pCheckDistanceLable = nullptr;
	QLabel*				m_pCheckIntersectionLable = nullptr;
	QLabel*				m_pCheckSubsectionLable = nullptr;
};

#endif
