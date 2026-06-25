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

/// @file UILineTypeBox.h
/// @brief 线型下拉列表框

#ifndef UILINETYPEBOX_H
#define UILINETYPEBOX_H

#include <QComboBox>
#include <QMessageBox>

#include "Datamodel.h"
#include "DmDocument.h"

class DmLineTypeTable;
class DmLineType;

/// @brief 线型下拉列表框
class UILineTypeBox : public QComboBox 
{
	Q_OBJECT
public:
	UILineTypeBox(QWidget* parent = nullptr);
	virtual ~UILineTypeBox();

	DmLineType* getLineType();
	void setLineType(DmLineType* t);
	void setLineType(DmLineType* t, DmDocument* d);
    int indexOf(DmLineType* t);
    DmLineType* lineTypeAt(int idx);

	void init(bool showByLayer);

	//update list
	void updateLineTypeTable();
	void updateLineTypeTable(DmDocument* doc);

private slots:
	void slotLineTypeChanged(int index);

signals:
	void lineTypeChanged(DmLineType* data);

private:
	bool				m_isShowByLayer = false;
    bool                m_isChangingByCode = false;

	DmLineType*			m_currentLineType = nullptr;
	DmLineTypeTable*	m_LineTypeTable = nullptr;    ///< 操作的线型表
	DmDocument*			m_document = nullptr;         ///< 线型表所在文档，默认为当前文档
	
};

#endif

