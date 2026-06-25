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

/// @file UIDimStyleListBox.cpp
/// @brief 标注样式列表控件，支持样式选择、新建、删除、重命名和修改操作

#include "UIDimStyleListBox.h"
#include "DmDimensionStyleTable.h"
#include "DmDocument.h"
#include "GuiDialogFactory.h"
#include "UIDlgDimensionStyleNew.h"
#include <QMenu>
#include <QContextMenuEvent>
#include <QMessageBox>
#include <QInputDialog>
#include "DmDimension.h"
#include "Transaction.h"

UIDimStyleListBox::UIDimStyleListBox(QWidget* parent /*= nullptr*/)
	: QListWidget(parent)
	, m_pDimStyleTable(nullptr)
	, m_pActiveStyle(nullptr)
	, m_pDocument(nullptr)
	, m_pSelectedStyle(nullptr)
{
	connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(slotItemSelectionChanged()));
	connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(slotItemDoubleClicked(QListWidgetItem*)));
}

void UIDimStyleListBox::init(DmDimensionStyleTable* dimStyleTable, DmDocument* document)
{
	m_pDimStyleTable = dimStyleTable;
	m_pDocument = document;
    updateUI();
}

DmDimensionStyle* UIDimStyleListBox::activeStyle() const
{
	return m_pActiveStyle;
}

bool UIDimStyleListBox::canSelectedDelete() const
{
	if (m_pActiveStyle == m_pSelectedStyle)
	{
		return false;
	}
	else
	{
		return true;
	}
}

DmDimensionStyle* UIDimStyleListBox::selectedStyle() const
{
	return m_pSelectedStyle;;
}

DmDimensionStyle* UIDimStyleListBox::styleAt(int idx) const
{
    DmDimensionStyle* foundStyle = nullptr;
    int i = 0;
    for (auto style : *m_pDimStyleTable)
    {
        if (idx == i)
        {
            foundStyle = style;
            break;
        }
        i++;
    }
    return foundStyle;
}

int UIDimStyleListBox::indexOf(DmDimensionStyle* t)
{
    int index = -1;
    int i = 0;
    for (auto style : *m_pDimStyleTable)
    {
        if (style->getName() == t->getName())
        {
            index = i;
            break;
        }
        i++;
    }
    return index;
}

bool UIDimStyleListBox::canRemoveDimStyle(DmDimensionStyle* style)
{
    // 判断是否存在采用该标注样式的标注
    auto entTable = m_pDocument->getEntityTable();
    bool found = false;
    for (auto& entity : *entTable)
    {
        DmDimension* dim = dynamic_cast<DmDimension*>(entity);
        if (dim != nullptr)
        {
            if (dim->getStyle() == style)
            {
                found = true;
                break;
            }
        }
    }
    return !found;
}

void UIDimStyleListBox::slotItemSelectionChanged()
{
	if (currentRow() < m_pDimStyleTable->count())
	{
		m_pSelectedStyle = styleAt(currentRow());
		emit selectedStyleChanged();
	}
}

void UIDimStyleListBox::slotItemDoubleClicked(QListWidgetItem* item)
{
	slotSetActiveDimStyle();
}

void UIDimStyleListBox::slotNewDimStyle()
{
	UIDlgDimensionStyleNew dlg;
	dlg.init(m_pSelectedStyle, m_pDocument);
	if (dlg.exec())
	{
        updateUI();
		setCurrentRow(count() - 1);
	}
}

void UIDimStyleListBox::slotDeleteDimStyle()
{
	//查找是否有使用该标注样式的实体，有则不能删除
	bool canRemove = canRemoveDimStyle(m_pSelectedStyle);
	if (!canRemove)
	{
		QMessageBox::critical(this, tr("Remove dimension style"), tr("Can not remove dimension style:%1, because it's in using.").arg(m_pSelectedStyle->getName()), QMessageBox::Close);
		return;
	}
	QMessageBox::StandardButton btn = QMessageBox::warning(this, tr("Remove dimension style"), tr("Do you really want to delete dimension style:%1?").arg(m_pSelectedStyle->getName()), QMessageBox::Ok |QMessageBox::Cancel);
	if (btn == QMessageBox::Ok)
	{
        Transaction t(tr("Delete dimension style").toStdString(), m_pDocument);
        t.start();
        m_pDimStyleTable->remove(m_pSelectedStyle);
        t.commit();
        updateUI();
	}
}

void UIDimStyleListBox::slotSetActiveDimStyle()
{
	if (nullptr == m_pSelectedStyle)
	{
		return;
	}
    Transaction t(tr("Activate dimension style").toStdString(), m_pDocument);
    t.start();
    m_pDimStyleTable->activate(m_pSelectedStyle);
    t.commit();
    emit activeStyleChanged(m_pSelectedStyle->getName());
}

void UIDimStyleListBox::slotRenameDimStyle()
{
	QInputDialog* pDlg = new QInputDialog(this);
	pDlg->setOkButtonText(tr("OK"));
	pDlg->setCancelButtonText(tr("Cancel"));
	pDlg->setWindowTitle(QString("%1%2").arg(tr("Rename dimension style:")).arg(m_pSelectedStyle->getName()));
	pDlg->setLabelText(tr("New name of dimension style:"));
	pDlg->setTextValue(m_pSelectedStyle->getName());
	pDlg->adjustSize();
	if (QDialog::Accepted != pDlg->exec())
	{
		return;
	}

	QString style = pDlg->textValue();
	if (m_pDimStyleTable->find(style))
	{
		QMessageBox::critical(this, tr("Rename dimension style"), tr("%0 is already exist !").arg(style), QMessageBox::Close);
		return;
	}

    Transaction t(tr("Rename dimension style").toStdString(), m_pDocument);
    t.start();
    m_pDimStyleTable->startModify(m_pSelectedStyle);
    m_pSelectedStyle->setName(style);
    t.commit();

	int idx = indexOf(m_pSelectedStyle);
	QListWidgetItem* pitem = item(idx);
	pitem->setText(style);
	if (m_pSelectedStyle == m_pActiveStyle)
	{
		emit activeStyleChanged(m_pActiveStyle->getName());
	}
}

void UIDimStyleListBox::slotModifyDimStyle()
{
	bool res = GUIDIALOGFACTORY->requestDimStyleModifyDialog(m_pSelectedStyle, m_pDocument);
	if (res)
	{
		emit styleChanged();
	}
}

void UIDimStyleListBox::updateUI()
{
	//移除所有项
	clear();

    for (auto& style : *m_pDimStyleTable)
    {
        addItem(style->getName());
    }

	//获得文档中当前的标注样式
    m_pActiveStyle = m_pDocument->getDimStyleTable()->getActive();

	if (count() > 0)
	{
		int idx = indexOf(m_pActiveStyle);
		if (idx >= 0)
		{
			setCurrentRow(idx);
			m_pSelectedStyle = m_pActiveStyle;
		}
	}
}
