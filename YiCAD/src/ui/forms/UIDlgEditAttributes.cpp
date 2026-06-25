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

/// @file UIDlgEditAttributes.cpp
/// @brief 块属性编辑对话框，用于设置和获取块引用的属性值

#include "UIDlgEditAttributes.h"
#include "DmAttributeDefinition.h"
#include "DmAttribute.h"

UIDlgEditAttributes::UIDlgEditAttributes(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(true);
    setupUi(this);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slotOk()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void UIDlgEditAttributes::setData(const QString& blockName, const std::list<DmAttributeDefinition*>& attrDefs)
{
    lBlockName->setText(blockName);
    m_attrDefs.reserve(attrDefs.size());
    m_attrDefs.insert(m_attrDefs.end(), attrDefs.begin(), attrDefs.end());

    // 加载数据到表格
    tblAttributes->clear();

    tblAttributes->setColumnCount(2);
    tblAttributes->setRowCount(m_attrDefs.size());
    tblAttributes->setHorizontalHeaderLabels(QStringList{ tr("Attributes"), tr("Values") });
    tblAttributes->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tblAttributes->verticalHeader()->hide();
    tblAttributes->horizontalHeader()->setSectionsClickable(false);
    tblAttributes->setEditTriggers(QAbstractItemView::AllEditTriggers);
    tblAttributes->setSortingEnabled(false);
    for (int i = 0; i < m_attrDefs.size(); i++)
    {
        auto item = m_attrDefs.at(i);
        QString tag = item->getTag();
        QString prompt = item->getPrompt();
        QString def = item->getText();
        QString key = prompt.isEmpty() ? tag : prompt;
        QString val = def;

        QTableWidgetItem* keyItem = new QTableWidgetItem(key);
        keyItem->setFlags(keyItem->flags() ^ Qt::ItemIsEditable ^ Qt::ItemIsSelectable);
        keyItem->setTextAlignment(Qt::AlignCenter);
        tblAttributes->setItem(i, 0, keyItem);
        QTableWidgetItem* valItem = new QTableWidgetItem(val);
        valItem->setTextAlignment(Qt::AlignCenter);
        tblAttributes->setItem(i, 1, valItem);
    }
}

std::list<DmAttribute*> UIDlgEditAttributes::getAttributes() const
{
    return m_attrs;
}

void UIDlgEditAttributes::slotCancel()
{
    m_attrs.clear();
    for (int i = 0; i < m_attrDefs.size(); i++)
    {
        auto item = m_attrDefs.at(i);
        DmAttribute* attr = new DmAttribute(nullptr, item->getData(), AttributeData(item->getTag()));
        attr->setPen(DmPen(DmColor(DM::FlagByBlock), DM::LineWidth::Width00, DmLineTypeTable::Continuous));
        attr->setLayer(nullptr);
        attr->setText(item->getText());
        attr->update();
        m_attrs.emplace_back(attr);
    }
    QDialog::reject();
}

void UIDlgEditAttributes::slotOk()
{
    m_attrs.clear();
    for (int i = 0; i < m_attrDefs.size(); i++)
    {
        auto item = m_attrDefs.at(i);
        QString val = tblAttributes->item(i, 1)->text().trimmed();
        DmAttribute* attr = new DmAttribute(nullptr, item->getData(), AttributeData(item->getTag()));
        attr->setPen(DmPen(DmColor(DM::FlagByBlock), DM::LineWidth::Width00, DmLineTypeTable::Continuous));
        attr->setLayer(nullptr);
        attr->setText(val);
        attr->update();
        m_attrs.emplace_back(attr);
    }
    QDialog::accept();
}
