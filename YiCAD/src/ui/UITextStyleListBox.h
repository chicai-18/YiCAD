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

/// @file UITextStyleListBox.h
/// @brief 文字样式列表控件

#ifndef UITEXTSTYLELISTBOX_H
#define UITEXTSTYLELISTBOX_H

#include <QListWidget>
class DmTextStyleTable;
class DmTextStyle;
class QAction;
class DmDocument;
struct DmTextStyleData;

class UITextStyleListBox : public QListWidget
{
	Q_OBJECT
public:
		UITextStyleListBox(QWidget* parent = nullptr);
		virtual ~UITextStyleListBox();

public:
	void setStyleList(DmTextStyleTable* textStyleTable, DmDocument* document);
	DmTextStyle* activeStyle() const;
	DmTextStyle* selectedStyle() const;
	//DmTextStyleData getData();
	bool isLoaded() { return m_isLoaded;  /*return count() > 0;*/ }

private slots:
	void itemSelectionChanged();
	void itemDoubleClicked(QListWidgetItem* item);

public slots:
	void newTextStyle();
	void deleteTextStyle();
	void setActiveTextStyle();
	void renameTextStyle();

public:
	bool canSelectedRename() const;
	bool canSelectedDelete() const;
    /// @brief 是否可删除文字样式。模型中没有使用的文字样式可删除
    bool canRemoveTextStyle(DmTextStyle* style);
	QAction* getActionNew() const;
	QAction* getActionActivate() const;
	QAction* getActionRename() const;
	QAction* getActionDelete() const;
protected:
    /// @brief 重写该事件实现选项改变前做判断
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
	void setActiveStyle(const QString& name);
	void update();

signals:
	void activeStyleChanged(QString);
	void selectedStyleChanging(bool& canChange);
	void selectedStyleChanged();

private:
	QAction* m_pActNew = nullptr;
	QAction* m_pActDelete = nullptr;
	QAction* m_pActActive = nullptr;
	QAction* m_pActRename = nullptr;
	DmTextStyleTable* m_pTextStyleTable = nullptr;
	DmTextStyle* m_pActiveStyle = nullptr;
	DmTextStyle* m_pSelectedStyle = nullptr;
	DmDocument* m_pDocument = nullptr;
	bool m_isLoaded = false;

};
#endif //UITEXTSTYLELISTBOX_H