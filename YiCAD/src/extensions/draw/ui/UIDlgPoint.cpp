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

/// @file UIDlgPoint.cpp
/// @brief 点实体属性编辑对话框

#include "UIDlgPoint.h"

#include <QPushButton>

#include "DmPoint.h"
#include "DmDocument.h"
#include "Math2d.h"
#include "Transaction.h"

UIDlgPoint::UIDlgPoint(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

    this->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    this->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
}

UIDlgPoint::~UIDlgPoint()
{
}

void UIDlgPoint::languageChange()
{
    retranslateUi(this);
}

void UIDlgPoint::setPoint(DmPoint& p)
{
    m_pPoint = &p;
    wPen->setPen(m_pPoint->getPen(false), true, tr("Pen"));
    DmDocument* document = m_pPoint->getDocument();
    if (document != nullptr)
    {
        cbLayer->init(*(document->getLayerTable()), false);
    }
    DmLayer* lay = m_pPoint->getLayer(false);
    if (lay != nullptr)
    {
        cbLayer->setLayer(*lay);
    }

    QString s;
    s.setNum(m_pPoint->getPos().x);
    lePosX->setText(s);
    s.setNum(m_pPoint->getPos().y);
    lePosY->setText(s);
}

void UIDlgPoint::updatePoint()
{
    Transaction t(tr("Modify point").toStdString(), m_pPoint->getDocument());
    t.start();
    m_pPoint->getDocument()->getEntityTable()->startModify(m_pPoint);
    m_pPoint->setPos(DmVector(Math2d::eval(lePosX->text()), Math2d::eval(lePosY->text())));
    m_pPoint->setPen(wPen->getPen());
    m_pPoint->setLayer(cbLayer->currentText());
    t.commit();
}
