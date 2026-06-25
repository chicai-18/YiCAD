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

/// @file UIDimStyleListBox.h
/// @brief 标注样式列表控件，支持新建、删除、修改标注样式

#ifndef UIDIMSTYLELISTBOX_H
#define UIDIMSTYLELISTBOX_H

#include <QListWidget>
#include <QAction>
class DmDocument;
class DmDimensionStyleTable;
class DmDimensionStyle;
class UIDimStyleListBox : public QListWidget
{
	Q_OBJECT
public:
	UIDimStyleListBox(QWidget* parent = nullptr);
	virtual ~UIDimStyleListBox() = default;

public:
	void init(DmDimensionStyleTable* dimStyleTable, DmDocument* document);
	DmDimensionStyle* activeStyle() const;
	bool canSelectedDelete() const;
	DmDimensionStyle* selectedStyle() const;
    DmDimensionStyle* styleAt(int i) const;
    int indexOf(DmDimensionStyle* t);
    /// @brief 是否可删除标注样式
    bool canRemoveDimStyle(DmDimensionStyle* style);

private slots:
	void slotItemSelectionChanged();
	void slotItemDoubleClicked(QListWidgetItem* item);

public slots:
	void slotNewDimStyle();
	void slotDeleteDimStyle();
	void slotSetActiveDimStyle();
	void slotRenameDimStyle();
	void slotModifyDimStyle();

signals:
    /// @brief 当前样式修改后触发的信号
	void activeStyleChanged(QString);
	void selectedStyleChanged();
	/// @brief 标注样式修改后触发的信号
	void styleChanged();

private:
	void updateUI();

private:
	DmDocument*				m_pDocument = nullptr;
	DmDimensionStyleTable*	m_pDimStyleTable = nullptr;
	DmDimensionStyle*		m_pActiveStyle = nullptr;
	DmDimensionStyle*		m_pSelectedStyle = nullptr;
};

#endif 