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

/// @file UIDlgInsert.cpp
/// @brief 块参照插入/编辑对话框

#include "UIDlgInsert.h"

#include <QPushButton>

#include "DmBlockReference.h"
#include "DmAttribute.h"
#include "DmDocument.h"
#include "Math2d.h"
#include "Transaction.h"

UIDlgInsert::UIDlgInsert(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);
}

UIDlgInsert::~UIDlgInsert()
{
}

void UIDlgInsert::languageChange()
{
    retranslateUi(this);
}

void UIDlgInsert::setInsert(DmBlockReference& i)
{
    m_pInsert = &i;
    wPen->setPen(m_pInsert->getPen(false), true, tr("Pen"));
    DmDocument* document = m_pInsert->getDocument();
    if (document != nullptr)
    {
        cbLayer->init(*(document->getLayerTable()), false);
    }
    DmLayer* lay = m_pInsert->getLayer(false);
    if (lay != nullptr)
    {
        cbLayer->setLayer(*lay);
    }
    QString s;
    s.setNum(m_pInsert->getInsertionPoint().x);
    leInsertionPointX->setText(s);
    s.setNum(m_pInsert->getInsertionPoint().y);
    leInsertionPointY->setText(s);
    s.setNum(m_pInsert->getScale().x);
    leScaleX->setText(s);
    s.setNum(m_pInsert->getScale().y);
    leScaleY->setText(s);
    s.setNum(Math2d::rad2deg(m_pInsert->getAngle()));
    leAngle->setText(s);
    s.setNum(m_pInsert->getRows());
    leRows->setText(s);
    s.setNum(m_pInsert->getCols());
    leCols->setText(s);
    s.setNum(m_pInsert->getSpacing().y);
    leRowSpacing->setText(s);
    s.setNum(m_pInsert->getSpacing().x);
    leColSpacing->setText(s);

    // 属性
    std::list<DmAttribute*> attrs = i.getAttributes();
    int attrSize = attrs.size();
    if (attrSize > 0)
    {
        // 隐藏行列等控件
        lRows->hide();
        leRows->hide();
        lCols->hide();
        leCols->hide();
        lRowSpacing->hide();
        leRowSpacing->hide();
        lColSpacing->hide();
        leColSpacing->hide();

        // 设置属性
        tblAttributes->clear();
        tblAttributes->setColumnCount(2);
        tblAttributes->setRowCount(attrSize);
        tblAttributes->setHorizontalHeaderLabels(QStringList{ tr("Attributes"), tr("Values") });
        tblAttributes->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tblAttributes->verticalHeader()->hide();
        tblAttributes->horizontalHeader()->setSectionsClickable(false);
        tblAttributes->setEditTriggers(QAbstractItemView::AllEditTriggers);
        tblAttributes->setSortingEnabled(false);
        int row = 0;
        for (auto item = attrs.begin(); item != attrs.end(); ++item)
        {
            auto attr = *item;
            QString key = attr->getTag();
            QString val = attr->getText();
            QTableWidgetItem* keyItem = new QTableWidgetItem(key);
            keyItem->setFlags(keyItem->flags() ^ Qt::ItemIsEditable ^ Qt::ItemIsSelectable);
            keyItem->setTextAlignment(Qt::AlignCenter);
            tblAttributes->setItem(row, 0, keyItem);
            QTableWidgetItem* valItem = new QTableWidgetItem(val);
            valItem->setTextAlignment(Qt::AlignCenter);
            tblAttributes->setItem(row, 1, valItem);
            row++;
        }
    }
    else
    {
        gbAttributes->hide();
    }
}

void UIDlgInsert::updateInsert()
{
    Transaction t(tr("Modify block reference").toStdString(), m_pInsert->getDocument());
    t.start();
    m_pInsert->getDocument()->getEntityTable()->startModify(m_pInsert);
    DmVector oldInsertPt = m_pInsert->getInsertionPoint();
    DmVector oldScale = m_pInsert->getScale();
    double oldAngle = m_pInsert->getAngle();

    // 设置参数，update()再外部调用
    m_pInsert->setInsertionPoint(DmVector(Math2d::eval(leInsertionPointX->text()), Math2d::eval(leInsertionPointY->text())));
    m_pInsert->setScale(DmVector(Math2d::eval(leScaleX->text()), Math2d::eval(leScaleY->text())));
    m_pInsert->setAngle(Math2d::deg2rad(Math2d::eval(leAngle->text())));
    m_pInsert->setRows(Math2d::round(Math2d::eval(leRows->text())));
    m_pInsert->setCols(Math2d::round(Math2d::eval(leCols->text())));
    m_pInsert->setSpacing(DmVector(Math2d::eval(leColSpacing->text()), Math2d::eval(leRowSpacing->text())));
    m_pInsert->setPen(wPen->getPen());
    m_pInsert->setLayer(cbLayer->currentText());

    // 属性文字
    DmVector curScale = m_pInsert->getScale();
    DmVector curInsertPt = m_pInsert->getInsertionPoint();
    double curAngle = m_pInsert->getAngle();
    std::list<DmAttribute*> attrs = m_pInsert->getAttributes();
    int attrSize = attrs.size();
    int row = 0;
    for (auto item = attrs.begin(); item != attrs.end(); ++item)
    {
        auto attr = *item;
        QString val = tblAttributes->item(row, 1)->text().trimmed();
        attr->move(curInsertPt - oldInsertPt);
        attr->scale(curInsertPt, DmVector(curScale.x / oldScale.x, curScale.y / oldScale.y));
        attr->rotateAngle(curInsertPt, curAngle - oldAngle);
        attr->setText(val);
        attr->update();
        row++;
    }
    t.commit();
}
