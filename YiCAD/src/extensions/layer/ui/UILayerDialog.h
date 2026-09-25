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

/// @file UILayerDialog.h
/// @brief 图层配置弹窗

#ifndef UILAYERDIALOG_H
#define UILAYERDIALOG_H

#include "ui_UILayerDialog.h"

class DmLayer;
class DmLayerTable;

/// @brief 图层配置弹窗
class UILayerDialog : public QDialog, public Ui::UILayerDialog
{
	Q_OBJECT

public:
	/// @brief 构造函数
	/// @param parent 父窗口指针
	/// @param name 对象名称
	/// @param modal 是否模态
	/// @param fl 窗口标志
	UILayerDialog(QWidget* parent = nullptr, QString name = QString(), bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());
	~UILayerDialog() = default;

public slots:
	virtual void setLayer(DmLayer* l);
	virtual void updateLayer();
	virtual void validate();
	virtual void setLayerTable(DmLayerTable* layerTable);
	virtual void setEditLayer(bool el);

	/// @return a reference to the QLineEdit object.
	virtual QLineEdit* getQLineEdit();

protected:
	DmLayer*		m_pLayer = nullptr;        ///< 图层对象指针
	DmLayerTable*	m_pLayerTable = nullptr;   ///< 图层表指针
	QString			m_strLayerName;            ///< 图层名称
	bool			m_isEditLayer = false;     ///< 是否为编辑图层模式

protected slots:
	virtual void languageChange();

private:
	void init();

};

#endif // UILAYERDIALOG_H
