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

/// @file UITextStyleListBox.cpp
/// @brief 文字样式列表控件，支持新建、删除、重命名和激活文字样式

#include "UITextStyleListBox.h"
#include <QAction>
#include <QMenu>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QMessageBox>
#include "DmTextStyleTable.h"
#include "DmDocument.h"
#include "DmText.h"
#include "DmMText.h"
#include "Transaction.h"

UITextStyleListBox::UITextStyleListBox(QWidget* parent /*= nullptr*/)
	: QListWidget(parent)
	, m_pTextStyleTable(nullptr)
	, m_pActiveStyle(nullptr)
	, m_pDocument(nullptr)
	, m_pSelectedStyle(nullptr)
	, m_isLoaded(false)
{
	m_pActNew = new QAction(tr("New"));
	m_pActDelete = new QAction(tr("Delete"));
	m_pActActive = new QAction(tr("Set as active"));
	m_pActRename = new QAction(tr("Rename"));
	connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));
	connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(itemDoubleClicked(QListWidgetItem*)));
	connect(m_pActNew, SIGNAL(triggered()), this, SLOT(newTextStyle()));
	connect(m_pActDelete, SIGNAL(triggered()), this, SLOT(deleteTextStyle()));
	connect(m_pActActive, SIGNAL(triggered()), this, SLOT(setActiveTextStyle()));
	connect(m_pActRename, SIGNAL(triggered()), this, SLOT(renameTextStyle()));
}

UITextStyleListBox::~UITextStyleListBox()
{
}

void UITextStyleListBox::setStyleList(DmTextStyleTable* textStyleTable, DmDocument* document)
{
	m_pTextStyleTable = textStyleTable;
	m_pDocument = document;
    m_pActiveStyle = textStyleTable->getActive();
    m_pSelectedStyle = m_pActiveStyle;
	update();
	m_isLoaded = true;
}

DmTextStyle* UITextStyleListBox::activeStyle() const
{
	return m_pActiveStyle;
}

DmTextStyle* UITextStyleListBox::selectedStyle() const
{
	return m_pSelectedStyle;
}

//DmTextStyleData UITextStyleListBox::getData()
//{
//	return m_pSelectedStyle->getData();
//}

void UITextStyleListBox::setActiveStyle(const QString& name)
{
    Transaction t(tr("Activate TextStyle").toStdString(), m_pDocument);
    t.start();
    m_pTextStyleTable->activate(name);
    t.commit();
    m_pActiveStyle = m_pTextStyleTable->getActive();
    emit activeStyleChanged(m_pActiveStyle->getName());
}

void UITextStyleListBox::update()
{
	//移除所有项
	clear();

	if (m_pTextStyleTable)
	{
        auto activeStyle = m_pDocument->getTextStyleTable()->getActive();
		for (auto it = m_pTextStyleTable->begin(); it != m_pTextStyleTable->end(); ++it)
		{
			addItem((*it)->getName());
		}
	}

	if (count() > 0)
	{
        int i = 0;
        for (auto it = m_pTextStyleTable->begin(); it != m_pTextStyleTable->end(); ++it)
        {
            if (m_pSelectedStyle == *it)
            {
                setCurrentRow(i);
            }
            i++;
        }
	}
}

void UITextStyleListBox::itemSelectionChanged()
{
    QString text = currentItem()->text();
    auto style = m_pTextStyleTable->find(text);
    m_pSelectedStyle = style;
    emit selectedStyleChanged();
}

void UITextStyleListBox::itemDoubleClicked(QListWidgetItem* item)
{
	setActiveTextStyle();
}

void UITextStyleListBox::newTextStyle()
{
	QInputDialog* pDlg = new QInputDialog(this);
	pDlg->setOkButtonText(tr("OK"));
	pDlg->setCancelButtonText(tr("Cancel"));
	pDlg->setWindowTitle(tr("New text style"));
	pDlg->setLabelText(tr("New text style name:"));
	pDlg->setTextValue(QString("%1 %2").arg(m_pSelectedStyle->getName()).arg(tr("copy")));
	pDlg->adjustSize();
	if (QDialog::Accepted != pDlg->exec())
	{
		return;
	}

	QString style = pDlg->textValue().trimmed();
	if (style.isEmpty())
	{
		QMessageBox::critical(this, tr("New text style"), tr("Style name can not be empty !"), QMessageBox::Close);
		return;
	}
	if (m_pTextStyleTable->find(style))
	{
		QMessageBox::critical(this, tr("New text style"), tr("%0 is already exist !").arg(style), QMessageBox::Close);
		return;
	}
	DmTextStyle* pNewStyle = new DmTextStyle(*activeStyle(), style);
    Transaction t(tr("New TextStyle").toStdString(), m_pDocument);
    t.start();
    pNewStyle->setDocument(m_pDocument);
	m_pTextStyleTable->add(pNewStyle);
    t.commit();
	m_pSelectedStyle = pNewStyle;
	addItem(style);
	setCurrentRow(count() - 1);
}

void UITextStyleListBox::deleteTextStyle()
{
	//查找是否有使用该文字样式的实体，有则不能删除
	bool canRemove = canRemoveTextStyle(m_pSelectedStyle);
	if (!canRemove)
	{
		QMessageBox::critical(this, tr("Remove text style"), tr("Can not remove text style:%1, because it's in using.").arg(m_pSelectedStyle->getName()), QMessageBox::Close);
		return;
	}
    Transaction t(tr("Remove TextStyle").toStdString(), m_pDocument);
    t.start();
	m_pTextStyleTable->remove(m_pSelectedStyle);
    t.commit();
    m_pSelectedStyle = m_pActiveStyle;
	update();
}

void UITextStyleListBox::setActiveTextStyle()
{
	setActiveStyle(m_pSelectedStyle->getName());
}

void UITextStyleListBox::renameTextStyle()
{
	QInputDialog* pDlg = new QInputDialog(this);
	pDlg->setOkButtonText(tr("OK"));
	pDlg->setCancelButtonText(tr("Cancel"));
	pDlg->setWindowTitle(QString("%1%2").arg(tr("Rename text style:")).arg(m_pSelectedStyle->getName()));
	pDlg->setLabelText(tr("New name of text style:"));
	pDlg->setTextValue(m_pSelectedStyle->getName());
	pDlg->adjustSize();
	if (QDialog::Accepted != pDlg->exec())
	{
		return;
	}

	QString style = pDlg->textValue();
	if (m_pTextStyleTable->find(style))
	{
		QMessageBox::critical(this, tr("Rename text style"), tr("%0 is already exist !").arg(style), QMessageBox::Close);
		return;
	}

    auto originName = m_pSelectedStyle->getName();
    Transaction t(tr("Rename TextStyle").toStdString(), m_pDocument);
    t.start();
    m_pSelectedStyle->setName(style);
    t.commit();
    //重命名列表项目
    for (int i = 0; i < count(); i++)
    {
        auto itemPtr = item(i);
        if (itemPtr->text() == originName)
        {
            itemPtr->setText(style);
            break;
        }
    }
    if (m_pSelectedStyle == m_pActiveStyle)
    {
        emit activeStyleChanged(m_pActiveStyle->getName());
    }
}

bool UITextStyleListBox::canSelectedRename() const
{
	if (!m_pSelectedStyle->isStandard())
	{
		return true;
	}
	return false;
}

bool UITextStyleListBox::canSelectedDelete() const
{
	if (!m_pSelectedStyle->isStandard() && m_pActiveStyle != m_pSelectedStyle)
	{
		return true;
	}
	return false;
}

QAction* UITextStyleListBox::getActionNew() const
{
	return m_pActNew;
}

QAction* UITextStyleListBox::getActionActivate() const
{
	return m_pActActive;
}

QAction* UITextStyleListBox::getActionRename() const
{
	return m_pActRename;
}

QAction* UITextStyleListBox::getActionDelete() const
{
	return m_pActDelete;
}

bool UITextStyleListBox::canRemoveTextStyle(DmTextStyle* style)
{
    bool found = false;
    auto entTable = m_pDocument->getEntityTable();
    //检索文字是否使用
    for (auto it = entTable->begin(); it != entTable->end(); ++it)
    {
        DmEntity* entity = *it;
        if (entity->getEntityType() == DM::EntityText)
        {
            DmText* text = dynamic_cast<DmText*>(entity);
            if (style == text->getStyle())
            {
                found = true;
                break;
            }
        }
        else if (entity->getEntityType() == DM::EntityMText)
        {
            DmMText* mtext = dynamic_cast<DmMText*>(entity);
            if (style == mtext->getStyle())
            {
                found = true;
                break;
            }
        }
    }

    //检索标注样式是否使用
    if (!found)
    {
        for (auto& dimstyle : *m_pDocument->getDimStyleTable())
        {
            if (dimstyle->getDataRef().textStyle() == style)
            {
                found = true;
                break;
            }
        }
    }
    return !found;
}

void UITextStyleListBox::mousePressEvent(QMouseEvent* event)
{
    // 获取当前点击的项
    QListWidgetItem* item = itemAt(event->pos());
    if (item)
    {
        bool canChange = false;
        emit selectedStyleChanging(canChange);
        if (canChange)
        {
            QListWidget::mousePressEvent(event);
        }
        else
        {
            //阻止选择,阻止事件传递
            event->ignore();
        }
    }
    else
    {
        QListWidget::mousePressEvent(event); // 继续默认处理
    }
}

void UITextStyleListBox::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)
    {
        // 处理方向键选择前的逻辑
        // 例如：获取当前选中项，并判断是否允许切换
        bool canChange = false;
        emit selectedStyleChanging(canChange);
        if (canChange)
        {
            QListWidget::keyPressEvent(event);
        }
        else
        {
            //阻止选择,阻止事件传递
            event->ignore();
        }
    }
    QListWidget::keyPressEvent(event);
}

//void UITextStyleListBox::contextMenuEvent(QContextMenuEvent* event)
//{
//	QMenu menu;
//	menu.addAction(m_pActNew);
//	if (m_pActiveStyle != m_pSelectedStyle)
//	{
//		menu.addAction(m_pActActive);
//	}
//	if (!m_pSelectedStyle->isStandard())
//	{
//		menu.addAction(m_pActRename);
//	}
//	if (!m_pSelectedStyle->isStandard() && m_pActiveStyle != m_pSelectedStyle)
//	{
//		menu.addAction(m_pActDelete);
//	}
//	menu.exec(event->globalPos());
//	event->accept();
//}
