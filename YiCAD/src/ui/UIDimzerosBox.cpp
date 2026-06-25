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

/// @file UIDimzerosBox.cpp
/// @brief 标注零值处理下拉框控件，控制前导零和结尾零的显示规则

#include "UIDimzerosBox.h"

#include <QTableView>
#include <QListWidgetItem>

UIDimzerosBox::UIDimzerosBox(QWidget* parent) 
	: QComboBox(parent)
{
	m_isDimLine = false;
	m_pView = new QListView();
	m_pModel = new QStandardItemModel(3, 1);
	QStandardItem* item = new QStandardItem(tr("select:"));
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	m_pModel->setItem(0, 0, item);
	item = new QStandardItem(tr("remove left"));
	item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
	item->setData(Qt::Unchecked, Qt::CheckStateRole);
	m_pModel->setItem(1, 0, item);
	item = new QStandardItem(tr("remove right"));
	item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
	item->setData(Qt::Unchecked, Qt::CheckStateRole);
	m_pModel->setItem(2, 0, item);

	setModel(m_pModel);
	setView(m_pView);
	setEditable(false);
	setEditText("selectar:");
}

UIDimzerosBox::~UIDimzerosBox() 
{
	delete m_pModel;
	delete m_pView;
}

void UIDimzerosBox::setLinear() 
{
	m_isDimLine = true;
	QStandardItem* item = new QStandardItem(tr("remove 0'"));
	item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
	item->setData(Qt::Unchecked, Qt::CheckStateRole);
	m_pModel->appendRow(item);
	item = new QStandardItem(tr("remove 0\""));
	item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
	item->setData(Qt::Unchecked, Qt::CheckStateRole);
	m_pModel->appendRow(item);
}

void UIDimzerosBox::setData(int i) 
{
	if (m_isDimLine) 
	{
		if (i & 1) 
		{
			if (i & 2)
			{
				m_pModel->item(3)->setCheckState(Qt::Checked);
			}
		}
		else 
		{
			m_pModel->item(4)->setCheckState(Qt::Checked);
			if (!(i & 2))
			{
				m_pModel->item(3)->setCheckState(Qt::Checked);
			}
		}
		if (i & 4)
		{
			m_pModel->item(1)->setCheckState(Qt::Checked);
		}
		if (i & 8)
		{
			m_pModel->item(2)->setCheckState(Qt::Checked);
		}
	}
	else 
	{
		if (i & 1)
		{
			m_pModel->item(1)->setCheckState(Qt::Checked);
		}
		if (i & 2)
		{
			m_pModel->item(2)->setCheckState(Qt::Checked);
		}
	}
}

int UIDimzerosBox::getData() 
{
	int ret = 0;
	if (m_isDimLine) 
	{
		if (m_pModel->item(1)->checkState() == Qt::Checked)
		{
			ret |= 4;
		}
		if (m_pModel->item(2)->checkState() == Qt::Checked)
		{
			ret |= 8;
		}
		// imperial:
		if (m_pModel->item(3)->checkState() == Qt::Checked) 
		{
			if (m_pModel->item(4)->checkState() == Qt::Unchecked)
			{
				ret |= 3;
			}
		}
		else 
		{
			if (m_pModel->item(4)->checkState() == Qt::Checked)
			{
				ret |= 2;
			}
			else
			{
				ret |= 1;
			}
		}
	}
	else 
	{
		if (m_pModel->item(1)->checkState() == Qt::Checked)
		{
			ret |= 1;
		}
		if (m_pModel->item(2)->checkState() == Qt::Checked)
		{
			ret |= 2;
		}
	}
	return ret;
}

// helper function for DIMZIN var.
int UIDimzerosBox::convertDimZin(int v, bool toIdx) 
{
	if (toIdx) 
	{
		if (v < 5)
		{
			return 0;
		}
		int res = 0;
		if (v & 4)
		{
			res = 3;
		}
		if (v & 8)
		{
			return (res == 3) ? 5 : 4;
		}
	}
	// toIdx false
	switch (v) 
	{
	case 3:
		return 4;
		break;
	case 4:
		return 8;
		break;
	case 5:
		return 12;
		break;
	default:
		break;
	}
	return 1;
}
