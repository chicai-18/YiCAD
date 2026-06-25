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

/// @file UILayerBox.h
/// @brief 图层选择下拉列表框

#ifndef UILAYERBOX_H
#define UILAYERBOX_H

#include <QComboBox>

class DmLayer;
class DmLayerTable;

/// @brief 图层选择下拉列表框
class UILayerBox : public QComboBox 
{
	Q_OBJECT

public:
	UILayerBox(QWidget* parent = nullptr);
	virtual ~UILayerBox();

	DmLayer* getLayer();
	void setLayer(DmLayer& layer);
	void setLayer(QString& layer);

	void init(DmLayerTable& layerList, bool showByBlock);

private slots:
	void slotLayerChanged(int index);

signals:
	void layerChanged(DmLayer* layer);

private:
	DmLayerTable*	m_pLayerTable = nullptr;
	DmLayer*		m_pCurrentLayer = nullptr;
	bool			m_isShowByBlock = false;
};

#endif
